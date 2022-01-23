#pragma once

#include "object.h"
#include "symbols.h"

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
        virtual bool hasFunction(const std::wstring& name) = 0;
        virtual object callFunction(const std::wstring& name, const object::list& parameters) = 0;
    };

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

    private:
        static bool isOperator(std::wstring expr, const std::string& op, int n);
        static int reverseFind(const std::wstring& source, const std::wstring& searchW, int start);
        static std::vector<std::wstring> parseParameters(const std::wstring& expStr);
        static double getOneDouble(const object::list& paramList, const std::string& function);

        object::list processParameters(const std::vector<std::wstring>& expStrs);
        object executeFunction(const std::wstring& functionW, const object::list& paramList);

        symbol_table& m_symbols;
        callable& m_callable;
    };
}
