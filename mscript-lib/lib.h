#pragma once

#include "exports.h"
#include "utils.h"

#include <Windows.h>

#include <memory>
#include <string>
#include <vector>
#include <unordered_set>

namespace mscript
{
	class lib
	{
	public:
		lib(const std::wstring& filePath)
			: m_module(NULL)
		{
			m_module = ::LoadLibrary(filePath.c_str());
			if (m_module == NULL)
				raiseWError(L"Loading library failed: " + filePath);
		}

		~lib()
		{
			::FreeLibrary(m_module);
		}

		static std::vector<std::shared_ptr<lib>>& libs()
		{
			static std::vector<std::shared_ptr<lib>> s_libs;
			return s_libs;
		}



	private:
		HMODULE m_module;

		std::unordered_set<std::wstring> m_importedFunctionNames;

		ExecuteExportFunction m_executer;
		FreeStringFunction m_strFreer;

		typedef wchar_t* (*GetExportsFunction)();
	};
}