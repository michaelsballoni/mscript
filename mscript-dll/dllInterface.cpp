#include "pch.h"
#include "dllinterface.h"

#include "../mscript-core/object.h"
#include "../mscript-core/object_json.h"
#include "../mscript-core/utils.h"

#pragma comment(lib, "mscript-core")

using namespace mscript;

static wchar_t* cloneString(const wchar_t* str)
{
	size_t len = wcslen(str);
	wchar_t* output = new wchar_t[len + 1];
	memcpy(output, str, len * sizeof(wchar_t));
	return output;
}

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
	return cloneString(L"mscript_dll_sum, mscript_dll_cat");
}

void mscript_FreeString(wchar_t* str)
{
	delete[] str;
}

wchar_t* mscript_ExecuteFunction(const wchar_t* functionName, const wchar_t* parametersJson)
{
	try
	{
		auto paramNumbers = stringToNumbers(parametersJson);
		std::wstring funcName = functionName;

		if (funcName == L"mscript_dll_sum")
		{
			double retVal = 0.0;
			for (double numVal : paramNumbers)
				retVal += numVal;
			std::wstring json = objectToJson(retVal);
			return cloneString(json.c_str());
		}
		else if (funcName == L"mscript_dll_cat")
		{
			std::wstring retVal;
			for (double numVal : paramNumbers)
				retVal += num2wstr(numVal);
			std::wstring json = objectToJson(retVal);
			return cloneString(json.c_str());
		}
		else
			raiseWError(L"Unknown function: " + funcName);
	}
	catch (const std::exception& exp)
	{
		object errorObj = L"mscript EXCEPTION ~~~ mscript_ExecuteFunction(" + std::wstring(functionName) + L"): " + toWideStr(exp.what());
		std::wstring errorJson = objectToJson(errorObj);
		return cloneString(errorJson.c_str());
	}
	catch (...)
	{
		return nullptr;
	}
}
