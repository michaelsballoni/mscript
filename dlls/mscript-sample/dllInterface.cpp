#include "pch.h"

using namespace mscript;

wchar_t* __cdecl mscript_GetExports()
{
	std::vector<std::wstring> exports
	{
		L"ms_sample_sum",
		L"ms_sample_cat"
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
		if (funcName == L"ms_sample_sum")
		{
			double retVal = 0.0;
			for (double numVal : module_utils::getNumberParams(parametersJson))
				retVal += numVal;
			return module_utils::jsonStr(retVal);
		}
		else if (funcName == L"ms_sample_cat")
		{
			std::wstring retVal;
			for (double numVal : module_utils::getNumberParams(parametersJson))
				retVal += num2wstr(numVal);
			return module_utils::jsonStr(retVal);
		}
		else
			raiseWError(L"mscript-sample: unknown function: " + funcName);
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
