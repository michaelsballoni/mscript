#include "includes.h"
#include "script_processor.h"
#include "utils.h"
#pragma comment(lib, "mscript-core")
#pragma comment(lib, "mscript-lib")

#include <filesystem>
#include <fstream>
#include <string>
#include <map>

#undef min
#undef max

namespace fs = std::filesystem;
using namespace mscript;

std::wstring readFileIntoString(const std::string& filePath)
{
	std::ifstream inputStream(filePath);
	std::string fileContents((std::istreambuf_iterator<char>(inputStream)),
							 (std::istreambuf_iterator<char>()));
	return toWideStr(fileContents);
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("Usage: <tests directory> [specific filename]");
		return 0;
	}

	std::wstring testDirPath = toWideStr(argv[1]);
	std::string specificTest = argc >= 3 ? argv[2] : "";

	std::map<fs::path, std::wstring> testFiles;
	for (auto path : fs::directory_iterator(testDirPath))
	{
		if (path.is_directory())
			continue;

		fs::path filePath = path.path();
		if (specificTest.empty() || filePath.filename().string().find(specificTest) != std::string::npos)
			testFiles.insert({ filePath, readFileIntoString(path.path().string()) });
	}

	for (auto it : testFiles)
	{
		printf("%s\n", it.first.filename().string().c_str());
		std::wstring fileText = it.second;

		size_t separatorIdx = fileText.find(L"===");
		if (separatorIdx == 0 || separatorIdx == std::wstring::npos)
		{
			if (it.first.filename().extension() != ".txt")
				continue;

			printf("ERROR: Test lacks == divider\n");
			return 1;
		}

		std::wstring script = trim(fileText.substr(0, separatorIdx));
		std::wstring expected = fileText.substr(separatorIdx + strlen("==="));
		expected = trim(replace(expected, L"\r\n", L"\n"));

		std::wstring output;
#ifndef _DEBUG
		try
#endif
		{
			{
				symbol_table symbols;
				script_processor
					processor
					(
						[=](const std::wstring&, const std::wstring& filename)
						{
							bool isExternal = fs::path(filename).extension() == ".ms";
							if (isExternal)
							{
								std::string externalFilePath = toNarrowStr(fs::path(testDirPath).append(filename));
								std::wstring externalScript = trim(replace(readFileIntoString(externalFilePath), L"\r\n", L"\n"));
								return split(externalScript, '\n');
							}
							else
								return split(script, '\n');
						},
						[=](const std::wstring& filename)
						{
							std::wstring module_file_path = fs::path(testDirPath).append(filename);
							return module_file_path;
						},
						symbols,
						[]() { return L"input"; },
						[&output](const std::wstring& text) 
						{ 
							if (!startsWith(text, L"TRACE: "))
								output += text + L"\n";
							else
								printf("%S\n", text.c_str());
						}
					);
				processor.process(std::wstring(), it.first.filename());
				output = trim(output);
			}
		}
#ifndef _DEBUG
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
#endif
		if (output != expected)
		{
			printf("\nERROR: Test fails!\n");

			auto expectedLines = split(expected, '\n');
			auto outputLines = split(output, '\n"');

			size_t lineCount = std::min(expectedLines.size(), outputLines.size());
			for (size_t idx = 0; idx < lineCount; ++idx)
			{
				std::wstring currOutputLine = outputLines[idx];
				std::wstring currExpectedLine = expectedLines[idx];
				if (currOutputLine != currExpectedLine)
				{
					printf("Line %d differs:\n"
						"Expected: %S\n"
						"Got:      %S\n",
						(int)idx + 1, currExpectedLine.c_str(), currOutputLine.c_str());
					return 1;
				}
			}
			if (expectedLines.size() != outputLines.size())
			{
				printf("Line counts differ: expected: %d - got: %d\n", 
					   int(expectedLines.size()), int(outputLines.size()));
				return 1;
			}

			printf(" - Output:\n%S\n", output.c_str());
			printf(" - Expected:\n%S\n", expected.c_str());
			return 1;
		}
	}

	printf("\nSUCCESS: All done. Tests pass!\n");
	return 0;
}
