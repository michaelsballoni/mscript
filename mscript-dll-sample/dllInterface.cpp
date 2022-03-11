#include "pch.h"
#include "dllinterface.h"

#include "../mscript-core/module_utils.h"
#include "../mscript-core/object.h"
#include "../mscript-core/object_json.h"
#include "../mscript-core/utils.h"
#pragma comment(lib, "mscript-core")

using namespace mscript;

static std::vector<double> stringToNumbers(const wchar_t* str)
{
	object topObj = objectFromJson(str);
	if (topObj.type() != object::LIST)
		throw std::runtime_error("Parameter is not a list");

	std::vector<double> retVal;
	retVal.reserve(topObj.listVal().size());
	for (const object& obj : topObj.listVal())
	{
		if (obj.type() != object::NUMBER)
			throw std::runtime_error("List entry is not a number");
		else
			retVal.push_back(obj.numberVal());
	}
	return retVal;
}

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
		auto paramNumbers = stringToNumbers(parametersJson);
		if (funcName == L"ms_sample_sum")
		{
			double retVal = 0.0;
			for (double numVal : paramNumbers)
				retVal += numVal;
			return module_utils::jsonStr(retVal);
		}
		else if (funcName == L"ms_sample_cat")
		{
			std::wstring retVal;
			for (double numVal : paramNumbers)
				retVal += num2wstr(numVal);
			return module_utils::jsonStr(retVal);
		}
		else
			raiseWError(L"Unknown function: " + funcName);
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
