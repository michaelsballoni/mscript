#pragma once

#include "exports.h"
#include "object.h"

#ifdef WIN32
#include <Windows.h> // HMODULE
#endif

#include <string>
#include <vector>
#include <unordered_set>

namespace mscript
{
	class lib
	{
	public:
		lib(const std::wstring& filePath);
		~lib();

		const auto& getFunctionNames() const
		{
			return m_importedFunctionNames;
		}

		object executeFunction(const std::wstring& name, const object& param);

	private:
#ifdef WIN32
		HMODULE m_module;
#endif
		std::wstring m_filePath;

		std::unordered_set<std::wstring> m_importedFunctionNames;

		FreeStringFunction m_freer;
		ExecuteExportFunction m_executer;
	};
}
