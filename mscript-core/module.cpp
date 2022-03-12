#include "pch.h"
#include "module.h"

namespace mscript
{
	wchar_t* module_utils::cloneString(const wchar_t* str)
	{
		size_t len = wcslen(str);
		wchar_t* output = new wchar_t[len + 1];
		memcpy(output, str, len * sizeof(wchar_t));
		output[len] = '\0';
		return output;
	}

	wchar_t* module_utils::getExports(const std::vector<std::wstring>& exports)
	{
		return cloneString(join(exports, L", ").c_str());
	}

	object::list module_utils::getParams(const wchar_t* parametersJson)
	{
		object obj = objectFromJson(parametersJson);
		if (obj.type() == object::LIST)
			return obj.listVal();
		else
			return object::list{ obj };
	}

	wchar_t* module_utils::jsonStr(const object& obj)
	{
		const std::wstring json = objectToJson(obj);
		return cloneString(json.c_str());
	}

	wchar_t* module_utils::jsonStr(const std::string& str)
	{
		return jsonStr(toWideStr(str));
	}

	wchar_t* module_utils::errorStr(const std::wstring& function, const std::exception& exp)
	{
		std::wstring errorStr = L"mscript EXCEPTION ~~~ mscript_ExecuteFunction(" + function + L"): " + toWideStr(exp.what());
#if defined(_WIN32) || defined(_WIN64)
		DWORD dwLastError = ::GetLastError();
		if (dwLastError != NO_ERROR)
			errorStr += L" - GetLastError: " + num2wstr(dwLastError);
#endif
		return module_utils::jsonStr(errorStr);
	}
}
