#pragma once

#include "vectormap.h"

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
			, m_list(listVal)
		{}
		object(const index& indexVal)
			: m_type(INDEX)
			, m_index(indexVal)
		{}

		object_type type() const { return m_type; }

		std::wstring toString() const;

		static std::string getTypeName(object_type typeVal);

		bool isNull() const { return m_type == NOTHING; }

		double numberVal() const { validateType(NUMBER); return m_number; }

		const std::wstring& stringVal() const { validateType(STRING); return m_string; }
		std::wstring& stringVal() { validateType(STRING); return m_string; }

		bool boolVal() const { validateType(BOOL); return m_bool; }

		const list& listVal() const { validateType(LIST); return m_list; }
		list& listVal() { validateType(LIST); return m_list; }

		const index& indexVal() const { validateType(INDEX); return m_index; }
		index& indexVal() { validateType(INDEX); return m_index; }

		bool operator==(const object& other) const
		{
			return m_type == other.m_type && toString() == other.toString();
		}
		bool operator!=(const object& other) const { return !operator==(other); }

		void validateType(object_type shouldBe) const;

		double getOneDouble(const std::string& function);

	private:
		object_type m_type = NOTHING;

		double m_number = 0.0;
		std::wstring m_string;
		bool m_bool = false;

		list m_list;
		index m_index;
	};
}
