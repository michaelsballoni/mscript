#pragma once

#include "expressions.h"
#include "functions.h"
#include "object.h"
#include "symbols.h"
#include "script_exception.h"
#include "tracing.h"
#include "user_exception.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mscript
{
    /// <summary>
    /// Central class for taking a script and processing it
    /// </summary>
    class script_processor : public callable
	{
	public:
        /// <summary>
        /// Create a script processor
        /// </summary>
        /// <param name="filename">Name of the script to process</param>
        /// <param name="fileLoader">Function for loading scripts</param>
        /// <param name="symbols">Symbol table</param>
        /// <param name="functions">Function table</param>
        /// <param name="input">Function for reading from the input</param>
        /// <param name="output">Function for writing to the output</param>
        script_processor
        (
            std::function<std::vector<std::wstring>(const std::wstring& current, const std::wstring& filename)> scriptLoader,
            std::function<std::wstring(const std::wstring& filename)> moduleLoader,
            symbol_table& symbols,
            std::function<std::optional<std::wstring>()> input,
            std::function<void (const std::wstring& text)> output
        )
        : m_scriptLoader(scriptLoader)
        , m_moduleLoader(moduleLoader)
        , m_symbols(symbols)
        , m_input(input)
        , m_output(output)
        {}

        /// <summary>
        /// Process an entire script
        /// </summary>
        object process(const std::wstring& currentFilename, const std::wstring& newFilename);

        // Callable implementation
        virtual bool hasFunction(const std::wstring& name) const;
        virtual object callFunction(const std::wstring& name, const object::list& parameters);

    private:
        /// <summary>
        /// What was the outcome of processing a block of script?
        /// </summary>
        struct process_outcome
        {
            bool Continue = false;
            bool Leave = false;
            bool Return = false;

            object ReturnValue;
        };

        /// <summary>
        /// Process a section of the script, yielding an outcome
        /// </summary>
        /// <param name="startLine">What line of the script to start on?</param>
        /// <param name="endLine">What line of the script to end with?</param>
        /// <param name="outcome">Outcome object filled in by processing the script</param>
        /// <returns>Return value from a function call</returns>
        object process
        (
            const std::wstring& previousFilename,
            const std::wstring& filename,
            int startLine,
            int endLine, 
            process_outcome& outcome, 
            unsigned callDepth
        );
        
        void preprocessFunctions(const std::wstring& previousFilename, const std::wstring& filename);

        void handleException(const std::exception& exp, const std::wstring& filename, const std::wstring& line, int l);
        object evaluate(const std::wstring& valueStr, unsigned callDepth);

    private:
        std::function<std::vector<std::wstring>(const std::wstring& current, const std::wstring& filename)> m_scriptLoader;
        std::function<std::wstring(const std::wstring& filename)> m_moduleLoader;

        std::unordered_map<std::wstring, std::vector<std::wstring>> m_linesDb;

        symbol_table& m_symbols;
        std::unordered_map<std::wstring, std::shared_ptr<script_function>> m_functions;

        unsigned m_tempCallDepth = 0;

        std::function<std::optional<std::wstring>()> m_input;
        std::function<void(const std::wstring& text)> m_output;

        tracing m_traceInfo;
    };
}
