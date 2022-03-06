#include "pch.h"
#include "lib.h"

#include "utils.h"
#include "object_json.h"

namespace mscript
{
	lib::lib(const std::wstring& filePath)
		: m_filePath(filePath)
#ifdef WIN32
		, m_module(nullptr)
#endif
	{
#ifdef WIN32
		m_module = ::LoadLibrary(m_filePath.c_str());
		if (m_module == nullptr)
			raiseWError(L"Loading library failed: " + m_filePath);

		m_freer = (FreeStringFunction)::GetProcAddress(m_module, "mscript_FreeString");
		if (m_freer == nullptr)
			raiseWError(L"Getting mscript_FreeString function failed: " + m_filePath);

		std::wstring exports_str;
		{
			GetExportsFunction get_exports_func = (GetExportsFunction)::GetProcAddress(m_module, "mscript_GetExports");
			if (get_exports_func == nullptr)
				raiseWError(L"Getting mscript_GetExports function failed: " + m_filePath);

			wchar_t* exports = get_exports_func();
			if (exports == nullptr)
				raiseWError(L"Getting exports from funcion mscript_GetExports failed: " + m_filePath);
			exports_str = exports;

			m_freer(exports);
			exports = nullptr;
		}

		std::vector<std::wstring> exports_list = split(exports_str, L",");
		for (const auto& export_item : exports_list)
		{
			std::wstring export_item_trimmed = trim(export_item);
			if (export_item_trimmed.empty())
				raiseWError(L"Empty export from funcion mscript_GetExports: " + m_filePath);
			m_importedFunctionNames.insert(export_item_trimmed);
		}

		m_executer = (ExecuteExportFunction)::GetProcAddress(m_module, "mscript_ExecuteFunction");
		if (m_executer == nullptr)
			raiseWError(L"Getting mscript_ExecuteFunction function failed: " + m_filePath);
#endif
	}

	lib::~lib()
	{
#ifdef WIN32
		if (m_module != nullptr)
			::FreeLibrary(m_module);
#endif
	}

	object lib::executeFunction(const std::wstring& name, const object& param)
	{
		if (m_importedFunctionNames.find(name) == m_importedFunctionNames.end())
			raiseWError(L"Function to execute not found: " + m_filePath + L" - " + name);

		const std::wstring input_json = objectToJson(param);
		wchar_t* output_json_str = m_executer(name.c_str(), input_json.c_str());
		if (output_json_str == nullptr)
			raiseWError(L"Executing function failed: " + m_filePath + L" - " + name);

		std::wstring output_json = output_json_str;
		m_freer(output_json_str);
		output_json_str = nullptr;

		const object output_obj = objectFromJson(output_json);
		return output_obj;
	}
}
