#include "pch.h"
#include "object_json.h"
#include "utils.h"
#include "../../json/single_include/nlohmann/json.hpp"

#undef max

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
			raiseError("Binary values are not allowed in mscript JSON");
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
		void set_obj_val(const object& obj)
		{
			object& cur_obj = m_objStack.back();
			if (cur_obj.type() == object::INDEX)
			{
				if (m_keyStack.empty())
					raiseError("No object key in context");
				cur_obj.indexVal().set(m_keyStack.back(), obj);
				m_keyStack.pop_back();
			}
			else if (cur_obj.type() == object::LIST)
				cur_obj.listVal().push_back(obj);
			else
				cur_obj = obj;
		}

		bool on_end()
		{
			object back_obj = m_objStack.back();
			m_objStack.pop_back();

			object& cur_obj = m_objStack.back();
			if (cur_obj.type() == object::INDEX)
			{
				if (m_keyStack.empty())
					raiseError("No object key in context");
				cur_obj.indexVal().set(m_keyStack.back(), back_obj);
				m_keyStack.pop_back();
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
		object final = my_sax.final();
		return final;
	}

	std::wstring objectToJson(const object& obj)
	{
		switch (obj.type())
		{
		case object::NOTHING:
			return L"null";
		case object::BOOL:
			return obj.boolVal() ? L"true" : L"false";
		case object::NUMBER:
			return num2wstr(obj.numberVal());
		case object::STRING:
		{
			std::wstring out_str;
			out_str.reserve(std::max(size_t(2), obj.stringVal().capacity() * 2));
			out_str += L"\"";
			for (wchar_t c : obj.stringVal())
			{
				switch (c)
				{
				case '\\': out_str += L"\\\\"; break;
				case '/': out_str += L"\\/"; break;
				case '\"': out_str += L"\\\""; break;
				case '\b': out_str += L"\\b"; break;
				case '\f': out_str += L"\\f"; break;
				case '\n': out_str += L"\\n"; break;
				case '\r': out_str += L"\\r"; break;
				case '\t': out_str += L"\\t"; break;
				default: out_str += c;
				}
			}
			out_str += L"\"";
			return out_str;
		}
		case object::LIST:
		{
			const object::list& list = obj.listVal();
			std::vector<std::wstring> strs;
			strs.reserve(list.size());
			for (size_t e = 0; e < list.size(); ++e)
				strs.push_back(objectToJson(list[e]));
			return L"[" + join(strs, L", ") + L"]";
		}
		case object::INDEX:
		{
			const object::index& index = obj.indexVal();
			std::vector<std::wstring> strs;
			strs.reserve(index.size());
			for (size_t e = 0; e < index.vec().size(); ++e)
			{
				const auto& pair = index.vec()[e];
				strs.push_back(objectToJson(pair.first) + L": " + objectToJson(pair.second));
			}
			return L"{" + join(strs, L", ") + L"}";
		}

		default:
			raiseError("Invalid object type of conversion to JSON: " + num2str((int)obj.type()));
		}
	}
}
