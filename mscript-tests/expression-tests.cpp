#include "pch.h"
#include "CppUnitTest.h"

#include "expressions.h"
#include "utils.h"

#pragma comment(lib, "mscript-lib")

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace mscript
{
    class TestCallable : public callable
    {
    public:
        bool hasFunction(const std::wstring& name) const
        {
            return name == L"length" || name == L"length2";
        }

        object callFunction(const std::wstring& name, const object::list& parameters)
        {
            if (parameters.size() != 1 || parameters[0].type() != object::LIST)
                raiseWError(L"Invalid parameters for " + name);
            return double(parameters[0].listVal().size() + 10);
        }
    };

    TEST_CLASS(ExpressionTests)
    {
    public:
        static double round2places(double value)
        {
            return round(value * 10000.0) / 10000.0;
        }

        static void ValidateExpression(const std::string& expStr, object expected, symbol_table& symbols)
        {
            no_op_callable callable;
            expression exp(symbols, callable);
            object answer = exp.evaluate(toWideStr(expStr));

            Assert::IsTrue(answer.type() == expected.type());
            if (expected.isNull())
                Assert::IsTrue(answer.isNull());
            else if (expected.type() == object::BOOL)
                Assert::AreEqual(expected.boolVal(), answer.boolVal());
            else if (expected.type() == object::STRING) // handles LIST and INDEX with toString
                Assert::AreEqual(expected.stringVal(), answer.toString());
            else
                Assert::AreEqual(round2places(expected.numberVal()), round2places(answer.numberVal()));
        }

        TEST_METHOD(TestFunctionPrecedence)
        {
            symbol_table symtable;
            TestCallable callable;
            expression exp(symtable, callable);
            {
                object result = exp.evaluate(L"length(list(1.0, 2.0))");
                Assert::AreEqual(2.0, result.numberVal());
            }
            {
                object result = exp.evaluate(L"length2(list(1.0, 2.0))");
                Assert::AreEqual(12.0, result.numberVal());
            }
        }

        TEST_METHOD(TestOperators)
        {
            Assert::IsTrue(expression::isOperator(L"", "-", -1));

            Assert::IsTrue(expression::isOperator(L"59", "-", -1));
            Assert::IsTrue(!expression::isOperator(L"-59", "-", 0));

            Assert::IsTrue(expression::isOperator(L"1 - 59", "-", 2));
            Assert::IsTrue(expression::isOperator(L"1 -59", "-", 2));
            Assert::IsTrue(expression::isOperator(L"1- 59", "-", 1));
            Assert::IsTrue(expression::isOperator(L"1-59", "-", 1));

            Assert::IsTrue(!expression::isOperator(L"1 + -59", "-", 4));
            Assert::IsTrue(!expression::isOperator(L"1+-59", "-", 2));

            Assert::IsTrue(!expression::isOperator(L"59.6E-2", "-", 5));

            Assert::IsTrue(expression::isOperator(L"59+2", "+", 2));
            Assert::IsTrue(expression::isOperator(L"59 +2", "+", 3));
            Assert::IsTrue(expression::isOperator(L"59+ 2", "+", 2));
            Assert::IsTrue(!expression::isOperator(L"59.6E+2", "+", 5));

            Assert::IsTrue(!expression::isOperator(L"record", "or", 3));
            Assert::IsTrue(!expression::isOperator(L"random", "and", 1));
        }

        TEST_METHOD(TestReverseFind)
        {
            // string source, string search, int start
            Assert::AreEqual(-1, expression::reverseFind(L"", L"and", 0));
            Assert::AreEqual(0, expression::reverseFind(L"and", L"and", 2));
            Assert::AreEqual(0, expression::reverseFind(L"and bother", L"and", 9));
            Assert::AreEqual(0, expression::reverseFind(L"and bother", L"and", 4));
            Assert::AreEqual(7, expression::reverseFind(L"bother and", L"and", 9));
            Assert::AreEqual(7, expression::reverseFind(L"bother and another", L"and", 16));
            Assert::AreEqual(7, expression::reverseFind(L"bother and another", L"and", 10));
            Assert::AreEqual(7, expression::reverseFind(L"bother and another", L"and", 9));
        }

        TEST_METHOD(TestExpressions)
        {
            symbol_table symbols;

            ValidateExpression("3<=4", true, symbols);
            ValidateExpression("3<=4 and 3<4", true, symbols);
            ValidateExpression("3<4 and 5 <= 4 or 12<=50 and 6 <= 13", true, symbols);

            ValidateExpression("2 + 3", 5.0, symbols);
            ValidateExpression("2 - 3", -1.0, symbols);
            ValidateExpression("2 * 3", 6.0, symbols);
            ValidateExpression("2 / 3", 2.0 / 3, symbols);
            ValidateExpression("8 % 3", 2.0, symbols);
            ValidateExpression("8 % 2", 0.0, symbols);
            ValidateExpression("2 ^ 3", 8.0, symbols);

            symbols.set(toWideStr("a"), 10.0);
            ValidateExpression("a", 10.0, symbols);

            symbols.set(toWideStr("b"), 5.0);
            ValidateExpression("a", 10.0, symbols);
            ValidateExpression("b", 5.0, symbols);

            ValidateExpression("a + b + 10", 25.0, symbols);

            ValidateExpression("5 + -4", 1.0, symbols);
            ValidateExpression("5 * -4", -20.0, symbols);

            symbols.set(toWideStr("x"), 5.0);
            symbols.set(toWideStr("y"), 4.0);
            ValidateExpression("x * y", 20.0, symbols);
            ValidateExpression("-x * y", -20.0, symbols);
            ValidateExpression("x * -y", -20.0, symbols);
            ValidateExpression("-x * -y", 20.0, symbols);

            symbols.set(toWideStr("q"), true);
            symbols.set(toWideStr("r"), false);
            ValidateExpression("q", true, symbols);
            ValidateExpression("not q", false, symbols);
            ValidateExpression("not q", false, symbols);
            ValidateExpression("r", false, symbols);
            ValidateExpression("not r", true, symbols);
            ValidateExpression("not r", true, symbols);
            ValidateExpression("!q or !r", true, symbols);
            ValidateExpression("not q or not r", true, symbols);

            ValidateExpression("5E-2", 5.0e-2, symbols);
            ValidateExpression("5E+2", 5.0e+2, symbols);

            ValidateExpression("9 + 5E+4", 9.0 + 5.0e+4, symbols);

            ValidateExpression("(5+6)", 11.0, symbols);
            ValidateExpression("1+(5+6)", 12.0, symbols);
            ValidateExpression("(5+6)+1", 12.0, symbols);

            ValidateExpression("5 + 6 * 9", 59.0, symbols);
            ValidateExpression("(5 + 6) * 9", 99.0, symbols);

            ValidateExpression("(3 * 2) + (9 - 2)", 13.0, symbols);

            symbols.set(toWideStr("pi"), M_PI);
            ValidateExpression("sin(pi / 4)^2 + cos(pi / 4)^2", 1.0, symbols);

            ValidateExpression("\"foo\"", toWideStr("foo"), symbols);
            ValidateExpression("\"foo\" + \"bar\"", toWideStr("foobar"), symbols);
            ValidateExpression("\"foo\" + 10 + \"bar\"", toWideStr("foo10bar"), symbols);
            ValidateExpression("\"foo + 10\" + 20", toWideStr("foo + 1020"), symbols);
            ValidateExpression("8 + \"f+1\" + 2", toWideStr("8f+12"), symbols);

            ValidateExpression("1 < 2", true, symbols);
            ValidateExpression("1 < 2 and 6 < 7", true, symbols);
            ValidateExpression("1 <= 2", true, symbols);
            ValidateExpression("1<=2", true, symbols);
            ValidateExpression("3 <= 2", false, symbols);
            ValidateExpression("3<=2", false, symbols);
            ValidateExpression("1<=2 and 3<=2", false, symbols);
            ValidateExpression("3<=2 and 1<=2", false, symbols);
            ValidateExpression("1<=2 or 3<=2", true, symbols);
            ValidateExpression("3<=2 or 1<=2", true, symbols);

            ValidateExpression("number(12.0)", 12.0, symbols);
            ValidateExpression("number(\"12.0\")", 12.0, symbols);
            ValidateExpression("number(true)", 1.0, symbols);
            ValidateExpression("number(false)", 0.0, symbols);

            ValidateExpression("string(12.0)", toWideStr("12"), symbols);
            ValidateExpression("string(true)", toWideStr("true"), symbols);
            ValidateExpression("string(false)", toWideStr("false"), symbols);

            ValidateExpression("length(\"foo\")", 3.0, symbols);
            ValidateExpression("length(list(1, 2 ,3))", 3.0, symbols);
            ValidateExpression("length(index(1, 1, 2, 2, 3, 3))", 3.0, symbols);

            ValidateExpression("toUpper(\"lower\")", toWideStr("LOWER"), symbols);
            ValidateExpression("toLower(\"UPPER\")", toWideStr("upper"), symbols);

            ValidateExpression("replaced(\"foo2bar\", \"o2b\", \"y3z\")", toWideStr("foy3zar"), symbols);
            ValidateExpression("replaced(\"foo2bar\", \"Z\", \"y3b\")", toWideStr("foo2bar"), symbols);

            ValidateExpression("random(0, 1) >= 0 and random(0, 1) <= 1", true, symbols);
            ValidateExpression("random(0, 1) != random(0, 1)", true, symbols);

            ValidateExpression("isMatch(\"\", \"^[a-z]+$\")", false, symbols);
            ValidateExpression("isMatch(\"abc\", \"^[a-z]+$\")", true, symbols);
            ValidateExpression("isMatch(\"xYz\", \"^[a-z]+$\")", false, symbols);
            ValidateExpression("isMatch(\"123\", \"^[a-z]+$\")", false, symbols);

            ValidateExpression("join(list(1,2,3,4))", toWideStr("1234"), symbols);
            ValidateExpression("join(list(1,2,3,4), \", \")", toWideStr("1, 2, 3, 4"), symbols);

            ValidateExpression("join(split(\"foo bar\", \" \"), \" blet \")", toWideStr("foo blet bar"), symbols);
        }
    };
}

/*
        [Test]

        [Test]
        public void TestExpressions()
        {
        }

        [Test]
        public void TestEscapes()
        {
            var symbols = new SymbolTable();
            ValidateExpression("\"\"", "", symbols);
            ValidateExpression("\"" + "\\\"" + "\"", "\"", symbols);
            ValidateExpression("\"" + "\\t" + "\"", "\t", symbols);
            ValidateExpression("\"" + "\\t\\r\\n" + "\"", "\t\r\n", symbols);
            ValidateExpression("\"a\\t\\n\\rb\"", "a\t\n\rb", symbols);
            ValidateExpression("\"foo\\tbar\\r\\n\"", "foo\tbar\r\n", symbols);
            ValidateExpression("\"foo\\t\\\"bar\\\" blet \\\"monkey\\\"\\r\\n\"",
                "foo\t\"bar\" blet \"monkey\"\r\n", symbols);
        }

        private static void ValidateParams(string expStr, string[] expected)
        {
            var expParams = Expression.ParseParameters(expStr);
            Assert.AreEqual(string.Join(", ", expected), string.Join(", ", expParams));
        }

        [Test]
        public void TestParseParams()
        {
            ValidateParams("", new string[0]);
            ValidateParams("foo", new[] { "foo" });
            ValidateParams("foo, bar", new[] { "foo", "bar" });
            ValidateParams("\"foo\", bar", new[] { "\"foo\"", "bar" });
            ValidateParams("\"foo, bar\", blet", new[] { "\"foo, bar\"", "blet" });
            ValidateParams("monkey, \"foo, bar\", blet", new[] { "monkey", "\"foo, bar\"", "blet" });
        }

        [Test]
        public void TestFindMatchingEnd()
        {
            {
                List<string> lines = new List<string>();
                try
                {
                    ScriptProcessor.FindMatchingEnd(lines.ToArray(), 0, lines.Count - 1);
                    Assert.Fail();
                }
                catch { }
            }

            {
                List<string> lines = new List<string>();
                lines.Add("}");
                try
                {
                    ScriptProcessor.FindMatchingEnd(lines.ToArray(), 0, lines.Count - 1);
                    Assert.Fail();
                }
                catch { }
            }

            {
                List<string> lines = new List<string>();
                lines.Add("{");
                lines.Add("}");
                int endIndex = ScriptProcessor.FindMatchingEnd(lines.ToArray(), 0, lines.Count - 1);
                Assert.AreEqual(1, endIndex);
            }

            {
                List<string> lines = new List<string>();
                lines.Add("O");
                lines.Add("    $ s = 1");
                lines.Add("}");
                int endIndex = ScriptProcessor.FindMatchingEnd(lines.ToArray(), 0, lines.Count - 1);
                Assert.AreEqual(2, endIndex);
            }

            {
                List<string> lines = new List<string>();
                lines.Add("O");
                lines.Add("    $ s = 1");
                lines.Add("    $ q = 3");
                lines.Add("}");
                lines.Add("$ x = y + 5");
                int endIndex = ScriptProcessor.FindMatchingEnd(lines.ToArray(), 0, lines.Count - 1);
                Assert.AreEqual(3, endIndex);
            }

            {
                List<string> lines = new List<string>();
                lines.Add("O");
                lines.Add("    O");
                lines.Add("        $ s = 1");
                lines.Add("        $ q = 3");
                lines.Add("    }");
                lines.Add("}");
                lines.Add("$ x = y + 5");
                int endIndex = ScriptProcessor.FindMatchingEnd(lines.ToArray(), 0, lines.Count - 1);
                Assert.AreEqual(5, endIndex);
            }
        }

        private static void ValidateIntLists(int[] expected, int[] observed)
        {
            Assert.AreEqual(expected.Length + 1, observed.Length);
            Assert.AreEqual(0, observed[0]);
            for (int l = 1; l < observed.Length; ++l)
                Assert.AreEqual(expected[l - 1], observed[l]);
        }


        [Test]
        public void TestFindElses()
        {
            {
                List<string> lines = new List<string>();
                try
                {
                    ScriptProcessor.FindElses(lines.ToArray(), 0, lines.Count - 1);
                    Assert.Fail();
                }
                catch { }
            }

            {
                List<string> lines = new List<string>();
                lines.Add("}");
                try
                {
                    ScriptProcessor.FindElses(lines.ToArray(), 0, lines.Count - 1);
                    Assert.Fail();
                }
                catch { }
            }

            {
                List<string> lines = new List<string>();
                lines.Add("? x = 1");
                try
                {
                    ScriptProcessor.FindElses(lines.ToArray(), 0, lines.Count - 1);
                    Assert.Fail();
                }
                catch { }
            }

            {
                List<string> lines = new List<string>();
                lines.Add("? x = 1");
                lines.Add("}");
                var markers = ScriptProcessor.FindElses(lines.ToArray(), 0, lines.Count - 1);
                ValidateIntLists(new[] { 1 }, markers.ToArray());
            }

            {
                List<string> lines = new List<string>();
                lines.Add("? x = 1");
                lines.Add("    $ y = x + 1");
                lines.Add("}");
                var markers = ScriptProcessor.FindElses(lines.ToArray(), 0, lines.Count - 1);
                ValidateIntLists(new[] { 2 }, markers.ToArray());
            }

            {
                List<string> lines = new List<string>();
                lines.Add("? x = 1");
                lines.Add("    $ y = x + 1");
                lines.Add("}");
                lines.Add("$ z = x + y");
                var markers = ScriptProcessor.FindElses(lines.ToArray(), 0, lines.Count - 1);
                ValidateIntLists(new[] { 2 }, markers.ToArray());
            }

            {
                List<string> lines = new List<string>();
                lines.Add("? x = 1");
                lines.Add("    $ y = x + 1");
                lines.Add("<>");
                lines.Add("    $ y = x + 3");
                lines.Add("}");
                lines.Add("$ z = x + y");
                var markers = ScriptProcessor.FindElses(lines.ToArray(), 0, lines.Count - 1);
                ValidateIntLists(new[] { 2, 4 }, markers.ToArray());
            }

            {
                List<string> lines = new List<string>();
                lines.Add("? x = 1");
                lines.Add("    $ y = x + 1");
                lines.Add("    $ a = y + 1");
                lines.Add("? x = 3");
                lines.Add("    $ y = x + 7");
                lines.Add("<>");
                lines.Add("    $ y = x + 3");
                lines.Add("}");
                lines.Add("$ z = x + y");
                var markers = ScriptProcessor.FindElses(lines.ToArray(), 0, lines.Count - 1);
                ValidateIntLists(new[] { 3, 5, 7 }, markers.ToArray());
            }

            {
                List<string> lines = new List<string>();
                lines.Add("? x = 1");
                lines.Add("    $ y = x + 1");
                lines.Add("    $ a = y + 1");
                lines.Add("? x = 3");
                lines.Add("    $ y = x + 7");
                lines.Add("<>");
                lines.Add("    $ y = x + 3");
                lines.Add("}");
                lines.Add("$ z = x + y");
                var markers = ScriptProcessor.FindElses(lines.ToArray(), 0, lines.Count - 1);
                ValidateIntLists(new[] { 3, 5, 7 }, markers.ToArray());
            }

            {
                List<string> lines = new List<string>();
                lines.Add("? x = 1");
                lines.Add("    $ y = x + 1");
                lines.Add("    O");
                lines.Add("        $ y = x + 7");
                lines.Add("        $ y = x + 3");
                lines.Add("    }");
                lines.Add("}");
                lines.Add("$ z = x + y");
                var markers = ScriptProcessor.FindElses(lines.ToArray(), 0, lines.Count - 1);
                ValidateIntLists(new[] { 6 }, markers.ToArray());
            }

            {
                List<string> lines = new List<string>();
                lines.Add("? x = 1");
                lines.Add("    $ y = x + 1");
                lines.Add("    O");
                lines.Add("        ? x = 1");
                lines.Add("            $ y = x + 1");
                lines.Add("        <>");
                lines.Add("            $ y = x + 3");
                lines.Add("        }");
                lines.Add("    }");
                lines.Add("}");
                lines.Add("$ z = x + y");
                var markers = ScriptProcessor.FindElses(lines.ToArray(), 0, lines.Count - 1);
                ValidateIntLists(new[] { 9 }, markers.ToArray());
            }
        }

        [Test]
        public void TestUniqueId()
        {
            var symbols = new SymbolTable();
            var exp = new Expression(symbols, callable: null);
            var answer1 = exp.EvaluateAsync("uniqueId()").Result;
            var answer2 = exp.EvaluateAsync("uniqueId()").Result;
            Assert.AreNotEqual(Guid.Parse(answer1.ToString()), Guid.Parse(answer2.ToString()));
        }

        [Test]
        public void TestToPrettyDate()
        {
            var symbols = new SymbolTable();
            var exp = new Expression(symbols, callable: null);

            var today = exp.EvaluateAsync("today()").Result;
            symbols.Set("str", today);

            var pretty = exp.EvaluateAsync("str.toPrettyDate()").Result;
            Assert.AreEqual(DateTime.Parse(today.ToString()), DateTime.Parse(pretty.ToString()));
        }

        [Test]
        public void TestStringFunction()
        {
            var symbols = new SymbolTable();
            var exp = new Expression(symbols, callable: null);

            Assert.AreEqual("1, 2, 3", exp.EvaluateAsync("string(list(1, 2, 3))").Result);
            Assert.AreEqual("a: 1, b: 2", exp.EvaluateAsync("string(index(\"a\", 1, \"b\", 2))").Result);

            symbols.Set("num", 12);
            Assert.AreEqual("12", exp.EvaluateAsync("string(num)").Result);
        }

        [Test]
        public void TestNumberFunction()
        {
            var symbols = new SymbolTable();
            var exp = new Expression(symbols, callable: null);

            Assert.AreEqual(1.0, exp.EvaluateAsync("number(true)").Result);
            Assert.AreEqual(0.0, exp.EvaluateAsync("number(false)").Result);
            Assert.AreEqual(12.0, exp.EvaluateAsync("number(\"12.0\")").Result);
            Assert.AreEqual(12.0, exp.EvaluateAsync("number(12.0)").Result);

            symbols.Set("num", 12);
            Assert.AreEqual(12.0, exp.EvaluateAsync("number(num)").Result);
        }

        [Test]
        public void TestGetType()
        {
            var symbols = new SymbolTable();
            var exp = new Expression(symbols, callable: null);

            Assert.AreEqual("number", exp.EvaluateAsync("getType(12.0)").Result);
            Assert.AreEqual("string", exp.EvaluateAsync("getType(\"str\")").Result);
            Assert.AreEqual("bool", exp.EvaluateAsync("getType(true)").Result);
            Assert.AreEqual("list", exp.EvaluateAsync("getType(list(1, 2, 3, 4))").Result);
            Assert.AreEqual("index", exp.EvaluateAsync("getType(index(1, 2, 3, 4))").Result);
            Assert.AreEqual("null", exp.EvaluateAsync("getType(null)").Result);
        }
    }
}
*/
