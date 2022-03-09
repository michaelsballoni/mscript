#pragma once

#include "exports.h"
#include "object.h"
#include "object_json.h"
#include "utils.h"

namespace mscript
{
	class module_utils
	{
	public:
		static wchar_t* cloneString(const wchar_t* str)
		{
			size_t len = wcslen(str);
			wchar_t* output = new wchar_t[len + 1];
			memcpy(output, str, len * sizeof(wchar_t));
			output[len] = '\0';
			return output;
		}

		static wchar_t* getExports(const std::vector<std::wstring>& exports)
		{
			return cloneString(join(exports, L", ").c_str());
		}

		static wchar_t* jsonStr(const object& obj)
		{
			const std::wstring json = objectToJson(obj);
			return cloneString(json.c_str());
		}

		static wchar_t* jsonStr(const std::string& str)
		{
			return jsonStr(toWideStr(str));
		}

		static wchar_t* errorStr(const std::wstring& function, const std::exception& exp)
		{
			object errorObj = L"mscript EXCEPTION ~~~ mscript_ExecuteFunction(" + function + L"): " + toWideStr(exp.what());
			return module_utils::jsonStr(errorObj);
		}
	};
}
