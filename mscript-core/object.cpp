#include "pch.h"
#include "object.h"
#include "utils.h"

// Finish up the custom hasher for the object type
std::size_t std::hash<mscript::object>::operator()(const mscript::object& obj) const
{
	return std::hash<std::wstring>()(obj.toString());
}

namespace mscript
{
	object object::clone() const
	{
		switch (m_type)
		{
		case NOTHING:
			return object();

		case STRING:
			return m_string;

		case NUMBER:
			return m_number;

		case BOOL:
			return m_bool;

		case LIST:
		{
			object::list retVal;
			retVal.reserve(m_list->size());
			for (const auto& obj : *m_list)
				retVal.push_back(obj.clone());
			return retVal;
		}

		case INDEX:
		{
			object::index retVal;
			for (const auto& kvp : m_index->vec())
				retVal.set(kvp.first.clone(), kvp.second.clone());
			return retVal;
		}

		default:
			raiseError("Invalid object type for clone(): " + num2str(int(m_type)));
		}
	}

	std::wstring object::toString() const
	{
		switch (m_type)
		{
		case NOTHING:
			return L"null";

		case STRING:
			return m_string;

		case NUMBER:
			return num2wstr(m_number);

		case BOOL:
			return m_bool ? L"true" : L"false";

		case LIST:
		{
			std::vector<std::wstring> listStrs;
			for (const auto& obj : *m_list)
				listStrs.push_back(obj.toString());
			return join(listStrs, L", ");
		}

		case INDEX:
		{
			std::vector<std::wstring> indexStrs;
			for (const auto& kvp : m_index->vec())
				indexStrs.push_back(kvp.first.toString() + L": " + kvp.second.toString());
			return join(indexStrs, L", ");
		}

		default:
			raiseError("Invalid object type for toString(): " + num2str(int(m_type)));
		}
	}

	double object::toNumber() const
	{
		switch (m_type)
		{
		case STRING: return std::stod(m_string.c_str());
		case NUMBER: return m_number;
		case BOOL: return m_bool ? 1.0 : 0.0;
		default: raiseError("Cannot convert to number: " + typeStr());
		}
	}

	size_t object::length() const
	{
		switch (m_type)
		{
		case STRING: return m_string.length();
		case LIST: return m_list->size();
		case INDEX: return m_index->size();
		default: raiseError("Invalid type for length(): " + typeStr());
		}
	}

	std::string object::getTypeName(object_type typeVal)
	{
		switch (typeVal)
		{
		case NOTHING:
			return "nothing";
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

	void object::validateType(object_type shouldBe) const
	{
		if (m_type != shouldBe)
			raiseError("Invalid type access: " + getTypeName(shouldBe) + ", should be " + getTypeName(m_type));
	}

	bool object::operator==(const object& other) const
	{
		if (m_type == NOTHING || other.m_type == NOTHING)
			return m_type == other.m_type; // if both null, match, otherwise fail
		
		if (m_type != other.m_type)
			raiseError("Type mismatch for equality comparison: " + typeStr() + " and " + other.typeStr());
		
		switch (m_type)
		{
		case NUMBER:
		{
			if (m_number == other.m_number)
				return true;
			else if (int64_t(m_number) != int64_t(other.m_number))
				return false;
			else
				return num2str(m_number) == num2str(other.m_number); // handle rounding
		}
		case STRING:
		{
			return m_string == other.m_string;
		}
		case BOOL:
		{
			return m_bool == other.m_bool;
		}
		case LIST:
		{
			const auto& list1 = *m_list;
			const auto& list2 = *other.m_list;
			if (list1.size() != list2.size())
				return false;
			for (size_t i = 0; i < list1.size(); ++i)
			{
				if (list1[i] != list2[i])
					return false;
			}
			return true;
		}
		case INDEX:
		{
			const auto& keys1 = m_index->vec();
			const auto& keys2 = other.m_index->vec();
			if (keys1.size() != keys2.size())
				return false;
			for (size_t i = 0; i < keys1.size(); ++i)
			{
				if (keys1[i].first != keys2[i].first)
					return false;

				if (keys1[i].second != keys2[i].second)
					return false;
			}
			return true;
		}
		default:
			raiseError("Invalid type: " + num2str(int(m_type)));
		}
	}

	bool object::operator<(const object& other) const
	{
		if (m_type != other.m_type)
			raiseError("Type mismatch for order comparison: " + typeStr() + " and " + other.typeStr());
		
		switch (m_type)
		{
		case NUMBER:
			return m_number < other.m_number;
		case STRING:
			return m_string < other.m_string;
		default:
			raiseError("Invalid type for comparison: " + typeStr());
		}
	}
}
