#pragma once

#include "utils.h"
#include "includes.h"

#pragma warning(disable: 26812) // enum 

// We need a customer hasher the object type, so we forward declare object
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
			NUMBER,
			STRING,
			BOOL,
			LIST,
			INDEX
		};

		object(double number = 0.0)
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

		std::wstring toString() const
		{
			switch (m_type)
			{
			case STRING:
				return m_string;

			case NUMBER:
				return num2wstr(m_number);

			case BOOL:
				return m_bool ? L"true" : L"false";

			case LIST:
			{
				std::vector<std::wstring> listStrs;
				for (const auto& obj : m_list)
					listStrs.push_back(obj.toString());
				return join(listStrs, L", ");
			}

			case INDEX:
			{
				std::vector<std::wstring> indexStrs;
				for (const auto& kvp : m_index.vec())
					indexStrs.push_back(kvp.first.toString() + L": " + kvp.second.toString());
				return join(indexStrs, L", ");
			}

			default:
				raiseError("Invalid object type: " + num2str(int(m_type)));
			}
		}

		static std::string getTypeName(object_type typeVal)
		{
			switch (typeVal)
			{
			case NUMBER:
				return "number";
			case STRING:
				return "string";
			case BOOL:
				return "bool";
			case LIST:
				return "list";
			case INDEX:
				return "index";
			default:
				raiseError("Invalid type: " + num2str(int(typeVal)));
			}
		}

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

		void validateType(object_type shouldBe) const
		{
			if (m_type != shouldBe)
				raiseError("Invalid type access: should be " + getTypeName(shouldBe) + ", is " + getTypeName(m_type));
		}

	private:
		object_type m_type = NUMBER;

		double m_number = 0.0;
		std::wstring m_string;
		bool m_bool = false;

		list m_list;
		index m_index;
	};
}

std::size_t std::hash<mscript::object>::operator()(const mscript::object& obj) const
{
	return std::hash<std::wstring>()(obj.toString());
}
