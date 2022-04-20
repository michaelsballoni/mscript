#pragma once

#include "user_exception.h"

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#endif

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
    std::wstring toUpper(const std::wstring& str);

    std::wstring join(const std::vector<std::wstring>& strs, const wchar_t* seperator);

    std::wstring trim(const std::wstring& str);

    std::vector<std::wstring> split(const std::wstring& str, const wchar_t seperator);

    std::wstring replace(const std::wstring& str, const std::wstring& from, const std::wstring& to);

    bool startsWith(const std::wstring& str, const std::wstring& starter);
    bool endsWith(const std::wstring& str, const std::wstring& finisher);

#if defined(_WIN32) || defined(_WIN64)
    std::wstring getLastErrorMsg(DWORD dwErrorCode = ::GetLastError());
#endif
}
