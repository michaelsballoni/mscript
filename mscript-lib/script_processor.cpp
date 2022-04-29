#include "pch.h"
#include "script_processor.h"
#include "script_utils.h"
#include "preprocess.h"
#include "syncheck.h"
#include "names.h"
#include "lib.h"

#undef min
#undef max

// DEBUG
#define CATCH_SCRIPT_EXCEPTIONS

namespace mscript
{
    object script_processor::process(const std::wstring& currentFilename, const std::wstring& newFilename)
    {
        // Don't process scripts more than once
        if (m_linesDb.find(newFilename) != m_linesDb.end())
            return object();

        std::vector<std::wstring> lines = m_scriptLoader(currentFilename, newFilename);
        
        preprocess(lines);
        syncheck(newFilename, lines, 0, int(lines.size()) - 1);

        m_linesDb.emplace(newFilename, lines);

        preprocessFunctions(currentFilename, newFilename); // scan for functions first

        process_outcome outcome;
        object ret_val =
            process
            (
                currentFilename, 
                newFilename, 
                0, 
                int(m_linesDb[newFilename].size()) - 1, 
                outcome, 
                0U
            );
        return ret_val;
    }

    void script_processor::preprocessFunctions(const std::wstring& previousFilename, const std::wstring& filename)
    {
        const std::vector<std::wstring>& lines = m_linesDb[filename];
        int lineCount = int(lines.size());
        for (int l = 0; l < lineCount; ++l)
        {
            const std::wstring& line = lines[l];
            if (line.empty())
                continue;
#ifndef _DEBUG
            try
#endif
            {
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
                paramListStr = replace(paramListStr, L"(", L"");
                paramListStr = replace(paramListStr, L")", L"");
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

                int loopEnd = findMatchingEnd(lines, l, int(lines.size()) - 1);
                int loopStart = l;
                l = loopEnd;

                syncheck(filename, lines, loopStart + 1, loopEnd - 1);

                script_function function;
                function.previousFilename = previousFilename;
                function.filename = filename;
                function.name = name;
                function.paramNames = paramList;
                function.startIndex = loopStart + 1;
                function.endIndex = loopEnd - 1;

                m_functions.insert({ name, std::make_shared<script_function>(function) });
            }
#ifndef _DEBUG
            catch (const std::exception& exp)
            {
                handleException(exp, filename, line, l);
            }
#endif
        }
    }

    object 
    script_processor::process
    (
        const std::wstring& previousFilename, 
        const std::wstring& filename,
        int startLine, 
        int endLine, 
        process_outcome& outcome, 
        unsigned callDepth
    )
    {
        user_exception curException;
        const std::vector<std::wstring>& lines = m_linesDb[filename];
        for (int l = startLine; l <= endLine; ++l)
        {
            std::wstring line = lines[l];
            if (line.empty()) // skip blank lines
                continue;

            auto first = line[0];
#ifdef CATCH_SCRIPT_EXCEPTIONS
            try
#endif
            {
                if (startsWith(line, L">>")) // single line verbatim print
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
                else if (first == '$') // variable declaration, initial value optional
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
                else if (first == '&') // variable assignment
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
                else if (first == '!') // exception handler
                {
                    int loopEnd = findMatchingEnd(lines, l, endLine);
                    int loopStart = l;
                    l = loopEnd;

                    if (curException.obj != object::NOTHING)
                    {
                        line = line.substr(1);
                        size_t spaceIndex = line.find(' ');
                        if (spaceIndex == std::wstring::npos)
                            raiseError("Exception handler lacks catch variable");

                        std::wstring label = trim(line.substr(spaceIndex + 1));
                        validateName(label);

                        symbol_stacker stacker(m_symbols);
                        m_symbols.set(label, curException.obj);
                        process_outcome ourOutcome;
                        process
                        (
                            previousFilename,
                            filename,
                            loopStart + 1,
                            loopEnd - 1,
                            ourOutcome,
                            callDepth + 1
                        );
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
                }
                else if (line == L"O") // infinite loop
                {
                    int loopEnd = findMatchingEnd(lines, l, endLine);
                    int loopStart = l;
                    l = loopEnd;

                    while (true)
                    {
                        symbol_stacker stacker(m_symbols);
                        process_outcome ourOutcome;
                        process
                        (
                            previousFilename, 
                            filename, 
                            loopStart + 1, 
                            loopEnd - 1, 
                            ourOutcome, 
                            callDepth + 1
                        );
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
                    int loopEnd = findMatchingEnd(lines, l, endLine);
                    int loopStart = l;
                    l = loopEnd;

                    symbol_stacker stacker(m_symbols);
                    process_outcome ourOutcome;
                    process
                    (
                        previousFilename,
                        filename,
                        loopStart + 1,
                        loopEnd - 1,
                        ourOutcome,
                        callDepth + 1
                    );
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
                else if (line == L"<-") // void return statement
                {
                    outcome.Return = true;
                    return object();
                }
                else if (startsWith(line, L"<- ")) // valued return statement
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
                else if (first == '+')
                {
                    std::wstring newFilename = trim(line.substr(1));
                    if (newFilename.empty())
                        raiseError("import statement has no file name");
                    object filenameObj = evaluate(newFilename, callDepth);
                    if (filenameObj.type() != object::STRING)
                        raiseError("import statement does not evaluate as string");
                    newFilename = filenameObj.stringVal();
                    if (newFilename.empty())
                        raiseError("import statement evaluates to an empty string");

                    if (endsWith(newFilename, L".ms"))
                    {
                        process(filename, newFilename);
                    }
                    else
                    {
                        std::wstring moduleFilePath = m_moduleLoader(newFilename);
                        lib::loadLib(moduleFilePath);
                    }
                }
                else if (first == '?') // if else
                {
                    bool seenQuestion = false;
                    bool seenEndingElse = false;
                    
                    auto markers = findElses(lines, l, endLine);

                    int endMarker = markers.back();
                    l = endMarker;

                    const int max_markers_idx = int(markers.size()) - 1;
                    for (int m = 0; m <= max_markers_idx; ++m)
                    {
                        const int marker_line_idx = markers[m];
                        const std::wstring& marker_line = lines[marker_line_idx];
                        if (marker_line == L"}")
                            continue;

                        if (m >= max_markers_idx)
                            raiseError("No ? or <> at end of statement");

                        int next_marker_line_idx = markers[m + 1];
                        const std::wstring& next_marker_line = lines[next_marker_line_idx];
                        if (next_marker_line != L"}")
                            --next_marker_line_idx;

                        object answer;
                        if (startsWith(marker_line, L"? "))
                        {
                            seenQuestion = true;

                            if (seenEndingElse)
                                raiseError("Already seen <> statement");

                            size_t spaceIndex = marker_line.find(' ');
                            std::wstring criteria = marker_line.substr(spaceIndex + 1);
                            answer = evaluate(criteria, callDepth);
                            if (answer.type() != object::BOOL)
                                raiseError("? expression does not evaluate to true or false");
                        }
                        else if (marker_line == L"<>")
                        {
                            if (seenEndingElse)
                                raiseError("Already seen <> statement");

                            if (!seenQuestion)
                                raiseError("No ? statement before <> statement");

                            seenEndingElse = true;
                            answer = true;
                        }
                        else
                            raiseWError(L"Invalid line, not ? or <>: " + marker_line);

                        if (!answer.boolVal())
                            continue;

                        symbol_stacker stacker(m_symbols);
                        process_outcome ourOutcome;
                        process
                        (
                            previousFilename,
                            filename,
                            marker_line_idx + 1,
                            next_marker_line_idx - 1,
                            ourOutcome,
                            callDepth + 1
                        );
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

                    int loopEnd = findMatchingEnd(lines, l, endLine);
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
                            process
                            (
                                previousFilename,
                                filename,
                                loopStart + 1,
                                loopEnd - 1,
                                ourOutcome,
                                callDepth + 1
                            );
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
                    static int arrowLen = int(strlen(" -> "));
                    int theRestSize = int(theRest.size());
                    for (int f = 0; f < theRestSize - arrowLen; ++f)
                    {
                        auto c = theRest[f];
                        if (c == '\"')
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

                    int loopEnd = findMatchingEnd(lines, l, endLine);
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
                                process
                                (
                                    previousFilename,
                                    filename,
                                    loopStart + 1,
                                    loopEnd - 1,
                                    ourOutcome,
                                    callDepth + 1
                                );
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
                                process
                                (
                                    previousFilename,
                                    filename,
                                    loopStart + 1,
                                    loopEnd - 1,
                                    ourOutcome,
                                    callDepth + 1
                                );
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
                else if (first == '~') // function declaration, just here to skip it
                {
                    if (callDepth != 0)
                        raiseError("Functions cannot defined within anything else");
                    
                    // function has already been processed, just skip past it
                    l = findMatchingEnd(lines, l, endLine);
                }
                else if (line == L"^") // continue
                {
                    outcome.Continue = true;
                    return object();
                }
                else if (line == L"v" || line == L"V") // break
                {
                    outcome.Leave = true;
                    return object();
                }
                // execute code with a side-effect, like *some_list.add('something')
                else if (first == '*')
                {
                    std::wstring valueStr = trim(line.substr(1));
                    evaluate(valueStr, callDepth);
                }
                else
                {
                    raiseWError(L"Invalid statement: " + line);
                }

                curException.obj = object();
            }
#ifdef CATCH_SCRIPT_EXCEPTIONS
            catch (const user_exception& userExp)
            {
                if (userExp.isSyntaxError)
                    throw userExp;

                if (curException.obj.type() != object::NOTHING)
                    throw curException;

                curException.obj = userExp.obj;

                if (curException.filename.empty())
                {
                    curException.filename = filename;
                    curException.lineNumber = l;
                    curException.line = line;
                }

                bool foundHandler = false;
                while (++l < endLine)
                {
                    if (lines[l][0] == '!')
                    {
                        --l;
                        foundHandler = true;
                        break;
                    }
                }
                if (foundHandler)
                    continue;
                else
                    throw curException;
            }
#endif
#ifndef _DEBUG
            catch (const std::exception& exp)
            {
                handleException(exp, filename, line, l);
            }
#endif
        }
        return object();
    }

    void script_processor::handleException(const std::exception& exp, const std::wstring& filename, const std::wstring& line, int l)
    {
        throw script_exception(exp.what(), filename, l + 1, line);
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
        return name == L"input" ||m_functions.find(name) != m_functions.end();
    }

    object script_processor::callFunction(const std::wstring& name, const object::list& parameters)
    {
        if (name == L"input")
        {
            auto input = m_input();
            if (input.has_value())
                return *input;
            else
                return object();
        }

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

            process_outcome outcome;
            object returnValue =
                process
                (
                    func->previousFilename,
                    func->filename,
                    func->startIndex,
                    func->endIndex,
                    outcome,
                    m_tempCallDepth + 1
                );
            return returnValue;
        }
    }
}
