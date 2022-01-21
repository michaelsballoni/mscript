#include "pch.h"
#include "utils.h"
#include "includes.h"

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

    std::string mscript::toNarrowStr(const std::wstring& str)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        return converter.to_bytes(str);
    }

    std::wstring mscript::toWideStr(const std::string& str)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(str);
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
}
