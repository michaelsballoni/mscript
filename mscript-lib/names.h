#pragma once

#include <string>

namespace mscript
{
	void validateName(const std::wstring& name);
	bool isName(const std::wstring& name);
	bool isReserved(const std::wstring& name);
}
