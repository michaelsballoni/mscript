#include "pch.h"
#include "CppUnitTest.h"

#include "preprocess.h"
#pragma comment(lib, "mscript-core")
#pragma comment(lib, "mscript-lib")

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

bool AreStringVectorsEqual(const std::vector<std::wstring>& expected, const std::vector<std::wstring>& got)
{
	if (expected.size() != got.size())
		return false;
	for (size_t g = 0; g < expected.size(); ++g)
	{
		if (got[g] != expected[g])
			return false;
	}
	return true;
}

namespace mscript
{
	TEST_CLASS(TestPreprocess)
	{
	public:
		TEST_METHOD(PreprocessTests)
		{
			{
				std::vector<std::wstring> lines;
				std::vector<std::wstring> expected;
				preprocess(lines);
				Assert::IsTrue(AreStringVectorsEqual(expected, lines));
			}

			{
				std::vector<std::wstring> lines{ L"foo" };
				std::vector<std::wstring> expected{ L"foo" };
				preprocess(lines);
				Assert::IsTrue(AreStringVectorsEqual(expected, lines));
			}

			{
				std::vector<std::wstring> lines{ L"foo", L"bar" };
				std::vector<std::wstring> expected{ L"foo", L"bar" };
				preprocess(lines);
				Assert::IsTrue(AreStringVectorsEqual(expected, lines));
			}

			{
				std::vector<std::wstring> lines{ L"foo \\", L"bar" };
				std::vector<std::wstring> expected{ L"foo bar", L"" };
				preprocess(lines);
				Assert::IsTrue(AreStringVectorsEqual(expected, lines));
			}

			{
				std::vector<std::wstring> lines{ L"foo \\", L"bar", L"blet" };
				std::vector<std::wstring> expected{ L"foo bar", L"", L"blet" };
				preprocess(lines);
				Assert::IsTrue(AreStringVectorsEqual(expected, lines));
			}

			{
				std::vector<std::wstring> lines{ L"foo \\", L"bar", L"blet", L"monkey \\" };
				std::vector<std::wstring> expected{ L"foo bar", L"", L"blet", L"monkey" };
				preprocess(lines);
				Assert::IsTrue(AreStringVectorsEqual(expected, lines));
			}
		}
	};
}
