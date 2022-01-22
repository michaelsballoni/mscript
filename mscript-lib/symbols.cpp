#include "pch.h"
#include "symbols.h"
#include "names.h"
#include "utils.h"

namespace mscript
{    /// <summary>
    /// Remove all frames but the top global frame and return those
    /// </summary>
    /// <returns></returns>
    symbol_table::stack symbol_table::smackFrames()
    {
        if (m_symbols.empty())
            return symbol_table::stack();

        stack retVal;
        for (size_t i = 1; i < m_symbols.size(); ++i)
            retVal.push_back(m_symbols[i]);
        m_symbols = stack{ m_symbols[0] };
        return retVal;
    }

    /// <summary>
    /// Given previously smacked frames, restore them
    /// </summary>
    /// <param name="frames">Stack frames to restore</param>
    void symbol_table::restoreFrames(const symbol_table::stack& frames)
    {
        m_symbols.insert(m_symbols.end(), frames.begin(), frames.end());
    }

    /// <summary>
    /// Does a name exist in the symbol table?
    /// </summary>
    bool symbol_table::contains(const std::wstring& name)
    {
        for (int s = int(m_symbols.size()) - 1; s >= 0; --s)
        {
            const auto& curMap = m_symbols[s];
            if (curMap.find(name) != curMap.end())
                return true;
        }
        return false;
    }

    /// <summary>
    /// Set a new named variable with an initial value
    /// </summary>
    void symbol_table::set(const std::wstring& name, const object& value)
    {
        validateName(name);

        auto& dict = m_symbols.back();

        if (dict.find(name) != dict.end())
            raiseWError(L"Name already set, you have to use a different name: " + name);

        dict.insert({ name, value });
    }

    /// <summary>
    /// Update the value of a named variable
    /// </summary>
    void symbol_table::assign(const std::wstring& name, object value)
    {
        for (int s = int(m_symbols.size()) - 1; s >= 0; --s)
        {
            auto& curMap = m_symbols[s];
            if (curMap.find(name) != curMap.end())
            {
                curMap[name] = value;
                return;
            }
        }
        raiseWError(L"Name not set yet: " + name);
    }

    /// <summary>
    /// Try to get the value of a named variable
    /// </summary>
    bool symbol_table::tryGet(const std::wstring& name, object& answer)
    {
        answer = object();
        for (int s = int(m_symbols.size()) - 1; s >= 0; --s)
        {
            const auto& curMap = m_symbols[s];
            const auto& it = curMap.find(name);
            if (it != curMap.end())
            {
                answer = it->second;
                return true;
            }
        }
        return false;
    }

    /// <summary>
    /// Get the value of a named variable
    /// </summary>
    object symbol_table::get(const std::wstring& name)
    {
        object answer;
        if (!tryGet(name, answer))
            raiseWError(L"Name has not been assigned a value: " + name);
        return answer;
    }
}
