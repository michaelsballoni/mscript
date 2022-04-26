#include "pch.h"
#include "preprocess.h"
#include "utils.h"

void mscript::preprocess(std::vector<std::wstring>& lines)
{
    // Trim lines once and hopefully for all
    for (auto& line : lines)
    {
        if (!line.empty())
            line.assign(trim(line));
    }

    // Pre-process comments to avoid machinations in the script processor
    bool inBlockComment = false;
    for (size_t l = 0; l < lines.size(); ++l)
    {
        auto& line = lines[l];
        if (line.empty())
            continue;

        size_t lineCommentStart = line.find(L"//");
        if (lineCommentStart != std::wstring::npos)
        {
            if (lineCommentStart == 0)
                line.clear();
            else // keep leading up to start of comment
                line.assign(trim(line.substr(0, lineCommentStart)));
        }
        
        if (!inBlockComment) // don't start new block comment when in existing
        {
            static std::wstring block_comment_starter = L"/*";
            size_t blockCommentStart = line.find(block_comment_starter);
            if (blockCommentStart != std::wstring::npos)
            {
                if (blockCommentStart == 0)
                    line.clear();
                else
                    line.assign(trim(line.substr(0, blockCommentStart)));
                inBlockComment = true;
            }
        }
        else if (inBlockComment)
        {
            static std::wstring block_comment_ender = L"*/";
            size_t blockCommentEnd = line.find(block_comment_ender);
            if (blockCommentEnd != std::wstring::npos)
            {
                line.assign(trim(line.substr(blockCommentEnd + block_comment_ender.size())));
                inBlockComment = false;
            }
            else
                line.clear();
        }
        
        if (!line.empty() && line[0] == '/') // traditional single-line comment
            line.clear();
    }
    if (inBlockComment)
        raiseError("Incomplete block comment");

    // Deal with line continuations
    const std::wstring continue_str = L" \\";
    for (size_t l = 0; l < lines.size(); ++l)
    {
        if (endsWith(lines[l], continue_str))
        {
            size_t continue_start = l;
            size_t continue_end = continue_start;
            for (size_t l2 = l + 1; l2 < lines.size(); ++l2)
            {
                continue_end = l2;
                if (!endsWith(lines[l2], continue_str))
                    break;
            }

            std::wstring one_line;
            for (size_t l3 = continue_start; l3 <= continue_end; ++l3)
            {
                std::wstring cur_line = lines[l3];
                size_t continuer_idx = cur_line.rfind(continue_str);
                if (continuer_idx != std::wstring::npos)
                    cur_line = cur_line.substr(0, continuer_idx);
                one_line += trim(cur_line) + L" ";
            }
            lines[continue_start] = trim(one_line);

            for (size_t l4 = continue_start + 1; l4 <= continue_end; ++l4)
                lines[l4].clear();

            l = continue_end;
        }
    }
}
