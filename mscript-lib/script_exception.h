#pragma once

#include "utils.h"

namespace mscript
{
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
        int lineNumber = -1;
    };
}
