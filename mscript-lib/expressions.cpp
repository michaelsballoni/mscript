#include "pch.h"
#include "expressions.h"
#include "utils.h"
#include "names.h"

namespace mscript
{
    std::vector<std::string> sm_ops
    {
        "or",
        "||",

        "and",
        "&&",

        "not",
        "!",

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

        if (narrow.empty())
            return object::NOTHING;

        if (narrow == "null")
            return object::NOTHING;

        if (narrow == "true")
            return bool(true);

        if (narrow == "false")
            return bool(false);

        {
            double number;
            auto result = std::from_chars(narrow.data(), narrow.data() + narrow.size(), number);
            if (result.ec == std::errc())
                return number;
        }

        {
            object symvalue;
            if (m_symbols.tryGet(expStr, symvalue))
                return symvalue;
        }

        if (isName(expStr)) // should have been found in symbol table
            raiseWError(L"Unknown variable name: " + expStr);

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
                    if (s > 0 && expStr[s - 1] == '\\')
                    {
                        str += L"\\\"";
                    }
                    else
                    {
                        foundEnd = true;
                        foundAtEnd = s == expStr.size() - 1;
                        break;
                    }
                }
                else
                {
                    str += c;
                }
            }
            if (!foundEnd)
                raiseError("Unfinished string");

            if (foundAtEnd)
            {
                replace(str, L"\\\\", L"\\");
                replace(str, L"\\\"", L"\"");
                replace(str, L"\\\"", L"\"");
                replace(str, L"\\'", L"\'");
                replace(str, L"\\t", L"\t");
                replace(str, L"\\n", L"\n");
                replace(str, L"\\r", L"\r");
                return str;
            }
        }

        for (size_t opdx = 0; opdx < sm_ops.size(); ++opdx)
        {
            std::string op = sm_ops[opdx];
            size_t opLen = op.size();

            size_t parenCount = 0;
            bool inString = false;

            for (int idx = expStr.size() - 1; idx >= 0; --idx)
            {
                wchar_t c = expStr[idx];

                if (c == '\"' && (idx == 0 || expStr[idx - 1] != '\\'))
                    inString = !inString;
                if (inString)
                    continue;

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
                            opMatches = expStr[idx + 1] == op[1]
                                && expStr[idx + 2] == op[2];
                        }
                    }
                    else
                        raiseError("Invalid op length: " + std::to_string(opLen));
                }
                if (opMatches)
                {
                    if (isOperator(expStr, op, idx))
                    {
                        object value;

                        std::wstring leftStr = expStr.substr(0, idx);
                        std::wstring rightStr = expStr.substr(idx + opLen);

                        object leftVal = evaluate(leftStr);

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
                            object rightVal = evaluate(rightStr);

                            if (leftVal.isNull() || rightVal.isNull())
                            {
                                if (op == "=")
                                    value = leftVal == rightVal;
                                else if (op == "!=")
                                    value = leftVal != rightVal;
                                else
                                    raiseError("Invalid operator for null values: " + op);
                            }
                            else if (leftVal.type() == object::STRING || rightVal.type() == object.STRING)
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
                                        raiseError("Unrecognized string operator: " + op);
                                    }
                                }
                                else
                                {
                                    if (op == "!=")
                                        value = leftValStr != rightValStr;
                                    else
                                        raiseError("Unrecognized string operator: " + op);
                                }
                            }
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
                                        default: raiseError("Unrecognized numeric operator: " + op);
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
                                            raiseError("Unrecognized numeric operator: " + op);
                                    }
                                }
                            }
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
                                    raiseError("Unrecognized boolean operator: " + op);
                            }
                        }
                        return value;
                    }
                    else
                    {
                        if (idx > 0)
                            idx = reverseFind(expStr, op, idx);
                    }
                }
            }
        }

        size_t leftParen = expStr.find('(');
        if (expStr.size() > 2 && leftParen != std::wstring::npos && expStr.back() == ')')
        {
            std::wstring functionName = expStr.substr(0, leftParen);

            int subStrLen = (expStr.size() - 1) - leftParen - 1;

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
            object answer = evaluate(expStr.substr(strlen("not ")));
            return !answer.boolVal();
        }

        raiseWError(L"Expression not evaluated: " + expStr);
    }

    bool expression::isOperator(std::wstring expr, std::string op, int n)
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

        if (op == "!" || op == "not")
        {
            if (n <= 0)
                return false;

            // Get the start up to previous charcater, trim it, then get the last char
            std::string bang = toNarrowStr(trim(expr.substr(0, n)));
            bang = bang.substr(bang.length() - 1);

            // If the last char is an operator, then this is a unary ! / not, not an operator
            if (std::find(sm_ops.begin(), sm_ops.end(), bang) != sm_ops.end())
                return false;
        }

        return true;
    }

    int expression::reverseFind(std::wstring source, const std::wstring& search, int start)
    {
        int searchLen = search.length();
        if (searchLen > source.length())
            return -1;

        bool inString = false;

        int openP = 0;
        int closeP = 0;

        for (int p = start; p >= 0; --p)
        {
            if (source[p] == '\"' && (p == 0 || source[p - 1] != '\\'))
                inString = !inString;
            if (inString)
                continue;

            if (source.length() - (p + searchLen) >= 0)
            {
                std::wstring sign = source.substr(p, searchLen);
                if (sign == search && openP == closeP)
                    return p;
            }

            if (source[p] == '(')
                ++openP;
            else if (source[p] == ')')
                ++closeP;
        }

        return -1;
    }

    object::list expression::processParameters(const std::vector<std::wstring> expStrs)
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
        bool inString = false;
        int parenCount = 0;
        for (size_t idx = 0; idx < expStr.size(); ++idx)
        {
            wchar_t c = expStr[idx];
            if (c == '\"' && (idx == 0 || expStr[idx - 1] != '\\'))
                inString = !inString;

            if (!inString)
            {
                if (c == '(')
                    ++parenCount;
                else if (c == ')')
                    --parenCount;
            }

            if (!inString && parenCount == 0 && c == ',')
            {
                curExp = trim(curExp);
                if (curExp.empty())
                    raiseError("Missing parameter");

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
            raiseError("Function " + function + " requires one numeric parameter");
        else
            return paramList[0].numberVal();
    }

    object expression::executeFunction(const std::wstring& function, const object::list& paramList)
    {
        object first = paramList.size() == 0 ? object::NOTHING : paramList[0];

        if (function == L"abs") return abs(getOneDouble(paramList, "abs"));

        else if (function == L"sqrt") return sqrt(getOneDouble(paramList, "sqrt"));
        else if (function == L"ceil") return ceil(getOneDouble(paramList, "ceil"));
        else if (function == L"floor") return floor(getOneDouble(paramList, "floor"));
        else if (function == L"round") return round(getOneDouble(paramList, "round"));

        else if (function == L"exp") return exp(getOneDouble(paramList, "exp"));
        else if (function == L"log") return log(getOneDouble(paramList, "log"));
        else if (function == L"log2") return log2(getOneDouble(paramList, "log2"));
        else if (function == L"log10") return log10(getOneDouble(paramList, "log10"));

        else if (function == L"sin") return sin(getOneDouble(paramList, "sin"));
        else if (function == L"cos") return cos(getOneDouble(paramList, "cos"));
        else if (function == L"tan") return tan(getOneDouble(paramList, "tan"));

        else if (function == L"asin") return asin(getOneDouble(paramList, "asin"));
        else if (function == L"acos") return acos(getOneDouble(paramList, "acos"));
        else if (function == L"atan") return atan(getOneDouble(paramList, "atan"));

        else if (function == L"sinh") return sinh(getOneDouble(paramList, "sinh"));
        else if (function == L"cosh") return cosh(getOneDouble(paramList, "cosh"));
        else if (function == L"tanh") return tanh(getOneDouble(paramList, "tanh"));

        if (function == L"getType")
        {
            if (paramList.size() != 1)
                raiseError("getType() takes one parameter");
            return toWideStr(first.getTypeName(first.type()));
        }

        if (function == L"number")
        {
            if (paramList.size() != 1)
                raiseError("number() takes one parameter");
            return first.toNumber();
        }

        if (function == L"string")
        {
            if (paramList.size() != 1)
                raiseError("string() takes one parameter");
            return first.toString();
        }

        if (function == L"length")
        {
            if (paramList.size() != 1)
                raiseError("length() takes one parameter");
            return double(first.toLength());
        }

        if (function == L"list")
            return object::list(paramList);

        if (function == L"index")
        {
            if ((paramList.size() % 2) != 0)
                raiseError("index() parameters must be an even count, key-value pairs");

            object::index newIndex;
            for (size_t i = 0; i < paramList.size(); i += 2)
            {
                if (newIndex.contains(paramList[i]))
                    raiseWError(L"index() key repeated: " + paramList[i].toString());
                newIndex.insert(paramList[i], paramList[i + 1]);
            }
            return newIndex;
        }

        case "add": // only function that has side-effects -> quasi-functional
            if (first is list)
            {
                list l = (list)first;
                for (int v = 1; v < paramList.Count; ++v)
                    l.Add(paramList[v]);
                return l;
            }
            else if (first is index)
            {
                if (((paramList.Count - 1) % 2) != 0)
                    throw new ScriptException("add parameters for index must be an even count: " + paramList.Count);
                index i = (index)first;
                for (int a = 1; a < paramList.Count; a += 2)
                {
                    if (i.ContainsKey(paramList[a]))
                        throw new ScriptException("add function index key repeated: " + paramList[a]);
                    i.Add(paramList[a], paramList[a + 1]);
                }
                return i;
            }
            else
                throw new ScriptException("add function works only with list and index");

        case "get":
        {
            if (paramList.Count != 2)
                throw new ScriptException("get function takes item to get from, and key to use");
            if (first is string)
            {
                if (!(paramList[1] is double))
                    throw new ScriptException("get function parameter is not a number");
                string s = (string)first;
                int idx = (int)(double)paramList[1];
                if (idx < 0 || idx >= s.Length)
                    throw new ScriptException("get function index is out of range: " + idx + " - length: " + s.Length);
                return s[idx].ToString();
            }
            else if (first is list)
            {
                if (!(paramList[1] is double))
                    throw new ScriptException("get function parameter is not a number");
                list l = (list)first;
                int idx = Convert.ToInt32(paramList[1]);
                if (idx < 0 || idx >= l.Count)
                    throw new ScriptException("get function index is out of range: " + idx + " - length: " + l.Count);
                return l[idx];
            }
            else if (first is index)
            {
                object key = paramList[1];
                index i = (index)first;
                if (!i.ContainsKey(key))
                    throw new ScriptException("get function key not found in index: " + key);
                return i[key];
            }
            else
                throw new ScriptException("get function only works with string, list, and index");
        }

        case "has":
        {
            if (paramList.Count != 2)
                throw new ScriptException("has function takes collection to look in, and value to look for");
            if (first is string)
                return ((string)first).Contains(Utils.ToString(paramList[1]));
            else if (first is list)
                return ((list)first).Contains(paramList[1]);
            else if (first is index)
                return ((index)first).ContainsKey(paramList[1]);
            else
                throw new ScriptException("has function only works with string, list, and index");
        }

        case "keys":
            if (paramList.Count != 1)
                throw new ScriptException("keys function takes index to work with");
            if (first is index)
            {
                list keys = new list(((index)first).Keys);
                return keys;
            }
            else
                throw new ScriptException("keys function only works with index");

        case "values":
            if (paramList.Count != 1)
                throw new ScriptException("values function takes index to work with");
            if (first is index)
            {
                list keys = new list(((index)first).Values);
                return keys;
            }
            else
                throw new ScriptException("values function only works with index");

        case "reversed":
        {
            if (paramList.Count != 1)
                throw new ScriptException("reversed function only works with one item");
            if (first is string)
            {
                var listy = new List<char>(((string)first).ToCharArray());
                listy.Reverse();
                return new string(listy.ToArray());
            }
            else if (first is list)
            {
                list copy = new list((list)first);
                copy.Reverse();
                return copy;
            }
            else if (first is index)
            {
                var entries = new List<KeyValuePair<object, object>>(((index)first).Entries);
                entries.Reverse();
                index copy = new index();
                foreach(var kvp in entries)
                    copy.Add(kvp.Key, kvp.Value);
                return copy;
            }
            else
                throw new ScriptException("reversed function only works with string, list, and index");
        }

        case "sorted":
        {
            if (paramList.Count != 1)
                throw new ScriptException("sorted function only works with one item");
            if (first is string)
            {
                var chars = ((string)first).ToCharArray().ToList();
                chars.Sort();
                return new string(chars.ToArray());
            }
            else if (first is list)
            {
                list copy = new list((list)first);
                copy.Sort();
                return copy;
            }
            else if (first is index)
            {
                index original = (index)first;
                list sortedKeys = new list(original.Keys);
                sortedKeys.Sort();
                index copy = new index();
                foreach(var key in sortedKeys)
                    copy.Add(key, original[key]);
                return copy;
            }
            else
                throw new ScriptException("sorted function only works with string, list, and index");
        }

        case "join":
        {
            if (paramList.Count > 2)
                throw new ScriptException("join function takes item to work with, and an optional separator");

            string separator = paramList.Count == 2 ? Utils.ToString(paramList[1]) : "";
            if (first is list)
                return string.Join(separator, (list)first);
            else if (first is index)
                return string.Join(separator, ((index)first).Keys);
            else
                throw new ScriptException("join function only works with list and index");
        }

        case "split":
        {
            if (paramList.Count != 2)
                throw new ScriptException("split works with an item and separator");

            if (!(first is string))
                throw new ScriptException("split only works with strings");

            string separator = Utils.ToString(paramList[1]);

            var initial = ((string)first).Split(new[] { separator }, StringSplitOptions.None);
            return initial.Select(val = > (object)val).ToList();
        }

        case "toUpper":
            if (paramList.Count != 1)
                throw new ScriptException("toUpper takes one string parameter");
            if (!(first is string))
                throw new ScriptException("toUpper only works with strings");
            return ((string)first).ToUpper();

        case "toLower":
            if (paramList.Count != 1)
                throw new ScriptException("toLower takes one string parameter");
            if (!(first is string))
                throw new ScriptException("toLower only works with strings");
            return ((string)first).ToLower();

        case "replaced":
            if
                (
                    paramList.Count != 3
                    ||
                    !(paramList[0] is string)
                    ||
                    !(paramList[1] is string)
                    ||
                    !(paramList[2] is string)
                    )
            {
                throw new ScriptException("replaced takes three string parameters");
            }
            string input = (string)paramList[0];
            string toFind = (string)paramList[1];
            string toReplaceWith = (string)paramList[2];
            return input.Replace(toFind, toReplaceWith);

        case "random":
            if
                (
                    paramList.Count != 2
                    ||
                    !(paramList[0] is double)
                    ||
                    !(paramList[1] is double)
                    )
            {
                throw new ScriptException("random takes two number parameters, min and max");
            }
            double parm1 = (double)paramList[0];
            double parm2 = (double)paramList[1];
            double min = Math.Min(parm1, parm2);
            double max = Math.Max(parm1, parm2);
            lock(sm_random)
            {
                double rnd = sm_random.NextDouble() * (max - min) + min;
                return rnd;
            }

        case "firstLocation":
            if (paramList.Count != 2)
                throw new ScriptException("firstLocation works with an item to look in and a key to look for");
            if (first is string)
            {
                if (!(paramList[1] is string))
                    throw new ScriptException("Invalid second param for firstLocation, must be string");
                return (double)first.ToString().IndexOf(Utils.ToString(paramList[1]));
            }
            else if (first is list)
            {
                list l = (list)first;
                for (int i = 0; i < l.Count; ++i)
                {
                    if (l[i].Equals(paramList[1]))
                        return (double)i;
                }
                return (double)-1;
            }
            else if (first is index)
            {
                index l = (index)first;
                for (int i = 0; i < l.Count; ++i)
                {
                    if (l.Entries[i].Key.Equals(paramList[1]))
                        return (double)i;
                }
                return (double)-1;
            }
            else
            {
                throw new ScriptException("firstLocation only works with string, list, and index");
            }

        case "lastLocation":
            if (paramList.Count != 2)
                throw new ScriptException("lastLocation works with an item ");
            if (first is string)
            {
                if (!(paramList[1] is string))
                    throw new ScriptException("Invalid second param for lastLocation, must be string");
                return (double)first.ToString().LastIndexOf(Utils.ToString(paramList[1]));
            }
            else if (first is list)
            {
                list l = (list)first;
                for (int i = l.Count - 1; i >= 0; --i)
                {
                    if (l[i].Equals(paramList[1]))
                        return (double)i;
                }
                return (double)-1;
            }
            else if (first is index)
            {
                index l = (index)first;
                for (int i = l.Count - 1; i >= 0; --i)
                {
                    if (l.Entries[i].Key.Equals(paramList[1]))
                        return (double)i;
                }
                return (double)-1;
            }
            else
            {
                throw new ScriptException("lastLocation only works with string, list, or index");
            }

        case "subset":
            if (paramList.Count != 2 && paramList.Count != 3)
                throw new ScriptException("subset only works with a string and a start index and an optional length");
            if (!(paramList[1] is double))
                throw new ScriptException("subset start index must be number");
            int startIndex = (int)(double)paramList[1];
            if (startIndex < 0)
                throw new ScriptException("subset start index must be greater than or equal zero");
            if (paramList.Count == 2)
            {
                if (first is string)
                {
                    string s = (string)first;
                    if (startIndex >= s.Length)
                        throw new ScriptException("subset start index must be less than or equal to the length of the string");
                    return ((string)first).Substring(startIndex);
                }
                else if (first is list)
                {
                    list l = (list)first;
                    if (startIndex >= l.Count)
                        throw new ScriptException("subset start index must be less than or equal to the length of the list");
                    list subset = new list();
                    for (int i = startIndex; i < l.Count; ++i)
                        subset.Add(l[i]);
                    return subset;
                }
                else if (first is index)
                {
                    index i = (index)first;
                    if (startIndex >= i.Count)
                        throw new ScriptException("subset start index must be less than or equal to the length of the index");
                    index subset = new index();
                    for (int j = startIndex; j < i.Count; ++j)
                        subset.Add(i.Entries[j].Key, i.Entries[j].Value);
                    return subset;
                }
                else
                {
                    throw new ScriptException("subset works with string, list, and index");
                }
            }
            else
            {
                if (!(paramList[2] is double))
                    throw new ScriptException("substring third param must be number");
                int length = (int)(double)paramList[2];
                if (length < 0)
                    throw new ScriptException("subset length must be greater than or equal zero");
                if (first is string)
                {
                    string s = (string)first;
                    if (startIndex >= s.Length)
                        throw new ScriptException("subset start index must be less than or equal to the length of the string");
                    if (startIndex + length > s.Length)
                        throw new ScriptException("subset must be exceed length of string");
                    return s.Substring(startIndex, length);
                }
                else if (first is list)
                {
                    list l = (list)first;
                    if (startIndex >= l.Count)
                        throw new ScriptException("subset start index must be less than or equal to the length of the list");
                    if (startIndex + length > l.Count)
                        throw new ScriptException("subset must be exceed length of list");
                    list subset = new list();
                    for (int i = startIndex; i < startIndex + length; ++i)
                        subset.Add(l[i]);
                    return subset;
                }
                else if (first is index)
                {
                    index i = (index)first;
                    if (startIndex >= i.Count)
                        throw new ScriptException("subset start index must be less than or equal to the length of the index");
                    if (startIndex + length > i.Count)
                        throw new ScriptException("subset must be exceed length of list");
                    index subset = new index();
                    for (int j = startIndex; j < startIndex + length; ++j)
                        subset.Add(i.Entries[j].Key, i.Entries[j].Value);
                    return subset;
                }
                else
                {
                    throw new ScriptException("subset only works with string, list, and index");
                }
            }

        case "htmlEncode":
            if (paramList.Count != 1)
                throw new ScriptException("htmlEncode works with one parameter");
            if (!(first is string))
                throw new ScriptException("htmlEncode only works with strings");
            return System.Web.HttpUtility.HtmlEncode(first);

        case "urlEncode":
            if (paramList.Count != 1)
                throw new ScriptException("urlEncode works with one parameter");
            if (!(first is string))
                throw new ScriptException("urlEncode only works with strings");
            return System.Web.HttpUtility.UrlEncode((string)first);

        case "isMatch":
            if (paramList.Count != 2)
                throw new ScriptException("isMatch works with two parameters");
            if (!(paramList[0] is string))
                throw new ScriptException("isMatch only works with string input");
            if (!(paramList[1] is string))
                throw new ScriptException("isMatch only works with string pattern");
            return Regex.IsMatch((string)paramList[0], (string)paramList[1]);

        case "uniqueId":
            if (paramList.Count != 0)
                throw new ScriptException("uniqueId takes no parameters");
            return Guid.NewGuid().ToString();

        case "today":
            if (paramList.Count != 0)
                throw new ScriptException("today takes no parameters");
            return DateTime.Now.Date.ToString("yyyy/MM/dd");

        case "year":
        {
            if (paramList.Count != 1)
                throw new ScriptException("year takes one parameter");
            if (!(first is string))
                throw new ScriptException("year only works with strings");

            DateTime dt;
            if (!DateTime.TryParse((string)first, out dt))
                throw new ScriptException("year function cannot parse date: " + first);
            return (double)dt.Year;
        }

        case "month":
        {
            if (paramList.Count != 1)
                throw new ScriptException("month takes one parameter");
            if (!(first is string))
                throw new ScriptException("month only works with strings");

            DateTime dt;
            if (!DateTime.TryParse((string)first, out dt))
                throw new ScriptException("month function cannot parse date: " + first);
            return (double)dt.Month;
        }

        case "day":
        {
            if (paramList.Count != 1)
                throw new ScriptException("day takes one parameter");
            if (!(first is string))
                throw new ScriptException("day only works with strings");

            DateTime dt;
            if (!DateTime.TryParse((string)first, out dt))
                throw new ScriptException("day function cannot parse date: " + first);
            return (double)dt.Day;
        }

        case "addDays":
        {
            {
                if (paramList.Count != 2)
                    throw new ScriptException("addDays takes two parameters");
                if (!(first is string))
                    throw new ScriptException("addDays first parameter needs to be a string");
                if (!(paramList[1] is double))
                    throw new ScriptException("addDays second parameter needs to be a number of days to add");
                if ((int)(double)(paramList[1]) != (double)(paramList[1]))
                    throw new ScriptException("addDays days to add must be a whole number of days: " + paramList[1]);

                DateTime dt;
                if (!DateTime.TryParse((string)first, out dt))
                    throw new ScriptException("addDays function cannot parse date: " + first);
                dt += new TimeSpan((int)(double)paramList[1], 0, 0, 0);
                return dt.Date.ToString("yyyy/MM/dd");
            }
        }

        case "daysBetween":
        {
            {
                if (paramList.Count != 2)
                    throw new ScriptException("daysBetween takes two parameters");
                if (!(first is string))
                    throw new ScriptException("daysBetween first parameter needs to be a date string");
                if (!(paramList[1] is string))
                    throw new ScriptException("daysBetween first parameter needs to be a date string");

                DateTime dt1;
                if (!DateTime.TryParse((string)first, out dt1))
                    throw new ScriptException("daysBetween function cannot parse first date: " + first);

                DateTime dt2;
                if (!DateTime.TryParse((string)paramList[1], out dt2))
                    throw new ScriptException("daysBetween function cannot parse first date: " + first);

                double rawDaysBetween = (dt1 - dt2).TotalDays;
                int daysBetween;
                if (rawDaysBetween >= 0.0)
                    daysBetween = (int)Math.Floor(rawDaysBetween);
                else
                    daysBetween = (int)Math.Ceiling(rawDaysBetween);
                return (double)daysBetween;
            }
        }

        case "toPrettyDate":
        {
            if (paramList.Count != 1)
                throw new ScriptException("toPrettyDate takes one parameter");
            if (!(first is string))
                throw new ScriptException("toPrettyDate only works with strings");

            DateTime dt;
            if (!DateTime.TryParse((string)first, out dt))
                throw new ScriptException("toPrettyDate function cannot parse date: " + first);
            return dt.ToLongDateString();
        }

        default:
        {
            if (m_callable != null && m_callable.HasFunction(function))
            {
                object answer = await m_callable.CallFunctionAsync(function, paramList);
                return answer;
            }

            int dotIndex = function.IndexOf('.');
            if (dotIndex > 0) // somelist.reversed()
            {
                string symbol = function.Substring(0, dotIndex);
                function = function.Substring(dotIndex + 1);

                object value;
                if (m_symbols.TryGet(symbol, out value))
                {
                    list newVals = new list(paramList.Count + 1);
                    newVals.Add(value);
                    newVals.AddRange(paramList);
                    return await ExecuteFunctionAsync(function, newVals);
                }
            }
            else // someindex("some key") or somelist(14)
            {
                object value;
                if (m_symbols.TryGet(function, out value))
                {
                    list newVals = new list(paramList.Count + 1);
                    newVals.Add(value);
                    newVals.AddRange(paramList);
                    return await ExecuteFunctionAsync("get", newVals);
                }
            }
            break;
        }
        }

        throw new ScriptException("Function not defined: " + function);
    }
}
