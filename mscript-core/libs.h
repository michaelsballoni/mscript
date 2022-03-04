#pragma once

namespace mscript
{
	typedef wchar_t* (*GetExportsFunction)();
	typedef wchar_t* (*ExecuteExportFunction)(const wchar_t* functionName, const wchar_t* parametersJson);

	typedef void (*FreeStringFunction)(wchar_t* str);
}
