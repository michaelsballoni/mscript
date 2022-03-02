#pragma once

#include <string>
#include <vector>

namespace mscript
{
	typedef wchar_t* (*GetExportsFunction)();
	typedef wchar_t* (*ExecuteExportFunction)(const wchar_t* functionName, const wchar_t* parametersJson);

	typedef void (*FreeStringFunction)(wchar_t* str);

	struct lib
	{
		GetExportsFunction getExportsFunction;
		ExecuteExportFunction executeExportFunction;
		FreeStringFunction freeStrringFunction;

		const std::wstring libFilename;
	};
	std::vector<lib> loadLibraries();
}
