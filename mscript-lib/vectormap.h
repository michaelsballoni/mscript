#pragma once

#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace mscript
{
    /// <summary>
    /// A vectormap combines a map with its fast key->value lookups
    /// with a vector to preserve the order that key-value pairs were added
    /// </summary>
    /// <typeparam name="K">Key type of the map</typeparam>
    /// <typeparam name="V">Value type of the map</typeparam>
    template <typename K, typename V>
    class vectormap
    {
    public:
        /// <summary>
        /// Access the vector directly
        /// </summary>
        const std::vector<std::pair<K, V>>& vec() const
        {
            return m_vec;
        }

        /// <summary>
        /// Access the map directly
        /// </summary>
        const std::unordered_map<K, V>& map() const
        {
            return m_map;
        }

        /// <summary>
        /// What is the value for a given key?
        /// NOTE: By returning the value by, well, value
        ///       this function can stay const
        ///       and users of the class can use pointers 
        ///       if they want side effects down the line.
        /// </summary>
        V get(const K& key) const
        {
            if (!contains(key))
                throw std::runtime_error("key not found");
            return m_map.find(key)->second;
        }

        /// <summary>
        /// How many key-value pairs are in this?
        /// </summary>
        size_t size() const
        {
            return m_vec.size();
        }

        /// <summary>
        /// Does this map contain this key?
        /// </summary>
        bool contains(const K& key) const
        {
            return m_map.find(key) != m_map.end();
        }

        /// <summary>
        /// Associate a value with a key
        /// </summary>
        void set(const K& key, const V& val)
        {
            if (!contains(key))
            {
                m_map.insert({ key, val });
                m_vec.push_back({ key, val });
                return;
            }

            m_map[key] = val;

            for (auto& kvp : m_vec)
            {
                if (kvp.first == key)
                {
                    kvp.second = val;
                    break;
                }
            }
        }

        /// <summary>
        /// Get a value, checking if it exists first
        /// </summary>
        /// <param name="key">Key value to look up</param>
        /// <param name="val">Value to populate</param>
        /// <returns>true if a value exists for the key, false otherwise</returns>
        bool tryGet(const K& key, V& val) const
        {
            auto it = m_map.find(key);
            if (it == m_map.end())
                return false;

            val = it->second;
            return true;
        }

        /// <summary>
        /// Get a list of the keys of this map
        /// </summary>
        std::vector<K> keys() const
        {
            std::vector<K> retVal;
            retVal.reserve(m_vec.size());
            for (size_t k = 0; k < m_vec.size(); ++k)
                retVal.push_back(m_vec[k].first);
            return retVal;
        }

        /// <summary>
        /// Get a list of the values of this map
        /// </summary>
        std::vector<V> values() const
        {
            std::vector<V> retVal;
            retVal.reserve(m_vec.size());
            for (size_t v = 0; v < m_vec.size(); ++v)
                retVal.push_back(m_vec[v].second);
            return retVal;
        }

    private:
        std::unordered_map<K, V> m_map;
        std::vector<std::pair<K, V>> m_vec;
    };
}
