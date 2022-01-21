#pragma once

#include "includes.h"

namespace mscript
{
    inline std::wstring num2wstr(double num)
    {
        std::wstringstream ss;
        ss << num;
        return ss.str();
    }

    inline std::string num2str(double num)
    {
        std::stringstream ss;
        ss << num;
        return ss.str();
    }

    inline std::string toNarrowStr(const std::wstring& str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        return converter.to_bytes(str);
    }

    inline std::wstring toWideStr(const std::string& str)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(str);
    }

    // Use macros for exception raising helpers to not pollute the stack trace
#define raiseError(msg) { throw std::runtime_error(std::string((msg)).c_str()); }
#define raiseWError(msg) { throw std::runtime_error(std::string(toNarrowStr((msg))).c_str()); }

    inline std::wstring join(const std::vector<std::wstring>& strs, const wchar_t* seperator)
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
}
