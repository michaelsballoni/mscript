#include "includes.h"
#include "script_processor.h"
#include "utils.h"
#pragma comment(lib, "mscript-lib")

#include <filesystem>
#include <fstream>
#include <string>
#include <map>

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

	std::string testDirPath = argv[1];
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
			printf("ERROR: Test lacks == divider\n");
			return 1;
		}

		std::wstring script = trim(fileText.substr(0, separatorIdx));
		std::wstring expected = fileText.substr(separatorIdx + strlen("==="));
		replace(expected, L"\r\n", L"\n");
		expected = trim(expected);

		std::wstring output;
		{
			symbol_table symbols;
			std::unordered_map<std::wstring, std::shared_ptr<script_function>> functions;
			script_processor
				processor
				(
					it.first.filename().wstring(),
					[script](const std::wstring&) { return split(script, L"\n"); },
					symbols,
					functions,
					[&output](const std::wstring& text) { output += text + L"\n"; }
				);
			processor.process();
			output = trim(output);
		}

		if (output != expected)
		{
			printf("\nERROR: Test fails!\n");

			auto expectedLines = split(expected, L"\n");
			auto outputLines = split(output, L"\n");

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
