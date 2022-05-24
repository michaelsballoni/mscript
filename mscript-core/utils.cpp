#include "pch.h"
#include "utils.h"

#include <Windows.h>

namespace mscript
{
    std::wstring mscript::num2wstr(double num)
    {
        std::wstringstream ss;
        ss << std::setprecision(10) << num;
        return ss.str();
    }

    std::string mscript::num2str(double num)
    {
        std::stringstream ss;
        ss << std::setprecision(10) << num;
        return ss.str();
    }

    std::wstring mscript::toWideStr(const std::string& str)
    {
        if (str.empty())
            return std::wstring();

        bool allNarrow = true;
        {
            const unsigned char* bytes = reinterpret_cast<const unsigned char*>(str.data());
            for (size_t i = 0; i < str.size(); ++i)
            {
                if (bytes[i] > 127)
                {
                    allNarrow = false;
                    break;
                }
            }
        }

        if (allNarrow)
        {
            std::wstring retVal;
            retVal.reserve(str.size());
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
            if (c <= 0 || c > 127)
            {
                allAscii = false;
                break;
            }
        }

        if (allAscii)
        {
            std::string retVal;
            retVal.reserve(str.size());
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

    std::wstring mscript::toLower(const std::wstring& str)
    {
        std::wstring retVal;
        retVal.reserve(str.size());
        for (auto c : str)
            retVal += towlower(c);
        return retVal;
    }

    std::wstring mscript::toUpper(const std::wstring& str)
    {
        std::wstring retVal;
        retVal.reserve(str.size());
        for (auto c : str)
            retVal += towupper(c);
        return retVal;
    }

    std::wstring mscript::join(const std::vector<std::wstring>& strs, const std::wstring& seperator)
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
            return std::wstring();

        if (str.length() == 1)
            return iswspace(str[0]) ? std::wstring() : str;

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
    if (str.empty() || from.empty())
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

bool mscript::endsWith(const std::wstring& str, const std::wstring& finisher)
{
    if (str.empty() || finisher.empty())
        return false;

    if (finisher.length() > str.length())
        return false;

    size_t startingPoint = str.length() - finisher.length();
    for (size_t s = 0; s < finisher.length(); ++s)
    {
        if (str[s + startingPoint] != finisher[s])
            return false;
    }

    return true;
}

std::vector<std::wstring> mscript::split(const std::wstring& str, const std::wstring& seperator)
{
    std::vector<std::wstring> retVal;
    if (seperator.empty())
    {
        retVal.push_back(str);
    }
    else if (seperator.size() == 1)
    {
        std::wstring acc;
        const wchar_t sep = seperator[0];
        for (wchar_t c : str)
        {
            if (c == sep)
            {
                retVal.push_back(acc);
                acc.clear();
            }
            else
                acc.push_back(c);
        }
        if (!acc.empty())
            retVal.push_back(acc);
    }
    else
    {
        wchar_t* last_sep = const_cast<wchar_t*>(str.c_str());
        size_t sep_len = seperator.length();
        while (last_sep != nullptr && last_sep[0] != '\0')
        {
            wchar_t* next_sep = wcsstr(last_sep, seperator.c_str());
            if (next_sep == nullptr)
            {
                retVal.push_back(last_sep);
                break;
            }
            else
            {
                retVal.emplace_back(last_sep, next_sep);
                last_sep = next_sep + sep_len;
            }
        }
    }
    return retVal;
}

#if defined(_WIN32) || defined(_WIN64)
std::wstring mscript::getLastErrorMsg(DWORD dwErrorCode)
{
    wchar_t* error_str = nullptr;
    if
    (
        !FormatMessage
        (
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            dwErrorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&error_str,
            0,
            NULL
        )
    )
    {
        return L"getLastErrorMsg failed: " + std::to_wstring(dwErrorCode);
    }

    std::wstring output = 
        trim(std::wstring(error_str)) + L" (" + std::to_wstring(dwErrorCode) + L")";

    LocalFree(error_str);
    error_str = nullptr;

    return output;
}
#endif
