#pragma once

__declspec(dllexport) wchar_t* __cdecl mscript_GetExportsFunction();
__declspec(dllexport) void __cdecl mscript_FreeStringFunction(wchar_t* str);
__declspec(dllexport) wchar_t* __cdecl mscript_ExecuteExportFunction(const wchar_t* functionName, const wchar_t* parametersJson);
