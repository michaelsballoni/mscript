#pragma once

#include "callable.h"
#include "object.h"
#include "symbols.h"
#include "tracing.h"

#include <string>

namespace mscript
{
    /// <summary>
    /// Expressions take symbols and outside functions
    /// and evaluate expression strings into values
    /// </summary>
    class expression
    {
        friend class ExpressionTests;

    public:
        /// <summary>
        /// Expression objects need symbols and access to functions
        /// for processing non-trivial expressions
        /// </summary>
        /// <param name="symbols"></param>
        /// <param name="callable"></param>
        expression(symbol_table& symbols, callable& callable, tracing& traceInfo, bool allowDynamicCalls = false)
            : m_symbols(symbols)
            , m_callable(callable)
            , m_traceInfo(traceInfo)
            , m_allowDynamicCalls(allowDynamicCalls)
        {}

        /// <summary>
        /// Evaluate an expression
        /// </summary>
        /// <param name="expStr">The expression string to evaluate</param>
        /// <returns>The value from evaluating the expression</returns>
        object evaluate(std::wstring expStr);

    private: // implementation
        static bool isCharAlphaOpBoundary(wchar_t c);
        static bool isOperator(const std::wstring& expr, const std::string& op, int n);
        static int reverseFind(const std::wstring& source, const std::wstring& searchW, int start);
        static std::vector<std::wstring> parseParameters(const std::wstring& expStr);
        static double getOneDouble(const object::list& paramList, const std::string& function);

        // Implement expressions that have function calls
        // This is the core runtime of mscript
        object::list processParameters(const std::vector<std::wstring>& expStrs);
        object executeFunction(std::wstring functionW, const object::list& paramList);

    private: // member data
        symbol_table& m_symbols;
        callable& m_callable;
        tracing& m_traceInfo;
        bool m_allowDynamicCalls;
    };
}
