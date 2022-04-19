#include "pch.h"
#include "CppUnitTest.h"

#include "object_json.h"
#include "utils.h"
#pragma comment(lib, "mscript-core")

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace mscript
{
	TEST_CLASS(TestJson)
	{
	public:
		TEST_METHOD(ToJsonTests)
		{
			object obj, obj2;
			std::wstring str;

			// null
			obj = object();
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("null"), str);
			obj2 = objectFromJson(str);
			Assert::IsTrue(obj2 == obj);

			// bool 
			obj = true;
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("true"), str);
			obj2 = objectFromJson(str);
			Assert::IsTrue(obj2 == obj);

			obj = false;
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("false"), str);
			obj2 = objectFromJson(str);
			Assert::IsTrue(obj2 == obj);

			// number
			obj = 12.0;
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("12"), str);
			obj2 = objectFromJson(str);
			Assert::IsTrue(obj2 == obj);

			obj = 3.1415926;
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("3.1415926"), str);
			obj2 = objectFromJson(str);
			Assert::IsTrue(obj2 == obj);

			// string
			obj = toWideStr("");
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("\"\""), str);
			obj2 = objectFromJson(str);
			Assert::IsTrue(obj2 == obj);

			obj = toWideStr("foo");
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("\"foo\""), str);
			obj2 = objectFromJson(str);
			Assert::IsTrue(obj2 == obj);

			obj = toWideStr("foo\nbar\tsome\"th\\ing");
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("\"foo\\nbar\\tsome\\\"th\\\\ing\""), str);
			obj2 = objectFromJson(str);
			Assert::IsTrue(obj2 == obj);

			// list
			obj = object::list();
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("[]"), str);
			obj2 = objectFromJson(str);
			Assert::IsTrue(obj2 == obj);

			obj = object::list{ true, 2.0, toWideStr("blet")};
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("[true, 2, \"blet\"]"), str);
			obj2 = objectFromJson(str);
			Assert::IsTrue(obj2 == obj);

			obj = object::list{ true, 2.0, object::list{ 3.0, false } };
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("[true, 2, [3, false]]"), str);
			obj2 = objectFromJson(str);
			Assert::IsTrue(obj2 == obj);

			// index
			obj = object::index();
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("{}"), str);
			obj2 = objectFromJson(str);
			Assert::IsTrue(obj2 == obj);

			obj = object::index();
			obj.indexVal().set(toWideStr("foo"), 3.4);
			obj.indexVal().set(toWideStr("bar"), true);
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("{\"foo\": 3.4, \"bar\": true}"), str);
			obj2 = objectFromJson(str);
			Assert::IsTrue(obj2 == obj);

			obj = object::index();
			obj.indexVal().set(toWideStr("foo"), object::list{ 1.0, 2.0, 3.0 });
			str = objectToJson(obj);
			Assert::AreEqual(toWideStr("{\"foo\": [1, 2, 3]}"), str);
			obj2 = objectFromJson(str);
			Assert::IsTrue(obj2 == obj);
		}
	};
}
