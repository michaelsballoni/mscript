#pragma once

#include "callable.h"
#include "object.h"
#include "symbols.h"

#include <string>

namespace mscript
{
    /// <summary>
    /// Expressions take symbols and outside functions
    /// and evaluate expression strings into values
    /// </summary>
    class expression
    {
    public:
        expression(symbol_table& symbols, callable& callable)
            : m_symbols(symbols)
            , m_callable(callable)
        {
        }

        /// <summary>
        /// Evaluate an expression string
        /// </summary>
        /// <param name="expStr">The expression string to evaluate.</param>
        /// <returns>The value from evaluating the expression</returns>
        object evaluate(std::wstring expStr);

        // Only public for unit testing...just call evaluate!
        static bool isOperator(std::wstring expr, const std::string& op, int n);
        static int reverseFind(const std::wstring& source, const std::wstring& searchW, int start);
        static std::vector<std::wstring> parseParameters(const std::wstring& expStr);
        static double getOneDouble(const object::list& paramList, const std::string& function);

        object::list processParameters(const std::vector<std::wstring>& expStrs);
        object executeFunction(const std::wstring& functionW, const object::list& paramList);

    private:
        symbol_table& m_symbols;
        callable& m_callable;
    };
}
