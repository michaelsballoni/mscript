#pragma once

#include "object.h"

#include <string>
#include <vector>

namespace mscript
{
    /// <summary>
    /// User-defined functions have a filename, name, parameter names,
    /// and where in the source the function body is found
    /// </summary>
    struct script_function
    {
        std::wstring previousFilename;
        std::wstring filename;

        std::wstring name;
        std::vector<std::wstring> paramNames;

        int startIndex = -1;
        int endIndex = -1;
    };
}
