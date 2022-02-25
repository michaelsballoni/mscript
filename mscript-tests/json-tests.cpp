#include "pch.h"
#include "CppUnitTest.h"

#include "object_json.h"
#include "utils.h"
#pragma comment(lib, "mscript-lib")

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace mscript
{
	TEST_CLASS(TestJson)
	{
	public:
		TEST_METHOD(ToJsonTests)
		{
			object obj;
			std::wstring str;

			// null
			obj = object();
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("null"), str);

			// bool
			obj = true;
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("true"), str);

			obj = false;
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("false"), str);

			// number
			obj = 12.0;
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("12"), str);

			obj = 3.1415926;
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("3.1415926"), str);

			// string
			obj = toWideStr("");
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("\"\""), str);

			obj = toWideStr("foo");
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("\"foo\""), str);

			obj = toWideStr("foo\nbar\tsome\"thing");
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("\"foo\\nbar\\tsome\\\"thing\""), str);

			// list FORNOW

			// index FORNOW
		}
	};
}
