#pragma once

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
        /// Access the vector directly.
        /// </summary>
        const std::vector<std::pair<K, V>>& vec() const
        {
            return m_vec;
        }

        /// <summary>
        /// Access the map directly.
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
        /// Add a key-value pair to this.
        /// </summary>
        void insert(const K& key, const V& val)
        {
            if (contains(key))
                throw std::runtime_error("key already exists");
            m_map.insert({ key, val });
            m_vec.push_back({ key, val });
        }

        /// <summary>
        /// Get a value, checking if it exists first.
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

    private:
        std::unordered_map<K, V> m_map;
        std::vector<std::pair<K, V>> m_vec;
    };
}
