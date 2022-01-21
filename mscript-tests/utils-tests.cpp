#include "pch.h"
#include "CppUnitTest.h"

#include "object.h"

#pragma comment(lib, "mscript-lib")

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace mscript
{
	TEST_CLASS(TestUtils)
	{
	public:
		TEST_METHOD(UtilsTests)
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
	};
}
