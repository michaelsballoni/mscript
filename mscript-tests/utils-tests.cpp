#include "pch.h"
#include "CppUnitTest.h"

#include "object.h"
#include "names.h"
#include "utils.h"

#pragma comment(lib, "mscript-lib")

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace mscript
{
	TEST_CLASS(TestUtils)
	{
	public:
		TEST_METHOD(OneDoubleTests)
		{
			object::list list;

			list.clear();
			list.push_back(12.0);
			{
				object obj(list);
				Assert::AreEqual(12.0, obj.getOneDouble("good"));
			}

			list.clear();
			list.push_back(12.0);
			list.push_back(13.0);
			{
				object obj(list);
				try
				{
					Assert::AreEqual(12.0, obj.getOneDouble("bad1"));
					Assert::Fail();
				}
				catch (const std::exception& exp)
				{
					Assert::IsTrue(std::string(exp.what()).find("bad1") != std::string::npos);
				}
			}

			list.clear();
			list.push_back(toWideStr("foo"));
			{
				object obj(list);
				try
				{
					Assert::AreEqual(12.0, obj.getOneDouble("bad2"));
					Assert::Fail();
				}
				catch (const std::exception& exp)
				{
					Assert::IsTrue(std::string(exp.what()).find("bad2") != std::string::npos);
				}
			}
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
			replace(str, L"", L"");
			Assert::AreEqual(toWideStr(""), str);

			str = L"";
			replace(str, L"foo", L"bar");
			Assert::AreEqual(toWideStr(""), str);

			str = L"blet";
			replace(str, L"foo", L"bar");
			Assert::AreEqual(toWideStr("blet"), str);

			str = L"foobar";
			replace(str, L"foo", L"bar");
			Assert::AreEqual(toWideStr("barbar"), str);

			str = L"foobletfoo";
			replace(str, L"foo", L"bar");
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

	};
}
