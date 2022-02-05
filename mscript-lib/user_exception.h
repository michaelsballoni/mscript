#pragma once

#include "utils.h"

namespace mscript
{
    /// <summary>
    /// user_exception has the message the scripter is sending
    /// </summary>
    class user_exception : public std::runtime_error
    {
    public:
        user_exception(const std::string& msg)
            : std::runtime_error(msg.c_str())
        {}
    };
}
