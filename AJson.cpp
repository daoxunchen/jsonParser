#include "AJson.h"

#include <cassert>
#include <cerrno>
#include <cmath>

#define EXPECT(c, ch)            \
    do {                         \
        assert(*c.json == (ch)); \
        c.json++;                \
    } while (0)

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) > '0' && (ch) <= '9')

/* ws = *(%x20 / %x09 / %x0A / %x0D) */
void AJ_parseWhitespace(AJ_context& c)
{
    const char* p = c.json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        ++p;
    c.json = p;
}

int AJ_parseLiteral(AJ_context& c, AJ_value& v, const char* literal, AJ_type type)
{
    EXPECT(c, literal[0]);
    size_t i;
    for (i = 0; literal[i + 1]; ++i)
        if (c.json[i] != literal[i + 1])
            return AJ_PARSE_INVALID_VALUE;
    c.json += i;
    v.type = type;
    return AJ_PARSE_OK;
}

int AJ_parseNumber(AJ_context& c, AJ_value& v)
{
    const char* p = c.json;

    if (*p == '-')
        ++p;
    if (*p == '0')
        ++p;
    else {
        if (!ISDIGIT1TO9(*p))
            return AJ_PARSE_INVALID_VALUE;
        for (++p; ISDIGIT(*p); ++p)
            ;
    }
    if (*p == '.') {
        ++p;
        if (!ISDIGIT(*p))
            return AJ_PARSE_INVALID_VALUE;
        for (++p; ISDIGIT(*p); ++p)
            ;
    }
    if (*p == 'e' || *p == 'E') {
        ++p;
        if (*p == '-' || *p == '+')
            ++p;
        if (!ISDIGIT(*p))
            return AJ_PARSE_INVALID_VALUE;
        for (++p; ISDIGIT(*p); ++p)
            ;
    }

    errno = 0;
    v.n = strtod(c.json, nullptr);
    if (errno == ERANGE && (v.n == HUGE_VAL || v.n == -HUGE_VAL)) {
        return AJ_PARSE_NUMBER_TOO_BIG;
    }
    c.json = p;
    v.type = AJ_NUMBER;
    return AJ_PARSE_OK;
}

#ifndef AJ_PARSE_STACK_INIT_SIZE
#define AJ_PARSE_STACK_INIT_SIZE 256
#endif

void* AJ_contextPush(AJ_context& c, size_t size)
{
    void* res;
    assert(size > 0);
    if (c.top + size >= c.size) {
        if (c.size == 0)
            c.size = AJ_PARSE_STACK_INIT_SIZE;
        while (c.top + size > c.size)
            c.size += c.size >> 1;
        c.stack = reinterpret_cast<char*>(realloc(c.stack, c.size));
    }
    res = c.stack + c.top;
    c.top += size;
    return res;
}

void* AJ_contextPop(AJ_context& c, size_t size)
{
    assert(c.top >= size);
    return c.stack + (c.top -= size);
}

#define PUTC(c, ch)                                                       \
    do {                                                                  \
        *reinterpret_cast<char*>(AJ_contextPush(c, sizeof(char))) = (ch); \
    } while (0)

int AJ_parseString(AJ_context& c, AJ_value& v)
{
    size_t head = c.top, len;
    EXPECT(c, '\"');
    const char* p = c.json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
        case '\"':
            len = c.top - head;
            AJ_setString(v, reinterpret_cast<char*>(AJ_contextPop(c, len)), len);
            c.json = p;
            return AJ_PARSE_OK;
        case '\\':
            ch = *p++;
            switch (ch) {
            case '"':
                PUTC(c, '\"');
                break;
            case '\\':
                PUTC(c, '\\');
                break;
            case 'b':
                PUTC(c, '\b');
                break;
            case 'f':
                PUTC(c, '\f');
                break;
            case 'r':
                PUTC(c, '\r');
                break;
            case 't':
                PUTC(c, '\t');
                break;
            case 'n':
                PUTC(c, '\n');
                break;
            case '/':
                PUTC(c, '/');
                break;
            default:
                c.top = head;
                return AJ_PARSE_INVALID_STRING_ESCAPE;
            }
            break;
        case '\0':
            c.top = head;
            return AJ_PARSE_MISS_QUOTATION_MARK;
        default:
            if (ch < 0x20) {
                c.top = head;
                return AJ_PARSE_INVALID_STRING_CHAR;
            }
            PUTC(c, ch);
        }
    }
}

int AJ_parseValue(AJ_context& c, AJ_value& v)
{
    switch (*c.json) {
    case 'n':
        return AJ_parseLiteral(c, v, "null", AJ_NULL);
    case 't':
        return AJ_parseLiteral(c, v, "true", AJ_TRUE);
    case 'f':
        return AJ_parseLiteral(c, v, "false", AJ_FALSE);
    case '\"':
        return AJ_parseString(c, v);
    case '\0':
        return AJ_PARSE_EXPECT_VALUE;
    default:
        if (*c.json == '-' || ISDIGIT(*c.json))
            return AJ_parseNumber(c, v);
        return AJ_PARSE_INVALID_VALUE;
    }
}

int AJ_parse(AJ_value& v, const char* const json)
{
    AJ_context c;
    c.json = json;
    c.stack = nullptr;
    c.size = c.top = 0;
    AJ_parseWhitespace(c);
    auto res = AJ_parseValue(c, v);
    if (res == AJ_PARSE_OK) {
        AJ_parseWhitespace(c);
        if (*c.json != '\0') {
            res = AJ_PARSE_ROOT_NOT_SINGULAR;
            v.type = AJ_NULL;
        }
    } else {
        v.type = AJ_NULL;
    }
    free(c.stack);
    return res;
}

AJ_type AJ_getType(const AJ_value& v)
{
    return v.type;
}

void AJ_setBool(AJ_value& v, bool b)
{
    v.type = b ? AJ_TRUE : AJ_FALSE;
}

bool AJ_getBool(const AJ_value& v)
{
    assert(v.type == AJ_TRUE || v.type == AJ_FALSE);
    if (v.type == AJ_TRUE)
        return true;
    return false;
}

void AJ_setNumber(AJ_value& v, double n)
{
    v.n = n;
    v.type = AJ_NUMBER;
}

double AJ_getNumber(const AJ_value& v)
{
    assert(v.type == AJ_NUMBER);
    return v.n;
}

void AJ_setString(AJ_value& v, const char* s, size_t len)
{
    assert(s != nullptr || len == 0);
    v.s = std::string(s, len);
    v.type = AJ_STRING;
}

const char* AJ_getString(const AJ_value& v)
{
    assert(v.type == AJ_STRING);
    return v.s.c_str();
}

size_t AJ_getStringLength(const AJ_value& v)
{
    assert(v.type == AJ_STRING);
    return v.s.size();
}
