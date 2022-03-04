#include "pch.h"
#include "CppUnitTest.h"

#include "object.h"
#include "names.h"
#include "utils.h"
#pragma comment(lib, "mscript-core")
#pragma comment(lib, "mscript-lib")

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace mscript
{
	TEST_CLASS(TestUtils)
	{
	public:
		TEST_METHOD(UtilsTests)
		{
			Assert::AreEqual(toWideStr(""), toLower(L""));
			Assert::AreEqual(toWideStr("a"), toLower(L"a"));
			Assert::AreEqual(toWideStr("a"), toLower(L"A"));
			Assert::AreEqual(toWideStr("ab"), toLower(L"Ab"));
			Assert::AreEqual(toWideStr("ab"), toLower(L"AB"));
			Assert::AreEqual(toWideStr("ab"), toLower(L"aB"));
		}

		TEST_METHOD(NameTests)
		{
			Assert::IsTrue(!isName(L""));
			Assert::IsTrue(!isName(L"9"));
			Assert::IsTrue(!isName(L"_"));
			Assert::IsTrue(isName(L"a"));
			Assert::IsTrue(isName(L"ab"));
			Assert::IsTrue(isName(L"a9"));
			Assert::IsTrue(isName(L"a_"));
			Assert::IsTrue(!isName(L"a "));

			Assert::IsTrue(!isReserved(L"disney"));
			Assert::IsTrue(isReserved(L"null"));
		}

		TEST_METHOD(TrimTests)
		{
			Assert::AreEqual(toWideStr(""), trim(L""));
			
			Assert::AreEqual(toWideStr(""), trim(L" "));
			Assert::AreEqual(toWideStr(""), trim(L"\t"));
			Assert::AreEqual(toWideStr(""), trim(L"\n"));

			Assert::AreEqual(toWideStr(""), trim(L"  "));
			Assert::AreEqual(toWideStr(""), trim(L"   "));

			Assert::AreEqual(toWideStr("a"), trim(L"a"));
			Assert::AreEqual(toWideStr("ab"), trim(L"ab"));
			Assert::AreEqual(toWideStr("abc"), trim(L"abc"));

			Assert::AreEqual(toWideStr("a"), trim(L" a"));
			Assert::AreEqual(toWideStr("ab"), trim(L" ab"));
			Assert::AreEqual(toWideStr("abc"), trim(L" abc"));

			Assert::AreEqual(toWideStr("a"), trim(L"a "));
			Assert::AreEqual(toWideStr("ab"), trim(L"ab "));
			Assert::AreEqual(toWideStr("abc"), trim(L"abc "));

			Assert::AreEqual(toWideStr("a"), trim(L" a "));
			Assert::AreEqual(toWideStr("ab"), trim(L" ab "));
			Assert::AreEqual(toWideStr("abc"), trim(L" abc "));
		}

		TEST_METHOD(ReplaceTests)
		{
			std::wstring str;

			str = L"";
			str = replace(str, L"", L"");
			Assert::AreEqual(toWideStr(""), str);

			str = L"";
			str = replace(str, L"foo", L"bar");
			Assert::AreEqual(toWideStr(""), str);

			str = L"blet";
			str = replace(str, L"foo", L"bar");
			Assert::AreEqual(toWideStr("blet"), str);

			str = L"foobar";
			str = replace(str, L"foo", L"bar");
			Assert::AreEqual(toWideStr("barbar"), str);

			str = L"foobletfoo";
			str = replace(str, L"foo", L"bar");
			Assert::AreEqual(toWideStr("barbletbar"), str);
		}

		TEST_METHOD(StartsWithTests)
		{
			Assert::IsTrue(!startsWith(L"", L""));
			Assert::IsTrue(!startsWith(L"foo", L""));
			Assert::IsTrue(!startsWith(L"", L"foo"));
			Assert::IsTrue(!startsWith(L"fo", L"foo"));
			Assert::IsTrue(!startsWith(L"fob", L"foo"));
			Assert::IsTrue(!startsWith(L"foblle", L"foo"));

			Assert::IsTrue(startsWith(L"foo", L"foo"));
			Assert::IsTrue(startsWith(L"food", L"foo"));
			Assert::IsTrue(startsWith(L"foodie", L"foo"));
		}

		TEST_METHOD(SplitTests)
		{
			Assert::AreEqual(toWideStr(""), join(split(L"", L""), L", "));
			Assert::AreEqual(toWideStr(""), join(split(L"foo", L""), L", "));
			Assert::AreEqual(toWideStr("foo, bar"), join(split(L"foo bar", L" "), L", "));
			Assert::AreEqual(toWideStr("foo, bar"), join(split(L"foo//bar", L"//"), L", "));
		}
	};
}
