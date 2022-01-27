#pragma once

#include <stdexcept>
#include <string>
#include <vector>

// Use macros for exception raising helpers to not pollute the stack trace
#define raiseError(msg) throw mscript::script_exception(std::string((msg)).c_str())
#define raiseWError(msg) throw mscript::script_exception(std::wstring(((msg))).c_str())

namespace mscript
{
    std::wstring num2wstr(double num);
    std::string num2str(double num);

    std::string toNarrowStr(const std::wstring& str);
    std::wstring toWideStr(const std::string& str);

    std::wstring join(const std::vector<std::wstring>& strs, const wchar_t* seperator);

    std::wstring trim(const std::wstring& str);

    std::vector<std::wstring> split(const std::wstring& str, const wchar_t* seperator);

    void replace(std::wstring& str, const std::wstring& from, const std::wstring& to);

    bool startsWith(const std::wstring& str, const std::wstring& starter);

    /// <summary>
    /// script_exception has the line number and text of the line where the exception occurred
    /// </summary>
    class script_exception : public std::runtime_error
    {
    public:
        script_exception(const std::wstring& msg)
            : script_exception(toNarrowStr(msg))
        { }

        script_exception(const std::string& msg)
            : std::runtime_error(msg.c_str())
        { }

        std::wstring filename;
        std::wstring line;
        size_t lineNumber = 0;
    };
}
