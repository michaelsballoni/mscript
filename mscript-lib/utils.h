#pragma once

#include <stdexcept>
#include <string>
#include <vector>

// Use macros for exception raising helpers to not pollute the stack trace
#define raiseError(msg) throw std::runtime_error(std::string(msg).c_str())
#define raiseWError(msg) throw std::runtime_error(toNarrowStr(msg).c_str())

namespace mscript
{
    std::wstring num2wstr(double num);
    std::string num2str(double num);

    std::string toNarrowStr(const std::wstring& str);
    std::wstring toWideStr(const std::string& str);

    std::wstring join(const std::vector<std::wstring>& strs, const wchar_t* seperator);

    std::wstring trim(const std::wstring& str);

    std::vector<std::wstring> split(std::wstring str, const wchar_t* seperator);

    std::wstring replace(const std::wstring& str, const std::wstring& from, const std::wstring& to);

    bool startsWith(const std::wstring& str, const std::wstring& starter);
}
