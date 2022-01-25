#pragma once

#include "vectormap.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#pragma warning(disable: 26812) // enum 

// We need a custom hasher for the object type, so we forward declare object
// and leave the hasher implementation until after the object implementation
namespace mscript
{
	class object;
}
namespace std
{
	template <>
	struct hash<mscript::object>
	{
		std::size_t operator()(const mscript::object& obj) const;
	};
}

namespace mscript
{
	class object
	{
	public:
		// list and index are easy to type
		typedef std::vector<object> list;
		typedef vectormap<object, object> index;

		// Who needs more the five kinds of things?
		enum object_type
		{
			NOTHING,
			NUMBER,
			STRING,
			BOOL,
			LIST,
			INDEX
		};

		object(object_type objType = NOTHING)
			: m_type(objType)
		{}
		object(double number)
			: m_type(NUMBER)
			, m_number(number)
		{}
		object(const std::wstring& stringVal)
			: m_type(STRING)
			, m_string(stringVal)
		{}
		object(bool boolVal)
			: m_type(BOOL)
			, m_bool(boolVal)
		{}
		object(const list& listVal)
			: m_type(LIST)
			, m_list(std::make_shared<list>(listVal))
		{}
		object(const index& indexVal)
			: m_type(INDEX)
			, m_index(std::make_shared<index>(indexVal))
		{}

		object_type type() const { return m_type; }
		std::string typeStr() const { return getTypeName(m_type); }

		std::wstring toString() const;
		double toNumber() const;
		size_t length() const;

		static std::string getTypeName(object_type typeVal);

		bool isNull() const { return m_type == NOTHING; }

		double numberVal() const { validateType(NUMBER); return m_number; }

		const std::wstring& stringVal() const { validateType(STRING); return m_string; }
		std::wstring& stringVal() { validateType(STRING); return m_string; }

		bool boolVal() const { validateType(BOOL); return m_bool; }

		const list& listVal() const { validateType(LIST); return *m_list; }
		list& listVal() { validateType(LIST); return *m_list; }

		const index& indexVal() const { validateType(INDEX); return *m_index; }
		index& indexVal() { validateType(INDEX); return *m_index; }

		// unordered...
		bool operator==(const object& other) const;
		bool operator!=(const object& other) const { return !operator==(other); }

		// sort
		bool operator<(const object& other) const;
		bool operator<=(const object& other) const { return *this < other || *this == other; }
		bool operator>(const object& other) const { return !(*this < other) && *this != other; }
		bool operator>=(const object& other) const { return *this > other || *this == other; }

		void validateType(object_type shouldBe) const;

	private:
		object_type m_type = NOTHING;

		double m_number = 0.0;
		std::wstring m_string;
		bool m_bool = false;

		std::shared_ptr<list> m_list;
		std::shared_ptr<index> m_index;
	};
}
