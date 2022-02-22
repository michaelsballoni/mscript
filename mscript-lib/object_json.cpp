#include "pch.h"
#include "object_json.h"
#include "utils.h"

using json = nlohmann::json;

namespace mscript
{
	class mscript_json_sax
	{
	public:
		mscript_json_sax()
		{
			m_objStack.emplace_back();
		}

		object final() const
		{
			return m_objStack.front();
		}

		object& cur()
		{
			return m_objStack.back();
		}

		void setObjVal(const object& obj)
		{
			if (!m_indexStack.empty() && !m_keyStack.empty())
			{
				m_indexStack.back()->set(object(*m_keyStack.back()), obj);
				delete m_keyStack.back();
				m_keyStack.pop_back();
			}
			else if (!m_listStack.empty())
			{
				m_listStack.back()->push_back(obj);
			}
			else
				cur() = obj;
		}

		bool null()
		{
			setObjVal(object());
			return true;
		}

		bool boolean(bool val)
		{
			setObjVal(val);
			return true;
		}

		bool number_integer(json::number_integer_t val)
		{
			setObjVal(double(val));
			return true;
		}
		bool number_unsigned(json::number_unsigned_t val)
		{
			setObjVal(double(val));
			return true;
		}
		bool number_float(json::number_float_t val, const json::string_t&)
		{
			setObjVal(double(val));
			return true;
		}

		bool string(json::string_t& val)
		{
			setObjVal(toWideStr(val));
			return true;
		}

		bool binary(json::binary_t&)
		{
			raiseError("Binary values are not allowed");
		}

		bool start_object(std::size_t)
		{
			m_indexStack.push_back(new object::index());
			return true;
		}

		bool end_object()
		{
			object::index* index = m_indexStack.back();
			cur() = *index;
			m_indexStack.pop_back();
			delete index;
			return true;
		}

		bool start_array(std::size_t)
		{
			m_listStack.push_back(new object::list());
			return true;
		}

		bool end_array()
		{
			object::list* list = m_listStack.back();
			cur() = *list;
			m_listStack.pop_back();
			delete list;
			return true;
		}
		
		bool key(json::string_t& val)
		{
			m_keyStack.push_back(new std::wstring(toWideStr(val)));
			return true;
		}

		bool parse_error(std::size_t, const std::string&, const nlohmann::detail::exception&)
		{
			return false;
		}

	private:
		std::vector<object::list*> m_listStack;
		std::vector<object::index*> m_indexStack;
		std::vector<std::wstring*> m_keyStack;
		std::vector<object> m_objStack;
		// FORNOW - Fix this cur() / m_objStack business
	};

	object mscript::objectFromJson(const std::wstring& json)
	{
		mscript_json_sax my_sax;
		if (!json::sax_parse(json, &my_sax))
			raiseError("JSON parsing failed");
		return my_sax.final();
	}

	std::wstring objectToJson(const object&)
	{
		// FORNOW
		return L"";
	}
}
