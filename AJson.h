#ifndef AJson_H
#define AJson_H

#include <cstdlib>
#include <string>

enum AJ_type { AJ_NULL,
    AJ_FALSE,
    AJ_TRUE,
    AJ_NUMBER,
    AJ_STRING,
    AJ_ARRAY,
    AJ_OBJECT };

enum { AJ_PARSE_OK,
    AJ_PARSE_EXPECT_VALUE,
    AJ_PARSE_INVALID_VALUE,
    AJ_PARSE_ROOT_NOT_SINGULAR,
    AJ_PARSE_NUMBER_TOO_BIG,
    AJ_PARSE_MISS_QUOTATION_MARK,
    AJ_PARSE_INVALID_STRING_ESCAPE,
    AJ_PARSE_INVALID_STRING_CHAR,
    AJ_PARSE_INVALID_UNICODE_HEX,
    AJ_PARSE_INVALID_UNICODE_SURROGATE,
    AJ_PARSE_MISS_COMMA_OR_SQUARE_BRACKET
};

struct AJ_value {
    double n;
    std::string s;
    AJ_value* e;
    size_t size;
    AJ_type type;
};

struct AJ_context {
    const char* json;
    char* stack;
    size_t size, top;
};

void AJ_parseWhitespace(AJ_context&);
int AJ_parseNumber(AJ_context&, AJ_value&);
int AJ_parseString(AJ_context&, AJ_value&);
int AJ_parseValue(AJ_context&, AJ_value&);
int AJ_parse(AJ_value&, const char* const);

AJ_type AJ_getType(const AJ_value&);
void AJ_setNull(AJ_value&);
void AJ_setBool(AJ_value&, bool);
bool AJ_getBool(const AJ_value&);
void AJ_setNumber(AJ_value&, double);
double AJ_getNumber(const AJ_value&);
void AJ_setString(AJ_value&, const char*, size_t);
const char* AJ_getString(const AJ_value&);
size_t AJ_getStringLength(const AJ_value&);
size_t AJ_getArraySize(const AJ_value&);
AJ_value* AJ_getArrayElement(const AJ_value&, size_t);

#endif /* AJson_H */
