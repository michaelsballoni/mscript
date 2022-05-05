#include "pch.h"
#include "syncheck.h"
#include "names.h"
#include "script_utils.h"
#include "utils.h"

void 
mscript::syncheck
(
    const std::wstring& filename, 
    const std::vector<std::wstring>& lines, 
    int startLine, 
    int endLine
)
{
    for (int l = startLine; l <= endLine; ++l)
    {
        std::wstring line = lines[l];
        if (line.empty())
            continue;

        try
        {
            wchar_t first = line[0];

            if (startsWith(line, L">>"))
            {
            }
            else if (first == '>')
            {
            }
            else if (first == '$')
            {
                line = line.substr(1);
                size_t equalsIndex = line.find('=');
                if (equalsIndex == std::wstring::npos)
                    raiseError("Variable declaraion lacks initial value");
            }
            else if (first == '&')
            {
                line = line.substr(1);
                size_t equalsIndex = line.find('=');
                if (equalsIndex == std::wstring::npos)
                    raiseError("Variable assignment lacks value");
            }
            else if (first == '!')
            {
                int loopEnd = findMatchingEnd(lines, l, endLine);
                int loopStart = l;
                l = loopEnd;

                line = line.substr(1);
                size_t spaceIndex = line.find(' ');
                if (spaceIndex == std::wstring::npos)
                    raiseError("Exception handler lacks catch variable");

                std::wstring label = trim(line.substr(spaceIndex + 1));
                validateName(label);

                syncheck(filename, lines, loopStart + 1, loopEnd - 1);
            }
            else if (line == L"O")
            {
                int loopEnd = findMatchingEnd(lines, l, endLine);
                int loopStart = l;
                l = loopEnd;

                syncheck(filename, lines, loopStart + 1, loopEnd - 1);
            }
            else if (first == '{')
            {
                int loopEnd = findMatchingEnd(lines, l, endLine);
                int loopStart = l;
                l = loopEnd;

                syncheck(filename, lines, loopStart + 1, loopEnd - 1);
            }
            else if (line == L"<-")
            {
            }
            else if (startsWith(line, L"<- "))
            {
            }
            else if (first == '#' || startsWith(line, L"++") || startsWith(line, L"--"))
            {
                std::string err_prefix =
                    first == '#'
                    ? "#"
                    : startsWith(line, L"++")
                    ? "++"
                    : "--";
                size_t firstSpace = line.find(' ');
                if (firstSpace == std::wstring::npos)
                    raiseError(err_prefix + " statement missing first space");

                size_t nextSpace = line.find(' ', firstSpace + 1);
                if (nextSpace == std::wstring::npos)
                    raiseError(err_prefix + " statement lacks counter variable");

                std::wstring label = trim(line.substr(firstSpace, nextSpace - firstSpace));
                validateName(label);

                size_t thirdSpace = line.find(' ', nextSpace + 1);
                if (thirdSpace == std::wstring::npos)
                    raiseError(err_prefix + " statement lacks from part");

                std::wstring from = trim(line.substr(nextSpace, thirdSpace - nextSpace));
                if (from != L":")
                    raiseError(err_prefix + " statement invalid : part");

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

                int loopEnd = findMatchingEnd(lines, l, endLine);
                int loopStart = l;
                l = loopEnd;

                syncheck(filename, lines, loopStart + 1, loopEnd - 1);
            }
            else if (first == '+')
            {
            }
            else if (first == '?')
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

                    if (startsWith(marker_line, L"? "))
                    {
                        seenQuestion = true;

                        if (seenEndingElse)
                            raiseError("Already seen <> statement");
                    }
                    else if (marker_line == L"<>")
                    {
                        if (seenEndingElse)
                            raiseError("Already seen <> statement");

                        if (!seenQuestion)
                            raiseError("No ? statement before <> statement");

                        seenEndingElse = true;
                    }
                    else
                        raiseWError(L"Invalid line, not ? or <>: " + marker_line);

                    syncheck(filename, lines, marker_line_idx + 1, next_marker_line_idx - 1);
                }
            }
            else if (first == '@')
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

                syncheck(filename, lines, loopStart + 1, loopEnd - 1);
            }
            else if (first == '~')
            {
                l = findMatchingEnd(lines, l, endLine);
            }
            else if (line == L"^")
            {
            }
            else if (line == L"v" || line == L"V")
            {
            }
            else if (first == '*')
            {
            }
            else
                raiseWError(L"Invalid statement: " + line);
        }
        catch (user_exception userExp)
        {
            userExp.isSyntaxError = true;
            if (userExp.filename.empty())
            {
                userExp.filename = filename;
                userExp.lineNumber = l + 1;
                userExp.line = line;
            }
            throw userExp;
        }
    }
}
