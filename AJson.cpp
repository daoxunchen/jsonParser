#include "AJson.h"

#include <cmath>
#include <cstdlib>
//#include <cstdio>

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) > '0' && (ch) <= '9')
#define PUTC(ch)	\
    do {			\
        *static_cast<char*>(contextPush(sizeof(char))) = (ch); \
    } while (0)
#define PUTS(s,len)	\
	do{				\
		memcpy(contextPush(sizeof(char) * len), s, len); \
	} while (0)

namespace AJson {
	Context Value::s_c;
	char Value::s_table[] = { "0123456789ABCDEF" };

	ParseResult Value::parse(const char *s)
	{
		assert(s != nullptr);
		s_c.size = s_c.top = 0;
		s_c.json = s;
		parseWhitespace();
		auto res = parseValue();
		if (res == PARSE_OK) {
			parseWhitespace();
			if (*s_c.json != '\0') {
				res = PARSE_ROOT_NOT_SINGULAR;
				m_type = VALUE_TYPE_NULL;
			}
		} else {
			m_type = VALUE_TYPE_NULL;
		}
		assert(s_c.top == 0);
		free(s_c.stack);
		s_c.stack = nullptr;
		return res;
	}

	void Value::setString(const char *s, size_t len)
	{
		assert(s != nullptr || len == 0);
		freeMem();
		m_s.s = (char *)malloc(sizeof(char) * (len + 1));
		memcpy(m_s.s, s, len);
		m_s.s[len] = '\0';
		m_s.len = len;
		m_type = VALUE_TYPE_STRING;
	}

	const char* Value::getObjectKey(size_t index) const
	{
		assert(m_type == VALUE_TYPE_OBJECT && index < m_o.size);
		return (m_o.m + index)->k;
	}

	size_t Value::getObjectKeyLength(size_t index) const
	{
		assert(m_type == VALUE_TYPE_OBJECT && index < m_o.size);
		return (m_o.m + index)->klen;
	}
	Value* Value::getObjectValue(size_t index) const
	{
		assert(m_type == VALUE_TYPE_OBJECT && index < m_o.size);
		return &((m_o.m + index)->v);
	}

	std::string Value::stringify()
	{
		s_c.stack = static_cast<char *>(malloc(s_c.size = AJ_PARSE_STRINGIFY_INIT_SIZE));
		s_c.top = 0;

		if (stringifyValue() != STRINGIFY_OK) {
			free(s_c.stack);
			s_c.stack = nullptr;
			return std::string();
		}

		PUTC('\0');
		std::string res(s_c.stack);
		free(s_c.stack);
		s_c.stack = nullptr;
		return res;
	}

	ParseResult Value::parseValue()
	{
		switch (*s_c.json) {
		case 'n': return parseLiteral("null", VALUE_TYPE_NULL);
		case 't': return parseLiteral("true", VALUE_TYPE_TRUE);
		case 'f': return parseLiteral("false", VALUE_TYPE_FALSE);
		case '\"': return parseString();
		case '\0': return PARSE_EXPECT_VALUE;
		case '[': return parseArray();
		case '{': return parseObject();
		default:
			if (*s_c.json == '-' || ISDIGIT(*s_c.json))
				return parseNumber();
			return PARSE_INVALID_VALUE;
		}
	}

	/* ws = *(%x20 / %x09 / %x0A / %x0D) */
	void Value::parseWhitespace()
	{
		const char* p = s_c.json;
		while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
			++p;
		s_c.json = p;
	}

	ParseResult Value::parseLiteral(const char* literal, ValueType type)
	{
		size_t i = 1;
		for (; literal[i]; ++i)
			if (s_c.json[i] != literal[i])
				return PARSE_INVALID_VALUE;
		s_c.json += i;
		m_type = type;

		return PARSE_OK;
	}

	ParseResult Value::parseNumber()
	{
		const char* p = s_c.json;

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
		m_n = strtod(s_c.json, nullptr);
		if (errno == ERANGE && (m_n == HUGE_VAL || m_n == -HUGE_VAL)) {
			return PARSE_NUMBER_TOO_BIG;
		}
		s_c.json = p;
		m_type = VALUE_TYPE_NUMBER;
		return PARSE_OK;
	}

#define STRING_ERROR(ret)	\
    do {					\
        s_c.top = head;		\
        return ret;			\
    } while (0)

	ParseResult Value::parseStringRaw(char *&str, size_t &len)
	{
		size_t head = s_c.top;
		const char* p = ++s_c.json;
		for (;;) {
			char ch = *p++;
			switch (ch) {
			case '\"':
				len = s_c.top - head;
				//setString((char *)contextPop(len), len);
				str = (char *)contextPop(len);
				s_c.json = p;
				return PARSE_OK;
			case '\\':
				ch = *p++;
				switch (ch) {
				case '"': PUTC('\"'); break;
				case '\\': PUTC('\\'); break;
				case 'b': PUTC('\b'); break;
				case 'f': PUTC('\f'); break;
				case 'r': PUTC('\r'); break;
				case 't': PUTC('\t'); break;
				case 'n': PUTC('\n'); break;
				case '/': PUTC('/'); break;
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
				default: STRING_ERROR(PARSE_INVALID_STRING_ESCAPE);
				}
				break;
			case '\0': STRING_ERROR(PARSE_MISS_QUOTATION_MARK);
			default:
				if (ch < 0x20) {
					STRING_ERROR(PARSE_INVALID_STRING_CHAR);
				}
				PUTC(ch);
			}
		}
	}

	ParseResult Value::parseString()
	{
		char *s;
		size_t len;
		ParseResult ret;
		if ((ret = parseStringRaw(s, len)) == PARSE_OK) {
			setString(s, len);
		}
		return ret;
	}

	ParseResult Value::parseArray()
	{
		++s_c.json;
		parseWhitespace();
		if (*s_c.json == ']') {
			++s_c.json;
			freeMem();
			m_type = VALUE_TYPE_ARRAY;
			m_a.e = nullptr;
			m_a.size = 0;
			return PARSE_OK;
		}

		size_t size = 0;
		Value e;
		ParseResult ret;
		for (;;) {
			if ((ret = e.parseValue()) != PARSE_OK) {
				break;
			}
			memcpy(contextPush(sizeof(Value)), &e, sizeof(Value));
			switch (e.m_type) {
			case VALUE_TYPE_ARRAY:
				e.m_a.e = nullptr;
				break;
			case VALUE_TYPE_STRING:
				e.m_s.s = nullptr;
				break;
			case VALUE_TYPE_OBJECT:
				e.m_o.m = nullptr;
				break;
			}
			e.m_type = VALUE_TYPE_NULL;
			++size;
			parseWhitespace();
			if (*s_c.json == ',') {
				++s_c.json;
				parseWhitespace();
			} else if (*s_c.json == ']') {
				++s_c.json;
				freeMem();
				m_type = VALUE_TYPE_ARRAY;
				m_a.size = size;
				size *= sizeof(Value);
				memcpy(m_a.e = (Value *)malloc(size), contextPop(size), size);
				return PARSE_OK;
			} else {				
				ret = PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
				break;
			}
		}

		for (size_t i = 0; i < size; ++i) {
			((Value *)contextPop(sizeof(Value)))->freeMem();
		}
		return ret;
	}

	ParseResult Value::parseObject()
	{
		++s_c.json;
		parseWhitespace();
		if (*s_c.json == '}') {
			++s_c.json;
			freeMem();
			m_type = VALUE_TYPE_OBJECT;
			m_o.m = nullptr;
			m_o.size = 0;
			return PARSE_OK;
		}

		size_t size = 0;
		Member m;
		ParseResult ret;
		for (;;) {
			char *k;
			size_t klen;
			if ((ret = parseStringRaw(k, klen)) != PARSE_OK) {
				ret = PARSE_MISS_KEY;
				break;
			}
			m.k = (char *)malloc(sizeof(char) * (klen + 1));
			memcpy(m.k, k, klen);
			m.k[klen] = '\0';
			m.klen = klen;
			parseWhitespace();
			if (*s_c.json != ':') {
				ret = PARSE_MISS_COLON;
				free(m.k);
				break;
			}
			++s_c.json;
			parseWhitespace();

			if ((ret = m.v.parseValue()) != PARSE_OK) {
				free(m.k);
				break;
			}
			memcpy(contextPush(sizeof(Member)), &m, sizeof(Member));
			switch (m.v.m_type) {
			case VALUE_TYPE_ARRAY:
				m.v.m_a.e = nullptr;
				break;
			case VALUE_TYPE_STRING:
				m.v.m_s.s = nullptr;
				break;
			case VALUE_TYPE_OBJECT:
				m.v.m_o.m = nullptr;
				break;
			}
			m.v.m_type = VALUE_TYPE_NULL;
			++size;
			parseWhitespace();
			if (*s_c.json == ',') {
				++s_c.json;
				parseWhitespace();
			} else if (*s_c.json == '}') {
				++s_c.json;
				freeMem();
				m_type = VALUE_TYPE_OBJECT;
				m_o.size = size;
				size *= sizeof(Member);
				memcpy(m_o.m = (Member *)malloc(size), contextPop(size), size);
				return PARSE_OK;
			} else {
				ret = PARSE_MISS_COMMA_OR_CURLY_BRACKET;
				break;
			}
		}

		for (size_t i = 0; i < size; ++i) {
			auto p = (Member *)contextPop(sizeof(Member));
			free(p->k);
			p->v.freeMem();
		}
		return ret;
	}

	StringifyResult Value::stringifyValue()
	{
		switch (m_type) {
		case VALUE_TYPE_NULL:PUTS("null", 4); break;
		case VALUE_TYPE_FALSE:PUTS("false", 5); break;
		case VALUE_TYPE_TRUE:PUTS("true", 4); break;
		case VALUE_TYPE_NUMBER: 
			s_c.top -= 32 - sprintf(static_cast<char *>(contextPush(32)), "%.17g", m_n);
			break;
		case VALUE_TYPE_ARRAY:
			PUTC('[');
			for (size_t i = 0; i < m_a.size; i++) {
				if (i > 0) PUTC(',');
				StringifyResult ret = m_a.e[i].stringifyValue();
				if (ret != STRINGIFY_OK)return ret;
			}
			PUTC(']');
			break;
		case VALUE_TYPE_STRING: stringifyString(m_s.s, m_s.len); break;
		case VALUE_TYPE_OBJECT:
			PUTC('{');
			for (size_t i = 0; i < m_o.size; i++) {
				if (i > 0) PUTC(',');
				stringifyString(m_o.m[i].k, m_o.m[i].klen);
				PUTC(':');
				m_o.m[i].v.stringifyValue();
			}			
			PUTC('}');
			break;
		}
		return STRINGIFY_OK;
	}

	StringifyResult Value::stringifyString(const char *s, size_t len)
	{
		assert(s != nullptr);
		assert(len > 0);
		PUTC('"');
		const char *p = s;
		for (size_t i = 0; i < len; i++) {
			char ch = *p++;
			if (ch < 0x20) {
				stringHex4(ch);
			} else if (ch & 0x80) {
				unsigned j = 1, u;
				while (ch & (0x1 << (6 - j)))++j;
				u = ch & ~(0xff << (6 - j));
				i += 3;
				while (j--) {
					u <<= 6;
					u |= (*p++) & 0x3f;
				}
				if (u >= 0x10000) {
					stringHex4(((u - 0x10000) >> 10) - 0xd800);	// high
					u = (u & 0xffff) - 0xdc00;	// low
				}
				stringHex4(u);
			} else {
				switch (ch) {
				case '\"': PUTS("\\\"", 2); break;
				case '\\': PUTS("\\\\", 2); break;
				case '\b': PUTS("\\b", 2); break;
				case '\f': PUTS("\\f", 2); break;
				case '\r': PUTS("\\r", 2); break;
				case '\t': PUTS("\\t", 2); break;
				case '\n': PUTS("\\n", 2); break;
				default: PUTC(ch); break;
				}
			}		
		}
		PUTC('"');
		return STRINGIFY_OK;
	}

	void Value::stringHex4(unsigned u)
	{
		char ustr[6] = { '\\','u' };
		ustr[2] = s_table[(u >> 12) & 0xf];
		ustr[3] = s_table[(u >> 8) & 0xf];
		ustr[4] = s_table[(u >> 4) & 0xf];
		ustr[5] = s_table[u & 0xf];
		PUTS(ustr, 6);
	}

	void Value::freeMem()
	{
		switch (m_type) {
		case VALUE_TYPE_STRING:free(m_s.s); break;
		case VALUE_TYPE_ARRAY:
			for (size_t i = 0; i < m_a.size; ++i)
				(m_a.e + i)->freeMem();
			free(m_a.e);
			break;
		case VALUE_TYPE_OBJECT:
			for (size_t i = 0; i < m_o.size; ++i) {
				(m_o.m + i)->v.freeMem();
				free((m_o.m + i)->k);
			}
			free(m_o.m);
			break;
		default:
			break;
		}

		m_type = VALUE_TYPE_NULL;
	}

	void* Value::contextPush(size_t size)
	{
		assert(size > 0);
		if (s_c.top + size >= s_c.size) {
			if (s_c.size == 0)
				s_c.size = AJ_PARSE_STACK_INIT_SIZE;
			while (s_c.top + size > s_c.size)
				s_c.size += s_c.size >> 1;
			s_c.stack = (char *)realloc(s_c.stack, s_c.size);
		}
		void* res = s_c.stack + s_c.top;
		s_c.top += size;
		return res;
	}

	void* Value::contextPop(size_t size)
	{
		assert(s_c.top >= size);
		return s_c.stack + (s_c.top -= size);
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
