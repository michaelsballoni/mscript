#include "pch.h"
#include "syncheck.h"
#include "names.h"
#include "script_utils.h"
#include "utils.h"

void mscript::syncheck(const std::vector<std::wstring>& lines, int startLine, int endLine)
{
    for (int l = startLine; l <= endLine; ++l)
    {
        std::wstring line = lines[l];
        if (line.empty())
            continue;

        wchar_t first = line[0];

        if (startsWith(line, L">>")) // single line verbatim print
        {
        }
        else if (first == '>') // single line expression print
        {
        }
        else if (first == '$') // variable declaration, initial value optional
        {
            line = line.substr(1);
            size_t equalsIndex = line.find('=');
            if (equalsIndex == std::wstring::npos)
                raiseError("Variable declaraion lacks initial value");
        }
        else if (first == '&') // variable assignment
        {
            line = line.substr(1);
            size_t equalsIndex = line.find('=');
            if (equalsIndex == std::wstring::npos)
                raiseError("Variable assignment lacks value");
        }
        else if (first == '!') // exception handler
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

            syncheck(lines, loopStart + 1, loopEnd - 1);
        }

    }
}