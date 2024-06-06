#pragma once

#include <string>
#include <vector>

namespace mscript
{
	int findMatchingEnd(const std::vector<std::wstring>& lines, int startIndex, int endIndex);

	std::vector<int> 
		findElses
		(
			const std::vector<std::wstring>& lines, 
			int startIndex, 
			int endIndex, 
			const wchar_t* lineStart,
			const wchar_t* lineEnd,
			bool multiStart
		);

	bool isLineBlockBegin(const std::wstring& line);
}
