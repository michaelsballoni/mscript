#include "pch.h"

using namespace mscript;

wchar_t* __cdecl mscript_GetExports()
{
	std::vector<std::wstring> exports
	{
		L"msreg_create_key",
		L"msreg_delete_key",

		L"msreg_get_settings",
		L"msreg_set_settings",
	};
	return module_utils::getExports(exports);
}

void mscript_FreeString(wchar_t* str)
{
	delete[] str;
}

wchar_t* mscript_ExecuteFunction(const wchar_t* functionName, const wchar_t* parametersJson)
{
	try
	{
		std::wstring funcName = functionName;
		
		object::list params = module_utils::getParams(parametersJson);
		if (params.size() < 1)
			raiseError("Registry functions take a registry key string parameter");
		object registry_key_param = params[0];
		if (registry_key_param.type() != object::STRING)
			raiseError("Registry functions take a registry key string parameter");
		std::wstring registry_key = registry_key_param.stringVal();
		
		// FORNOW
		std::wstring reg_root;
		{

		}

		// FORNOW
		if (funcName == L"msreg_create_key")
		{
			return module_utils::jsonStr(true);
		}
		else
			raiseWError(L"Unknown mscript-dll-sample function: " + funcName);
	}
	catch (const user_exception& exp)
	{
		return module_utils::errorStr(functionName, exp);
	}
	catch (const std::exception& exp)
	{
		return module_utils::errorStr(functionName, exp);
	}
	catch (...)
	{
		return nullptr;
	}
}
