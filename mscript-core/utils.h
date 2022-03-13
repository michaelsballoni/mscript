#pragma once

#include "user_exception.h"

#include <string>
#include <vector>

// Use macros for exception raising helpers to not pollute the stack trace
#define raiseError(msg) throw mscript::user_exception(toWideStr(msg))
#define raiseWError(msg) throw mscript::user_exception(std::wstring(msg))

namespace mscript
{
    std::wstring num2wstr(double num);
    std::string num2str(double num);

    std::string toNarrowStr(const std::wstring& str);
    std::wstring toWideStr(const std::string& str);

    std::wstring toLower(const std::wstring& str);

    std::wstring join(const std::vector<std::wstring>& strs, const wchar_t* seperator);

    std::wstring trim(const std::wstring& str);

    std::vector<std::wstring> split(std::wstring str, const wchar_t* seperator);

    std::wstring replace(const std::wstring& str, const std::wstring& from, const std::wstring& to);

    bool startsWith(const std::wstring& str, const std::wstring& starter);
    bool endsWith(const std::wstring& str, const std::wstring& finisher);
}
