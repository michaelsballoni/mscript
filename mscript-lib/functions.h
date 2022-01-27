#pragma once

#include "object.h"

#include <string>
#include <vector>

namespace mscript
{
    /// <summary>
    /// What do user-defined functions look like?
    /// </summary>
    struct script_function
    {
        std::wstring name;
        std::vector<std::wstring> paramNames;

        std::wstring filename;
        size_t startIndex = 0;
        size_t endIndex = 0;
    };
}
