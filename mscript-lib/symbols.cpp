#include "pch.h"
#include "symbols.h"
#include "names.h"
#include "utils.h"

namespace mscript
{
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

    void symbol_table::restoreFrames(const symbol_table::stack& frames)
    {
        m_symbols.insert(m_symbols.end(), frames.begin(), frames.end());
    }

    bool symbol_table::contains(const std::wstring& name)
    {
        std::wstring name_lower = toLower(name);
        for (int s = int(m_symbols.size()) - 1; s >= 0; --s)
        {
            const auto& curMap = m_symbols[s];
            if (curMap.find(name_lower) != curMap.end())
                return true;
        }
        return false;
    }

    void symbol_table::set(const std::wstring& name, const object& value)
    {
        validateName(name);

        auto& dict = m_symbols.back();

        std::wstring name_lower = toLower(name);
        if (dict.find(name_lower) != dict.end())
            raiseWError(L"Name already set, you have to use a different name: " + name);

        dict.insert({ name_lower, value });
    }

    void symbol_table::assign(const std::wstring& name, const object& value, bool createIfMissing)
    {
        std::wstring name_lower = toLower(name);
        for (int s = int(m_symbols.size()) - 1; s >= 0; --s)
        {
            auto& curMap = m_symbols[s];
            if (curMap.find(name_lower) != curMap.end())
            {
                stack_entry& entry = curMap[name_lower];
                // Implement type-safe assignment
                // If it ever had a non-null value, subsequent assignments
                // have to be to values of the same type
                if 
                (
                    entry.everType == object::NOTHING 
                    || 
                    entry.value.type() == value.type()
                )
                {
                    entry.value = value;
                    if (value.type() != object::NOTHING)
                        entry.everType = value.type();
                    return;
                }
                else
                    raiseWError(L"Invalid assignment, type mismatch: " + name);
            }
        }
        if (createIfMissing)
            set(name, value);
        else
            raiseWError(L"Name not set: " + name);
    }

    bool symbol_table::tryGet(const std::wstring& name, object& answer)
    {
        std::wstring name_lower = toLower(name);
        answer = object();
        for (int s = int(m_symbols.size()) - 1; s >= 0; --s)
        {
            const auto& curMap = m_symbols[s];
            const auto& it = curMap.find(name_lower);
            if (it != curMap.end())
            {
                answer = it->second.value;
                return true;
            }
        }
        return false;
    }

    object symbol_table::get(const std::wstring& name)
    {
        object answer;
        if (!tryGet(name, answer))
            raiseWError(L"Name not assigned a value: " + name);
        return answer;
    }
}
