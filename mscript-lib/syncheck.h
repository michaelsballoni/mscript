#pragma once

#include <string>
#include <vector>

namespace mscript
{
	void 
		syncheck
		(
			const std::wstring& filename, 
			const std::vector<std::wstring>& lines, 
			int startLine, 
			int endLine
		);
}
