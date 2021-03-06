#ifndef MYSQL_VALUE_H
#define MYSQL_VALUE_H

#include <stdint.h>
#include <time.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <limits>
#include <assert.h>

#include "jsoncpp/json/value.h"

#include "logevent.hpp"
#include "columndesc.hpp"

namespace mysql {

class CValue //: public IItem
{
public:
	typedef uint16_t TYear;
	typedef struct
	{
		TYear y;
		uint8_t m;
		uint8_t d;
	} TDate;

	typedef struct
	{
		uint8_t h;
		uint8_t m;
		uint8_t s;
	} TTime;
	
	
	typedef struct
	{
		TDate date;
		TTime time;
	} TDateTime;
	
	enum EColumnType
	{
		MYSQL_TYPE_DECIMAL, 
		MYSQL_TYPE_TINY,
		MYSQL_TYPE_SHORT,  
		MYSQL_TYPE_LONG = 3,
		MYSQL_TYPE_FLOAT,  
		MYSQL_TYPE_DOUBLE,
		MYSQL_TYPE_NULL,
		MYSQL_TYPE_TIMESTAMP = 7,
		MYSQL_TYPE_LONGLONG,
		MYSQL_TYPE_INT24,
		MYSQL_TYPE_DATE = 10,
		MYSQL_TYPE_TIME = 11,
		MYSQL_TYPE_DATETIME = 12, 
		MYSQL_TYPE_YEAR = 13,
		MYSQL_TYPE_NEWDATE = 14,
		MYSQL_TYPE_VARCHAR = 15,
		MYSQL_TYPE_BIT,
		MYSQL_TYPE_TIMESTAMP2,
		MYSQL_TYPE_DATETIME2,
		MYSQL_TYPE_TIME2,
        MYSQL_TYPE_JSON = 245,
		MYSQL_TYPE_NEWDECIMAL = 246,
		MYSQL_TYPE_ENUM = 247,
		MYSQL_TYPE_SET = 248,
		MYSQL_TYPE_TINY_BLOB = 249,
		MYSQL_TYPE_MEDIUM_BLOB = 250,
		MYSQL_TYPE_LONG_BLOB = 251,
		MYSQL_TYPE_BLOB = 252,
		MYSQL_TYPE_VAR_STRING = 253,
		MYSQL_TYPE_STRING = 254,
		MYSQL_TYPE_GEOMETRY = 255
	};

    enum EJSONBType
    {
        JSONB_TYPE_SMALL_OBJECT = 0x0,
        JSONB_TYPE_LARGE_OBJECT = 0x1,
        JSONB_TYPE_SMALL_ARRAY = 0x2,
        JSONB_TYPE_LARGE_ARRAY = 0x3,
        JSONB_TYPE_LITERAL = 0x4,
        JSONB_TYPE_INT16 = 0x5,
        JSONB_TYPE_UINT16 = 0x6,
        JSONB_TYPE_INT32 = 0x7,
        JSONB_TYPE_UINT32 = 0x8,
        JSONB_TYPE_INT64 = 0x9,
        JSONB_TYPE_UINT64 = 0xA,
        JSONB_TYPE_DOUBLE = 0xB,
        JSONB_TYPE_STRING = 0xC,
        JSONB_TYPE_OPAQUE = 0xF
    };
	
    enum EJSONBLiteralType
    {
        JSONB_LITERAL_NULL = 0x0,
        JSONB_LITERAL_TRUE = 0x1,
        JSONB_LITERAL_FALSE = 0x2
    };

	typedef std::vector<std::string> TSet;
	
	static int calc_metadata_size(CValue::EColumnType ftype);
	static int calc_field_size(CValue::EColumnType ftype, const uint8_t* pfield, uint32_t metadata);

public:
	CValue();
	CValue(const CValue& val);
	virtual ~CValue() throw();

	CValue& operator=(const CValue& val);
	bool operator==(const CValue& val) const;
	bool operator!=(const CValue& val) const;
	
	bool is_valid() const;
	bool is_null() const;
	void reset();
	int pos() const {return _pos;}
	void pos(int p) {_pos = p;}

	int tune(CValue::EColumnType ftype, const uint8_t* pfield, uint32_t metadata, size_t length);

	// numeric
	template<class T> T as_int() const 
	{
		if (is_null() || !is_valid() || sizeof(T)>8) return (T)0;
		
		switch (_size)
		{
		case 1: return (T)*_storage;
		case 2: return (T)uint2korr(_storage);
		case 3: return (T)uint3korr(_storage);
		case 4: return (T)uint4korr(_storage);
		case 8: return (T)uint8korr(_storage);
		}
		return (T)0;
	}
	int8_t as_int8() const { return as_int<int8_t>(); }
	int16_t as_int16() const { return as_int<int16_t>(); }
	int32_t as_int32() const { return as_int<int32_t>(); }
	int64_t as_int64() const { return as_int<int64_t>(); }
	uint8_t as_uint8() const { return as_int<uint8_t>(); }
	uint16_t as_uint16() const { return as_int<uint16_t>(); }
	uint32_t as_uint32() const { return as_int<uint32_t>(); }
	uint64_t as_uint64() const { return as_int<uint64_t>(); }
	
	
	double as_double() const;
	float as_float() const { return (float)as_double(); }
	double as_real() const { return as_double(); }

	uint64_t as_bit() const;
	bool as_boolean() const;
	
	// date - time
	
	time_t as_timestamp() const;
    TDateTime as_datetime() const;
	TDate as_date() const;
	TTime as_time() const;
	TYear as_year() const;
	
	// etc
	std::string as_string() const;
	const char* as_string(size_t* length) const;
	void as_string(std::string& dst) const;
	
	const uint8_t* as_blob(size_t* length) const;
	
	uint64_t as_enum() const;
	void as_enum(const mysql::CColumnDesc& desc, std::string& result) const;
	std::string as_enum(const mysql::CColumnDesc& desc) const;
	
	uint64_t as_set() const;
	void as_set(const mysql::CColumnDesc& desc, CValue::TSet& result) const;
	std::string as_set(const mysql::CColumnDesc& desc) const;

#ifdef MYSQLSLAVE_JSON
    Json::Value as_json() const;

private:
    Json::Value _read_jsonb_inline_or_offset(uint8_t &type, uint32_t &offset,
                                             bool &is_offset, const uint8_t **src,
                                             bool large) const;
    Json::Value _read_jsonb_inlined(uint8_t type, const uint8_t **src) const;
    Json::Value _read_jsonb_string(const uint8_t **src) const;
    Json::Value _read_jsonb_array(const uint8_t **src, bool large) const;
    Json::Value _read_jsonb_object(const uint8_t **src, bool large) const;
    Json::Value _read_jsonb_type(uint8_t type, const uint8_t **src) const;
#endif

private:
    uint64_t _read_bit_slice(uint64_t binary, uint8_t start, uint8_t size, uint8_t data_length) const;
    TDateTime _as_datetime() const;
    TDateTime _as_datetime2() const;

protected:
	friend std::ostream& operator<<(std::ostream&, const CValue&);
	friend std::ostream& operator<<(std::ostream&, const CValue::TDate&);
	friend std::ostream& operator<<(std::ostream&, const CValue::TTime&);
	friend std::ostream& operator<<(std::ostream&, const CValue::TDateTime&);
	
public:
	EColumnType _type;

protected:
	const uint8_t* _storage;
	size_t _size;
	uint32_t _metadata;
	bool _is_null;
	int _pos;
};

}

#endif

