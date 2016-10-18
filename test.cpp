#include "AJson.h"

#include <cstdio>
#include <cstring>
#include <iostream>

int main_ret = 0;
int test_count = 0;
int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual)            \
    do {                                                    \
        ++test_count;                                       \
        if (equality)                                       \
            ++test_pass;                                    \
        else {                                              \
            std::cerr << __FILE__ << ":" << __LINE__;       \
            std::cerr << ": expect:" << expect;             \
            std::cerr << " actual:" << actual << std::endl; \
            main_ret = 1;                                   \
        }                                                   \
    } while (0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual)
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual)
#define EXPECT_EQ_STRING(expect, actual, len) EXPECT_EQ_BASE(sizeof(expect) - 1 == len && memcmp(expect, actual, len) == 0, expect, actual)

#define EXPECT_TRUE(actual) EXPECT_EQ_BASE(true == actual, "true", "false")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE(false == actual, "false", "true")

#define TEST_ERROR(error, json)                  \
    do {                                         \
        AJ_value v;                              \
        v.type = AJ_TRUE;                        \
        EXPECT_EQ_INT(error, AJ_parse(v, json)); \
        EXPECT_EQ_INT(AJ_NULL, AJ_getType(v));   \
    } while (0)

void test_parse_expect_value()
{
    TEST_ERROR(AJ_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(AJ_PARSE_EXPECT_VALUE, "    ");
}

void test_parse_invalid_value()
{
    TEST_ERROR(AJ_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(AJ_PARSE_INVALID_VALUE, "?");

    /* invalid number */
    TEST_ERROR(AJ_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(AJ_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(AJ_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(AJ_PARSE_INVALID_VALUE, "1."); /* at least one digit after '.' */
    TEST_ERROR(AJ_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(AJ_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(AJ_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(AJ_PARSE_INVALID_VALUE, "nan");
}

void test_parse_root_not_singular()
{
    TEST_ERROR(AJ_PARSE_ROOT_NOT_SINGULAR, "null jdkfs");
    /* invalid number */
    TEST_ERROR(AJ_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
    TEST_ERROR(AJ_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(AJ_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

void test_parse_literal(AJ_type type, const char* json)
{
    AJ_value v;
    v.type = AJ_TRUE;
    EXPECT_EQ_INT(AJ_PARSE_OK, AJ_parse(v, json));
    EXPECT_EQ_INT(type, AJ_getType(v));
}

void test_parse_null()
{
    test_parse_literal(AJ_NULL, "null");
}

void test_parse_true_false()
{
    test_parse_literal(AJ_TRUE, "true");
    test_parse_literal(AJ_FALSE, "false");
}

void test_parse_number_too_big()
{
    TEST_ERROR(AJ_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(AJ_PARSE_NUMBER_TOO_BIG, "-1e309");
}

void test_parse_string_miss_quotation_mark()
{
    TEST_ERROR(AJ_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(AJ_PARSE_MISS_QUOTATION_MARK, "\"fskjkdk");
}

void test_parse_invalid_string_escape()
{
    TEST_ERROR(AJ_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(AJ_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(AJ_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(AJ_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

void test_parse_invalid_string_char()
{
    TEST_ERROR(AJ_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(AJ_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

#define TEST_NUMBER(expect, json)                      \
    do {                                               \
        AJ_value v;                                    \
        EXPECT_EQ_INT(AJ_PARSE_OK, AJ_parse(v, json)); \
        EXPECT_EQ_INT(AJ_NUMBER, AJ_getType(v));       \
        EXPECT_EQ_DOUBLE(expect, AJ_getNumber(v));     \
    } while (0)

void test_parse_number()
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

#define TEST_STRING(expect, json)                                         \
    do {                                                                  \
        AJ_value v;                                                       \
        EXPECT_EQ_INT(AJ_PARSE_OK, AJ_parse(v, json));                    \
        EXPECT_EQ_INT(AJ_STRING, AJ_getType(v));                          \
        EXPECT_EQ_STRING(expect, AJ_getString(v), AJ_getStringLength(v)); \
    } while (0)

void test_parse_string()
{
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\"\\/\b\f\n\r\t", "\"\\\"\\\\\\/\\b\\f\\n\\r\\t\"");
}

void test_parse_invalid_unicode_hex()
{
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
}

void test_parse_invalid_unicode_surrogate()
{
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(AJ_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

void test_parse_array()
{
    AJ_value v;
    v.type = AJ_NULL;
    EXPECT_EQ_INT(AJ_PARSE_OK, AJ_parse(v, "[ ]"));
    EXPECT_EQ_INT(AJ_ARRAY, AJ_getType(v));
    EXPECT_EQ_INT(0, AJ_getArraySize(v));
    EXPECT_EQ_INT(AJ_PARSE_OK, AJ_parse(v, " [null,false,true,123, \"abc\"] "));
    EXPECT_EQ_INT(AJ_ARRAY, AJ_getType(v));
    EXPECT_EQ_INT(5, AJ_getArraySize(v));
    EXPECT_EQ_INT(AJ_PARSE_OK, AJ_parse(v, "[ [],[0],[0,1],[0,1,2]]"));
    EXPECT_EQ_INT(AJ_ARRAY, AJ_getType(v));
    EXPECT_EQ_INT(4, AJ_getArraySize(v));
}

void test_parse()
{
    test_parse_null();
    test_parse_true_false();
    test_parse_number();
    test_parse_string();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_string_miss_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();
    test_parse_array();
}

void test_access_null()
{
    AJ_value v;
    AJ_setBool(v, true);
    AJ_setNull(v);
    EXPECT_EQ_INT(AJ_NULL, AJ_getType(v));
}

void test_access_bool()
{
    AJ_value v;
    AJ_setBool(v, true);
    EXPECT_TRUE(AJ_getBool(v));
    AJ_setBool(v, false);
    EXPECT_FALSE(AJ_getBool(v));
}

#define TEST_ACCESS_NUMBER(expect)                 \
    do {                                           \
        AJ_value v;                                \
        AJ_setNumber(v, expect);                   \
        EXPECT_EQ_DOUBLE(expect, AJ_getNumber(v)); \
    } while (0)

void test_access_number()
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

void test_access_string()
{
    AJ_value v;
    AJ_setString(v, "", 0);
    EXPECT_EQ_STRING("", AJ_getString(v), AJ_getStringLength(v));
    AJ_setString(v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", AJ_getString(v), AJ_getStringLength(v));
}

void test_access()
{
    test_access_null();
    test_access_bool();
    test_access_number();
    test_access_string();
}

int main()
{
    test_parse();
    test_access();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
