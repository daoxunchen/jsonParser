#ifndef AJson_H
#define AJson_H

#define AJ_MEMORY_LEAK_DETECT
#ifdef AJ_MEMORY_LEAK_DETECT
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif // AJ_MEMORY_LEAK_DETECT

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
		PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
		PARSE_MISS_KEY,
		PARSE_MISS_COLON,
		PARSE_MISS_COMMA_OR_CURLY_BRACKET
	};

	struct Context {
		const char *json = nullptr;
		char* stack = nullptr;
		size_t size, top;
	};

	struct Member;

	class Value {
	public:
		~Value() { freeMem(); }
		ParseResult parse(const char *);

		ValueType  type() const { return m_type; }
		void setNull() { freeMem(); }
		void setBool(bool b)
		{
			freeMem(); m_type = b ? VALUE_TYPE_TRUE : VALUE_TYPE_FALSE;
		}
		bool getBool() const
		{
			assert(m_type == VALUE_TYPE_TRUE || m_type == VALUE_TYPE_FALSE);
			return m_type == VALUE_TYPE_TRUE;
		}
		void setNumber(double n)
		{
			freeMem(); m_n = n; m_type = VALUE_TYPE_NUMBER;
		}
		double getNumber() const
		{
			assert(m_type == VALUE_TYPE_NUMBER); return m_n;
		}
		void setString(const char *, size_t);
		const char* getString() const
		{
			assert(m_type == VALUE_TYPE_STRING); return m_s.s;
		}
		size_t getStringLength() const
		{
			assert(m_type == VALUE_TYPE_STRING); return m_s.len;
		}
		size_t getArraySize() const
		{
			assert(m_type == VALUE_TYPE_ARRAY); return m_a.size;
		}
		Value* getArrayElement(size_t index)
		{
			assert(m_type == VALUE_TYPE_ARRAY && index < m_a.size);
			return m_a.e + index;
		}
		size_t getObjectSize() const
		{
			assert(m_type == VALUE_TYPE_OBJECT); return m_o.size;
		}
		const char* getObjectKey(size_t) const;
		size_t getObjectKeyLength(size_t) const;
		Value* getObjectValue(size_t) const;
	private:
		ValueType m_type = VALUE_TYPE_NULL;
		union {
			double m_n;
			struct { char *s; size_t len; } m_s;
			struct { Value *e; size_t size; } m_a;
			struct { Member *m; size_t size; } m_o;
		};

		ParseResult parseValue();
		void parseWhitespace();
		ParseResult parseLiteral(const char*, ValueType);
		ParseResult parseNumber();
		ParseResult parseStringRaw(char *&, size_t &);
		ParseResult parseString();
		ParseResult parseArray();
		ParseResult parseObject();
		void freeMem();

		bool parseHex4(const char*&, unsigned&);
		void encode_utf8(unsigned u);

		static Context m_c;
		static void* contextPush(size_t);
		static void* contextPop(size_t);
	};

	struct Member
	{
		char *k; size_t klen;
		Value v;
	};
}

#endif /* AJson_H */
