#ifndef AJson_H
#define AJson_H

#include <cassert>
#include <cstring>

#ifndef AJ_PARSE_STACK_INIT_SIZE
#define AJ_PARSE_STACK_INIT_SIZE 256
#endif

namespace AJson {
	enum ValueType {
		VALUE_TYPE_NULL,
		VALUE_TYPE_FALSE,
		VALUE_TYPE_TRUE,
		VALUE_TYPE_NUMBER,
		VALUE_TYPE_STRING,
		VALUE_TYPE_ARRAY,
		VALUE_TYPE_OBJECT
	};

	enum ParseResult {
		PARSE_OK,
		PARSE_EXPECT_VALUE,
		PARSE_INVALID_VALUE,
		PARSE_ROOT_NOT_SINGULAR,
		PARSE_NUMBER_TOO_BIG,
		PARSE_MISS_QUOTATION_MARK,
		PARSE_INVALID_STRING_ESCAPE,
		PARSE_INVALID_STRING_CHAR,
		PARSE_INVALID_UNICODE_HEX,
		PARSE_INVALID_UNICODE_SURROGATE,
		PARSE_MISS_COMMA_OR_SQUARE_BRACKET
	};

	class Value {
	public:
		ParseResult parse(const char *);

		ValueType  type() const { return m_type; }
		void setNull() { freeStr(); }
		void setBool(bool b) 
		{ freeStr(); m_type = b ? VALUE_TYPE_TRUE : VALUE_TYPE_FALSE; }
		bool getBool() const
		{
			assert(m_type == VALUE_TYPE_TRUE || m_type == VALUE_TYPE_FALSE);
			return m_type == VALUE_TYPE_TRUE;
		}
		void setNumber(double n) 
		{ freeStr(); m_n = n; m_type = VALUE_TYPE_NUMBER; }
		double getNumber() const
		{ assert(m_type == VALUE_TYPE_NUMBER); return m_n; }
		void setString(const char *, size_t);
		const char* getString() const
		{ assert(m_type == VALUE_TYPE_STRING); return m_s.s; }
		size_t getStringLength() const
		{ assert(m_type == VALUE_TYPE_STRING); return m_s.len; }

	private:
		ValueType m_type = VALUE_TYPE_NULL;
		union {
			double m_n;
			struct { char *s; size_t len; } m_s;
		};

		struct Context {
			const char *json = nullptr;
			char* stack = nullptr;
			size_t size, top;
		} m_c;

		ParseResult parseValue();
		void parseWhitespace();
		ParseResult parseLiteral(const char*, ValueType);
		ParseResult parseNumber();
		ParseResult parseString();
		ParseResult parseArray();
		void freeStr();
		void* contextPush(size_t);
		void* contextPop(size_t);
		bool parseHex4(const char*& p, unsigned& u);
		void encode_utf8(unsigned u);
	};
}

#endif /* AJson_H */
