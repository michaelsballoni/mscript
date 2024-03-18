#include "pch.h"
#include "CppUnitTest.h"

#include "expressions.h"
#include "utils.h"
#pragma comment(lib, "mscript-core")
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
            tracing trace_info;
            expression exp(symbols, callable, trace_info);
            object answer = exp.evaluate(toWideStr(expStr));

            Assert::IsTrue(answer.type() == expected.type());
            if (expected.isNull())
                Assert::IsTrue(answer.isNull());
            else if (expected.type() == object::BOOL)
                Assert::AreEqual(expected.boolVal(), answer.boolVal());
            else if (expected.type() == object::NUMBER) 
                Assert::AreEqual(round2places(expected.numberVal()), round2places(answer.numberVal()));
            else // handle LIST and INDEX with toString
                Assert::AreEqual(expected.toString(), answer.toString());
        }

        TEST_METHOD(TestFunctionPrecedence)
        {
            symbol_table symtable;
            TestCallable callable;
            tracing trace_info;
            expression exp(symtable, callable, trace_info);
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

            ValidateExpression("3=3", true, symbols);
            ValidateExpression("3==3", true, symbols);
            ValidateExpression("3 EQU 3", true, symbols);
            ValidateExpression("3!=3", false, symbols);
            ValidateExpression("3<>3", false, symbols);
            ValidateExpression("3 NEQ 3", false, symbols);
            ValidateExpression("3<=4", true, symbols);
            ValidateExpression("3 LEQ 4", true, symbols);
            ValidateExpression("3<4", true, symbols);
            ValidateExpression("3 LSS 4", true, symbols);
            ValidateExpression("3>4", false, symbols);
            ValidateExpression("3 GTR 4", false, symbols);
            ValidateExpression("3>=4", false, symbols);
            ValidateExpression("3 GEQ 4", false, symbols);
            ValidateExpression("3<=4 && 3<4", true, symbols);
            ValidateExpression("3 LEQ 4 AND 3 LSS 4", true, symbols);
            ValidateExpression("3<4 and 5 <= 4 or 12<=50 and 6 <= 13", true, symbols);
            ValidateExpression("3<4 AND 5 <= 4 OR 12<=50 AND 6 <= 13", true, symbols);

            symbols.set(toWideStr("str1"), std::wstring(L"a"));
            symbols.set(toWideStr("str2"), std::wstring(L"b"));
            symbols.set(toWideStr("str3"), std::wstring(L"c"));

            ValidateExpression("str1 = str1", true, symbols);
            ValidateExpression("str1 == str1", true, symbols);
            ValidateExpression("str1 EQU str1", true, symbols);

            ValidateExpression("str1 != str2", true, symbols);
            ValidateExpression("str1 <> str2", true, symbols);
            ValidateExpression("str1 NEQ str2", true, symbols);

            ValidateExpression("str1 < str2", true, symbols);
            ValidateExpression("str1 LSS str2", true, symbols);

            ValidateExpression("str1 <= str2", true, symbols);
            ValidateExpression("str1 LEQ str2", true, symbols);

            ValidateExpression("str2 > str1", true, symbols);
            ValidateExpression("str2 GTR str1", true, symbols);

            ValidateExpression("str2 >= str1", true, symbols);
            ValidateExpression("str2 GEQ str1", true, symbols);

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

            ValidateExpression("squote", toWideStr("\'"), symbols);
            ValidateExpression("a + squote + tab", toWideStr("10\'\t"), symbols);
            ValidateExpression("dquote", toWideStr("\""), symbols);
            ValidateExpression("a + dquote + tab", toWideStr("10\"\t"), symbols);
            ValidateExpression("a + crlf", toWideStr("10\r\n"), symbols);
            ValidateExpression("a + lf", toWideStr("10\n"), symbols);

            ValidateExpression("'foo (\"bar\")'", toWideStr("foo (\"bar\")"), symbols);
            ValidateExpression("\"foo (\'bar\')\"", toWideStr("foo (\'bar\')"), symbols);

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
            ValidateExpression("!q", false, symbols);
            ValidateExpression("not q", false, symbols);
            ValidateExpression("r", false, symbols);
            ValidateExpression("!r", true, symbols);
            ValidateExpression("not r", true, symbols);
            ValidateExpression("NOT r", true, symbols);
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

            ValidateExpression("sin(pi / 4)^2 + cos(pi / 4)^2", 1.0, symbols);

            ValidateExpression("round(pi)", 3.0, symbols);
            ValidateExpression("round(pi, 2)", 3.14, symbols);

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

            ValidateExpression("clone(null)", object(), symbols);
            ValidateExpression("clone(true)", true, symbols);
            ValidateExpression("clone(13.0)", 13.0, symbols);
            ValidateExpression("clone(\"foo\")", toWideStr("foo"), symbols);

            ValidateExpression("toUpper(\"lower\")", toWideStr("LOWER"), symbols);
            ValidateExpression("toLower(\"UPPER\")", toWideStr("upper"), symbols);

            ValidateExpression("replaced(\"foo2bar\", \"o2b\", \"y3z\")", toWideStr("foy3zar"), symbols);
            ValidateExpression("replaced(\"foo2bar\", \"Z\", \"y3b\")", toWideStr("foo2bar"), symbols);

            ValidateExpression("trimmed(\"foobar\")", toWideStr("foobar"), symbols);
            ValidateExpression("trimmed(\" foobar \")", toWideStr("foobar"), symbols);

            ValidateExpression("length(trimmed(\" foobar \"))", 6.0, symbols);

            ValidateExpression("length(replaced(trimmed(\" foobar \"), \"oo\", \"o\"))", 5.0, symbols);

            ValidateExpression("random(0, 1) >= 0 and random(0, 1) <= 1", true, symbols);
            ValidateExpression("random(0, 1) != random(0, 1)", true, symbols);

            ValidateExpression("isMatch(\"\", \"^[a-z]+$\")", false, symbols);
            ValidateExpression("isMatch(\"abc\", \"^[a-z]+$\")", true, symbols);
            ValidateExpression("isMatch(\"xYz\", \"^[a-z]+$\")", false, symbols);
            ValidateExpression("isMatch(\"123\", \"^[a-z]+$\")", false, symbols);

            ValidateExpression("join(list(1,2,3,4))", toWideStr("1234"), symbols);
            ValidateExpression("join(list(1,2,3,4), \", \")", toWideStr("1, 2, 3, 4"), symbols);
            ValidateExpression("join(split(\"foo bar\", \" \"), \" blet \")", toWideStr("foo blet bar"), symbols);

            ValidateExpression("get(exec(\"echo foobar\"), \"exit_code\")", 0.0, symbols);

            ValidateExpression("writeFile(\"test.txt\", \"testy test\", \"ascii\")", true, symbols);
            ValidateExpression("readFile(\"test.txt\", \"ascii\")", toWideStr("testy test"), symbols);

            ValidateExpression("writeFile(\"test.txt\", \"testy test\", \"utf-8\")", true, symbols);
            ValidateExpression("readFile(\"test.txt\", \"utf-8\")", toWideStr("testy test"), symbols);

            ValidateExpression("writeFile(\"test.txt\", \"testy test\", \"utf-16\")", true, symbols);
            ValidateExpression("readFile(\"test.txt\", \"utf-16\")", toWideStr("testy test"), symbols);

            ValidateExpression("toJson(list(1,2,3))", toWideStr("[1, 2, 3]"), symbols);
            ValidateExpression("fromJson(\"[1, 2, 3]\")", object::list{ 1.0, 2.0, 3.0 }, symbols);

            ValidateExpression("fmt(\"\")", toWideStr(""), symbols);
            ValidateExpression("fmt(\"{0} - {1}\")", toWideStr("{0} - {1}"), symbols);
            ValidateExpression("fmt(\"foobar\")", toWideStr("foobar"), symbols);
            ValidateExpression("fmt(\"foo{0}bar\", \"\")", toWideStr("foobar"), symbols);
            ValidateExpression("fmt(\"foo{0}bar\", \"123\")", toWideStr("foo123bar"), symbols);
            ValidateExpression("fmt(\"foo{0}bar{1}\", \"123\", 14)", toWideStr("foo123bar14"), symbols);
        }
    };
}
