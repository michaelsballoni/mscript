#pragma once

#include "object.h"

namespace mscript
{
    /// <summary>
    /// user_exception has the message the scripter is sending
    /// </summary>
    struct user_exception
    {
    public:
        user_exception(const object& obj = object()) : obj(obj) {}

        object obj;

        std::wstring filename;
        int lineNumber = -1;
        std::wstring line;
    };
}
