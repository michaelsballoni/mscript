#include "pch.h"
#include "http.h"

using namespace mscript;

wchar_t* __cdecl mscript_GetExports()
{
	std::vector<std::wstring> exports
	{
		L"mshttp_process_request",
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

		if (funcName == L"mshttp_process_request")
		{
			if (params.size() != 1)
				throw std::exception("mshttp_process_request takes one parameter");
			if (params[0].typeStr() != "index")
				throw std::exception("mshttp_process_request takes one parameter, an index");
			object::index param_idx = params[0].indexVal();

			http obj;
			object::index output_idx = obj.ProcessRequest(param_idx);
			return module_utils::jsonStr(output_idx);
		}
		else
			raiseWError(L"Unknown mshttp function: " + funcName);
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
