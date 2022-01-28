#pragma once

#include "expressions.h"
#include "functions.h"
#include "object.h"
#include "symbols.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace mscript
{
    /// <summary>
    /// Central function for taking a script and processing it.
    /// </summary>
    class script_processor : public callable
	{
	public:
        /// <summary>
        /// Create a script processor
        /// </summary>
        /// <param name="filename">Name of the script being processed</param>
        /// <param name="expStr">Script to process</param>
        /// <param name="symbols">Symbols </param>
        script_processor
        (
            const std::wstring& filename,
            std::function<std::vector<std::wstring>(const std::wstring& filename)> fileLoader,
            symbol_table& symbols,
            std::unordered_map<std::wstring, std::shared_ptr<script_function>>& functions,
            std::function<void (const std::wstring& text)> output
        )
        : m_filename(filename)
        , m_fileLoader(fileLoader)
        , m_symbols(symbols)
        , m_functions(functions)
        , m_output(output)
        {
            m_lines = m_fileLoader(m_filename);
        }

        /// <summary>
        /// Process the entire script
        /// </summary>
        /// <returns>Return value from the overall script</returns>
        object process();

        // Good, bad, and ugly
        std::wstring Error;
        int ErrorLineNumber = -1;
        std::wstring ErrorLine;

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
        /// <returns>Return value from a function call, or null</returns>
        object process
        (
            int startLine, 
            int endLine, 
            process_outcome& outcome, 
            unsigned callDepth
        );
        
        // Scan the script looking for function declarations
        // You can only call functions already loaded from another script
        // or functions declared in this script...
        // ...not functions in some unknown later script
        void preprocessFunctions();

        void handleException(const std::exception& exp, const std::wstring& line, int l);
        object evaluate(const std::wstring& valueStr, unsigned callDepth);

        static int findMatchingEnd(const std::vector<std::wstring>& lines, int startIndex, int endIndex);
        static std::vector<int> findElses(const std::vector<std::wstring>& lines, int startIndex, int endIndex);
        static bool isLineBlockBegin(std::wstring line);

    private:
        const std::wstring m_filename;
        std::function<std::vector<std::wstring>(const std::wstring& filename)> m_fileLoader;

        std::vector<std::wstring> m_lines;

        symbol_table& m_symbols;

        unsigned m_tempCallDepth = 0;

        std::unordered_map<std::wstring, std::shared_ptr<script_function>>& m_functions;

        std::function<void(const std::wstring& text)> m_output;
    };
}
