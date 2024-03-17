#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _CRT_SECURE_NO_WARNINGS

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#endif

#include "bin_crypt.h"
#include "includes.h"
#include "script_processor.h"
#include "exe_version.h"
#include "utils.h"
#pragma comment(lib, "mscript-core")
#pragma comment(lib, "mscript-lib")

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;
using namespace mscript;

static std::vector<std::wstring> readFileIntoString(const std::wstring& filePath)
{
	std::vector<std::wstring> ret_val;

	std::wifstream inputStream(filePath);
	if (!inputStream)
		raiseWError(L"File could not be opened: " + filePath);

	inputStream.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
	
	std::wstring line;
	for (;;)
	{
		std::getline(inputStream, line);
		if (!inputStream)
			break;
		else
			ret_val.push_back(line);
	}
	return ret_val;
}

struct ScriptInfo
{
	fs::path FullPath;
	std::vector<std::wstring> Contents;
};

std::unordered_map<std::wstring, ScriptInfo> FilenameToScriptInfo;

static std::vector<std::wstring> loadScript(const std::wstring& current, const std::wstring& filename)
{
	{
		const auto& scriptIt = FilenameToScriptInfo.find(filename);
		if (scriptIt != FilenameToScriptInfo.end())
			return scriptIt->second.Contents;
	}

	fs::path fileFullPath;
	if (!current.empty())
	{
		const auto& scriptIt = FilenameToScriptInfo.find(current);
		if (scriptIt != FilenameToScriptInfo.end())
			fileFullPath = scriptIt->second.FullPath.parent_path().append(filename);
	}
	if (fileFullPath.empty())
		fileFullPath = fs::absolute(filename);

	std::vector<std::wstring> contents = readFileIntoString(fileFullPath);

	ScriptInfo scriptInfo;
	scriptInfo.FullPath = fileFullPath;
	scriptInfo.Contents = contents;
	FilenameToScriptInfo[filename] = scriptInfo;

	return contents;
}

static std::wstring getModuleFilePath(const std::wstring& filename)
{
	if 
	(
		filename.find('/') != std::wstring::npos 
		|| 
		filename.find('\\') != std::wstring::npos 
		||
		filename.find(L"..") != std::wstring::npos
	)
	{
		raiseWError(L"Invalid module file path: " + filename);
	}

	const int max_path = 32 * 1024;
	std::unique_ptr<wchar_t[]> exe_file_path(new wchar_t[max_path + 1]);
	exe_file_path[max_path] = '\0';
#if defined(_WIN32) || defined(_WIN64)
	if (GetModuleFileName(NULL, exe_file_path.get(), max_path) == 0)
		raiseError("Loading mscript.exe file path failed");
#endif
	fs::path exe_dir_path = fs::path(exe_file_path.get()).parent_path();
	fs::path module_file_path = exe_dir_path.append(filename);
	/*
	const bin_crypt_info exe_crypt_info = getBinCryptInfo(exe_file_path.get());
	if
	(
		!exe_crypt_info.subject.empty() 
		&&
		!exe_crypt_info.publisher.empty()
	)
	{
		const bin_crypt_info module_crypt_info = getBinCryptInfo(module_file_path);
		if 
		(
			module_crypt_info.subject != exe_crypt_info.subject
			||
			module_crypt_info.publisher != exe_crypt_info.publisher
		)
		{
			raiseWError(L"Invalid module signing: " + module_file_path.wstring());
		}
	}
	*/
	return module_file_path;
}

int wmain(int argc, wchar_t* argv[])
{
	if (argc < 2 || _wcsicmp(argv[1], L"-?") == 0)
	{
		std::cout << std::endl;

		std::cout << "Usage: mscript3 <script path> ..." << std::endl;
		
		std::cout << std::endl;

		std::wstring mscript_exe_path = mscript::getExeFilePath();
		std::wcout << L"EXE path: " << mscript_exe_path << std::endl;
		std::wcout << L"Version:  " << toWideStr(getBinaryVersion(mscript_exe_path)) << std::endl;

		return 0;
	}
	try
	{
		std::wstring scriptPath = argv[1];

		object::list arguments;
		for (int a = 2; a < argc; ++a)
			arguments.push_back(std::wstring(argv[a]));

		symbol_table symbols;
		symbols.set(L"arguments", arguments);

		script_processor
			processor
			(
				[](auto currentFilename, auto filename)
				{
					return loadScript(currentFilename, filename);
				},
				[](const std::wstring& filename)
				{
					std::wstring module_file_path = getModuleFilePath(filename);
					return module_file_path;
				},
				symbols,
				[]() -> std::optional<std::wstring> // input
				{
					std::wstring line;
					if (!std::wcin)
						return std::nullopt;
					std::getline(std::wcin, line);
					return line;
				},
				[](const std::wstring& text) // output
				{ 
					printf("%S\n", text.c_str()); 
				}
			);
		object retVal = processor.process(std::wstring(), scriptPath);
		if (retVal.type() == object::NUMBER)
			return int(retVal.numberVal());
	}
	catch (const user_exception& exp)
	{
		printf("Object ERROR: %S - %S - line: %d: %S\n",
			   exp.obj.toString().c_str(), exp.filename.c_str(), exp.lineNumber, exp.line.c_str());
		return 1;
	}
	catch (const script_exception& exp)
	{
		printf("Script ERROR: %s - %S - line: %d: %S\n", 
			   exp.what(), exp.filename.c_str(), exp.lineNumber, exp.line.c_str());
		return 1;
	}
	catch (const std::exception& exp)
	{
		printf("Runtime ERROR: %s\n", exp.what());
		return 1;
	}
	catch (...)
	{
		printf("Unhandled ... ERROR\n");
		return 1;
	}
	return 0;
}
