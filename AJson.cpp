#include "AJson.h"

#include <cmath>
#include <cstdlib>

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) > '0' && (ch) <= '9')

namespace AJson {
	ParseResult Value::parse(const char *s)
	{
		m_c.stack = nullptr;
		m_c.size = m_c.top = 0;
		m_c.json = s;
		parseWhitespace();
		auto res = parseValue();
		if (res == PARSE_OK) {
			parseWhitespace();
			if (*m_c.json != '\0') {
				res = PARSE_ROOT_NOT_SINGULAR;
				m_type = VALUE_TYPE_NULL;
			}
		} else {
			m_type = VALUE_TYPE_NULL;
		}
		free(m_c.stack);
		return res;
	}

	void Value::setString(const char *s, size_t len)
	{
		assert(s != nullptr || len == 0);
		m_s.s = (char *)malloc(sizeof(char) * (len + 1));
		memcpy(m_s.s, s, len);
		m_s.s[len] = '\0';
		m_s.len = len;
		m_type = VALUE_TYPE_STRING;
	}

	ParseResult Value::parseValue()
	{
		switch (*m_c.json) {
		case 'n':
			return parseLiteral("null", VALUE_TYPE_NULL);
		case 't':
			return parseLiteral("true", VALUE_TYPE_TRUE);
		case 'f':
			return parseLiteral("false", VALUE_TYPE_FALSE);
		case '\"':
			return parseString();
		case '\0':
			return PARSE_EXPECT_VALUE;
		case '[':
			return parseArray();
		default:
			if (*m_c.json == '-' || ISDIGIT(*m_c.json))
				return parseNumber();
			return PARSE_INVALID_VALUE;
		}
	}

	/* ws = *(%x20 / %x09 / %x0A / %x0D) */
	void Value::parseWhitespace()
	{
		const char* p = m_c.json;
		while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
			++p;
		m_c.json = p;
	}

	ParseResult Value::parseLiteral(const char* literal, ValueType type)
	{
		size_t i = 1;
		for (; literal[i]; ++i)
			if (m_c.json[i] != literal[i])
				return PARSE_INVALID_VALUE;
		m_c.json += i;
		m_type = type;
		
		return PARSE_OK;
	}

	ParseResult Value::parseNumber()
	{
		const char* p = m_c.json;

		if (*p == '-')
			++p;
		if (*p == '0')
			++p;
		else {
			if (!ISDIGIT1TO9(*p))
				return PARSE_INVALID_VALUE;
			for (++p; ISDIGIT(*p); ++p)
				;
		}
		if (*p == '.') {
			++p;
			if (!ISDIGIT(*p))
				return PARSE_INVALID_VALUE;
			for (++p; ISDIGIT(*p); ++p)
				;
		}
		if (*p == 'e' || *p == 'E') {
			++p;
			if (*p == '-' || *p == '+')
				++p;
			if (!ISDIGIT(*p))
				return PARSE_INVALID_VALUE;
			for (++p; ISDIGIT(*p); ++p)
				;
		}

		errno = 0;
		m_n = strtod(m_c.json, nullptr);
		if (errno == ERANGE && (m_n == HUGE_VAL || m_n == -HUGE_VAL)) {
			return PARSE_NUMBER_TOO_BIG;
		}
		m_c.json = p;
		m_type = VALUE_TYPE_NUMBER;
		return PARSE_OK;
	}

#define PUTC(ch)	\
    do {			\
        *reinterpret_cast<char*>(contextPush(sizeof(char))) = (ch); \
    } while (0)

#define STRING_ERROR(ret)	\
    do {					\
        m_c.top = head;		\
        return ret;			\
    } while (0)

	ParseResult Value::parseString()
	{
		size_t head = m_c.top, len;
		const char* p = ++(m_c.json);
		for (;;) {
			char ch = *p++;
			switch (ch) {
			case '\"':
				len = m_c.top - head;
				setString((char *)contextPop(len), len);
				m_c.json = p;
				return PARSE_OK;
			case '\\':
				ch = *p++;
				switch (ch) {
				case '"':
					PUTC('\"');
					break;
				case '\\':
					PUTC('\\');
					break;
				case 'b':
					PUTC('\b');
					break;
				case 'f':
					PUTC('\f');
					break;
				case 'r':
					PUTC('\r');
					break;
				case 't':
					PUTC('\t');
					break;
				case 'n':
					PUTC('\n');
					break;
				case '/':
					PUTC('/');
					break;
				case 'u':
					unsigned u;
					if (!parseHex4(p, u))
						STRING_ERROR(PARSE_INVALID_UNICODE_HEX);
					if (u >= 0xd800 && u <= 0xdbff) {
						unsigned ul;
						if ((*p++) == '\\' && (*p++) == '\\' && (*p++) == 'u'
							&& parseHex4(p, ul) && ul >= 0xdc00 && ul <= 0xdfff) {
							u = 0x10000 | ((u & 0x7ff) << 10) | (ul & 0x3ff);
						} else {
							STRING_ERROR(PARSE_INVALID_UNICODE_SURROGATE);
						}
					}
					encode_utf8(u);
					break;
				default:
					STRING_ERROR(PARSE_INVALID_STRING_ESCAPE);
				}
				break;
			case '\0':
				STRING_ERROR(PARSE_MISS_QUOTATION_MARK);
			default:
				if (ch < 0x20) {
					STRING_ERROR(PARSE_INVALID_STRING_CHAR);
				}
				PUTC(ch);
			}
		}
	}

	ParseResult Value::parseArray()
	{
		return PARSE_OK;
	}

	void Value::freeStr()
	{
		if (m_type == VALUE_TYPE_STRING) free(m_s.s);
		m_type = VALUE_TYPE_NULL;
	}

	void* Value::contextPush(size_t size)
	{
		assert(size > 0);
		if (m_c.top + size >= m_c.size) {
			if (m_c.size == 0)
				m_c.size = AJ_PARSE_STACK_INIT_SIZE;
			while (m_c.top + size > m_c.size)
				m_c.size += m_c.size >> 1;
			m_c.stack = (char *)realloc(m_c.stack, m_c.size);
		}
		void* res = m_c.stack + m_c.top;
		m_c.top += size;
		return res;
	}

	void* Value::contextPop(size_t size)
	{
		assert(m_c.top >= size);
		return m_c.stack + (m_c.top -= size);
	}

	bool Value::parseHex4(const char*& p, unsigned& u)
	{
		u = 0;
		for (auto i = 0; i < 4; ++i) {
			u <<= 4;
			char ch = *p++;
			if (ch >= '0' && ch <= '9')
				u += ch - '0';
			else if (ch >= 'a' && ch <= 'f')
				u += ch - 'a' + 10;
			else if (ch >= 'A' && ch <= 'F')
				u += ch - 'A' + 10;
			else
				return false;
		}
		return true;
	}

	void Value::encode_utf8(unsigned u)
	{
		if (u < 0x80) {
			PUTC(0x7f & u);
		} else if (u < 0x800) {
			PUTC(0xc0 | ((u >> 6) & 0x1f));
			PUTC(0x80 | (u & 0x3f));
		} else if (u < 0x10000) {
			PUTC(0xe0 | ((u >> 12) & 0x0f));
			PUTC(0x80 | ((u >> 6) & 0x3f));
			PUTC(0x80 | (u & 0x3f));
		} else {
			PUTC(0xf0 | ((u >> 18) & 0x03));
			PUTC(0x80 | ((u >> 12) & 0x3f));
			PUTC(0x80 | ((u >> 6) & 0x3f));
			PUTC(0x80 | (u & 0x3f));
		}
	}
}
