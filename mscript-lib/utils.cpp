#include "pch.h"
#include "utils.h"
#include "includes.h"

#include <Windows.h>

namespace mscript
{
    std::wstring mscript::num2wstr(double num)
    {
        std::wstringstream ss;
        ss << num;
        return ss.str();
    }

    std::string mscript::num2str(double num)
    {
        std::stringstream ss;
        ss << num;
        return ss.str();
    }

    std::wstring mscript::toWideStr(const std::string& str)
    {
        bool allNarrow = true;
        for (char c : str)
        {
            if (c < 0 || c > 127)
            {
                allNarrow = false;
                break;
            }
        }

        if (allNarrow)
        {
            std::wstring retVal;
            for (auto c : str)
                retVal += char(c);
            return retVal;
        }

        int needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), int(str.size()), nullptr, 0);
        if (needed <= 0)
            raiseError("MultiByteToWideChar failed");

        std::wstring result(needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.data(), int(str.size()), result.data(), needed);
        return result;
    }

    std::string toNarrowStr(const std::wstring& str)
    {
        if (str.empty())
            return std::string();

        bool allAscii = true;
        for (wchar_t c : str)
        {
            if (c < 0 || c > 127)
            {
                allAscii = false;
                break;
            }
        }

        if (allAscii)
        {
            std::string retVal;
            for (auto c : str)
                retVal += char(c);
            return retVal;
        }

        int needed = WideCharToMultiByte(CP_UTF8, 0, str.data(), int(str.size()), nullptr, 0, nullptr, nullptr);
        if (needed <= 0)
            raiseError("WideCharToMultiByte failed");

        std::string output(needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, str.data(), int(str.size()), output.data(), needed, nullptr, nullptr);
        return output;
    }

    std::wstring mscript::join(const std::vector<std::wstring>& strs, const wchar_t* seperator)
    {
        std::wstring retVal;
        for (const auto& str : strs)
        {
            if (!retVal.empty())
                retVal += seperator;
            retVal += str;
        }
        return retVal;
    }

    std::wstring mscript::trim(const std::wstring& str)
    {
        if (str.empty())
            return L"";

        if (str.length() == 1)
        {
            if (iswspace(str[0]))
                return L"";
            else
                return str;
        }

        if (!iswspace(str.front()) && !iswspace(str.back()))
            return str;

        std::wstring retVal;
        retVal.reserve(str.length());

        // skip whitespace
        size_t c = 0;
        while (c < str.length() && iswspace(str[c]))
            ++c;

        // copy the rest
        while (c < str.length())
            retVal.push_back(str[c++]);

        // pop whitespace
        while (!retVal.empty() && iswspace(retVal.back()))
            retVal.pop_back();

        return retVal;
    }
}

std::wstring mscript::replace(const std::wstring& str, const std::wstring& from, const std::wstring& to)
{
    if (str.empty() || str.empty())
        return std::wstring();

    std::wstring retVal = str;
    size_t pos;
    size_t offset = 0;
    const size_t fromSize = from.size();
    const size_t increment = to.size();
    while ((pos = retVal.find(from, offset)) != std::wstring::npos)
    {
        retVal.replace(pos, fromSize, to);
        offset = pos + increment;
    }
    return retVal;
}

bool mscript::startsWith(const std::wstring& str, const std::wstring& starter)
{
    if (str.empty() || starter.empty())
        return false;

    if (starter.length() > str.length())
        return false;

    for (size_t s = 0; s < starter.length(); ++s)
    {
        if (str[s] != starter[s])
            return false;
    }

    return true;
}

std::vector<std::wstring> mscript::split(std::wstring str, const wchar_t* seperator)
{
    std::vector<std::wstring> retVal;
    if (str.empty() || *seperator == 0)
        return retVal;

    wchar_t* pt = nullptr;
    wchar_t* token = wcstok_s(str.data(), seperator, &pt);
    while (token != nullptr)
    {
        retVal.push_back(token);
        token = wcstok_s(nullptr, seperator, &pt);
    }
    return retVal;
}

