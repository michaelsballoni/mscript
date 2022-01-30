#include "pch.h"
#include "CppUnitTest.h"

#include "object.h"
#include "utils.h"

#pragma comment(lib, "mscript-lib")

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace mscript
{
	TEST_CLASS(TestObject)
	{
	public:
		TEST_METHOD(ObjectTests)
		{
			{
				object obj;
				Assert::IsTrue(obj.type() == object::NOTHING);
				Assert::AreEqual(toWideStr("null"), obj.toString());
				Assert::AreEqual(std::string("nothing"), obj.typeStr());
			}

			{
				object obj = 12.0;
				Assert::IsTrue(obj.type() == object::NUMBER);
				Assert::AreEqual(toWideStr("12"), obj.toString());
				Assert::AreEqual(12.0, obj.numberVal());
				Assert::AreEqual(std::string("number"), obj.typeStr());

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
				Assert::AreEqual(std::string("string"), obj.typeStr());

				object obj2 = toWideStr("foo bar");
				Assert::IsTrue(obj == obj2);

				object obj3 = toWideStr("blet monkey");
				Assert::IsTrue(obj != obj3);
			}

			{
				object obj = true;
				Assert::IsTrue(obj.type() == object::BOOL);
				Assert::AreEqual(toWideStr("true"), obj.toString());
				Assert::AreEqual(true, obj.boolVal());
				Assert::AreEqual(std::string("bool"), obj.typeStr());

				object obj2 = true;
				Assert::IsTrue(obj == obj2);

				object obj3 = false;
				Assert::IsTrue(obj != obj3);
			}

			{
				auto list = object::list();
				object obj(list);
				Assert::IsTrue(obj.type() == object::LIST);
				Assert::AreEqual(toWideStr(""), obj.toString());
				Assert::AreEqual(std::string("list"), obj.typeStr());
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
				Assert::AreEqual(std::string("index"), obj.typeStr());
				Assert::AreEqual(size_t(0), obj.indexVal().size());

				obj.indexVal().insert(toWideStr("foo"), toWideStr("bar"));
				obj.indexVal().insert(toWideStr("blet"), toWideStr("monkey"));
				Assert::AreEqual(size_t(2), obj.indexVal().size());
				Assert::AreEqual(toWideStr("foo: bar, blet: monkey"), obj.toString());
			}

			{
				object obj;
				obj = 1.2;
				Assert::AreEqual(1.2, obj.toNumber());

				obj = toWideStr("1.3");
				Assert::AreEqual(1.3, obj.toNumber());

				obj = true;
				Assert::AreEqual(1.0, obj.toNumber());
			}

			{
				object obj;
				obj = toWideStr("foo");
				Assert::AreEqual(size_t(3), obj.length());

				obj = object::list{ 1.0, 2.0, 3.0, 4.0 };
				Assert::AreEqual(size_t(4), obj.length());

				obj = object::index();
				obj.indexVal().insert(1.0, 2.0);
				obj.indexVal().insert(3.0, 4.0);
				Assert::AreEqual(size_t(2), obj.length());
			}

			{
				object nullVal;
				Assert::AreEqual(nullVal.clone().isNull(), nullVal.isNull());

				object boolVal = true;
				Assert::AreEqual(boolVal.clone().boolVal(), boolVal.boolVal());

				object num = 13.0;
				Assert::AreEqual(num.clone().numberVal(), num.numberVal());

				object str = toWideStr("foo");
				Assert::AreEqual(str.clone().stringVal(), str.stringVal());

				object::list innerList;
				innerList.push_back(15.0);

				object::list listVal;
				listVal.push_back(innerList);
				
				object list0 = listVal;
				object list1 = list0.clone();

				std::wstring originalListStr = list0.toString();

				Assert::AreEqual(originalListStr, list1.toString());

				list0.listVal().push_back(17.0);

				Assert::AreEqual(originalListStr, list1.toString());

				object::index indexVal;
				indexVal.insert(1.0, 2.0);

				object index0 = indexVal;
				object index1 = index0.clone();

				std::wstring originalIndexStr = index0.toString();

				Assert::AreEqual(originalIndexStr, index1.toString());

				index0.indexVal().insert(3.0, 4.0);

				Assert::AreEqual(originalIndexStr, index1.toString());
			}
		}
	};
}
