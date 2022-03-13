#pragma once

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h> // GetLastError
#endif

#include "object.h"
#include "object_json.h"
#include "utils.h"

#include "dllinterface.h"

namespace mscript
{
	class module_utils
	{
	public:
		static wchar_t* cloneString(const wchar_t* str);

		static wchar_t* getExports(const std::vector<std::wstring>& exports);

		static object::list getParams(const wchar_t* parametersJson);
		static std::vector<double> getNumberParams(const wchar_t* parametersJson);

		static wchar_t* jsonStr(const object& obj);
		static wchar_t* jsonStr(const std::string& str);

		static wchar_t* errorStr(const std::wstring& function, const user_exception& exp);
		static wchar_t* errorStr(const std::wstring& function, const std::exception& exp);

	private:
		module_utils() = delete;
		module_utils(const module_utils&) = delete;
		module_utils(module_utils&&) = delete;
		module_utils& operator=(const module_utils&) = delete;
	};
}
