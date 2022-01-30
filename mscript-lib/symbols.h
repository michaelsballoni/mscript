#pragma once

#include "object.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace mscript
{
	class symbol_table
	{
    public:
        struct stack_entry
        {
            stack_entry(object _value = object()) 
                : value(_value)
                , everType(value.type()) 
            {}

            object value;
            object::object_type everType;
        };
        typedef std::unordered_map<std::wstring, stack_entry> stack_frame;
        typedef std::vector<stack_frame> stack;

        symbol_table()
        {
            pushFrame();
        }

        void pushFrame()
        {
            m_symbols.emplace_back();
        }

        void popFrame()
        {
            m_symbols.pop_back();
        }

        /// <summary>
        /// Remove all frames but the top global frame and return those
        /// </summary>
        /// <returns></returns>
        stack smackFrames();

        /// <summary>
        /// Given previously smacked frames, restore them
        /// </summary>
        /// <param name="frames">Stack frames to restore</param>
        void restoreFrames(const stack& frames);

        /// <summary>
        /// Does a name exist in the symbol table?
        /// </summary>
        bool contains(const std::wstring& name);

        /// <summary>
        /// Set a new named variable with an initial value
        /// </summary>
        void set(const std::wstring& name, const object& value);

        /// <summary>
        /// Update the value of a named variable
        /// </summary>
        void assign(const std::wstring& name, object value);

        /// <summary>
        /// Try to get the value of a named variable
        /// </summary>
        bool tryGet(const std::wstring& name, object& answer);

        /// <summary>
        /// Get the value of a named variable
        /// </summary>
        object get(const std::wstring& name);

    private:
        stack m_symbols;
	};

    /// <summary>
    /// Remove all stack frames but the top global level on creation,
    /// then restore the frames on dispoal
    /// </summary>
    class symbol_smacker
    {
    public:
        symbol_smacker(symbol_table& table) : m_table(table)
        { 
            m_smackedFrames = m_table.smackFrames(); 
        }
        ~symbol_smacker()
        { 
            m_table.restoreFrames(m_smackedFrames); 
        }
    private:
        symbol_table& m_table;
        symbol_table::stack m_smackedFrames;
    };

    /// <summary>
    /// Push a frame on construction,
    /// the pop the frame on disposal
    /// </summary>
    class symbol_stacker
    {
    public:
        symbol_stacker(symbol_table& table) : m_table(table)
        { 
            m_table.pushFrame(); 
        }
        ~symbol_stacker()
        { 
            m_table.popFrame(); 
        }
    private:
        symbol_table& m_table;
    };
}
