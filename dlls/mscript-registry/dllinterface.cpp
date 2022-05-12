#include "pch.h"
#include "registry.h"
 
using namespace mscript;

wchar_t* __cdecl mscript_GetExports()
{
	std::vector<std::wstring> exports
	{
		L"msreg_create_key",
		L"msreg_delete_key",
		L"msreg_get_sub_keys",

		L"msreg_put_settings",
		L"msreg_get_settings",
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
		registry reg(params);

		if (funcName == L"msreg_create_key")
		{
			reg.createKey();
		}
		else if (funcName == L"msreg_delete_key")
		{
			reg.deleteKey();
		}
		else if (funcName == L"msreg_get_sub_keys")
		{
			object::list ret_val = reg.getSubKeys();
			return module_utils::jsonStr(ret_val);
		}
		else if (funcName == L"msreg_put_settings")
		{
			reg.putKeySettings();
		}
		else if (funcName == L"msreg_get_settings")
		{
			object::index ret_val = reg.getKeySettings();
			return module_utils::jsonStr(ret_val);
		}
		else
			raiseWError(L"Unknown mscript-registry function: " + funcName);

		return module_utils::jsonStr(true);
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
