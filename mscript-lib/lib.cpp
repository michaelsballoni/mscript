#include "pch.h"
#include "lib.h"

#include "utils.h"
#include "object_json.h"

namespace mscript
{
	std::unordered_map<std::wstring, std::shared_ptr<lib>> lib::s_funcLibs;

	lib::lib(const std::wstring& filePath)
		: m_filePath(filePath)
		, m_executer(nullptr)
		, m_freer(nullptr)
#if defined(_WIN32) || defined(_WIN64)
		, m_module(nullptr)
#endif
	{
#if defined(_WIN32) || defined(_WIN64)
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
		for (const auto& func_name : exports_list)
		{
			std::wstring func_name_trimmed = trim(func_name);
			if (func_name_trimmed.empty())
				raiseWError(L"Empty export from funcion mscript_GetExports: " + m_filePath);

			const auto& funcIt = s_funcLibs.find(func_name_trimmed);
			if (funcIt != s_funcLibs.end())
			{
				if (toLower(funcIt->second->m_filePath) != toLower(m_filePath))
					raiseWError(L"Function already defined in another export: function " + func_name_trimmed + L" - in " + m_filePath + L" - already defined in " + funcIt->second->m_filePath);
				else if (m_functions.find(func_name_trimmed) != m_functions.end())
					raiseWError(L"Duplicate export function: " + func_name_trimmed + L" in " + m_filePath);
				else
					continue;
			}

			m_functions.insert(func_name_trimmed);
		}

		m_executer = (ExecuteExportFunction)::GetProcAddress(m_module, "mscript_ExecuteFunction");
		if (m_executer == nullptr)
			raiseWError(L"Getting mscript_ExecuteFunction function failed: " + m_filePath);
#endif
	}

	lib::~lib()
	{
#if defined(_WIN32) || defined(_WIN64)
		if (m_module != nullptr)
			::FreeLibrary(m_module);
#endif
	}

	std::shared_ptr<lib> lib::loadLib(const std::wstring& filePath)
	{
		for (const auto& funcIt : s_funcLibs)
		{
			if (funcIt.second->m_filePath == filePath)
				return funcIt.second;
		}

		auto ptr = std::make_shared<lib>(filePath);

		for (const auto& funcName : ptr->m_functions)
			s_funcLibs.insert({ funcName, ptr });
		
		return ptr;
	}

	std::shared_ptr<lib> lib::getLib(const std::wstring& name)
	{
		const auto& funcIt = s_funcLibs.find(name);
		if (funcIt == s_funcLibs.end())
			return nullptr;
		else
			return funcIt->second;
	}

	object lib::executeFunction(const std::wstring& name, const object& param) const
	{
		const std::wstring input_json = objectToJson(param);
		wchar_t* output_json_str = m_executer(name.c_str(), input_json.c_str());
		if (output_json_str == nullptr)
			raiseWError(L"Executing function failed: " + m_filePath + L" - " + name);

		std::wstring output_json = output_json_str;
		m_freer(output_json_str);
		output_json_str = nullptr;

		object output_obj = objectFromJson(output_json);
		if (output_obj.type() == object::STRING)
		{
			if (startsWith(output_obj.stringVal(), L"mscript EXCEPTION ~~~"))
				raiseWError(L"Executing function failed: " + m_filePath + L" - " + name + L": " + output_obj.stringVal());
		}
		return output_obj;
	}
}
