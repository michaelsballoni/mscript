#include "pch.h"
#include "script_processor.h"
#include "names.h"
#include "script_exception.h"

namespace mscript
{
    object script_processor::script_processor::process()
    {
        preprocessFunctions();
        process_outcome outcome;
        return process(0U, int(m_lines.size()) - 1, outcome, 0U);
    }

    void script_processor::preprocessFunctions()
    {
        for (int l = 0; l < int(m_lines.size()); ++l)
        {
            std::wstring line = trim(m_lines[l]);
            if (line.empty())
                continue;
#ifndef _DEBUG
            try
#endif
            {
                if (line == L"/*")
                {
                    ++l;
                    while (trim(m_lines[l]) != L"*/")
                    {
                        ++l;
                        if (l >= m_lines.size())
                            raiseError("Unfinished block comment");
                    }
                }

                if (line[0] != '~')
                    continue;

                size_t firstSpace = line.find(' ');
                if (firstSpace == std::wstring::npos)
                    raiseError("function missing space before name");

                size_t openParen = line.find('(');
                if (openParen == std::wstring::npos)
                    raiseError("function missing opening parenthese");

                if (firstSpace > openParen)
                    raiseError("function parenthese precedes name");

                std::wstring name = trim(line.substr(firstSpace, openParen - firstSpace));
                validateName(name);
                if (m_functions.find(name) != m_functions.end())
                    raiseWError(L"function already defined: " + name);

                std::wstring paramListStr = line.substr(openParen);
                replace(paramListStr, L"(", L"");
                replace(paramListStr, L")", L"");
                auto paramList = split(paramListStr, L",");
                for (auto& param : paramList)
                    param = trim(param);
                if (paramList.size() == 1 && paramList[0].empty())
                {
                    paramList.clear();
                }
                else
                {
                    for (const auto& param : paramList)
                        validateName(param);
                }

                int loopEnd = findMatchingEnd(m_lines, l, int(m_lines.size()) - 1);
                int loopStart = l;
                l = loopEnd;

                script_function function;
                function.name = name;
                function.paramNames = paramList;
                function.startIndex = loopStart + 1;
                function.endIndex = loopEnd - 1;

                m_functions.insert({ name, std::make_shared<script_function>(function) });
            }
#ifndef _DEBUG
            catch (const std::exception& exp)
            {
                handleException(exp, line, l);
            }
#endif
        }
    }

    object 
    script_processor::process
    (
        int startLine, 
        int endLine, 
        process_outcome& outcome, 
        unsigned callDepth
    )
    {
        for (int l = startLine; l <= endLine; ++l)
        {
            std::wstring line = trim(m_lines[l]);
            if (line.empty()) // skip blank lines
                continue;

            std::string narrow = toNarrowStr(line);
            auto first = line[0];
#ifndef _DEBUG
            try
#endif
            {
                if (narrow == "/*") // block comment
                {
                    ++l;
                    while (trim(m_lines[l]) != L"*/")
                    {
                        ++l;
                        if (l >= m_lines.size())
                            raiseError("Unfinished block comment");
                    }
                }
                else if (first == '!') // single line comment
                {
                    // No op
                }
                else if (narrow == "{>>") // block verbatim print
                {
                    ++l;
                    while (trim(m_lines[l]) != L">>}")
                    {
                        m_output(m_lines[l]);
                        ++l;
                        if (l >= m_lines.size())
                            raiseError("Unfinished block print");
                    }
                }
                else if (startsWith(line, L">>")) // single line verbatim print
                {
                    static size_t verbLen = strlen(">>");
                    std::wstring valueStr = trim(line.substr(verbLen));
                    m_output(valueStr);
                }
                else if (first == '>') // single line expression print
                {
                    std::wstring valueStr = trim(line.substr(1));
                    object answer = evaluate(valueStr, callDepth);
                    m_output(answer.toString());
                }
                else if (first == '$') // variable declaration, initial value required
                {
                    line = line.substr(1);
                    size_t equalsIndex = line.find('=');
                    if (equalsIndex == std::wstring::npos)
                        raiseError("Variable declaraion lacks initial value");

                    std::wstring nameStr = trim(line.substr(0, equalsIndex));
                    std::wstring valueStr = trim(line.substr(equalsIndex + 1));

                    object answer = evaluate(valueStr, callDepth);

                    m_symbols.set(nameStr, answer);
                }
                else if (first == '&') // post-initialization assignment
                {
                    line = line.substr(1);
                    size_t equalsIndex = line.find('=');
                    if (equalsIndex == std::wstring::npos)
                        raiseError("Variable assignment lacks value");

                    std::wstring nameStr = trim(line.substr(0, equalsIndex));
                    std::wstring valueStr = trim(line.substr(equalsIndex + 1));

                    object answer = evaluate(valueStr, callDepth);

                    m_symbols.assign(nameStr, answer);
                }
                else if (narrow == "O") // infinite loop
                {
                    int loopEnd = findMatchingEnd(m_lines, l, endLine);
                    int loopStart = l;
                    l = loopEnd;

                    symbol_stacker stacker(m_symbols);
                    while (true)
                    {
                        process_outcome ourOutcome;
                        process(loopStart + 1, loopEnd - 1, ourOutcome, callDepth + 1);
                        if (ourOutcome.Return)
                        {
                            outcome.Return = true;
                            outcome.ReturnValue = ourOutcome.ReturnValue;
                            return ourOutcome.ReturnValue;
                        }
                        else if (ourOutcome.Continue)
                            continue;
                        else if (ourOutcome.Leave)
                            break;
                    }
                }
                else if (first == '{') // braced scope, for variable declaration containment
                {
                    int loopEnd = findMatchingEnd(m_lines, l, endLine);
                    int loopStart = l;
                    l = loopEnd;

                    symbol_stacker stacker(m_symbols);
                    process_outcome ourOutcome;
                    process(loopStart + 1, loopEnd - 1, ourOutcome, callDepth + 1);
                    if (ourOutcome.Return)
                    {
                        outcome.Return = true;
                        outcome.ReturnValue = ourOutcome.ReturnValue;
                        return ourOutcome.ReturnValue;
                    }
                    else if (ourOutcome.Continue)
                    {
                        outcome.Continue = true;
                        return object();
                    }
                    else if (ourOutcome.Leave)
                    {
                        outcome.Leave = true;
                        return object();
                    }
                }
                else if (narrow == "<-") // void return statement
                {
                    outcome.Return = true;
                    return object();
                }
                else if (startsWith(line, L"<- "))
                {
                    size_t space = line.find(' ');
                    if (space == std::wstring::npos)
                        raiseError("return statement has no value");

                    std::wstring expStr = line.substr(space + 1);

                    object returnValue = evaluate(expStr, callDepth);
                    outcome.ReturnValue = returnValue;
                    outcome.Return = true;
                    return outcome.ReturnValue;
                }
                else if (first == '?') // if else
                {
                    bool seenQuestion = false;
                    bool seenPlainElse = false;
                    
                    auto markers = findElses(m_lines, l, endLine);

                    int endMarker = markers.back();
                    l = endMarker;

                    for (int m = 0; m < int(markers.size()); ++m)
                    {
                        int marker = markers[m];
                        int nextMarker = markers[std::min(m + 1, int(markers.size()) - 1)];
                        std::wstring markerLine = trim(m_lines[marker]);

                        object answer;
                        size_t spaceIndex = markerLine.find(' ');
                        if (spaceIndex == std::wstring::npos)
                        {
                            if (seenPlainElse)
                                raiseError("Already seen <> statement");

                            if (!seenQuestion)
                                raiseError("No ? statement before <> statement");

                            seenPlainElse = true;
                            answer = true;
                        }
                        else
                        {
                            seenQuestion = true;

                            if (seenPlainElse)
                                raiseError("Already seen <> statement");

                            std::wstring criteria = markerLine.substr(spaceIndex + 1);
                            answer = evaluate(criteria, callDepth);
                            if (answer.type() != object::BOOL)
                                raiseError("? expression does not evaluate to true or false");
                        }

                        if (!answer.boolVal())
                            continue;

                        symbol_stacker stacker(m_symbols);
                        process_outcome ourOutcome;
                        process(marker + 1, nextMarker - 1, ourOutcome, callDepth + 1);
                        
                        outcome = ourOutcome;
                        if (ourOutcome.Return)
                            return ourOutcome.ReturnValue;
                        else if (ourOutcome.Continue)
                            return object();
                        else if (ourOutcome.Leave)
                            return object();
                        break;
                    }
                }
                else if (first == '@') // for each loop
                {
                    size_t firstSpace = line.find(' ');
                    if (firstSpace == std::wstring::npos)
                        raiseError("@ statement lacks loop variable name");
                    
                    size_t nextSpace = line.find(' ', firstSpace + 1);
                    if (nextSpace == std::wstring::npos)
                        raiseError("@ statement lacks : part");

                    size_t thirdSpace = line.find(' ', nextSpace + 1);
                    if (thirdSpace == std::wstring::npos)
                        raiseError("@ statement lacks collection expression");

                    std::wstring label = trim(line.substr(firstSpace, nextSpace - firstSpace));
                    validateName(label);

                    int loopEnd = findMatchingEnd(m_lines, l, endLine);
                    int loopStart = l;
                    l = loopEnd;

                    std::wstring expression = line.substr(thirdSpace + 1);
                    object answer = evaluate(expression, callDepth);
                    object::list enumerable;
                    {
                        if (answer.type() == object::STRING)
                        {
                            for (auto c : answer.stringVal())
                                enumerable.push_back(std::wstring{ c });
                        }
                        else if (answer.type() == object::LIST)
                            enumerable = answer.listVal();
                        else if (answer.type() == object::INDEX)
                            enumerable = answer.indexVal().keys();
                        else
                            raiseError("@ statements only work with strings, lists, and indexes");
                    }

                    {
                        symbol_stacker outerStacker(m_symbols);
                        m_symbols.set(label, object());
                        for (object val : enumerable)
                        {
                            symbol_stacker innerStacker(m_symbols);
                            m_symbols.assign(label, val);

                            process_outcome ourOutcome;
                            process(loopStart + 1, loopEnd - 1, ourOutcome, callDepth + 1);
                            if (ourOutcome.Return)
                            {
                                outcome = ourOutcome;
                                return ourOutcome.ReturnValue;
                            }
                            else if (ourOutcome.Continue)
                                continue;
                            else if (ourOutcome.Leave)
                                break;
                        }
                    }
                }
                else if (first == '#') // for x = a to y
                {
                    size_t firstSpace = line.find(' ');
                    if (firstSpace == std::wstring::npos)
                        raiseError("# statement missing first space");

                    size_t nextSpace = line.find(' ', firstSpace + 1);
                    if (nextSpace == std::wstring::npos)
                        raiseError("# statement lacks counter variable");

                    std::wstring label = trim(line.substr(firstSpace, nextSpace - firstSpace));
                    validateName(label);

                    size_t thirdSpace = line.find(' ', nextSpace + 1);
                    if (thirdSpace == std::wstring::npos)
                        raiseError("# statement lacks from part");

                    std::wstring from = trim(line.substr(nextSpace, thirdSpace - nextSpace));
                    if (from != L":")
                        raiseError("# statement invalid : part");

                    std::wstring theRest = line.substr(thirdSpace + 1);
                    std::wstring fromExpStr, toExpStr;
                    int parenCount = 0;
                    bool inString = false;
                    static size_t arrowLen = strlen(" -> ");
                    for (int f = 0; f < theRest.size() - arrowLen; ++f)
                    {
                        auto c = theRest[f];
                        if (c == '\"' && (f == 0 || theRest[f - 1] != '\\'))
                            inString = !inString;

                        if (!inString)
                        {
                            if (c == '(')
                                ++parenCount;
                            else if (c == ')')
                                --parenCount;
                        }

                        if (!inString && parenCount == 0)
                        {
                            if (startsWith(theRest.substr(f), L" -> "))
                            {
                                fromExpStr = trim(theRest.substr(0, f));
                                toExpStr = trim(theRest.substr(f + arrowLen));
                                break;
                            }
                        }
                    }

                    object fromValue = evaluate(fromExpStr, callDepth);
                    if (fromValue.type() != object::NUMBER)
                        raiseWError(L"Invalid from value: " + fromValue.toString());

                    object toValue = evaluate(toExpStr, callDepth);
                    if (toValue.type() != object::NUMBER)
                        raiseWError(L"Invalid to value: " + toValue.toString());

                    auto fromIdx = static_cast<int64_t>(fromValue.numberVal());
                    auto toIdx = static_cast<int64_t>(toValue.numberVal());

                    int loopEnd = findMatchingEnd(m_lines, l, endLine);
                    int loopStart = l;
                    l = loopEnd;

                    {
                        symbol_stacker outerStacker(m_symbols);
                        m_symbols.set(label, object());

                        if (fromIdx <= toIdx)
                        {
                            for (auto i = fromIdx; i <= toIdx; ++i)
                            {
                                m_symbols.assign(label, double(i));
                                symbol_stacker innerStacker(m_symbols);
                                process_outcome ourOutcome;
                                process(loopStart + 1, loopEnd - 1, ourOutcome, callDepth + 1);
                                if (ourOutcome.Return)
                                {
                                    outcome = ourOutcome;
                                    return ourOutcome.ReturnValue;
                                }
                                else if (ourOutcome.Continue)
                                    continue;
                                else if (ourOutcome.Leave)
                                    break;
                            }
                        }
                        else
                        {
                            for (auto i = fromIdx; i >= toIdx; --i)
                            {
                                m_symbols.assign(label, double(i));
                                symbol_stacker innerStacker(m_symbols);
                                process_outcome ourOutcome;
                                process(loopStart + 1, loopEnd - 1, ourOutcome, callDepth + 1);
                                if (ourOutcome.Return)
                                {
                                    outcome = ourOutcome;
                                    return ourOutcome.ReturnValue;
                                }
                                else if (ourOutcome.Continue)
                                    continue;
                                else if (ourOutcome.Leave)
                                    break;
                            }
                        }
                    }
                }
                else if (first == '~') // function declaration, just here to catch the statement beginning
                {
                    if (callDepth != 0)
                        raiseError("Functions cannot defined within anything else");
                    
                    // function has already been processed, just skip past it
                    l = findMatchingEnd(m_lines, l, endLine);
                }
                else if (first == '^') // continue
                {
                    outcome.Continue = true;
                    return object();
                }
                else if (first == 'v') // break
                {
                    outcome.Leave = true;
                    return object();
                }
                // a side-effect perhaps?  like some list.add or msdb.define...?
                else if (first == '*')
                {
                    std::wstring valueStr = trim(line.substr(1));
                    evaluate(valueStr, callDepth);
                }
                else
                {
                    raiseWError(L"Invalid statement: " + line);
                }
            }
#ifndef _DEBUG
            catch (const std::exception& exp)
            {
                handleException(exp, line, l);
            }
#endif
        }
        return object();
    }

    void script_processor::handleException(const std::exception& exp, const std::wstring& line, int l)
    {
        script_exception toThrow(std::string(exp.what()));
        toThrow.line = line;
        toThrow.lineNumber = l + 1;
        throw toThrow;
    }

    object script_processor::evaluate(const std::wstring& valueStr, unsigned callDepth)
    {
        m_tempCallDepth = callDepth;

        expression exp(m_symbols, *this);
        object answer = exp.evaluate(valueStr);
        return answer;
    }

    bool script_processor::hasFunction(const std::wstring& name) const
    {
        return m_functions.find(name) != m_functions.end();
    }

    object script_processor::callFunction(const std::wstring& name, const object::list& parameters)
    {
        auto funcIt = m_functions.find(name);
        if (funcIt == m_functions.end())
            raiseWError(L"Unknown function: " + name);

        auto func = funcIt->second;
        if (parameters.size() != func->paramNames.size())
            raiseWError(L"Function " + name + L" takes " + num2wstr(double(func->paramNames.size())) + L" parameters");

        symbol_smacker smacker(m_symbols);
        {
            symbol_stacker stacker(m_symbols);
            for (size_t p = 0; p < func->paramNames.size(); ++p)
                m_symbols.set(func->paramNames[p], parameters[p]);

            // FORNOW - Consider imported scripts
            //          m_functions should have them, but we need to process them here
            process_outcome outcome;
            object returnValue =
                process
                (
                    func->startIndex,
                    func->endIndex,
                    outcome,
                    m_tempCallDepth + 1
                );
            return returnValue;
        }
    }

    int script_processor::findMatchingEnd(const std::vector<std::wstring>& lines, int startIndex, int endIndex)
    {
        int blockCount = 0;
        bool lastBlockBeginWasWhen = false;
        for (int i = startIndex; i <= endIndex; ++i)
        {
            std::wstring line = trim(lines[i]);
            if (line == L"}")
            {
                --blockCount;
                lastBlockBeginWasWhen = false;
            }
            else if (isLineBlockBegin(line))
            {
                bool isWhen = startsWith(line, L"? ");
                if (!(isWhen && lastBlockBeginWasWhen))
                {
                    ++blockCount;
                    lastBlockBeginWasWhen = isWhen;
                }
            }

            if (blockCount < 0)
                raiseError("Too many } found");

            if (blockCount == 0)
                return i;
        }

        raiseError("End of statement not found");
    }

    std::vector<int> script_processor::findElses(const std::vector<std::wstring>& lines, int startIndex, int endIndex)
    {
        std::vector<int> retVal;

        std::vector<std::wstring> blockingLines;
        for (int i = startIndex; i <= endIndex; ++i)
        {
            std::wstring line = trim(lines[i]);
            bool isWhenBegin =
                line == L"<>"
                ||
                startsWith(line, L"? ");

            bool isEnd = line == L"}";

            std::wstring blockIn = blockingLines.empty() ? L"" : blockingLines.back();
            bool inWhenBlock =
                !blockIn.empty()
                &&
                (
                    blockIn == L"<>"
                    ||
                    startsWith(blockIn, L"? ")
                );

            if (isLineBlockBegin(line))
            {
                if (!(inWhenBlock && isWhenBegin))
                    blockingLines.push_back(line);
            }
            else if (isEnd)
                blockingLines.pop_back();

            if (blockingLines.size() == 1)
            {
                if (isWhenBegin)
                    retVal.push_back(i);
            }
            else if (blockingLines.empty())
            {
                if (isEnd)
                {
                    retVal.push_back(i);
                    return retVal;
                }
            }
        }

        raiseError("End of statement not found");
    }

    bool script_processor::isLineBlockBegin(std::wstring line)
    {
        line = trim(line);

        if (line == L"O" || line == L"{")
            return true;

        static std::vector<std::wstring> blockBeginnings{ L"?", L"@", L"#", L"~" };
        for (const auto& begin : blockBeginnings)
        {
            if (startsWith(line, begin))
                return true;
        }
        return false;
    }
}
