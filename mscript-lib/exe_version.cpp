#include "pch.h"
#include "exe_version.h"
#include "utils.h"

std::wstring mscript::getExeFilePath()
{
#if defined(_WIN32) || defined(_WIN64)
	const DWORD exe_file_path_size = 64 * 1024;
	std::unique_ptr<wchar_t[]> exe_file_path(new wchar_t[exe_file_path_size]);
	DWORD dw_exe_file_path =
		GetModuleFileName(NULL, exe_file_path.get(), exe_file_path_size);
	if (dw_exe_file_path == 0 || dw_exe_file_path >= exe_file_path_size)
		raiseError("Getting EXE file path failed");
	return exe_file_path.get();
#endif
}

std::string mscript::getBinaryVersion(const std::wstring& filePath)
{
#if defined(_WIN32) || defined(_WIN64)
	DWORD version_info_size = GetFileVersionInfoSize(filePath.c_str(), nullptr);
	if (version_info_size == 0)
		return "0.0.0.0";

	std::unique_ptr<uint8_t[]> version_data(new uint8_t[version_info_size]);
	if (!GetFileVersionInfo(filePath.c_str(), 0, version_info_size, version_data.get()))
		return "0.0.0.0";

	UINT size = 0;
	LPBYTE buffer = NULL;
	if (!VerQueryValue(version_data.get(), L"\\", (VOID FAR * FAR*) & buffer, &size))
		return "0.0.0.0";

	if (size == 0)
		return "0.0.0.0";

	VS_FIXEDFILEINFO* version_info = (VS_FIXEDFILEINFO*)buffer;
	if (version_info->dwSignature == 0xfeef04bd)
	{
		char output[1024];
		sprintf_s<sizeof(output)>
			(
				output,
				"%d.%d.%d.%d",
				(version_info->dwFileVersionMS >> 16) & 0xffff,
				(version_info->dwFileVersionMS >> 0) & 0xffff,
				(version_info->dwFileVersionLS >> 16) & 0xffff,
				(version_info->dwFileVersionLS >> 0) & 0xffff
				);
		return output;
	}
	else
		return "0.0.0.0";
#endif
}
