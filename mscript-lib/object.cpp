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
			raiseError("Invalid type access: should be " + getTypeName(shouldBe) + ", is " + getTypeName(m_type));
	}

	double object::getOneDouble(const std::string& function)
	{
		validateType(LIST);
		if (m_list.size() != 1 || m_list[0].type() != object::NUMBER)
			raiseError("Function " + function + " requires one numeric parameter");
		else
			return m_list[0].numberVal();
	}
}
