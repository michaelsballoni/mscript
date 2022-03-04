#include "includes.h"
#include "script_processor.h"
#include "utils.h"
#pragma comment(lib, "mscript-core")
#pragma comment(lib, "mscript-lib")

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;
using namespace mscript;

std::wstring readFileIntoString(const std::wstring& filePath)
{
	std::wifstream inputStream(filePath);
	inputStream.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
	std::wstring fileContents((std::istreambuf_iterator<wchar_t>(inputStream)),
		(std::istreambuf_iterator<wchar_t>()));
	return fileContents;
}

struct ScriptInfo
{
	fs::path FullPath;
	std::vector<std::wstring> Contents;
};

std::unordered_map<std::wstring, ScriptInfo> FilenameToScriptInfo;

std::vector<std::wstring> loadScript(const std::wstring& current, const std::wstring& filename)
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

	std::vector<std::wstring> contents = split(replace(readFileIntoString(fileFullPath), L"\r\n", L"\n"), L"\n");

	ScriptInfo scriptInfo;
	scriptInfo.FullPath = fileFullPath;
	scriptInfo.Contents = contents;
	FilenameToScriptInfo[filename] = scriptInfo;

	return contents;
}

int wmain(int argc, wchar_t* argv[])
{
	if (argc < 2)
	{
		printf("Usage: mscript <script path> ...\n");
		return 0;
	}
#ifndef _DEBUG
	try
#endif
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
#ifndef _DEBUG
	catch (const script_exception& exp)
	{
		printf("Script ERROR: %s - %S - line: %d - %S\n", 
			   exp.what(), exp.filename.c_str(), exp.lineNumber, exp.line.c_str());
		return 1;
	}
	catch (const std::exception& exp)
	{
		printf("Runtime ERROR: %s\n", exp.what());
		return 1;
	}
#endif
	return 0;
}
