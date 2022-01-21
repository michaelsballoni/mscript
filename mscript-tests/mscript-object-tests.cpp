#include "pch.h"
#include "CppUnitTest.h"

#include "object.h"

#pragma comment(lib, "mscript-lib")

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace mscript
{
	TEST_CLASS(TestObject)
	{
	public:
		TEST_METHOD(ObjectConstructorTests)
		{
			{
				object obj;
				Assert::IsTrue(obj.type() == object::NUMBER);
				Assert::AreEqual(0.0, obj.numberVal());
			}

			{
				object obj = 12.0;
				Assert::IsTrue(obj.type() == object::NUMBER);
				Assert::AreEqual(toWideStr("12"), obj.toString());
				Assert::AreEqual(12.0, obj.numberVal());
				Assert::AreEqual(std::string("number"), object::getTypeName(obj.type()));

				object obj2 = 12.0;
				Assert::IsTrue(obj == obj2);

				object obj3 = 13.0;
				Assert::IsTrue(obj != obj3);
			}

			{
				object obj = toWideStr("foo bar");
				Assert::IsTrue(obj.type() == object::STRING);
				Assert::AreEqual(toWideStr("foo bar"), obj.toString());
				Assert::AreEqual(toWideStr("foo bar"), obj.stringVal());
				Assert::AreEqual(std::string("string"), object::getTypeName(obj.type()));

				object obj2 = toWideStr("foo bar");
				Assert::IsTrue(obj == obj2);

				object obj3 = toWideStr("blet monkey");
				Assert::IsTrue(obj != obj3);
			}

			{
				object obj = bool(true);
				Assert::IsTrue(obj.type() == object::BOOL);
				Assert::AreEqual(toWideStr("true"), obj.toString());
				Assert::AreEqual(true, obj.boolVal());
				Assert::AreEqual(std::string("bool"), object::getTypeName(obj.type()));

				object obj2 = bool(true);
				Assert::IsTrue(obj == obj2);

				object obj3 = bool(false);
				Assert::IsTrue(obj != obj3);
			}

			{
				auto list = object::list();
				object obj(list);
				Assert::IsTrue(obj.type() == object::LIST);
				Assert::AreEqual(toWideStr(""), obj.toString());
				Assert::AreEqual(std::string("list"), object::getTypeName(obj.type()));
				Assert::AreEqual(size_t(0), obj.listVal().size());

				obj.listVal().push_back(object(toWideStr("foo")));
				obj.listVal().push_back(object(toWideStr("bar")));
				Assert::AreEqual(size_t(2), obj.listVal().size());
				Assert::AreEqual(toWideStr("foo, bar"), obj.toString());
			}

			{
				auto index = object::index();
				object obj(index);
				Assert::IsTrue(obj.type() == object::INDEX);
				Assert::AreEqual(toWideStr(""), obj.toString());
				Assert::AreEqual(std::string("index"), object::getTypeName(obj.type()));
				Assert::AreEqual(size_t(0), obj.indexVal().size());

				obj.indexVal().insert(toWideStr("foo"), toWideStr("bar"));
				obj.indexVal().insert(toWideStr("blet"), toWideStr("monkey"));
				Assert::AreEqual(size_t(2), obj.indexVal().size());
				Assert::AreEqual(toWideStr("foo: bar, blet: monkey"), obj.toString());
			}
		}
	};
}
