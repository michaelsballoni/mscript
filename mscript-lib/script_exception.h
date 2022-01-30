#pragma once

#include "utils.h"

namespace mscript
{
    /// <summary>
    /// script_exception has the message, filename, and line info
    /// </summary>
    class script_exception : public std::runtime_error
    {
    public:
        script_exception
        (
            const std::string& msg, 
            const std::wstring& filename, 
            int lineNumber, 
            const std::wstring& line
        )
        : std::runtime_error(msg.c_str())
        , filename(filename)
        , lineNumber(lineNumber)
        , line(line)
        {}

        std::wstring filename;
        int lineNumber = -1;
        std::wstring line;
    };
}
