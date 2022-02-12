#include "pch.h"
#include "expressions.h"
#include "utils.h"
#include "names.h"
#include "user_exception.h"

namespace mscript
{
    /// <summary>
    /// These are the binary operators supported by the expression processor
    /// They are in operator precedence order, least to most
    /// </summary>
    std::vector<std::string> sm_ops
    {
        "or",
        "||",
        "and",
        "&&",

        "!=",
        "<=",
        ">=",
        "<",
        ">",
        "=",

        "%",
        "-",
        "+",
        "/",
        "*",
        "^",
    };

    object expression::evaluate(std::wstring expStr)
    {
        expStr = trim(expStr);
        std::string narrow = toNarrowStr(expStr);

        // Check for easy stuff
        if (narrow.empty())
            raiseError("Empty expression");

        if (narrow == "null")
            return object::NOTHING;

        if (narrow == "true")
            return true;

        if (narrow == "false")
            return false;

        // Do an exact parsing of a number from the full expression string
        if (narrow[0] == '-' || isdigit(narrow[0]))
        {
            double number;
            const char* start = narrow.data();
            const char* end = start + narrow.size();
            auto result = std::from_chars(start, end, number);
            if (result.ptr == end && result.ec == std::errc())
                return number;
        }

        // Stay out of string constants
        if (expStr[0] == '\"')
        {
            std::wstring str;
            bool foundEnd = false;
            bool foundAtEnd = false;
            for (size_t s = 1; s < expStr.size(); ++s)
            {
                wchar_t c = expStr[s];
                if (c == '\"')
                {
                    foundEnd = true;
                    foundAtEnd = s == expStr.size() - 1;
                    break;
                }
                else
                    str += c;
            }
            if (!foundEnd)
                raiseWError(L"Unfinished string: " + expStr);
            else if (foundAtEnd)
                return str;
            // else it's a string at the start of an expression, like "foo" + QUOTE
            // It will get handled by the recursive evaluate of the two sides of the op
        }

        if (expStr[0] == '\'')
        {
            std::wstring str;
            bool foundEnd = false;
            bool foundAtEnd = false;
            for (size_t s = 1; s < expStr.size(); ++s)
            {
                wchar_t c = expStr[s];
                if (c == '\'')
                {
                    foundEnd = true;
                    foundAtEnd = s == expStr.size() - 1;
                    break;
                }
                else
                    str += c;
            }
            if (!foundEnd)
                raiseWError(L"Unfinished string: " + expStr);
            else if (foundAtEnd)
                return str;
            // else it's a string at the start of an expression, like 'foo' + squote
            // It will get handled by the recursive evaluate of the two sides of the op
        }

        if (narrow == "dquote")
            return toWideStr("\"");

        if (narrow == "squote")
            return toWideStr("\'");

        if (narrow == "tab")
            return toWideStr("\t");

        if (narrow == "lf")
            return toWideStr("\n");

        if (narrow == "crlf")
            return toWideStr("\r\n");

        if (narrow == "pi")
            return M_PI;

        if (narrow == "e")
            return M_E;

        {
            object symvalue;
            if (m_symbols.tryGet(expStr, symvalue))
                return symvalue;
        }

        if (isName(expStr)) // should have been found in symbol table
            raiseWError(L"Unknown variable name: " + expStr);

        // Walk the operators, least to most precedenced
        for (size_t opdx = 0; opdx < sm_ops.size(); ++opdx)
        {
            const std::string& op = sm_ops[opdx];
            size_t opLen = op.size();

            size_t parenCount = 0;

            bool inSingleString = false;
            bool inDoubleString = false;

            // Walk the exception from the end, so that we bind to
            // the right-side of the expression
            for (int idx = int(expStr.size()) - 1; idx >= 0; --idx)
            {
                wchar_t c = expStr[idx];

                // Stay out of strings
                if (!inDoubleString && c == '\'')
                    inSingleString = !inSingleString;
                if (!inSingleString && c == '\"')
                    inDoubleString = !inDoubleString;
                if (inSingleString || inDoubleString)
                    continue;

                // Stay out of parens
                if (c == '(')
                    ++parenCount;
                else if (c == ')')
                    --parenCount;
                if (parenCount != 0)
                    continue;

                bool opMatches = false;
                if (c == op[0]) // quick filter
                {
                    if (opLen == 1)
                    {
                        opMatches = true;
                    }
                    else if (opLen == 2)
                    {
                        if (idx < expStr.size() - 1)
                            opMatches = expStr[idx + 1] == op[1];
                    }
                    else if (opLen == 3)
                    {
                        if (idx < expStr.size() - 2)
                        {
                            opMatches = 
                                expStr[idx + 1] == op[1]
                                && 
                                expStr[idx + 2] == op[2];
                        }
                    }
                    else
                        raiseWError(L"Invalid operator length: " + expStr);
                }
                if (opMatches)
                {
                    // Make sure our operator isn't some 5E+5 nonsense
                    if (isOperator(expStr, op, idx))
                    {
                        object value;

                        // Split the string into left and right parts
                        std::wstring leftStr = expStr.substr(0, idx);
                        std::wstring rightStr = expStr.substr(idx + opLen);

                        // Evaluate the left part
                        object leftVal = evaluate(leftStr);

                        // Short circuitry
                        if ((op == "and" || op == "&&") && !leftVal.boolVal())
                        {
                            value = false;
                        }
                        else if ((op == "or" || op == "||") && leftVal.boolVal())
                        {
                            value = true;
                        }
                        else
                        {
                            // Evaluate the right part
                            object rightVal = evaluate(rightStr);

                            // Handle nulls with equivalent checks
                            if (leftVal.isNull() || rightVal.isNull())
                            {
                                if (op == "=")
                                    value = leftVal == rightVal;
                                else if (op == "!=")
                                    value = leftVal != rightVal;
                                else
                                    raiseWError(L"Invalid operator for null values: " + expStr);
                            }
                            // Handle string on either side, string promotion
                            else if (leftVal.type() == object::STRING || rightVal.type() == object::STRING)
                            {
                                std::wstring leftValStr = leftVal.toString();
                                std::wstring rightValStr = rightVal.toString();

                                if (op.length() == 1)
                                {
                                    switch (op[0])
                                    {
                                    case '+': value = leftValStr + rightValStr; break;
                                    case '=': value = leftValStr == rightValStr; break;
                                    case '<': value = _wcsicmp(leftValStr.c_str(), rightValStr.c_str()) < 0; break;
                                    case '>': value = _wcsicmp(leftValStr.c_str(), rightValStr.c_str()) > 0; break;
                                    default:
                                        raiseWError(L"Unrecognized string operator: " + expStr);
                                    }
                                }
                                else
                                {
                                    if (op == "!=")
                                        value = leftValStr != rightValStr;
                                    else
                                        raiseWError(L"Unrecognized string operator: " + expStr);
                                }
                            }
                            // Numbers are easy
                            else if (leftVal.type() == object::NUMBER && rightVal.type() == object::NUMBER)
                            {
                                double leftNum = leftVal.numberVal();
                                double rightNum = rightVal.numberVal();

                                if (isnan(leftNum) || isnan(rightNum))
                                {
                                    value = nan("");
                                }
                                else
                                {
                                    if (op.length() == 1)
                                    {
                                        switch (op[0])
                                        {
                                        case '+': value = leftNum + rightNum; break;
                                        case '-': value = leftNum - rightNum; break;
                                        case '*': value = leftNum * rightNum; break;
                                        case '/': value = leftNum / rightNum; break;
                                        case '%': value = double(int64_t(leftNum) % int64_t(rightNum)); break;
                                        case '^': value = pow(leftNum, rightNum); break;
                                        case '=': value = leftNum == rightNum; break;
                                        case '<': value = leftNum < rightNum; break;
                                        case '>': value = leftNum > rightNum; break;
                                        default: raiseWError(L"Unrecognized numeric operator: " + expStr);
                                        }
                                    }
                                    else
                                    {
                                        if (op == "!=")
                                            value = leftNum != rightNum;
                                        else if (op == "<=")
                                            value = leftNum <= rightNum;
                                        else if (op == ">=")
                                            value = leftNum >= rightNum;
                                        else
                                            raiseWError(L"Unrecognized numeric operator: " + expStr);
                                    }
                                }
                            }
                            // Bools are easy
                            else if (leftVal.type() == object::BOOL && rightVal.type() == object::BOOL)
                            {
                                bool leftBool = leftVal.boolVal();
                                bool rightBool = rightVal.boolVal();

                                if (op == "and")
                                    value = leftBool && rightBool;
                                else if (op == "&&")
                                    value = leftBool && rightBool;
                                else if (op == "or")
                                    value = leftBool || rightBool;
                                else if (op == "||")
                                    value = leftBool || rightBool;
                                else
                                    raiseWError(L"Unrecognized boolean operator: " + expStr);
                            }
                            else
                                raiseWError(L"Expression types do not match: " + expStr);
                        }
                        return value;
                    }
                    else // not an operator after all, so look for the next op
                    {
                        if (idx > 0)
                            idx = reverseFind(expStr, toWideStr(op), idx);
                    }
                }
            }
        }

        // Deal with unary operators
        if (expStr[0] == '-')
        {
            object answer = evaluate(expStr.substr(1));
            return -answer.numberVal();
        }
        else if (expStr[0] == '!')
        {
            object answer = evaluate(expStr.substr(1));
            return !answer.boolVal();
        }
        else if (startsWith(expStr, L"not "))
        {
            static size_t notLen = strlen("not ");
            object answer = evaluate(expStr.substr(notLen));
            return !answer.boolVal();
        }

        // Deal with parens, including function calls
        size_t leftParen = expStr.find('(');
        if (expStr.size() > 2 && leftParen != std::wstring::npos && expStr.back() == ')')
        {
            std::wstring functionName = expStr.substr(0, leftParen);

            int subStrLen = (int(expStr.size()) - 1) - int(leftParen) - 1;

            expStr = expStr.substr(leftParen + 1, subStrLen);

            auto expStrs = parseParameters(expStr);
            object::list values = processParameters(expStrs);

            if (!functionName.empty())
            {
                object functionValue = executeFunction(functionName, values);
                return functionValue;
            }
            else
                return values[0];
        }

        // Oh well, not processed, must not be a valid expression
        raiseWError(L"Expression not evaluated: " + expStr);
    }

    bool expression::isOperator(std::wstring expr, const std::string& op, int n)
    {
        expr = trim(expr);
        if (expr.empty())
            return true;

        if (n < 0 || n >= expr.length())
            return true;

        if (op == "and" || op == "&&" || op == "or" || op == "||")
        {
            wchar_t before = n - 1 >= 0 ? expr[n - 1] : 0;
            if (iswalpha(before))
                return false;

            wchar_t after = (n + op.length()) < expr.length() ? expr[n + op.length()] : 0;
            if (iswalpha(after))
                return false;
        }

        if (op == "-")
        {
            if (n <= 0)
                return false;

            // Get the start up to previous charcater, trim it, then get the last char
            std::string sign = toNarrowStr(trim(expr.substr(0, n)));
            sign = sign.substr(sign.length() - 1);

            // If the last char is an operator, then this is a unary -, not an operator
            if (std::find(sm_ops.begin(), sm_ops.end(), sign) != sm_ops.end())
                return false;

            // Exponent?
            bool expResult =
                towupper(expr[n - 1]) == 'E'
                &&
                n >= 2 && iswdigit(expr[n - 2]);
            if (expResult)
                return false;
        }

        // If there's a +, and we can look back to see the E and the number before that, then it's not an operator
        if (op == "+")
        {
            bool expResult =
                towupper(expr[n - 1]) == 'E'
                &&
                n >= 2 && iswdigit(expr[n - 2]);
            if (expResult)
                return false;
        }

        return true;
    }

    int expression::reverseFind(const std::wstring& source, const std::wstring& searchW, int start)
    {
        int searchLen = int(searchW.length());
        if (searchLen > source.length())
            return -1;

        bool inSingleString = false;
        bool inDoubleString = false;

        int openP = 0;
        int closeP = 0;

        for (int p = start; p >= 0; --p)
        {
            // Stay out of strings
            wchar_t c = source[p];
            if (!inDoubleString && c == '\'')
                inSingleString = !inSingleString;
            if (!inSingleString && c == '\"')
                inDoubleString = !inDoubleString;
            if (inSingleString || inDoubleString)
                continue;

            if (source.length() - (p + searchLen) >= 0)
            {
                std::wstring sign = source.substr(p, searchLen);
                if (sign == searchW && openP == closeP)
                    return p;
            }

            if (source[p] == '(')
                ++openP;
            else if (source[p] == ')')
                ++closeP;
        }

        return -1;
    }

    object::list expression::processParameters(const std::vector<std::wstring>& expStrs)
    {
        object::list values;
        values.reserve(expStrs.size());
        for (const auto& paramStr : expStrs)
        {
            object value = evaluate(paramStr);
            values.push_back(value);
        }
        return values;
    }

    std::vector<std::wstring> expression::parseParameters(const std::wstring& expStr)
    {
        std::vector<std::wstring> expStrs;
        std::wstring curExp;
        bool inSingleString = false;
        bool inDoubleString = false;
        int parenCount = 0;
        for (size_t idx = 0; idx < expStr.size(); ++idx)
        {
            // Stay out of strings
            wchar_t c = expStr[idx];
            if (!inDoubleString && c == '\'')
                inSingleString = !inSingleString;
            if (!inSingleString && c == '\"')
                inDoubleString = !inDoubleString;

            if (!inSingleString && !inDoubleString)
            {
                if (c == '(')
                    ++parenCount;
                else if (c == ')')
                    --parenCount;
            }

            if (!(inSingleString || inDoubleString) && parenCount == 0 && c == ',')
            {
                curExp = trim(curExp);
                if (curExp.empty())
                    raiseWError(L"Missing parameter: " + expStr);

                expStrs.push_back(curExp);
                curExp.clear();
                continue;
            }

            curExp += c;
        }

        curExp = trim(curExp);
        if (!curExp.empty())
            expStrs.push_back(curExp);

        return expStrs;
    }

    double expression::getOneDouble(const object::list& paramList, const std::string& function)
    {
        if (paramList.size() != 1 || paramList[0].type() != object::NUMBER)
            raiseError(function + "() works with one numeric parameter");
        else
            return paramList[0].numberVal();
    }

    object expression::executeFunction(const std::wstring& functionW, const object::list& paramList)
    {
        const std::string function = toNarrowStr(functionW);

        object first = paramList.size() == 0 ? object::NOTHING : paramList[0];

        //
        // Math
        //
        if (function == "abs") return abs(getOneDouble(paramList, "abs"));

        if (function == "sqrt") return sqrt(getOneDouble(paramList, "sqrt"));
        if (function == "ceil") return ceil(getOneDouble(paramList, "ceil"));
        if (function == "floor") return floor(getOneDouble(paramList, "floor"));

        if (function == "exp") return exp(getOneDouble(paramList, "exp"));
        if (function == "log") return log(getOneDouble(paramList, "log"));
        if (function == "log2") return log2(getOneDouble(paramList, "log2"));
        if (function == "log10") return log10(getOneDouble(paramList, "log10"));

        if (function == "sin") return sin(getOneDouble(paramList, "sin"));
        if (function == "cos") return cos(getOneDouble(paramList, "cos"));
        if (function == "tan") return tan(getOneDouble(paramList, "tan"));

        if (function == "asin") return asin(getOneDouble(paramList, "asin"));
        if (function == "acos") return acos(getOneDouble(paramList, "acos"));
        if (function == "atan") return atan(getOneDouble(paramList, "atan"));

        if (function == "sinh") return sinh(getOneDouble(paramList, "sinh"));
        if (function == "cosh") return cosh(getOneDouble(paramList, "cosh"));
        if (function == "tanh") return tanh(getOneDouble(paramList, "tanh"));

        if (function == "round")
        {
            if (paramList.size() == 1)
                return round(getOneDouble(paramList, "round"));
            if (paramList.size() != 2 || paramList[0].type() != object::NUMBER || paramList[1].type() != object::NUMBER)
                raiseError("round() works with a number and a number of places to round to");
            
            int slider = int(pow(10, int(paramList[1].numberVal())));
            return floor(paramList[0].numberVal() * slider) / slider;
        }

        //
        // Type Operations
        //
        if (function == "getType")
        {
            if (paramList.size() != 1)
                raiseError("getType() takes one parameter");
            return toWideStr(first.typeStr());
        }

        if (function == "number")
        {
            if (paramList.size() != 1)
                raiseError("number() takes one parameter");
            return first.toNumber();
        }

        if (function == "string")
        {
            if (paramList.size() != 1)
                raiseError("string() takes one parameter");
            return first.toString();
        }

        if (function == "list")
            return object::list(paramList);

        if (function == "index")
        {
            if ((paramList.size() % 2) != 0)
                raiseError("index() parameters must be an even count, key-value pairs");

            object::index newIndex;
            for (size_t i = 0; i < paramList.size(); i += 2)
            {
                if (newIndex.contains(paramList[i]))
                    raiseError("index() key repeated: " + num2str(double(i)));
                newIndex.set(paramList[i], paramList[i + 1]);
            }
            return newIndex;
        }

        if (function == "clone")
        {
            if (paramList.size() != 1)
                raiseError("clone() takes one parameter");
            return first.clone();
        }

        //
        // Collection Operations
        //
        if (function == "length")
        {
            if (paramList.size() != 1)
                raiseError("length() takes one parameter");
            return double(first.length());
        }

        if (function == "add")
        {
            if (first.type() == object::STRING)
            {
                for (int v = 1; v < paramList.size(); ++v)
                    first.stringVal() += paramList[v].toString();
                return first;
            }
            else if (first.type() == object::LIST)
            {
                auto& list = first.listVal();
                for (int v = 1; v < paramList.size(); ++v)
                    list.push_back(paramList[v]);
                return first;
            }
            else if (first.type() == object::INDEX)
            {
                if (((paramList.size() - 1) % 2) != 0)
                    raiseError("add() parameters for index must be an even count");

                auto& index = first.indexVal();
                for (int a = 1; a < paramList.size(); a += 2)
                {
                    if (index.contains(paramList[a]))
                        raiseError("add() index key repeated: " + num2str(a));
                    index.set(paramList[a], paramList[a + 1]);
                }
                return first;
            }
            else
                raiseError("add() only works with string, list, and index");
        }

        if (function == "set")
        {
            if (paramList.size() != 3)
                raiseError("set() works with an item, a key, and a value");

            if (first.type() != object::INDEX && paramList[1].type() != object::NUMBER)
                raiseError("set() key must be a numeric index");

            if (first.type() == object::STRING)
            {
                size_t idx = size_t(paramList[1].numberVal());
                if (idx >= first.stringVal().size())
                    raiseError("set() out of range");
                if (paramList[2].type() != object::STRING || paramList[2].stringVal().size() > 1)
                    raiseError("set() value must be a single character");
                first.stringVal()[idx] = paramList[2].stringVal()[0];
            }
            else if (first.type() == object::LIST)
            {
                size_t idx = size_t(paramList[1].numberVal());
                if (idx >= first.listVal().size())
                    raiseError("set() out of range");
                first.listVal()[idx] = paramList[2];
            }
            else if (first.type() == object::INDEX)
            {
                first.indexVal().set(paramList[1], paramList[2]);
            }
            else
                raiseError("set() only works with string, list, and index");
            return first;
        }

        if (function == "get")
        {
            if (paramList.size() != 2)
                raiseError("get() invalid argument count");
            
            if (first.type() == object::INDEX)
            {
                object key = paramList[1];
                auto& index = first.indexVal();
                if (!index.contains(key))
                    raiseError("get() key not found");
                return index.get(key);
            }

            if (paramList[1].type() != object::NUMBER)
                raiseError("get() parameter is not a number");

            int idx = int(paramList[1].numberVal());

            if (idx < 0 || idx >= first.length())
                raiseError("get() index is out of range");
            
            switch (first.type())
            {
            case object::STRING: return std::wstring{ first.stringVal()[idx] };
            case object::LIST: return first.listVal()[idx];
            default: raiseError("get() function only works with string, list, and index");
            }
        }

        if (function == "has")
        {
            if (paramList.size() != 2)
                raiseError("has() invalid parameter count");
            if (first.type() == object::STRING)
                return first.stringVal().find(paramList[1].toString()) != std::wstring::npos;
            else if (first.type() == object::LIST)
                return std::find(first.listVal().begin(), first.listVal().end(), paramList[1]) != first.listVal().end();
            else if (first.type() == object::INDEX)
                return first.indexVal().contains(paramList[1]);
            else
                raiseError("has() only works with string, list, and index");
        }

        if (function == "keys")
        {
            if (paramList.size() != 1)
                raiseError("keys() works with one index");
            if (first.type() != object::INDEX)
                raiseError("keys() works with index");
            return first.indexVal().keys(); // list == vector<object>, types match
        }

        if (function == "values")
        {
            if (paramList.size() != 1)
                raiseError("values() works with one index");
            if (first.type() != object::INDEX)
                raiseError("values() works with index");
            return first.indexVal().values(); // list == vector<object>, types match
        }

        if (function == "reversed")
        {
            if (paramList.size() != 1)
                raiseError("reversed() works with one item");

            if (first.type() == object::STRING)
            {
                auto copy = first.stringVal();
                std::reverse(copy.begin(), copy.end());
                return copy;
            }
            else if (first.type() == object::LIST)
            {
                auto copy = first.listVal();
                std::reverse(copy.begin(), copy.end());
                return copy;
            }
            else if (first.type() == object::INDEX)
            {
                auto keys = first.indexVal().keys();
                std::reverse(keys.begin(), keys.end());

                object::index newIndex;
                for (const auto& key : keys)
                    newIndex.set(key, first.indexVal().get(key));
                return newIndex;
            }
            else
                raiseError("reversed() only works with string, list, and index");
        }

        if (function == "sorted")
        {
            if (paramList.size() != 1)
                raiseError("sorted() works with one item");
            if (first.type() == object::STRING)
            {
                auto copy = first.stringVal();
                std::sort(copy.begin(), copy.end());
                return copy;
            }
            else if (first.type() == object::LIST)
            {
                auto copy = first.listVal();
                std::sort(copy.begin(), copy.end());
                return copy;
            }
            else if (first.type() == object::INDEX)
            {
                auto keys = first.indexVal().keys();
                std::sort(keys.begin(), keys.end());

                object::index newIndex;
                for (const auto& key : keys)
                    newIndex.set(key, first.indexVal().get(key));
                return newIndex;
            }
            else
                raiseError("sorted() only works with string, list, and index");
        }

        //
        // Strings
        //
        if (function == "join")
        {
            if (paramList.size() > 2)
                raiseError("join() takes item to work with, and optional separator");
            if (first.type() != object::LIST)
                raiseError("join() only works with list");

            std::wstring separator = paramList.size() == 2 ? paramList[1].toString() : L"";
            std::vector<std::wstring> strings;
            strings.reserve(first.listVal().size());
            for (const auto& obj : first.listVal())
                strings.push_back(obj.toString());
            return join(strings, separator.c_str());
        }

        if (function == "split")
        {
            if
            (
                paramList.size() != 2 
                || 
                first.type() != object::STRING 
                || 
                paramList[1].type() != object::STRING
            )
            {
                raiseError("split() works with an item and separator");
            }

            std::wstring separator = paramList[1].stringVal();
            auto splitted = split(first.stringVal(), separator.c_str());
            object::list splittedObjs;
            splittedObjs.reserve(splitted.size());
            for (const auto& str : splitted)
                splittedObjs.push_back(str);
            return splittedObjs;
        }

        if (function == "trim")
        {
            if (paramList.size() != 1 || first.type() != object::STRING)
                raiseError("trim() works one string");
            return trim(first.stringVal());
        }

        if (function == "toUpper")
        {
            if (paramList.size() != 1 || first.type() != object::STRING)
                raiseError("toUpper() works with one string");
            auto str = first.stringVal();
            for (auto& c : str)
                c = towupper(c);
            return str;
        }

        if (function == "toLower")
        {
            if (paramList.size() != 1 || first.type() != object::STRING)
                raiseError("toLower() works with one string");
            auto str = first.stringVal();
            for (auto& c : str)
                c = towlower(c);
            return str;
        }

        if (function == "replaced")
        {
            if
            (
                paramList.size() != 3
                ||
                (paramList[0].type() != object::STRING)
                ||
                (paramList[1].type() != object::STRING)
                ||
                (paramList[2].type() != object::STRING)
            )
            {
                raiseError("replaced() takes a string, a string to find, and a string to replace it with");
            }

            std::wstring input = paramList[0].stringVal();
            std::wstring toFind = paramList[1].stringVal();
            std::wstring toReplaceWith = paramList[2].stringVal();
            input = replace(input, toFind, toReplaceWith);
            return input;
        }

        if (function == "random")
        {
            if
            (
                paramList.size() != 2
                ||
                paramList[0].type() != object::NUMBER
                ||
                paramList[1].type() != object::NUMBER
            )
            {
                raiseError("random() takes two number parameters, min and max");
            }

            double parm1 = paramList[0].numberVal();
            double parm2 = paramList[1].numberVal();
 
            double min = std::min(parm1, parm2);
            double max = std::max(parm1, parm2);
            
            double rnd = (double(rand()) / RAND_MAX) * (max - min) + min;
            return rnd;
        }

        //
        // Searching and Slicing
        //
        if (function == "firstLocation")
        {
            if (paramList.size() != 2)
                raiseError("firstLocation() works with an item to look in and a key to look for");

            if (first.type() == object::STRING)
            {
                if (paramList[1].type() != object::STRING)
                    raiseError("firstLocation() invalid parameter");
                size_t idx = first.stringVal().find(paramList[1].stringVal());
                if (idx == std::wstring::npos)
                    return double(-1);
                else
                    return double(idx);
            }
            else if (first.type() == object::LIST)
            {
                const auto& l = first.listVal();
                for (size_t i = 0; i < l.size(); ++i)
                {
                    if (l[i] == paramList[1])
                        return double(i);
                }
                return double(-1);
            }
            else
                raiseError("firstLocation() only works with string and list");
        }

        if (function == "lastLocation")
        {
            if (paramList.size() != 2)
                raiseError("lastLocation() works with an item to look in and a key to look for");

            if (first.type() == object::STRING)
            {
                if (paramList[1].type() != object::STRING)
                    raiseError("lastLocation() invalid parameter");
                size_t idx = first.stringVal().rfind(paramList[1].stringVal());
                if (idx == std::wstring::npos)
                    return double(-1);
                else
                    return double(idx);
            }
            else if (first.type() == object::LIST)
            {
                const auto& l = first.listVal();
                for (int i = int(l.size()) - 1; i >= 0; --i)
                {
                    if (l[i] == paramList[1])
                        return double(i);
                }
                return double(-1);
            }
            else
                raiseError("lastLocation() only works with string and list");
        }

        if (function == "subset")
        {
            if (paramList.size() != 2 && paramList.size() != 3)
                raiseError("subset() works with an item and a start index and an optional length");
            
            int startIndex;
            {
                if (paramList[1].type() != object::NUMBER)
                    raiseError("subset() start index must be number");
                startIndex = int(paramList[1].numberVal());
                if (startIndex < 0)
                    raiseError("subset() start index must be greater than or equal zero");
            }

            if (paramList.size() == 2)
            {
                if (first.type() == object::STRING)
                {
                    const auto& s = first.stringVal();
                    if (startIndex >= s.size())
                        raiseError("subset() start index must be less than the length of the string");
                    return s.substr(startIndex);
                }
                else if (first.type() == object::LIST)
                {
                    const auto& list = first.listVal();
                    if (startIndex >= list.size())
                        raiseError("subset() start index must be less than the length of the list");
                    object::list subset;
                    for (int i = startIndex; i < list.size(); ++i)
                        subset.push_back(list[i]);
                    return subset;
                }
                else
                    raiseError("subset() works with string and list");
            }
            else
            {
                int length;
                {
                    if (paramList[2].type() != object::NUMBER)
                        raiseError("substring() invalid arguments");
                    length = int(paramList[2].numberVal());
                    if (length < 0)
                        raiseError("subset() length must be greater than or equal zero");
                }

                if (first.type() == object::STRING)
                {
                    const auto& s = first.stringVal();
                    if (startIndex >= s.size())
                        raiseError("subset() start index must be less than the length of the string");
                    return s.substr(startIndex, length);
                }
                else if (first.type() == object::LIST)
                {
                    const auto& list = first.listVal();
                    if (startIndex >= list.size())
                        raiseError("subset() start index must be less than the length of the list");
                    object::list subset;
                    int endIndex = std::min(int(list.size()) - 1, startIndex + length - 1);
                    if (endIndex >= list.size())
                        raiseError("subset() end index must be less than the length of the list");
                    for (int i = startIndex; i <= endIndex; ++i)
                        subset.push_back(list[i]);
                    return subset;
                }
                else
                    raiseError("subset() works with string and list");
            }
        }

        //
        // Regular Expressions
        //
        if (function == "isMatch")
        {
            if (paramList.size() != 2)
                raiseError("isMatch() works string to match and pattern");
            if (paramList[0].type() != object::STRING)
                raiseError("isMatch() only works with string input");
            if (paramList[1].type() != object::STRING)
                raiseError("isMatch() only works with string input");

            std::wregex re(paramList[1].stringVal());
            return std::regex_match(paramList[0].stringVal(), re);
        }

        if (function == "getMatches")
        {
            if (paramList.size() != 2)
                raiseError("getMatches() works string to match and pattern");
            if (paramList[0].type() != object::STRING)
                raiseError("getMatches() only works with string input");
            if (paramList[1].type() != object::STRING)
                raiseError("getMatches() only works with string input");
                
            std::wregex re(paramList[1].stringVal());

            object::list output;
            std::wsmatch sm;
            std::regex_match(paramList[0].stringVal(), sm, re);
            for (const auto& m : sm)
                output.push_back(m.str());
            return output;
        }

        //
        // Process Control
        //
        if (function == "exec")
        {
            if (paramList.size() != 1 || paramList[0].type() != object::STRING)
                raiseError("process() works with one command string");

            object::index retVal;
            retVal.set(toWideStr("success"), false);
            retVal.set(toWideStr("exit_code"), -1.0);
            retVal.set(toWideStr("output"), toWideStr(""));

            FILE* file = _wpopen(paramList[0].stringVal().c_str(), L"rt");
            if (file == nullptr)
                return retVal;

            char buffer[4096];
            std::string output;
            while (fgets(buffer, sizeof(buffer), file))
                output.append(buffer);
            retVal.set(toWideStr("output"), toWideStr(output));

            retVal.set(toWideStr("success"), bool(feof(file)));

            int result = _pclose(file);
            retVal.set(toWideStr("exit_code"), double(result));
            file = nullptr;

            return retVal;
        }

        if (function == "exit")
        {
            if (paramList.size() != 1 || first.type() != object::NUMBER)
                raiseError("exit() works with one exit code number");
            exit(int(first.numberVal()));
        }

        if (function == "error")
        {
            if (paramList.size() != 1 || first.type() != object::STRING)
                raiseError("error() works with one error message string");
            throw user_exception(toNarrowStr(first.stringVal()));
        }

        //
        // File I/O
        //
        if (function == "readFile")
        {
            if (paramList.size() != 2
                || paramList[0].type() != object::STRING
                || paramList[1].type() != object::STRING)
            {
                raiseError("readFile() works with a file path string and an encoding string");
            }

            std::wstring filePath = paramList[0].stringVal();
            std::wstring encoding = paramList[1].stringVal();

            if (encoding == L"ascii")
            {
                std::ifstream file(filePath);
                if (!file)
                    return object();

                std::stringstream stream;
                stream << file.rdbuf();
                
                std::string output = stream.str();
                return toWideStr(output);
            }

            if (encoding == L"utf-8" || encoding == L"utf-16")
            {
                std::wifstream file(filePath);
                if (encoding == L"utf-8")
                    file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
                if (!file)
                    return object();

                std::wstringstream stream;
                stream << file.rdbuf();

                std::wstring output = stream.str();
                return output;
            }

            raiseError("Unsupported readFile() encoding: must be ascii, utf-8, or utf-16");
        }

        if (function == "writeFile")
        {
            if (paramList.size() != 3
                || paramList[0].type() != object::STRING
                || paramList[1].type() != object::STRING
                || paramList[2].type() != object::STRING)
            {
                raiseError("writeFile() works with a file path string, file contents string, and an encoding string");
            }

            std::wstring filePath = paramList[0].stringVal();
            std::wstring contents = paramList[1].stringVal();
            std::wstring encoding = paramList[2].stringVal();

            if (encoding == L"ascii")
            {
                std::ofstream file(filePath, std::ofstream::trunc);
                if (!file)
                    return false;

                std::string narrow = toNarrowStr(contents);
                file.write(narrow.c_str(), narrow.size());
                return true;
            }

            if (encoding == L"utf-8" || encoding == L"utf-16")
            {
                std::wofstream file(filePath, std::wofstream::trunc);
                if (encoding == L"utf-8")
                    file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
                if (!file)
                    return false;
                file.write(contents.c_str(), contents.size());
                file.close();
                return true;
            }

            raiseError("Unsupported writeFile() encoding: must be ascii, utf-8, or utf-16");
        }

        if (m_callable.hasFunction(functionW))
        {
            object answer = m_callable.callFunction(functionW, paramList);
            return answer;
        }

        if (m_symbols.contains(functionW)) // function name ~= function pointer
        {
            object resolvedFunctionObj = m_symbols.get(functionW);
            if (resolvedFunctionObj.type() != object::STRING)
                raiseWError(L"Function name not a string: " + resolvedFunctionObj.toString());
            std::wstring resolvedFunction = resolvedFunctionObj.stringVal();
            return executeFunction(resolvedFunction, paramList);
        }

        // Do object-like member function-ish expression processing
        size_t dotIndex = function.find('.');
        if (dotIndex != std::string::npos)
        {
            std::wstring symbol = functionW.substr(0, dotIndex);
            std::wstring memberFunc = functionW.substr(dotIndex + 1);

            object value;
            if (m_symbols.tryGet(symbol, value))
            {
                object::list newVals;
                newVals.push_back(value);
                newVals.insert(newVals.end(), paramList.begin(), paramList.end());
                return executeFunction(memberFunc, newVals);
            }
        }

        // Not a function we know about after all
        raiseError("Function not defined: " + function);
    }
}
