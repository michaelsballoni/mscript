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

	std::map<std::string, std::wstring> testFiles;
	for (auto path : fs::directory_iterator(testDirPath))
	{
		if (path.is_directory())
			continue;
		std::string filename = path.path().filename().string();
		if (specificTest.empty() || filename.find(specificTest) != std::string::npos)
			testFiles.insert({ filename, readFileIntoString(path.path().string()) });
	}

	for (auto it : testFiles)
		printf("%s\n", it.first.c_str());

	return 0;
}

