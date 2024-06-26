#include "pch.h"
#include "expressions.h"
#include "utils.h"
#include "names.h"
#include "user_exception.h"
#include "exe_version.h"
#include "parse_args.h"
#include "object_json.h"
#include "lib.h"

#undef min
#undef max

namespace mscript
{
    /// <summary>
    /// These are the binary operators supported by the expression processor
    /// They are in operator precedence order, least to most
    /// </summary>
    std::vector<std::string> sm_ops
    {
        "||",
        "OR",

        "&&",
        "AND",

        "<>",
        "!=",
        "NEQ",

        "<=",
        "LEQ",

        ">=",
        "GEQ",

        "<",
        "LSS",

        ">",
        "GTR",

        "==",
        "=",
        "EQU",

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
        std::string upper = toNarrowStr(toUpper(expStr));
        std::string narrow = toNarrowStr(expStr);

        // Check for easy stuff
        if (narrow.empty())
            raiseError("Empty expression");

        if (upper == "NULL")
            return object::NOTHING;

        if (upper == "TRUE")
            return true;

        if (upper == "FALSE")
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

        if (upper == "DQUOTE")
            return toWideStr("\"");

        if (upper == "SQUOTE")
            return toWideStr("\'");

        if (upper == "TAB")
            return toWideStr("\t");

        if (upper == "CR")
            return toWideStr("\f");

        if (upper == "LF")
            return toWideStr("\n");

        if (upper == "CRLF")
            return toWideStr("\r\n");

        if (upper == "ESC")
            return toWideStr("\033");

        if (upper == "PI")
            return M_PI;

        if (upper == "E")
            return M_E;

        if (upper == "TRACE_NONE")
            return double(TRACE_NONE);

        if (upper == "TRACE_CRITICAL")
            return double(TRACE_CRITICAL);

        if (upper == "TRACE_ERROR")
            return double(TRACE_ERROR);

        if (upper == "TRACE_WARNING")
            return double(TRACE_WARNING);

        if (upper == "TRACE_INFO")
            return double(TRACE_INFO);

        if (upper == "TRACE_DEBUG")
            return double(TRACE_DEBUG);

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
            size_t opLen = op.length();

            bool is_op_alpha = true;
            for (size_t p = 0; p < opLen; ++p)
            {
                if (!isalpha(op[p])) 
                {
                    is_op_alpha = false;
                    break;
                }
            }

            size_t parenCount = 0;

            bool inSingleString = false;
            bool inDoubleString = false;

            // Walk the expression from the end, so that we bind to
            // the right-side of the expression
            int expStrSize = int(expStr.length());
            for (int idx = expStrSize - 1; idx >= 0; --idx)
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
                if (toupper(c) == op[0]) // quick filter
                {
                    if (opLen == 1)
                    {
                        opMatches = true;
                    }
                    else if (opLen == 2)
                    {
                        opMatches = idx < expStrSize - 1 && toupper(expStr[idx + 1]) == op[1];
                    }
                    else if (opLen == 3)
                    {
                        opMatches = 
                            idx < expStrSize - 2
                            &&
                            toupper(expStr[idx + 1]) == op[1]
                            && 
                            toupper(expStr[idx + 2]) == op[2];
                    }
                    else
                        raiseWError(L"Invalid operator length: " + expStr);
                }
                if (opMatches && is_op_alpha)
                {
                    // don't honor alpha ops inside other alpha stuff
                    // like some_request has an equ in it
                    // in fact, let's require that alpha operators have spaces around them
                    opMatches = 
                        isCharAlphaOpBoundary(idx > 0 ? expStr[idx - 1] : 0) 
                        && 
                        isCharAlphaOpBoundary(idx < expStrSize - int(opLen) ? expStr[idx + opLen] : 0);
                }
                if (opMatches)
                {
                    // Make sure our operator isn't some 5E+5 nonsense
                    if (isOperator(trim(expStr), op, idx))
                    {
                        object value;

                        // Split the string into left and right parts
                        std::wstring leftStr = expStr.substr(0, idx);
                        std::wstring rightStr = expStr.substr(idx + opLen);

                        // Evaluate the left part
                        object leftVal = evaluate(leftStr);

                        // Short circuitry
                        if ((op == "&&" || op == "AND") && !leftVal.boolVal())
                        {
                            value = false;
                        }
                        else if ((op == "||" || op == "OR") && leftVal.boolVal())
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
                                if (op == "=" || op == "==" || op == "EQU")
                                    value = leftVal == rightVal;
                                else if (op == "!=" || op == "<>" || op == "NEQ")
                                    value = leftVal != rightVal;
                                else
                                    raiseWError(L"Invalid operator for null values: " + expStr);
                            }
                            // Handle string on either side, string promotion
                            else if (leftVal.type() == object::STRING || rightVal.type() == object::STRING)
                            {
                                std::wstring leftValStr = leftVal.toString();
                                std::wstring rightValStr = rightVal.toString();

                                if (opLen == 1)
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
                                else if (opLen == 2)
                                {
                                    if (op == "==")
                                        value = leftValStr == rightValStr;
                                    else if (op == "!=" || op == "<>")
                                        value = leftValStr != rightValStr;
                                    else if (op == "<=")
                                        value = leftValStr <= rightValStr;
                                    else if (op == ">=")
                                        value = leftValStr >= rightValStr;
                                    else
                                        raiseWError(L"Unrecognized string operator: " + expStr);
                                }
                                else if (opLen == 3)
                                {
                                    if (_stricmp(op.c_str(), "EQU") == 0)
                                        value = leftValStr == rightValStr;
                                    else if (_stricmp(op.c_str(), "NEQ") == 0)
                                        value = leftValStr != rightValStr;
                                    else if (_stricmp(op.c_str(), "LSS") == 0)
                                        value = leftValStr < rightValStr;
                                    else if (_stricmp(op.c_str(), "LEQ") == 0)
                                        value = leftValStr <= rightValStr;
                                    else if (_stricmp(op.c_str(), "GTR") == 0)
                                        value = leftValStr > rightValStr;
                                    else if (_stricmp(op.c_str(), "GEQ") == 0)
                                        value = leftValStr >= rightValStr;
                                    else
                                        raiseWError(L"Unrecognized string operator: " + expStr);
                                }
                                else
                                    raiseWError(L"Unrecognized string operator: " + expStr);
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
                                    if (opLen == 1)
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
                                    else if (opLen == 2)
                                    {
                                        if (op == "==")
                                            value = leftNum == rightNum;
                                        else if (op == "!=" || op == "<>")
                                            value = leftNum != rightNum;
                                        else if (op == "<=")
                                            value = leftNum <= rightNum;
                                        else if (op == ">=")
                                            value = leftNum >= rightNum;
                                        else
                                            raiseWError(L"Unrecognized numeric operator: " + expStr);
                                    }
                                    else if (opLen == 3)
                                    {
                                        if (_stricmp(op.c_str(), "EQU") == 0)
                                            value = leftNum == rightNum;
                                        else if (_stricmp(op.c_str(), "NEQ") == 0)
                                            value = leftNum != rightNum;
                                        else if (_stricmp(op.c_str(), "LSS") == 0)
                                            value = leftNum < rightNum;
                                        else if (_stricmp(op.c_str(), "LEQ") == 0)
                                            value = leftNum <= rightNum;
                                        else if (_stricmp(op.c_str(), "GTR") == 0)
                                            value = leftNum > rightNum;
                                        else if (_stricmp(op.c_str(), "GEQ") == 0)
                                            value = leftNum >= rightNum;
                                        else
                                            raiseWError(L"Unrecognized numeric operator: " + expStr);
                                    }
                                    else
                                        raiseWError(L"Unrecognized numeric operator: " + expStr);
                                }
                            }
                            // Bools are easy
                            else if (leftVal.type() == object::BOOL && rightVal.type() == object::BOOL)
                            {
                                bool leftBool = leftVal.boolVal();
                                bool rightBool = rightVal.boolVal();

                                if (op == "&&" || op == "AND")
                                    value = leftBool && rightBool;
                                else if (op == "||" || op == "OR")
                                    value = leftBool || rightBool;
                                else if (op == "=" || op == "==" || op == "EQU")
                                    value = leftBool == rightBool;
                                else if (op == "!=" || op == "<>" || op == "NEQ")
                                    value = leftBool != rightBool;
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
        else if (startsWith(toUpper(expStr), L"NOT "))
        {
            static size_t notLen = strlen("NOT ");
            object answer = evaluate(expStr.substr(notLen));
            return !answer.boolVal();
        }

        // Deal with parens, including function calls
        auto trimmed_exp_str = trim(expStr);
        size_t leftParen = trimmed_exp_str.find('(');
        if (trimmed_exp_str.size() > 2 && leftParen != std::wstring::npos && trimmed_exp_str.back() == ')')
        {
            std::wstring functionName = trim(expStr.substr(0, leftParen));

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

    bool expression::isCharAlphaOpBoundary(wchar_t c)
    {
        return c == ' '; // || c == '(' || c == ')' || c == 0;
    }

    bool expression::isOperator(const std::wstring& expr, const std::string& op, int n)
    {
        if (expr.empty())
            return true;

        if (n < 0 || n >= int(expr.length()))
            return true;

        if
        (
            op == "&&" 
            || 
            op == "||"
            || 
            _stricmp(op.c_str(), "AND") == 0
            || 
            _stricmp(op.c_str(), "OR") == 0
        )
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
        if (searchLen > int(source.length()))
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

    object expression::executeFunction(std::wstring functionW, const object::list& paramList)
    {
        functionW = toLower(functionW);
        const std::string function = toNarrowStr(functionW);

        object first = paramList.size() == 0 ? object::NOTHING : paramList[0];

        static std::unordered_map<std::string, std::function<object(object& first, const object::list& paramList)>> functions
        {
            //
            // Math
            //
            { "abs", [](object& first, const object::list& paramList) -> object { (void)first; return abs(getOneDouble(paramList, "abs")); }},
            { "sqrt", [](object& first, const object::list& paramList) -> object { (void)first; return sqrt(getOneDouble(paramList, "sqrt")); }},

            { "ceil", [](object& first, const object::list& paramList) -> object { (void)first; return ceil(getOneDouble(paramList, "ceil")); }},
            { "floor", [](object& first, const object::list& paramList) -> object { (void)first; return floor(getOneDouble(paramList, "floor")); }},

            { "exp", [](object& first, const object::list& paramList) -> object { (void)first; return exp(getOneDouble(paramList, "exp")); }},
            { "log", [](object& first, const object::list& paramList) -> object { (void)first; return log(getOneDouble(paramList, "log")); }},
            { "log2", [](object& first, const object::list& paramList) -> object { (void)first; return log2(getOneDouble(paramList, "log2")); }},
            { "log10", [](object& first, const object::list& paramList) -> object { (void)first; return log10(getOneDouble(paramList, "log10")); }},

            { "sin", [](object& first, const object::list& paramList) -> object { (void)first; return sin(getOneDouble(paramList, "sin")); }},
            { "cos", [](object& first, const object::list& paramList) -> object { (void)first; return cos(getOneDouble(paramList, "cos")); }},
            { "tan", [](object& first, const object::list& paramList) -> object { (void)first; return tan(getOneDouble(paramList, "tan")); }},

            { "asin", [](object& first, const object::list& paramList) -> object { (void)first; return asin(getOneDouble(paramList, "asin")); }},
            { "acos", [](object& first, const object::list& paramList) -> object { (void)first; return acos(getOneDouble(paramList, "acos")); }},
            { "atan", [](object& first, const object::list& paramList) -> object { (void)first; return atan(getOneDouble(paramList, "atan")); }},

            { "sinh", [](object& first, const object::list& paramList) -> object { (void)first; return sinh(getOneDouble(paramList, "sinh")); }},
            { "cosh", [](object& first, const object::list& paramList) -> object { (void)first; return cosh(getOneDouble(paramList, "cosh")); }},
            { "tanh", [](object& first, const object::list& paramList) -> object { (void)first; return tanh(getOneDouble(paramList, "tanh")); }},

            { "round", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() == 1)
                    return round(getOneDouble(paramList, "round"));
                if (paramList.size() != 2 || first.type() != object::NUMBER || paramList[1].type() != object::NUMBER)
                    raiseError("round() works with a number and a number of places to round to");

                int slider = int(pow(10, int(paramList[1].numberVal())));
                return floor(first.numberVal() * slider) / slider;
            }},

            //
            // Type Operations
            //
            { "gettype", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1)
                    raiseError("getType() takes one parameter");
                return toWideStr(first.typeStr());
            }},

            { "number", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1)
                    raiseError("number() takes one parameter");
                return first.toNumber();
            }},

            { "string", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1)
                    raiseError("string() takes one parameter");
                return first.toString();
            }},

            { "list", [](object& first, const object::list& paramList) -> object {
                (void)first;
                return object::list(paramList);
            }},

            { "index", [](object& first, const object::list& paramList) -> object {
                (void)first;
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
            }},

            { "clone", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1)
                    raiseError("clone() takes one parameter");
                return first.clone();
            }},

            //
            // Collection Operations
            //
            { "length", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1)
                    raiseError("length() takes one parameter");
                return double(first.length());
            }},

            { "add", [](object& first, const object::list& paramList) -> object {
                if (first.type() == object::STRING)
                {
                    for (int v = 1; v < int(paramList.size()); ++v)
                        first.stringVal() += paramList[v].toString();
                    return first;
                }
                else if (first.type() == object::LIST)
                {
                    auto& list = first.listVal();
                    for (int v = 1; v < int(paramList.size()); ++v)
                        list.push_back(paramList[v]);
                    return first;
                }
                else if (first.type() == object::INDEX)
                {
                    if (((paramList.size() - 1) % 2) != 0)
                        raiseError("add() parameters for index must be an even count");

                    auto& index = first.indexVal();
                    for (int a = 1; a < int(paramList.size()); a += 2)
                    {
                        if (index.contains(paramList[a]))
                            raiseError("add() index key repeated: " + num2str(a));
                        index.set(paramList[a], paramList[a + 1]);
                    }
                    return first;
                }
                else
                    raiseError("add() only works with string, list, and index");
            }},
            
            { "set", [](object& first, const object::list& paramList) -> object {
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
            }},

            { "get", [](object& first, const object::list& paramList) -> object {
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

                if (idx < 0 || idx >= int(first.length()))
                    raiseError("get() index is out of range");

                switch (first.type())
                {
                case object::STRING: return object(std::wstring{ first.stringVal()[idx] });
                case object::LIST: return first.listVal()[idx];
                default: raiseError("get() function only works with string, list, and index");
                }
            } },

            { "has", [](object& first, const object::list& paramList) -> object {
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
            }},

            { "keys", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::INDEX)
                    raiseError("keys() works with one index");
                else
                    return first.indexVal().keys(); // list == vector<object>, types match
            }},

            { "values", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::INDEX)
                    raiseError("values() works with one index");
                else
                    return first.indexVal().values(); // list == vector<object>, types match
            } },

            { "reversed", [](object& first, const object::list& paramList) -> object {
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
            }},

            { "sorted", [](object& first, const object::list& paramList) -> object {
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
            }},

            //
            // Strings
            //
            { "join", [](object& first, const object::list& paramList) -> object {
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
            } },

            { "split", [](object& first, const object::list& paramList) -> object {
                if
                (
                    paramList.size() != 2
                    ||
                    first.type() != object::STRING
                    ||
                    paramList[1].type() != object::STRING
                )
                {
                    raiseError("split() works with an item and a separator string");
                }

                auto splitted = split(first.stringVal(), paramList[1].stringVal());
                object::list splittedObjs;
                splittedObjs.reserve(splitted.size());
                for (const auto& str : splitted)
                    splittedObjs.push_back(str);
                return splittedObjs;
            } },

            { "splitlines", [](object& first, const object::list& paramList) -> object {
                if
                (
                    paramList.size() != 1
                    ||
                    first.type() != object::STRING
                )
                {
                    raiseError("splitLines() works with a string parameter");
                }

                auto splitted = split(replace(first.stringVal(), L"\r\n", L"\n"), L"\n");
                object::list splittedObjs;
                splittedObjs.reserve(splitted.size());
                for (const auto& str : splitted)
                    splittedObjs.push_back(str);
                return splittedObjs;
            } },

            { "trimmed", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::STRING)
                    raiseError("trimmed() works with one string");
                return trim(first.stringVal());
            }},

            { "toupper", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::STRING)
                    raiseError("toUpper() works with one string");
                auto str = first.stringVal();
                for (auto& c : str)
                    c = towupper(c);
                return str;
            } },

            { "tolower", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::STRING)
                    raiseError("toLower() works with one string");
                auto str = first.stringVal();
                for (auto& c : str)
                    c = towlower(c);
                return str;
            } },

            { "replaced", [](object& first, const object::list& paramList) -> object {
                (void)first;
                if
                (
                    paramList.size() < 3
                    ||
                    ((paramList.size() - 1) % 2) != 0
                    ||
                    (paramList[0].type() != object::STRING)
                )
                {
                    raiseError("replaced() takes a string, a string to find, and a string to replace it with");
                }

                std::wstring input = paramList[0].stringVal();
                for (size_t r = 1; r < paramList.size(); r += 2)
                {
                    if
                    (
                        (paramList[r].type() != object::STRING)
                        ||
                        (paramList[r + 1].type() != object::STRING)
                    )
                    {
                        raiseError("replaced() replacement requires string to find, and string to replace it with");
                    }
                    std::wstring toFind = paramList[r].stringVal();
                    std::wstring toReplaceWith = paramList[r + 1].stringVal();
                    input = replace(input, toFind, toReplaceWith);
                }
                return input;
            } },

            { "random", [](object& first, const object::list& paramList) -> object {
                (void)first;
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
            } },

            { "fmt", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() < 1 || first.type() != object::STRING)
                    raiseError("fmt() one string, and other parameters to insert");
                std::wstring format = first.stringVal();
                for (size_t p = 1; p < paramList.size(); ++p)
                    format = replace(format, L"{" + std::to_wstring(p - 1) + L"}", paramList[p].toString());
                return format;
            } },
                
            //
            // Searching and Slicing
            //
            { "firstlocation", [](object& first, const object::list& paramList) -> object {
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
            } },

            { "lastlocation", [](object& first, const object::list& paramList) -> object {
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
            } },

            { "subset", [](object& first, const object::list& paramList) -> object {
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
                        if (startIndex >= int(s.size()))
                            raiseError("subset() start index must be less than the length of the string");
                        else
                            return s.substr(startIndex);
                    }
                    else if (first.type() == object::LIST)
                    {
                        const auto& list = first.listVal();
                        if (startIndex >= int(list.size()))
                            raiseError("subset() start index must be less than the length of the list");
                        object::list subset;
                        for (int i = startIndex; i < int(list.size()); ++i)
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
                        if (startIndex >= int(s.size()))
                            raiseError("subset() start index must be less than the length of the string");
                        else
                            return s.substr(startIndex, length);
                    }
                    else if (first.type() == object::LIST)
                    {
                        const auto& list = first.listVal();
                        if (startIndex >= int(list.size()))
                            raiseError("subset() start index must be less than the length of the list");
                        object::list subset;
                        int endIndex = std::min(int(list.size()) - 1, startIndex + length - 1);
                        if (endIndex >= int(list.size()))
                            raiseError("subset() end index must be less than the length of the list");
                        for (int i = startIndex; i <= endIndex; ++i)
                            subset.push_back(list[i]);
                        return subset;
                    }
                    else
                        raiseError("subset() works with string and list");
                }
            } },

            //
            // Regular Expressions
            //
            { "ismatch", [](object& first, const object::list& paramList) -> object {
                (void)first;
                bool full_match = false;
                if (paramList.size() < 2 || paramList.size() > 3)
                    raiseError("isMatch() works with a string to match and pattern and optional allow substring match");
                else if (paramList[0].type() != object::STRING)
                    raiseError("isMatch() only works with string input");
                else if (paramList[1].type() != object::STRING)
                    raiseError("isMatch() only works with string input");
                else if (paramList.size() == 3)
                {
                    if (paramList[2].type() != object::BOOL)
                        raiseError("isMatch() optional parameter to require full string match must be bool");
                    else
                        full_match = paramList[2].boolVal();
                }

                std::wregex re(paramList[1].stringVal());
                return 
                    full_match
                    ? std::regex_match(first.stringVal(), re)
                    : std::regex_search(first.stringVal(), re);
            } },

            { "getmatches", [](object& first, const object::list& paramList) -> object {
                bool full_match = false;
                if (paramList.size() < 2 || paramList.size() > 3)
                    raiseError("getMatches() works string to match and pattern");
                if (paramList[0].type() != object::STRING)
                    raiseError("getMatches() only works with string input");
                if (paramList[1].type() != object::STRING)
                    raiseError("getMatches() only works with string input");
                else if (paramList.size() == 3)
                {
                    if (paramList[2].type() != object::BOOL)
                        raiseError("getMatches() optional parameter to require full string match must be bool");
                    else
                        full_match = paramList[2].boolVal();
                }

                std::wregex re(paramList[1].stringVal());

                std::wsmatch sm;
                if (full_match)
                    std::regex_match(first.stringVal(), sm, re);
                else
                    std::regex_search(first.stringVal(), sm, re);

                object::list output;
                for (const auto& m : sm)
                    output.push_back(m.str());
                return output;
            } },

            { "getmatchlength", [](object& first, const object::list& paramList) -> object {
                (void)first;
                bool full_match = false;
                if (paramList.size() < 2 || paramList.size() > 3)
                    raiseError("getMatchLength() works with a string to match and pattern and optional allow substring match");
                else if (paramList[0].type() != object::STRING)
                    raiseError("getMatchLength() only works with string input");
                else if (paramList[1].type() != object::STRING)
                    raiseError("getMatchLength() only works with string input");
                else if (paramList.size() == 3)
                {
                    if (paramList[2].type() != object::BOOL)
                        raiseError("getMatchLength() optional parameter to require full string match must be bool");
                    else
                        full_match = paramList[2].boolVal();
                }

                std::wregex re(paramList[1].stringVal());

                std::wsmatch sm;
                if (full_match)
                    std::regex_match(first.stringVal(), sm, re);
                else
                    std::regex_search(first.stringVal(), sm, re);

                object::list output;
                for (const auto& m : sm)
                    return double(m.str().length());
                return double(-1);
            } },

            //
            // Process Control
            //
            { "exec", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() < 1 || first.type() != object::STRING)
                    raiseError("exec() works with a command string");

                if (paramList.size() == 2 && paramList[1].type() != object::INDEX)
                    raiseError("exec() works with a command string and an optional index");

                object::index options;
                if (paramList.size() >= 2)
                    options = paramList[1].indexVal();

                object::index retVal;
                retVal.set(toWideStr("success"), false);
                retVal.set(toWideStr("exit_code"), -1.0);
                retVal.set(toWideStr("output"), toWideStr(""));

                std::wstring method;
                bool ignore_errors = false;
                for (auto option : options.vec())
                {
                    if (option.first.type() != object::STRING)
                        raiseWError(L"Invalid option for exec() function; key is not a string: " + option.first.toString());

                    std::string optionName = toNarrowStr(option.first.stringVal());
                    if (optionName == "method")
                    {
                        object methodObj = option.second;
                        if (methodObj.type() != object::STRING)
                            raiseError("exec() method option must be a string, popen or system");
                        else
                            method = methodObj.stringVal();
                    }
                    else if (optionName == "ignore_errors")
                    {
                        object flagObj = option.second;
                        if (flagObj.type() != object::BOOL)
                            raiseError("exec() ignore_errors option must be true or false");
                        else
                            ignore_errors = flagObj.boolVal();
                    }
                    else
                        raiseError("Invalid option to exec(): " + optionName);
                }

                int exit_code = -1;
                if (method.empty() || method == L"popen")
                {
                    FILE* file = _wpopen(first.stringVal().c_str(), L"rt");
                    if (file == nullptr)
                        return retVal;

                    char buffer[4096];
                    std::string output;
                    while (fgets(buffer, sizeof(buffer), file))
                        output.append(buffer);
                    retVal.set(toWideStr("output"), toWideStr(output));

                    retVal.set(toWideStr("success"), bool(feof(file)));

                    exit_code = _pclose(file);
                    file = nullptr;
                }
                else if (method == L"system")
                {
                    exit_code = ::_wsystem(first.stringVal().c_str());
                    retVal.set(toWideStr("success"), true);
                }
                else
                    raiseError("exec() invalid method, must be popen or system");

                retVal.set(toWideStr("exit_code"), double(exit_code));

                if (!ignore_errors)
                {
                    if (!retVal.get(toWideStr("success")).boolVal())
                        raiseError("exec() failed executing command");
                    else if (exit_code != 0)
                        raiseError("exec() failed with exit code " + std::to_string(exit_code));
                }

                return retVal;
            } },

            { "system", [this](object& first, const object::list& paramList) -> object {
                if
                (
                    paramList.empty()
                    ||
                    paramList.size() > 2
                    ||
                    first.type() != object::object_type::STRING
                    ||
                    (paramList.size() == 2 && paramList[1].type() != object::object_type::BOOL)
                )
                {
                    raiseError("system() takes one string expression for the command to run, "
                               "and a boolean for whether to raise errors on failure");
                }

                // raise errors by default, users to pass true to suppress them
                bool raise_errors = paramList.size() < 2 || !paramList[1].boolVal();
                int exit_code = ::_wsystem(first.stringVal().c_str());
                if (raise_errors && exit_code != 0)
                    raiseError("system() failed with exit code " + std::to_string(exit_code));
                return object(double(exit_code));
            } },

            { "popen", [](object& first, const object::list& paramList) -> object {
                if
                (
                    paramList.empty()
                    ||
                    paramList.size() > 2
                    ||
                    first.type() != object::object_type::STRING
                    ||
                    (paramList.size() == 2 && paramList[1].type() != object::object_type::BOOL)
                )
                {
                    raiseError("popen() takes one string expression for the command to run, "
                               "and a boolean for whether to raise errors on failure");
                }

                // raise errors by default, users to pass true to suppress them
                bool raise_errors = paramList.size() < 2 || !paramList[1].boolVal();

                FILE* file = _wpopen(first.stringVal().c_str(), L"rt");
                if (file == nullptr)
                {
                    if (raise_errors)
                        raiseError("popen() failed starting command");
                    else
                        return std::wstring();
                }

                std::wstring output;
                char buffer[4096];
                std::string temp;
                while (fgets(buffer, sizeof(buffer), file))
                    temp.append(buffer);
                output = toWideStr(temp);

                bool success = feof(file) != 0;

                int exit_code = _pclose(file);
                file = nullptr;

                if (raise_errors)
                {
                    if (!success)
                        raiseError("popen() failed executing command");
                    else if (exit_code != 0)
                        raiseError("popen() failed with exit code " + std::to_string(exit_code));
                }

                return output;
            } },

            { "setenv", [](object& first, const object::list& paramList) -> object {
                (void)first;
                if (paramList.size() != 2 || first.type() != object::STRING || paramList[1].type() != object::STRING)
                    raiseError("setEnv() works with name and value string parameters");

                if (_wputenv((first.stringVal() + L"=" + paramList[1].stringVal()).c_str()) != 0)
                    raiseError("setEnv() setting environment variable failed");

                return object();
            } },

            { "getenv", [](object& first, const object::list& paramList) -> object {
                (void)first;
                if (paramList.size() != 1 || first.type() != object::STRING)
                    raiseError("getEnv() works with one name string parameter");

                std::wstring envValStr;
                {
                    const wchar_t* envVal = _wgetenv(first.stringVal().c_str());
                    if (envVal != nullptr)
                        envValStr = envVal;
                }
                return envValStr;
            } },
#if defined(_WIN32) || defined(_WIN64)
            { "expandedenvvars", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::STRING)
                    raiseError("expandedEnvVars() works with one string parameter");

                const unsigned int output_str_len = 32 * 1024;
                std::unique_ptr<wchar_t[]> output_str(new wchar_t[output_str_len]);
                ExpandEnvironmentStrings(first.stringVal().c_str(), output_str.get(), output_str_len);
                return std::wstring(output_str.get());
            } },
#endif
            { "getexefilepath", [](object&, const object::list& paramList) -> object {
                if (!paramList.empty())
                    raiseError("getExeFilePath() takes no parameters");
                return getExeFilePath();
            } },

            { "getbinaryversion", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::STRING)
                    raiseError("getBinaryVersion() takes one string parameter");
                return toWideStr(getBinaryVersion(first.stringVal()));
            } },

            { "parseargs", [](object& first, const object::list& paramList) -> object {
                if
                (
                    paramList.size() != 2
                    ||
                    first.type() != object::LIST
                    ||
                    paramList[1].type() != object::LIST
                )
                {
                    raiseError("parseArgs() works with a list of arguments and a list of argument specifications");
                }

                object ret_val = parseArgs(first.listVal(), paramList[1].listVal());
                return ret_val;
            } },

            { "exit", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::NUMBER)
                    raiseError("exit() works with one exit code number");
                exit(int(first.numberVal()));
            } },

            { "error", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1)
                    raiseError("error() works with one object for error handling");
                throw user_exception(first);
            } },

            { "sleep", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::NUMBER)
                    raiseError("sleep() works with one parameter, the number of seconds to sleep");
                std::this_thread::sleep_for(std::chrono::seconds(int(first.numberVal())));
                return true;
            } },

            { "cd", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::STRING)
                    raiseError("cd() works with one parameter, the directory to change to");
                (void)_wchdir(first.stringVal().c_str());
                return true;
            } },

            { "curdir", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() == 0)
                {
                    wchar_t* buffer = _wgetcwd(nullptr, 0);
                    if (buffer != nullptr)
                    {
                        object retVal = std::wstring(buffer);
                        ::free(buffer);
                        buffer = nullptr;
                        return retVal;
                    }
                    else
                        raiseError("curDir() failed to get the current directory");
                }
                else if (paramList.size() == 1)
                {
                    if
                    (
                        first.type() != object::STRING
                        || 
                        first.stringVal().size() > 2 
                        || 
                        !iswalpha(first.stringVal()[0])
                        ||
                        (first.stringVal().length() == 2 && first.stringVal()[1] != ':')
                    )
                    {
                        raiseError("curDir() drive must be a letter and an optional :");
                    }
                    int drive = 1 + (int(toUpper(first.stringVal())[0]) - int('A'));
                    wchar_t* buffer = _wgetdcwd(drive, nullptr, 0);
                    if (buffer != nullptr)
                    {
                        object retVal = std::wstring(buffer);
                        ::free(buffer);
                        buffer = nullptr;
                        return retVal;
                    }
                    else
                        raiseWError(L"curDir() failed to get the current directory for drive " + drive);
                }
                else
                    raiseError("curDir() works with no parameter, or the drive to get the current directory for");
            } },

#if defined(_WIN32) || defined(_WIN64)
            { "getlasterror", [](object&, const object::list& paramList) -> object {
                if (paramList.size() != 0)
                    raiseError("getLastError() takes no parameters");
                return double(::GetLastError());
            } },

            { "getlasterrormsg", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() == 0)
                {
                    return mscript::getLastErrorMsg();
                }
                else if (paramList.size() == 1)
                {
                    if (first.type() != object::NUMBER)
                        raiseError("getLastErrorMsg() first parameter must be an error number");
                    if (first.numberVal() < 0 || double(DWORD(first.numberVal())) != first.numberVal())
                        raiseError("getLastErrorMsg() first parameter must be a valid error number");
                    return mscript::getLastErrorMsg(DWORD(first.numberVal()));
                }
                else
                    raiseError("getLastErrorMsg() takes at most one parameter, the error number");
            } },

            { "getinistring", [](object&, const object::list& paramList) -> object {
                if
                (
                    paramList.size() != 4
                    ||
                    paramList[0].type() != object::STRING
                    ||
                    paramList[0].stringVal().empty()
                    ||
                    paramList[1].type() != object::STRING
                    ||
                    paramList[1].stringVal().empty()
                    ||
                    paramList[2].type() != object::STRING
                    ||
                    paramList[2].stringVal().empty()
                    ||
                    paramList[3].type() != object::STRING
                )
                {
                    raiseError("getIniString() takes INI file path, section name, key name, and default value");
                }
                std::wstring file_path, section, key, default_value;
                file_path = paramList[0].stringVal();
                section = paramList[1].stringVal();
                key = paramList[2].stringVal();
                default_value = paramList[3].stringVal();

                const unsigned int full_file_path_str_len = 64 * 1024;
                std::unique_ptr<wchar_t[]> full_file_path_str(new wchar_t[full_file_path_str_len]);
                DWORD full_file_path_result = ::GetFullPathName(file_path.c_str(), full_file_path_str_len, full_file_path_str.get(), nullptr);
                if (full_file_path_result == 0)
                    raiseWError(L"Invalid INI file path: " + file_path);

                const unsigned int output_str_len = 64 * 1024;
                std::unique_ptr<wchar_t[]> output_str(new wchar_t[output_str_len]);
                
                ::GetPrivateProfileString(section.c_str(), key.c_str(), default_value.c_str(), output_str.get(), output_str_len, full_file_path_str.get());
                
                return std::wstring(output_str.get());
            } },

            { "getininumber", [](object&, const object::list& paramList) -> object {
                if
                (
                    paramList.size() != 4
                    ||
                    paramList[0].type() != object::STRING
                    ||
                    paramList[0].stringVal().empty()
                    ||
                    paramList[1].type() != object::STRING
                    ||
                    paramList[1].stringVal().empty()
                    ||
                    paramList[2].type() != object::STRING
                    ||
                    paramList[2].stringVal().empty()
                    ||
                    paramList[3].type() != object::NUMBER
                )
                {
                    raiseError("getIniNumber() takes INI file path, section name, key name, and default value");
                }
                std::wstring file_path, section, key;
                file_path = paramList[0].stringVal();
                section = paramList[1].stringVal();
                key = paramList[2].stringVal();

                const unsigned int full_file_path_str_len = 64 * 1024;
                std::unique_ptr<wchar_t[]> full_file_path_str(new wchar_t[full_file_path_str_len]);
                DWORD full_file_path_result = ::GetFullPathName(file_path.c_str(), full_file_path_str_len, full_file_path_str.get(), nullptr);
                if (full_file_path_result == 0)
                    raiseWError(L"Invalid INI file path: " + file_path);

                int default_value = int(paramList[3].numberVal());

                UINT ret_val = ::GetPrivateProfileInt(section.c_str(), key.c_str(), default_value, full_file_path_str.get());
                return double(ret_val);
            } },
#endif

            //
            // File I/O
            //
            { "readfile", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 2
                    || first.type() != object::STRING
                    || paramList[1].type() != object::STRING)
                {
                    raiseError("readFile() works with a file path string and an encoding string");
                }

                std::wstring filePath = first.stringVal();
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

                if (encoding == L"utf-8" || encoding == L"utf-16" || encoding == L"utf8" || encoding == L"utf16")
                {
                    std::wifstream file(filePath);
                    if (encoding == L"utf-8" || encoding == L"utf8")
                        file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
                    if (!file)
                        return object();

                    std::wstringstream stream;
                    stream << file.rdbuf();

                    std::wstring output = stream.str();
                    return object(output);
                }

                raiseError("Unsupported readFile() encoding: must be ascii, utf-8, or utf-16");
            } },

            { "readfilelines", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 2
                    || first.type() != object::STRING
                    || paramList[1].type() != object::STRING)
                {
                    raiseError("readFileLines() works with a file path string and an encoding string");
                }

                std::wstring filePath = first.stringVal();
                std::wstring encoding = paramList[1].stringVal();

                if (encoding == L"ascii")
                {
                    std::ifstream file(filePath);
                    if (!file)
                        return object();

                    object::list ret_val;
                    std::string line;
                    for (;;)
                    {
                        if (!std::getline(file, line))
                            break;
                        ret_val.push_back(replace(toWideStr(line), L"\r", L""));
                    }
                    return ret_val;
                }

                if (encoding == L"utf-8" || encoding == L"utf-16" || encoding == L"utf8" || encoding == L"utf16")
                {
                    std::wifstream file(filePath);
                    if (encoding == L"utf-8" || encoding == L"utf8")
                        file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
                    if (!file)
                        return object();

                    object::list ret_val;
                    std::wstring line;
                    for (;;)
                    {
                        if (!std::getline(file, line))
                            break;
                        ret_val.push_back(replace(line, L"\r", L""));
                    }
                    return ret_val;
                }

                raiseError("Unsupported readFileLines() encoding: must be ascii, utf-8, or utf-16");
            } },

            { "writefile", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 3
                    || first.type() != object::STRING
                    || paramList[1].type() != object::STRING
                    || paramList[2].type() != object::STRING)
                {
                    raiseError("writeFile() works with a file path string, file contents string, and an encoding string");
                }

                std::wstring filePath = first.stringVal();
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

                if (encoding == L"utf-8" || encoding == L"utf-16" || encoding == L"utf8" || encoding == L"utf16")
                {
                    std::wofstream file(filePath, std::wofstream::trunc);
                    if (encoding == L"utf-8" || encoding == L"utf8")
                        file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
                    if (!file)
                        return false;
                    file.write(contents.c_str(), contents.size());
                    file.close();
                    return true;
                }

                raiseError("Unsupported writeFile() encoding: must be ascii, utf-8, or utf-16");
            } },

            //
            // JSON
            //
            { "tojson", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1)
                    raiseError("toJson() takes one object to turn into JSON");
                else
                    return objectToJson(first);
            } },

            { "fromjson", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::STRING)
                    raiseError("fromJson() takes one JSON string to turn into an object");
                else
                    return objectFromJson(first.stringVal());
            } },

            //
            // HTML & URL encodings
            //
            { "htmlencoded", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::object_type::STRING)
                    raiseError("htmlEncoded() takes one string to HTML encode");

                std::wstring input_str = first.stringVal();

                std::wstring output_str;
                output_str.reserve(input_str.size()); // most strings need no encoding
                for (std::wstring::value_type c : input_str)
                {
                    switch (c)
                    {
                    case '&': output_str += L"&amp;"; break;
                    case '\"': output_str += L"&quot;"; break;
                    case '\'': output_str += L"&apos;"; break;
                    case '<': output_str += L"&lt;"; break;
                    case '>': output_str += L"&gt;"; break;
                    default: output_str += c; break;
                    }
                }
                return output_str;
            } },

            { "htmldecoded", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::object_type::STRING)
                    raiseError("htmlDecoded() takes one string to HTML encode");

                std::wstring output_str = first.stringVal();
                output_str = replace(output_str, L"&gt;", L">");
                output_str = replace(output_str, L"&lt;", L"<");
                output_str = replace(output_str, L"&apos;", L"\'");
                output_str = replace(output_str, L"&quot;", L"\"");
                output_str = replace(output_str, L"&amp;", L"&");
                return output_str;
            } },

            { "urlencoded", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::object_type::STRING)
                    raiseError("urlEncoded() takes one string to URL encode");

                std::wstring input_str = first.toString();

                std::wstringstream escaped;
                escaped.fill('0');
                escaped << std::hex;
                for (std::wstring::value_type c : input_str)
                {
                    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                        escaped << c;
                        continue;
                    }

                    // Any other characters are percent-encoded
                    escaped << std::uppercase;
                    escaped << '%' << std::setw(2) << int((unsigned char)c);
                    escaped << std::nouppercase;
                }
                return escaped.str();
            } }, 

            { "urldecoded", [](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::object_type::STRING)
                    raiseError("urlDecoded() takes one string to URL decode");

                std::wstring input_str = first.toString();

                std::wstring output_str;
                for (size_t i = 0; i < input_str.length(); ++i)
                {
                    if (input_str[i] == '%')
                    {
                        int ii = 0;
                        if (swscanf(input_str.substr(i + 1, 2).c_str(), L"%x", &ii) <= 0)
                            raiseError("urlDecoded() fails to convert character code");
                        output_str += static_cast<std::wstring::value_type>(ii);
                        i = i + 2;
                    }
                    else
                        output_str += input_str[i];
                }
                return output_str;
            } },

            // section / level tracing
            { "settracing", [this](object& first, const object::list& paramList) -> object {
                if
                (
                    paramList.size() != 2
                    ||
                    first.type() != object::object_type::LIST
                    ||
                    paramList[1].type() != object::object_type::NUMBER
                )
                {
                    raiseError("setTracing() takes a list of sections to enable, and a level to trace at");
                }

                this->m_traceInfo.ActiveSections.clear();
                for (auto section_obj : first.listVal())
                    this->m_traceInfo.ActiveSections.push_back(section_obj.toString());

                this->m_traceInfo.CurrentTraceLevel = (TraceLevel)(int)paramList[1].numberVal();

                return object();
            } },

            // Add evil, I mean, eval()
            { "eval", [this](object& first, const object::list& paramList) -> object {
                if (paramList.size() != 1 || first.type() != object::object_type::STRING)
                    raiseError("eval() takes one string expression to evaluate");
                else
                    return evaluate(first.stringVal());
            } },
        };

        //
        // Function calls
        //

        // built in functions
        const auto& funcIt = functions.find(function);
        if (funcIt != functions.end())
            return funcIt->second(first, paramList);

        // user functions
        if (m_callable.hasFunction(functionW)) 
        {
            object answer = m_callable.callFunction(functionW, paramList);
            return answer;
        }

        // external libraries
        {
            const auto moduleLib = lib::getLib(functionW);
            if (moduleLib != nullptr)
            {
                object moduleResult = moduleLib->executeFunction(functionW, paramList);
                return moduleResult;
            }
        }

        // Do object-like member function-ish expression processing
        {
            size_t dotIndex = function.find('.');
            if (dotIndex != std::string::npos)
            {
                std::wstring symbol = functionW.substr(0, dotIndex);
                std::wstring memberFunc = functionW.substr(dotIndex + 1);

                object value;
                if (m_symbols.tryGet(symbol, value))
                {
                    object::list newVals;
                    newVals.reserve(paramList.size() + 1);
                    newVals.push_back(value);
                    newVals.insert(newVals.end(), paramList.begin(), paramList.end());
                    return executeFunction(memberFunc, newVals);
                }
            }
        }

        // If the function name is the name of a variable
        if (m_allowDynamicCalls)
        {
            object value;
            if (m_symbols.tryGet(functionW, value))
            {
                if (value.type() != object::STRING)
                    raiseWError(L"Dynamic function variable does not resolve to string function name: " + functionW);

                std::wstring new_function = value.stringVal();
                return executeFunction(new_function, paramList);
            }
        }

        // Not a function we know about after all
        raiseWError(L"Function not defined: " + functionW);
    }
}
