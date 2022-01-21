#pragma once

#include <stdexcept>
#include <string>
#include <vector>

// Use macros for exception raising helpers to not pollute the stack trace
#define raiseError(msg) throw std::runtime_error(std::string((msg)).c_str())
#define raiseWError(msg) throw std::runtime_error(std::string(toNarrowStr((msg))).c_str())

namespace mscript
{
    std::wstring num2wstr(double num);
    std::string num2str(double num);

    std::string toNarrowStr(const std::wstring& str);
    std::wstring toWideStr(const std::string& str);

    std::wstring join(const std::vector<std::wstring>& strs, const wchar_t* seperator);
}
