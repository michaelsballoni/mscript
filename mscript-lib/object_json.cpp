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

		void set_obj_val(const object& obj)
		{
			object& cur_obj = cur();
			if (cur_obj.type() == object::INDEX && !m_keyStack.empty())
			{
				if (!m_keyStack.empty())
				{
					cur_obj.indexVal().set(m_keyStack.back(), obj);
					m_keyStack.pop_back();
				}
				else
					raiseError("No object key in context");
			}
			else if (cur_obj.type() == object::LIST)
				cur_obj.listVal().push_back(obj);
			else
				cur_obj = obj;
		}

		bool null()
		{
			set_obj_val(object());
			return true;
		}

		bool boolean(bool val)
		{
			set_obj_val(val);
			return true;
		}

		bool number_integer(json::number_integer_t val)
		{
			set_obj_val(double(val));
			return true;
		}
		bool number_unsigned(json::number_unsigned_t val)
		{
			set_obj_val(double(val));
			return true;
		}
		bool number_float(json::number_float_t val, const json::string_t&)
		{
			set_obj_val(double(val));
			return true;
		}

		bool string(json::string_t& val)
		{
			set_obj_val(toWideStr(val));
			return true;
		}

		bool binary(json::binary_t&)
		{
			raiseError("Binary values are not allowed");
		}

		bool start_object(std::size_t)
		{
			m_objStack.push_back(object::index());
			return true;
		}

		bool end_object()
		{
			return on_end();
		}

		bool start_array(std::size_t)
		{
			m_objStack.push_back(object::list());
			return true;
		}

		bool end_array()
		{
			return on_end();
		}
		
		bool key(json::string_t& val)
		{
			m_keyStack.push_back(object(toWideStr(val)));
			return true;
		}

		bool parse_error(std::size_t pos, const std::string&, const nlohmann::detail::exception& exp)
		{
			raiseError("JSON parse error at " + std::to_string(pos) + ": " + std::string(exp.what()));
			//return false;
		}

	private:
		bool on_end()
		{
			object back_obj = m_objStack.back();
			m_objStack.pop_back();

			object& cur_obj = cur();
			if (cur_obj.type() == object::INDEX)
			{
				if (!m_keyStack.empty())
				{
					cur_obj.indexVal().set(m_keyStack.back(), back_obj);
					m_keyStack.pop_back();
				}
				else
					raiseError("No object key in context");
			}
			else if (cur_obj.type() == object::LIST)
				cur_obj.listVal().push_back(back_obj);
			else
				cur_obj = back_obj;
			return true;
		}

		std::vector<object> m_objStack;
		std::vector<object> m_keyStack;
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
