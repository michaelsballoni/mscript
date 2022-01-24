#pragma once

#include "object.h"
#include "utils.h"

#include <string>

namespace mscript
{
    /// <summary>
    /// Since expressions can involve function calls, 
    /// expressions need ways of detecting and calling functions
    /// </summary>
    class callable
    {
    public:
        virtual ~callable() {}
        virtual bool hasFunction(const std::wstring& name) const = 0;
        virtual object callFunction(const std::wstring& name, const object::list& parameters) = 0;
    };

    class no_op_callable : public callable
    {
        virtual bool hasFunction(const std::wstring& name) const
        {
            (void)name;
            return false;
        }

        virtual object callFunction(const std::wstring& name, const object::list& parameters)
        {
            (void)name;
            (void)parameters;
            raiseError("no_op_callable has no functions");
        }
    };
}