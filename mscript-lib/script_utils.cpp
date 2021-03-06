#include "pch.h"
#include "script_utils.h"
#include "utils.h"

int mscript::findMatchingEnd(const std::vector<std::wstring>& lines, int startIndex, int endIndex)
{
    if (startsWith(lines[startIndex], L"? "))
    {
        auto markers = findElses(lines, startIndex, endIndex);
        if (markers.empty())
            raiseError("No } found in ? statement");
        else
            return markers.back();
    }

    int block_depth = 0;
    for (int i = startIndex; i <= endIndex; ++i)
    {
        const std::wstring& line = lines[i];

        bool isEnd = line == L"}";
        bool is_block_begin = isLineBlockBegin(line);

        if (is_block_begin)
            ++block_depth;
        else if (isEnd)
            --block_depth;

        if (block_depth == 0)
            return i;
    }

    raiseError("End of statement not found");
}

std::vector<int> mscript::findElses(const std::vector<std::wstring>& lines, int startIndex, int endIndex)
{
    int block_depth = 0;

    bool last_when_start = false;
    bool last_when_end = false;

    std::vector<int> retVal;
    for (int i = startIndex; i <= endIndex; ++i)
    {
        const std::wstring& line = lines[i];

        bool isEnd = line == L"}";
        bool is_block_begin = isLineBlockBegin(line);

        if (is_block_begin)
            ++block_depth;
        else if (isEnd)
            --block_depth;

        if (block_depth == 1)
        {
            if (startsWith(line, L"? "))
            {
                if (last_when_end)
                    break;
                else // fresh ?
                    retVal.push_back(i);

                last_when_start = true;
                last_when_end = false;
            }
            else if (line == L"<>")
            {
                if (!last_when_start)
                    raiseError("No ? found for <>");
                else // fresh <>
                    retVal.push_back(i);

                last_when_start = false;
                last_when_end = true;
            }
            else if (is_block_begin)
                break;
        }
        else if (block_depth == 0)
        {
            if (isEnd)
            {
                if (last_when_start || last_when_end)
                    retVal.push_back(i);
                else
                    break;
            }
            else
                break;
        }
    }

    if (retVal.size() < 2)
        raiseError("End of ? / <> statement not found");
    else
        return retVal;
}


bool mscript::isLineBlockBegin(const std::wstring& line)
{
    if (line.empty())
        return false;

    if (iswalpha(line[0]) && line[0] != 'O')
        return false;

    wchar_t startC = line[0];
    static std::vector<char> blockBeginnings{ '?', '@', '#', '~', '!' };
    for (char blockC : blockBeginnings)
    {
        if (startC == blockC)
            return true;
    }

    if (line == L"<>" || line == L"{" || line == L"O")
        return true;

    if (startsWith(line, L"++") || startsWith(line, L"--"))
        return true;

    return false;
}