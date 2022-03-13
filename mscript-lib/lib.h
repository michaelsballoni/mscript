#pragma once

#include "exports.h"
#include "object.h"

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h> // HMODULE
#endif

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace mscript
{
	class lib
	{
	public:
		lib(const std::wstring& filePath);
		~lib();

		const std::wstring& getFilePath() const { return m_filePath; }
		object executeFunction(const std::wstring& name, const object::list& paramList) const;

		static std::shared_ptr<lib> loadLib(const std::wstring& filePath);
		static std::shared_ptr<lib> getLib(const std::wstring& name);

	private:
#if defined(_WIN32) || defined(_WIN64)
		HMODULE m_module;
#endif
		std::wstring m_filePath;

		FreeStringFunction m_freer;
		ExecuteExportFunction m_executer;

		std::unordered_set<std::wstring> m_functions;

		static std::unordered_map<std::wstring, std::shared_ptr<lib>> s_funcLibs;
	};
}
