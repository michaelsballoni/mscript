#pragma once

/// <summary>
/// Implement all of these functions to integrate your external module with mscript 
/// </summary>
namespace mscript
{
	typedef wchar_t* (*GetExportsFunction)();
	typedef void (*FreeStringFunction)(wchar_t* str);
	typedef wchar_t* (*ExecuteExportFunction)(const wchar_t* functionName, const wchar_t* parametersJson);
}
