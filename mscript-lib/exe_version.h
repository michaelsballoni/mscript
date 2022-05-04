#pragma once

#include <string>

namespace mscript
{
	std::wstring getExeFilePath();

	std::string getBinaryVersion(const std::wstring& filePath);
}
