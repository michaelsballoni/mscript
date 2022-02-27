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

			obj = object::list();
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("[]"), str);

			obj = object::list{ true, 2.0, toWideStr("blet")};
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("[true, 2, \"blet\"]"), str);

			obj = object::list{ true, 2.0, object::list{ 3.0, false } };
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("[true, 2, [3, false]]"), str);

			obj = object::index();
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("{}"), str);

			obj = object::index();
			obj.indexVal().set(toWideStr("foo"), 3.4);
			obj.indexVal().set(toWideStr("bar"), true);
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("{\"foo\": 3.4, \"bar\": true}"), str);

			obj = object::index();
			obj.indexVal().set(toWideStr("foo"), object::list{ 1.0, 2.0, 3.0 });
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("{\"foo\": [1, 2, 3]}"), str);
		}
	};
}
