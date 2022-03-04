#include "pch.h"
#include "CppUnitTest.h"

#include "symbols.h"
#include "utils.h"
#pragma comment(lib, "mscript-core")
#pragma comment(lib, "mscript-lib")

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace mscript
{
	TEST_CLASS(TestSymbols)
	{
	public:
		TEST_METHOD(SymbolTableTests)
		{
            symbol_table table;
            Assert::IsTrue(!table.contains(L"foo"));
            table.set(L"foo", 11.0);
            Assert::IsTrue(table.contains(L"foo"));
            Assert::AreEqual(11.0, table.get(L"foo").numberVal());

            Assert::IsTrue(!table.contains(L"bar"));
            table.set(L"bar", 9.0);
            Assert::IsTrue(table.contains(L"bar"));
            Assert::AreEqual(9.0, table.get(L"bar").numberVal());

            table.pushFrame();
            table.set(L"blet", toWideStr("monkey"));
            Assert::IsTrue(table.contains(L"blet"));
            Assert::AreEqual(toWideStr("monkey"), table.get(L"blet").stringVal());
            Assert::AreEqual(9.0, table.get(L"bar").numberVal());

            table.assign(L"bar", 10.0);

            table.popFrame();
            Assert::IsTrue(!table.contains(L"blet"));
            Assert::IsTrue(table.contains(L"bar"));
            Assert::AreEqual(10.0, table.get(L"bar").numberVal());

            try
            {
                table.assign(L"bar", toWideStr("foobar"));
                Assert::Fail();
            }
            catch (const std::exception&) {}

            table.set(L"startsNull", object(object::NOTHING));
            Assert::IsTrue(table.get(L"startsNull").isNull());

            table.assign(L"startsNull", object(object::NOTHING));
            Assert::IsTrue(table.get(L"startsNull").isNull());

            table.assign(L"startsNull", 13.0);
            Assert::AreEqual(13.0, table.get(L"startsNull").numberVal());

            try
            {
                table.assign(L"startsNull", toWideStr("foobar"));
                Assert::Fail();
            }
            catch (const std::exception&) {}

            table.assign(L"startsNull", 11.0);
            Assert::AreEqual(11.0, table.get(L"startsNull").numberVal());
        }

        TEST_METHOD(SymbolStackerTests)
        {
            symbol_table table;
            table.set(L"foo", toWideStr("bar"));
            {
                symbol_stacker stacker1(table);
                table.set(L"blet", toWideStr("monkey"));
                {
                    symbol_smacker smacker(table);
                    {
                        symbol_stacker stacker2(table);
                        Assert::IsTrue(table.contains(L"foo"));
                        Assert::IsFalse(table.contains(L"blet"));
                        table.set(L"something", toWideStr("else"));
                    }
                }
                Assert::IsTrue(table.contains(L"blet"));
                Assert::IsFalse(table.contains(L"something"));
            }
            Assert::IsTrue(table.contains(L"foo"));
            Assert::IsFalse(table.contains(L"blet"));
            Assert::IsFalse(table.contains(L"something"));
        }
	};
}
