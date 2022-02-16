#include "pch.h"
#include "CppUnitTest.h"

#include "vectormap.h"

#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace mscript
{
	TEST_CLASS(TestVectorMap)
	{
	public:
		TEST_METHOD(EmptyTest)
		{
			vectormap<int, int> map;

			Assert::AreEqual(size_t(0), map.size());

			Assert::IsTrue(map.vec().begin() == map.vec().end());

			try
			{
				map.get(914);
				Assert::Fail();
			}
			catch (const std::exception&) {}

			try
			{
				map.get_at(914);
				Assert::Fail();
			}
			catch (const std::exception&) {}

			Assert::IsTrue(!map.contains(0));

			int val = -1;
			Assert::IsTrue(!map.tryGet(0, val));

			Assert::IsTrue(map.keys().empty());
			Assert::IsTrue(map.values().empty());
		}

		TEST_METHOD(SingleTest)
		{
			vectormap<int, std::string> map;

			map.set(1, "foo");

			Assert::AreEqual(size_t(1), map.size());

			auto it = map.vec().begin();
			Assert::AreEqual(1, it->first);
			Assert::AreEqual(std::string("foo"), it->second);
			++it;
			Assert::IsTrue(it == map.vec().end());

			try
			{
				map.get(914);
				Assert::Fail();
			}
			catch (const std::exception&) {}
			Assert::AreEqual(std::string("foo"), map.get(1));

			try
			{
				map.get_at(914);
				Assert::Fail();
			}
			catch (const std::exception&) {}
			Assert::AreEqual(std::string("foo"), map.get_at(0));

			Assert::IsTrue(!map.contains(0));
			Assert::IsTrue(map.contains(1));

			std::string val;
			Assert::IsTrue(!map.tryGet(0, val));
			Assert::IsTrue(map.tryGet(1, val));
			Assert::AreEqual(std::string("foo"), val);

			auto keys = map.keys();
			Assert::AreEqual(size_t(1), keys.size());
			Assert::AreEqual(1, keys[0]);

			auto values = map.values();
			Assert::AreEqual(size_t(1), values.size());
			Assert::AreEqual(std::string("foo"), values[0]);
		}

		TEST_METHOD(MultiTest)
		{
			vectormap<int, std::string> map;

			map.set(1, "foo");
			map.set(3, "bar");
			map.set(2, "blet");

			Assert::AreEqual(size_t(3), map.size());

			auto it = map.vec().begin();
			Assert::AreEqual(1, it->first);
			Assert::AreEqual(std::string("foo"), it->second);
			++it;
			Assert::AreEqual(3, it->first);
			Assert::AreEqual(std::string("bar"), it->second);
			++it;
			Assert::AreEqual(2, it->first);
			Assert::AreEqual(std::string("blet"), it->second);
			++it;
			Assert::IsTrue(it == map.vec().end());

			try
			{
				map.get(914);
				Assert::Fail();
			}
			catch (const std::exception&) {}
			Assert::AreEqual(std::string("foo"), map.get(1));
			Assert::AreEqual(std::string("bar"), map.get(3));
			Assert::AreEqual(std::string("blet"), map.get(2));

			try
			{
				map.get_at(914);
				Assert::Fail();
			}
			catch (const std::exception&) {}
			Assert::AreEqual(std::string("foo"), map.get_at(0));
			Assert::AreEqual(std::string("bar"), map.get_at(1));
			Assert::AreEqual(std::string("blet"), map.get_at(2));

			Assert::IsTrue(!map.contains(0));
			Assert::IsTrue(map.contains(1));
			Assert::IsTrue(map.contains(3));
			Assert::IsTrue(map.contains(2));

			std::string val;
			Assert::IsTrue(!map.tryGet(0, val));
			Assert::IsTrue(map.tryGet(1, val));
			Assert::AreEqual(std::string("foo"), val);
			Assert::IsTrue(map.tryGet(3, val));
			Assert::AreEqual(std::string("bar"), val);
			Assert::IsTrue(map.tryGet(2, val));
			Assert::AreEqual(std::string("blet"), val);

			auto keys = map.keys();
			Assert::AreEqual(size_t(3), keys.size());
			Assert::AreEqual(1, keys[0]);
			Assert::AreEqual(3, keys[1]);
			Assert::AreEqual(2, keys[2]);

			auto values = map.values();
			Assert::AreEqual(size_t(3), values.size());
			Assert::AreEqual(std::string("foo"), values[0]);
			Assert::AreEqual(std::string("bar"), values[1]);
			Assert::AreEqual(std::string("blet"), values[2]);

			//
			// modify the map, test again
			//
			map.set(1, "monkey");
			map.set(3, "something");
			map.set(2, "else");

			it = map.vec().begin();
			Assert::AreEqual(1, it->first);
			Assert::AreEqual(std::string("monkey"), it->second);
			++it;
			Assert::AreEqual(3, it->first);
			Assert::AreEqual(std::string("something"), it->second);
			++it;
			Assert::AreEqual(2, it->first);
			Assert::AreEqual(std::string("else"), it->second);
			++it;
			Assert::IsTrue(it == map.vec().end());

			try
			{
				map.get(914);
				Assert::Fail();
			}
			catch (const std::exception&) {}
			Assert::AreEqual(std::string("monkey"), map.get(1));
			Assert::AreEqual(std::string("something"), map.get(3));
			Assert::AreEqual(std::string("else"), map.get(2));

			try
			{
				map.get_at(914);
				Assert::Fail();
			}
			catch (const std::exception&) {}
			Assert::AreEqual(std::string("monkey"), map.get_at(0));
			Assert::AreEqual(std::string("something"), map.get_at(1));
			Assert::AreEqual(std::string("else"), map.get_at(2));

			Assert::IsTrue(!map.contains(0));
			Assert::IsTrue(map.contains(1));
			Assert::IsTrue(map.contains(3));
			Assert::IsTrue(map.contains(2));

			Assert::IsTrue(!map.tryGet(0, val));
			Assert::IsTrue(map.tryGet(1, val));
			Assert::AreEqual(std::string("monkey"), val);
			Assert::IsTrue(map.tryGet(3, val));
			Assert::AreEqual(std::string("something"), val);
			Assert::IsTrue(map.tryGet(2, val));
			Assert::AreEqual(std::string("else"), val);

			keys = map.keys();
			Assert::AreEqual(size_t(3), keys.size());
			Assert::AreEqual(1, keys[0]);
			Assert::AreEqual(3, keys[1]);
			Assert::AreEqual(2, keys[2]);

			values = map.values();
			Assert::AreEqual(size_t(3), values.size());
			Assert::AreEqual(std::string("monkey"), values[0]);
			Assert::AreEqual(std::string("something"), values[1]);
			Assert::AreEqual(std::string("else"), values[2]);
		}
	};
}
