#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "AJson.h"
using namespace AJson;

TEST_CASE("parseLiteral", "[parse][literal]")
{
	Value v;
	REQUIRE(PARSE_OK == v.parse("null"));
	REQUIRE(VALUE_TYPE_NULL == v.type());
	REQUIRE(PARSE_OK == v.parse("true"));
	REQUIRE(VALUE_TYPE_TRUE == v.type());
	REQUIRE(PARSE_OK == v.parse("false"));
	REQUIRE(VALUE_TYPE_FALSE == v.type());
}

#define TEST_NUMBER(expect, json)			\
    do {									\
        Value v;							\
        REQUIRE(PARSE_OK == v.parse(json));	\
        REQUIRE(VALUE_TYPE_NUMBER == v.type());\
        REQUIRE(expect == v.getNumber());	\
    } while (0)

TEST_CASE("parseNumber", "[parse][number]")
{
	TEST_NUMBER(0.0, "0");
	TEST_NUMBER(0.0, "-0");
	TEST_NUMBER(0.0, "-0.0");
	TEST_NUMBER(1.0, "1");
	TEST_NUMBER(-1.0, "-1");
	TEST_NUMBER(1.5, "1.5");
	TEST_NUMBER(-1.5, "-1.5");
	TEST_NUMBER(3.1416, "3.1416");
	TEST_NUMBER(1E10, "1E10");
	TEST_NUMBER(1e10, "1e10");
	TEST_NUMBER(1E+10, "1E+10");
	TEST_NUMBER(1E-10, "1E-10");
	TEST_NUMBER(-1E10, "-1E10");
	TEST_NUMBER(-1e10, "-1e10");
	TEST_NUMBER(-1E+10, "-1E+10");
	TEST_NUMBER(-1E-10, "-1E-10");
	TEST_NUMBER(1.234E+10, "1.234E+10");
	TEST_NUMBER(1.234E-10, "1.234E-10");
	TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

								  // the smallest number > 1
	TEST_NUMBER(1.0000000000000002, "1.0000000000000002");
	// Min subnormal positive double
	TEST_NUMBER(4.94065645841246544176568792868221372365059802614325e-324, "4.94065645841246544176568792868221372365059802614325e-324");
	TEST_NUMBER(-4.94065645841246544176568792868221372365059802614325e-324, "-4.94065645841246544176568792868221372365059802614325e-324");
	// Max subnormal double
	TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");
	TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
	// Min normal positive double
	TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");
	TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
	// Max double
	TEST_NUMBER(1.7976931348623157e308, "1.7976931348623157e308");
	TEST_NUMBER(-1.7976931348623157e308, "-1.7976931348623157e308");
}

#define TEST_ERROR(error, json)             \
    do {                                    \
        Value v;                            \
        REQUIRE(error == v.parse(json));	\
        REQUIRE(VALUE_TYPE_NULL == v.type());\
    } while (0)

TEST_CASE("parseExpectValue", "[parse][error]")
{
	TEST_ERROR(PARSE_EXPECT_VALUE, "");
	TEST_ERROR(PARSE_EXPECT_VALUE, "    ");
}

TEST_CASE("parseInvalidValue", "[parse][error]")
{
	TEST_ERROR(PARSE_INVALID_VALUE, "nul");
	TEST_ERROR(PARSE_INVALID_VALUE, "?");

	/* invalid number */
	TEST_ERROR(PARSE_INVALID_VALUE, "+0");
	TEST_ERROR(PARSE_INVALID_VALUE, "+1");
	TEST_ERROR(PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
	TEST_ERROR(PARSE_INVALID_VALUE, "1."); /* at least one digit after '.' */
	TEST_ERROR(PARSE_INVALID_VALUE, "INF");
	TEST_ERROR(PARSE_INVALID_VALUE, "inf");
	TEST_ERROR(PARSE_INVALID_VALUE, "NAN");
	TEST_ERROR(PARSE_INVALID_VALUE, "nan");

	/* invalid value in array */
	TEST_ERROR(PARSE_INVALID_VALUE, "[1,]");
	TEST_ERROR(PARSE_INVALID_VALUE, "[\"a\", nul]");
}

TEST_CASE("parseRootNotSingular", "[parse][error]")
{
	TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "null jdkfs");
	/* invalid number */
	TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
	TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "0x0");
	TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "0x123");
}

TEST_CASE("parseNumberTooBig", "[parse][error]")
{
	TEST_ERROR(PARSE_NUMBER_TOO_BIG, "1e309");
	TEST_ERROR(PARSE_NUMBER_TOO_BIG, "-1e309");
}

TEST_CASE("parseMissQuotationMark", "[parse][error]")
{
	TEST_ERROR(PARSE_MISS_QUOTATION_MARK, "\"");
	TEST_ERROR(PARSE_MISS_QUOTATION_MARK, "\"fskjkdk");
}

TEST_CASE("parseInvalidStringEscape", "[parse][error]")
{
	TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
	TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
	TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
	TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

TEST_CASE("parseInvalidStringChar", "[parse][error]")
{
	TEST_ERROR(PARSE_INVALID_STRING_CHAR, "\"\x01\"");
	TEST_ERROR(PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

TEST_CASE("parseMissCommaOrSquareBracket", "[parse][error]")
{
	TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
	TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
	TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
	TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

#define REQUIRE_STRING(expect, str, len)		\
	do{											\
		REQUIRE(sizeof(expect) - 1 == (len));	\
		REQUIRE(memcmp(expect, str, len) == 0);	\
	} while (0)

#define TEST_STRING(expect, json)				\
    do {                                        \
        Value v;                                \
        REQUIRE(PARSE_OK == v.parse(json));		\
        REQUIRE(VALUE_TYPE_STRING == v.type());	\
		REQUIRE_STRING(expect, v.getString(), v.getStringLength());\
    } while (0)

TEST_CASE("ParseString", "[parse][string]")
{
	TEST_STRING("", "\"\"");
	TEST_STRING("Hello", "\"Hello\"");
	TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
	TEST_STRING("\"\\/\b\f\n\r\t", "\"\\\"\\\\\\/\\b\\f\\n\\r\\t\"");
}

TEST_CASE("parseInvalidUnicodeHex", "[parse][error]")
{
	TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
	TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
	TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
	TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
	TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
	TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
	TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
	TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
	TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
	TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
	TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
	TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
}

TEST_CASE("parseInvalidUnicodeSurrogate", "[parse][error]")
{
	TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
	TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
	TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
	TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
	TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

TEST_CASE("parseArray", "[parse][array]")
{
	Value v;
	REQUIRE(PARSE_OK == v.parse("[  ]"));
	REQUIRE(VALUE_TYPE_ARRAY == v.type());
	REQUIRE(0 == v.getArraySize());
	REQUIRE(PARSE_OK == v.parse("[null , false , true , 123 , \"abc\"]"));
	REQUIRE(VALUE_TYPE_ARRAY == v.type());
	REQUIRE(5 == v.getArraySize());
	REQUIRE(PARSE_OK == v.parse("[[] , [ 0] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
	REQUIRE(VALUE_TYPE_ARRAY == v.type());
	REQUIRE(4 == v.getArraySize());
	REQUIRE(VALUE_TYPE_ARRAY == v.getArrayElement(0)->type());
	REQUIRE(0 == v.getArrayElement(0)->getArraySize());
	REQUIRE(VALUE_TYPE_ARRAY == v.getArrayElement(1)->type());
	REQUIRE(1 == v.getArrayElement(1)->getArraySize());
	REQUIRE(VALUE_TYPE_ARRAY == v.getArrayElement(2)->type());
	REQUIRE(2 == v.getArrayElement(2)->getArraySize());
	REQUIRE(VALUE_TYPE_ARRAY == v.getArrayElement(3)->type());
	REQUIRE(3 == v.getArrayElement(3)->getArraySize());
}

TEST_CASE("accessNull", "[access][null]")
{
	Value v;
	v.setBool(true);
	v.setNull();
	REQUIRE(VALUE_TYPE_NULL == v.type());
}

TEST_CASE("accessBool", "[access][bool]")
{
	Value v;
	v.setBool(true);
	REQUIRE(v.getBool());
	v.setBool(false);
	REQUIRE_FALSE(v.getBool());
}

#define TEST_ACCESS_NUMBER(expect)	\
    do {							\
        Value v;					\
        v.setNumber(expect);		\
        REQUIRE(expect == v.getNumber()); \
    } while (0)

TEST_CASE("accessNumber", "[access][Number]")
{
	TEST_ACCESS_NUMBER(0.0);
	TEST_ACCESS_NUMBER(1.0);
	TEST_ACCESS_NUMBER(-1.0);
	TEST_ACCESS_NUMBER(1.5);
	TEST_ACCESS_NUMBER(-1.5);
	TEST_ACCESS_NUMBER(3.1416);
	TEST_ACCESS_NUMBER(1E10);
	TEST_ACCESS_NUMBER(1e10);
	TEST_ACCESS_NUMBER(1E+10);
	TEST_ACCESS_NUMBER(1E-10);
	TEST_ACCESS_NUMBER(-1E10);
	TEST_ACCESS_NUMBER(-1e10);
	TEST_ACCESS_NUMBER(-1E+10);
	TEST_ACCESS_NUMBER(-1E-10);
	TEST_ACCESS_NUMBER(1.234E+10);
	TEST_ACCESS_NUMBER(1.234E-10);

	// the smallest number > 1
	TEST_ACCESS_NUMBER(1.0000000000000002);
	// Min subnormal positive double
	TEST_ACCESS_NUMBER(4.94065645841246544176568792868221372365059802614325e-324);
	TEST_ACCESS_NUMBER(-4.94065645841246544176568792868221372365059802614325e-324);
	// Max subnormal double
	TEST_ACCESS_NUMBER(2.2250738585072009e-308);
	TEST_ACCESS_NUMBER(-2.2250738585072009e-308);
	// Min normal positive double
	TEST_ACCESS_NUMBER(2.2250738585072014e-308);
	TEST_ACCESS_NUMBER(-2.2250738585072014e-308);
	// Max double
	TEST_ACCESS_NUMBER(1.7976931348623157e308);
	TEST_ACCESS_NUMBER(-1.7976931348623157e308);
}

TEST_CASE("accessString", "[access][string]")
{
	Value v;
	v.setString("", 0);
	REQUIRE(sizeof("") - 1 == v.getStringLength());
	REQUIRE(memcmp("", v.getString(), v.getStringLength()) == 0);
	v.setString("Hello", 5);
	REQUIRE(sizeof("Hello") - 1 == v.getStringLength());
	REQUIRE(memcmp("Hello", v.getString(), v.getStringLength()) == 0);
}

TEST_CASE("parseMissKey", "[parse][error]")
{
	TEST_ERROR(PARSE_MISS_KEY, "{:1,");
	TEST_ERROR(PARSE_MISS_KEY, "{1:1,");
	TEST_ERROR(PARSE_MISS_KEY, "{true:1,");
	TEST_ERROR(PARSE_MISS_KEY, "{false:1,");
	TEST_ERROR(PARSE_MISS_KEY, "{null:1,");
	TEST_ERROR(PARSE_MISS_KEY, "{[]:1,");
	TEST_ERROR(PARSE_MISS_KEY, "{{}:1,");
	TEST_ERROR(PARSE_MISS_KEY, "{\"a\":1,");
}

TEST_CASE("parseMissColon", "[parse][error]")
{
	TEST_ERROR(PARSE_MISS_COLON, "{\"a\"}");
	TEST_ERROR(PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

TEST_CASE("parseMissCommaOrCurlyBracket", "[parse][error]")
{
	TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
	TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
	TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
	TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

TEST_CASE("parseObject", "[parse][object]")
{
	Value v;
	REQUIRE(PARSE_OK == v.parse(" { } "));
	REQUIRE(VALUE_TYPE_OBJECT == v.type());
	REQUIRE(0 == v.getObjectSize());

	REQUIRE(PARSE_OK == v.parse(
		" { "
		"\"n\" : null , "
		"\"f\" : false , "
		"\"t\" : true , "
		"\"i\" : 123 , "
		"\"s\" : \"abc\", "
		"\"a\" : [1, 2, 3],"
		"\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
		" } "
	));
	REQUIRE(VALUE_TYPE_OBJECT == v.type());
	REQUIRE(7 == v.getObjectSize());
	REQUIRE_STRING("n", v.getObjectKey(0), v.getObjectKeyLength(0));
	REQUIRE(VALUE_TYPE_NULL == v.getObjectValue(0)->type());
	REQUIRE_STRING("f", v.getObjectKey(1), v.getObjectKeyLength(1));
	REQUIRE(VALUE_TYPE_FALSE == v.getObjectValue(1)->type());
	REQUIRE_STRING("t", v.getObjectKey(2), v.getObjectKeyLength(2));
	REQUIRE(VALUE_TYPE_TRUE == v.getObjectValue(2)->type());
	REQUIRE_STRING("i", v.getObjectKey(3), v.getObjectKeyLength(3)); 
	REQUIRE(VALUE_TYPE_NUMBER == v.getObjectValue(3)->type());
	REQUIRE(123.0 == v.getObjectValue(3)->getNumber());
	REQUIRE_STRING("s", v.getObjectKey(4), v.getObjectKeyLength(4));
	REQUIRE(VALUE_TYPE_STRING == v.getObjectValue(4)->type());
	REQUIRE_STRING("abc", v.getObjectValue(4)->getString(), v.getObjectValue(4)->getStringLength());

	REQUIRE_STRING("a", v.getObjectKey(5), v.getObjectKeyLength(5));
	REQUIRE(VALUE_TYPE_ARRAY == v.getObjectValue(5)->type());
	REQUIRE(3 == v.getObjectValue(5)->getArraySize());
	for (auto i = 0; i < 3; ++i) {
		Value *e = v.getObjectValue(5)->getArrayElement(i);
		REQUIRE(VALUE_TYPE_NUMBER == e->type());
		REQUIRE((i + 1.0) == e->getNumber());
	}
	REQUIRE_STRING("o", v.getObjectKey(6), v.getObjectKeyLength(6));
	{
		Value* o = v.getObjectValue(6);
		REQUIRE(VALUE_TYPE_OBJECT == o->type());
		for (auto i = 0; i < 3; ++i) {
			REQUIRE(('1' + i) == o->getObjectKey(i)[0]);
			REQUIRE(1 == o->getObjectKeyLength(i));
			Value* ov = o->getObjectValue(i);
			REQUIRE(VALUE_TYPE_NUMBER == ov->type());
			REQUIRE((i + 1.0) == ov->getNumber());
		}
	}
}

#define TEST_ROUNDTRIP(json) \
	do{ \
		Value v;                                \
        REQUIRE(PARSE_OK == v.parse(json));		\
		std::string json2=v.stringify();		\
		REQUIRE_STRING(json, json2.c_str(), json2.length());\
	}while(0)

TEST_CASE("stringify", "[stringify]")
{
	TEST_ROUNDTRIP("null");
	TEST_ROUNDTRIP("true");
	TEST_ROUNDTRIP("false");
}

int main()
{
#ifdef AJ_MEMORY_LEAK_DETECT
	//_CrtDumpMemoryLeaks();
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif // AJ_MEMORY_LEAK_DETECT
	
	int result = Catch::Session().run();

	Value a;
	a.parse(" { "
		"\"n\" : null , "
		"\"f\" : false , "
		"\"t\" : true , "
		"\"i\" : 123 , "
		"\"s\" : \"abc\", "
		"\"a\" : [1, 2, 3],"
		"\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
		" } ");
	printf("%s\n", a.stringify().c_str());

	return result;
}