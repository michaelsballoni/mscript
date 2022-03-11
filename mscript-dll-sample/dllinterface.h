#pragma once

extern "C"
{
__declspec(dllexport) wchar_t* __cdecl mscript_GetExports();
__declspec(dllexport) void __cdecl mscript_FreeString(wchar_t* str);
__declspec(dllexport) wchar_t* __cdecl mscript_ExecuteFunction(const wchar_t* functionName, const wchar_t* parametersJson);
}
