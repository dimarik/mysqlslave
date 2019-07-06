#include <mysqlslave/value.hpp>
#include <jsoncpp/json/writer.h>

extern "C" {
#include <mysql/decimal.h>
}

namespace mysql
{

/* 
 * ========================================= CValue
 * ========================================================
 */	
int CValue::calc_metadata_size(CValue::EColumnType ftype)
{
	int rc;
	switch( ftype ) 
	{
	case MYSQL_TYPE_TIMESTAMP:
	case MYSQL_TYPE_TIMESTAMP2:
	case MYSQL_TYPE_FLOAT:
	case MYSQL_TYPE_DOUBLE:
	case MYSQL_TYPE_BLOB:
	case MYSQL_TYPE_GEOMETRY:
		rc = 1;
		break;
	case MYSQL_TYPE_VARCHAR:
	case MYSQL_TYPE_BIT:
	case MYSQL_TYPE_NEWDECIMAL:
	case MYSQL_TYPE_VAR_STRING:
	case MYSQL_TYPE_STRING:
		rc = 2;
		break;
	default:
		rc = 0;
	}
	return rc;
}
	
int CValue::calc_field_size(CValue::EColumnType ftype, const uint8_t *pfield, uint32_t metadata)
{
	uint32_t length;

	switch (ftype) {
	case MYSQL_TYPE_VAR_STRING:
	/* This type is hijacked for result set types. */
		length= metadata;
		break;
	case MYSQL_TYPE_NEWDECIMAL:
	{
		static const int dig2bytes[10] = {0, 1, 1, 2, 2, 3, 3, 4, 4, 4};
		int precision = (int)(metadata & 0xff);
		int scale = (int)(metadata >> 8);
		int intg = precision - scale;
		int intg0 = intg / 9;
		int frac0 = scale / 9;
		int intg0x = intg - intg0*9;
		int frac0x = scale - frac0*9;
		length = intg0 * sizeof(int32_t) + dig2bytes[intg0x] + frac0 * sizeof(int32_t) + dig2bytes[frac0x];
		break;
	}
	case MYSQL_TYPE_DECIMAL:
	case MYSQL_TYPE_FLOAT:
	case MYSQL_TYPE_DOUBLE:
		length = metadata;
		break;
	/*
	The cases for SET and ENUM are include for completeness, however
	both are mapped to type MYSQL_TYPE_STRING and their real types
	are encoded in the field metadata.
	*/
	case MYSQL_TYPE_SET:
	case MYSQL_TYPE_ENUM:
	case MYSQL_TYPE_STRING:
	{
		EColumnType type = (EColumnType)(metadata & 0xFF);
		int len = metadata >> 8;
		if ((type == MYSQL_TYPE_SET) || (type == MYSQL_TYPE_ENUM))
		{
			length = len;
		}
		else
		{
			length = len < 256 ? (unsigned int) *pfield + 1 : uint2korr(pfield);
		}
		break;
	}
	case MYSQL_TYPE_YEAR:
	case MYSQL_TYPE_TINY:
		length = 1;
		break;
	case MYSQL_TYPE_SHORT:
		length = 2;
		break;
	case MYSQL_TYPE_INT24:
		length = 3;
		break;
	case MYSQL_TYPE_LONG:
		length = 4;
		break;
	case MYSQL_TYPE_LONGLONG:
	    length= 8;
	    break;
	case MYSQL_TYPE_NULL:
		length = 0;
		break;
	case MYSQL_TYPE_NEWDATE:
	case MYSQL_TYPE_DATE:
	case MYSQL_TYPE_TIME:
	case MYSQL_TYPE_TIME2:
		length = 3;
		break;
	case MYSQL_TYPE_TIMESTAMP:
		length = 4;
		break;
	case MYSQL_TYPE_TIMESTAMP2:
		length = 4;
		break;
	case MYSQL_TYPE_DATETIME:
	case MYSQL_TYPE_DATETIME2:
		abort();
		length = 8;
		break;
	case MYSQL_TYPE_BIT:
	{
		/*
		Decode the size of the bit field from the master.
		from_len is the length in bytes from the master
		from_bit_len is the number of extra bits stored in the master record
		If from_bit_len is not 0, add 1 to the length to account for accurate
		number of bytes needed.
		*/
		uint32_t from_len = (metadata >> 8U) & 0x00ff;
		uint32_t from_bit_len = metadata & 0x00ff;
		length = from_len + ((from_bit_len > 0) ? 1 : 0);
		break;
	}
	case MYSQL_TYPE_VARCHAR:
	{
		length = metadata > 255 ? 2 : 1;
		length += length == 1 ? (uint32_t)*pfield : *((uint16_t *)pfield);
		break;
	}
	case MYSQL_TYPE_TINY_BLOB:
	case MYSQL_TYPE_MEDIUM_BLOB:
	case MYSQL_TYPE_LONG_BLOB:
	case MYSQL_TYPE_BLOB:
	case MYSQL_TYPE_GEOMETRY:
	{
		switch (metadata)
		{
			case 1:
				length = 1 + (uint32_t) pfield[0];
				break;
			case 2:
				length = 2 + (uint32_t) (*(uint16_t*)(pfield) & 0xFFFF);
				break;
			case 3:
				length = 3 + (uint32_t) (long) (*((uint32_t *) (pfield)) & 0xFFFFFF);
				break;
			case 4:
				length = 4 + (uint32_t) (long) *((uint32_t *) (pfield));
				break;
			default:
				length= 0;
				break;
		}
		break;
	}
	default:
		length = ~(uint32_t) 0;
	}
	
	return length;
}
	
	
CValue::CValue()
	: _type(MYSQL_TYPE_NULL)
	, _storage(NULL)
	, _size(0)
	, _metadata(0)
	, _is_null(true)
	, _pos(0)
{
}

CValue::CValue(const CValue& val)
{
	*this = val;
}

CValue::~CValue() throw()
{
}

CValue& CValue::operator=(const CValue &val)
{
	this->_type = val._type;
	this->_storage = val._storage;
	this->_size = val._size;
	this->_metadata = val._metadata;
	this->_is_null = val._is_null;
	this->_pos = val._pos;
	return *this;
}

bool CValue::operator==(const CValue &val) const
{
	return 
	(_type == val._type) &&
	(_storage == val._storage) &&
	(_size == val._size) &&
	(_metadata == val._metadata) &&
	(_is_null == val._is_null)
	;
			
}

bool CValue::operator!=(const CValue &val) const
{
	return !operator==(val);
}
	
void CValue::reset() 
{
	_type = MYSQL_TYPE_NULL;
	_size = 0;
	_storage = NULL;
	_metadata = 0;
	_is_null = true;
}

bool CValue::is_valid() const 
{
	return _size && _storage;
}

bool CValue::is_null() const 
{ 
	return _is_null || _type == MYSQL_TYPE_NULL; 
}


int CValue::tune(CValue::EColumnType ftype, const uint8_t* pfield, uint32_t metadata, size_t length)
{
	_metadata = metadata;
	_storage = pfield;
	_type = ftype;
	_size = length;
	_is_null = false;
	
	switch (ftype)
	{
		case MYSQL_TYPE_STRING:
		{
			EColumnType subtype = (EColumnType)(_metadata & 0xFF);
			int len = _metadata >> 8;
			switch (subtype)
			{
				case MYSQL_TYPE_STRING:
				{
					if (len > 255)
					{
						_storage += 2;
						_size = length - 2;
					}
					else
					{
						_storage = pfield + 1;
						_size = length - 1;
					}
					break;
				}
                case MYSQL_TYPE_JSON:
				case MYSQL_TYPE_ENUM:
				case MYSQL_TYPE_SET:
				{
					_type = subtype;
					_size = len;
					break;
				}
				default:
				{
					_is_null = true; // invalid case
				}
			}
			
			_type = subtype;
			break;
		}
		case MYSQL_TYPE_VARCHAR:
		{
			if (_metadata < 256)
			{
				_size = *_storage;
				_storage++;
			}
			else
			{
				_size = uint2korr(_storage);
				_storage += 2;
			}
			break;
		}
		case MYSQL_TYPE_BLOB:
		{
			switch (_metadata)
			{
				case 1: _size = *_storage; break;
				case 2: _size = uint2korr(_storage); break;
				case 3: _size = uint3korr(_storage); break;
				case 4: _size = uint4korr(_storage); break;
				default: 
				{
					_size = 0; 
					_is_null = true;
				}
			}
			
			if (!_is_null)
			{
				_storage += _metadata;
			}
			break;
		}
		default:
		{
			;
		}
	}
	
	return 0;
}

#ifndef likely
#define likely(x)	__builtin_expect((x),1)
#endif
#ifndef unlikely
#define unlikely(x)	__builtin_expect((x),0)
#endif

#define E_DEC_OK                0
#define E_DEC_TRUNCATED         1
#define E_DEC_OVERFLOW          2
#define E_DEC_DIV_ZERO          4
#define E_DEC_BAD_NUM           8
#define E_DEC_OOM              16

#define E_DEC_ERROR            31
#define E_DEC_FATAL_ERROR      30

#define FIX_INTG_FRAC_ERROR(len, intg1, frac1, error)                   \
        do                                                              \
        {                                                               \
          if (unlikely(intg1+frac1 > (len)))                            \
          {                                                             \
            if (unlikely(intg1 > (len)))                                \
            {                                                           \
              intg1=(len);                                              \
              frac1=0;                                                  \
              error=E_DEC_OVERFLOW;                                     \
            }                                                           \
            else                                                        \
            {                                                           \
              frac1=(len)-intg1;                                        \
              error=E_DEC_TRUNCATED;                                    \
            }                                                           \
          }                                                             \
          else                                                          \
            error=E_DEC_OK;                                             \
        } while(0)



#define mi_sint1korr(A) ((int8)(*A))
#define mi_uint1korr(A) ((uint8)(*A))

#define mi_sint2korr(A) ((int16) (((int16) (((uchar*) (A))[1])) +\
                                  ((int16) ((int16) ((char*) (A))[0]) << 8)))
#define mi_sint3korr(A) ((int32) (((((uchar*) (A))[0]) & 128) ? \
                                  (((uint32) 255L << 24) | \
                                   (((uint32) ((uchar*) (A))[0]) << 16) |\
                                   (((uint32) ((uchar*) (A))[1]) << 8) | \
                                   ((uint32) ((uchar*) (A))[2])) : \
                                  (((uint32) ((uchar*) (A))[0]) << 16) |\
                                  (((uint32) ((uchar*) (A))[1]) << 8) | \
                                  ((uint32) ((uchar*) (A))[2])))
#define mi_sint4korr(A) ((int32) (((int32) (((uchar*) (A))[3])) +\
                                  ((int32) (((uchar*) (A))[2]) << 8) +\
                                  ((int32) (((uchar*) (A))[1]) << 16) +\
                                  ((int32) ((int16) ((char*) (A))[0]) << 24)))


typedef int32_t decimal_digit_t;
typedef decimal_digit_t dec1;
typedef longlong dec2;

#define DIG_PER_DEC1 9
#define DIG_MASK     100000000
#define DIG_BASE     1000000000
#define DIG_MAX      (DIG_BASE-1)
#define DIG_BASE2    ((dec2)DIG_BASE * (dec2)DIG_BASE)

static int _init_decimal(decimal_t& decimal, dec1* to_buf, size_t to_buf_len, int precision, int scale, const uint8_t* storage)
{
    const uint8_t *from = storage;
    int error = E_DEC_OK, intg=precision-scale,
        intg0=intg/DIG_PER_DEC1, frac0=scale/DIG_PER_DEC1,
        intg0x=intg-intg0*DIG_PER_DEC1, frac0x=scale-frac0*DIG_PER_DEC1,
        intg1=intg0+(intg0x>0), frac1=frac0+(frac0x>0),
        mask=(*from & 0x80) ? 0 : -1,
        to_sign, to_intg, to_frac;

    static const int dig2bytes[DIG_PER_DEC1+1]={0, 1, 1, 2, 2, 3, 3, 4, 4, 4};
    static const dec1 powers10[DIG_PER_DEC1+1]={ 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

    dec1* buf = to_buf;
    FIX_INTG_FRAC_ERROR(512, intg1, frac1, error);
    if (unlikely(error))
    {
        if (intg1 < intg0+(intg0x>0))
        {
            from+=dig2bytes[intg0x]+sizeof(dec1)*(intg0-intg1);
            frac0=frac0x=intg0x=0;
            intg0=intg1;
        }
        else
        {
            frac0x=0;
            frac0=frac1;
        }
    }

    to_sign=(mask != 0);
    to_intg=intg0*DIG_PER_DEC1+intg0x;
    to_frac=frac0*DIG_PER_DEC1+frac0x;

    if (0 != intg0x)
    {
        dec1 x;
        int i=dig2bytes[intg0x];

        if (storage == from)
        {
            uint8_t tmp[4];
            memcpy(tmp, from, 4);
            tmp[0] ^= 0x80;
            switch (i)
            {
                case 1: x=mi_sint1korr(tmp); break;
                case 2: x=mi_sint2korr(tmp); break;
                case 3: x=mi_sint3korr(tmp); break;
                case 4: x=mi_sint4korr(tmp); break;
                default: {assert(0); return E_DEC_ERROR;};
            }
        }
        else
        {
            switch (i)
            {
                case 1: x=mi_sint1korr(from); break;
                case 2: x=mi_sint2korr(from); break;
                case 3: x=mi_sint3korr(from); break;
                case 4: x=mi_sint4korr(from); break;
                default: {assert(0); return E_DEC_ERROR;};
            }
        }

        from+=i;
        *buf=x ^ mask;
        if (((ulonglong)*buf) >= (ulonglong) powers10[intg0x+1])
        {
            assert(0);
            return E_DEC_ERROR;
        }

        if (buf > to_buf || *buf != 0)
            buf++;
        else
            to_intg-=intg0x;
    }

    for (const uint8_t* stop=from+intg0*sizeof(dec1); from < stop; from+=sizeof(dec1))
    {
        if (storage == from)
        {
            uint8_t tmp[4];
            memcpy(tmp, from, 4);
            tmp[0] ^= 0x80;
            *buf=mi_sint4korr(tmp) ^ mask;
        }
        else
            *buf=mi_sint4korr(from) ^ mask;

        if (((uint32)*buf) > DIG_MAX)
        {
            assert(0);
            return E_DEC_ERROR;
        }

        if (buf > to_buf || *buf != 0)
            buf++;
        else
            to_intg-=DIG_PER_DEC1;
    }

    assert(to_intg >= 0);

    for (const uint8_t* stop=from+frac0*sizeof(dec1); from < stop; from+=sizeof(dec1))
    {
        if (storage == from)
        {
            uint8_t tmp[4];
            memcpy(tmp, from, 4);
            tmp[0] ^= 0x80;
            *buf=mi_sint4korr(tmp) ^ mask;
        }
        else
            *buf=mi_sint4korr(from) ^ mask;

        if (((uint32)*buf) > DIG_MAX)
        {
            assert(0);
            return E_DEC_ERROR;

        }
        buf++;
    }

    if (0 != frac0x)
    {
        int i=dig2bytes[frac0x];
        dec1 x;
        if (storage == from)
        {
            uint8_t tmp[4];
            memcpy(tmp, from, 4);
            tmp[0] ^= 0x80;
            switch (i)
            {
                case 1: x=mi_sint1korr(tmp); break;
                case 2: x=mi_sint2korr(tmp); break;
                case 3: x=mi_sint3korr(tmp); break;
                case 4: x=mi_sint4korr(tmp); break;
                default: {assert(0); return E_DEC_ERROR;};
            }
        }
        else
        {
            switch (i)
            {
                case 1: x=mi_sint1korr(from); break;
                case 2: x=mi_sint2korr(from); break;
                case 3: x=mi_sint3korr(from); break;
                case 4: x=mi_sint4korr(from); break;
                default: {assert(0); return E_DEC_ERROR;};
            }
        }

        *buf=(x ^ mask) * powers10[DIG_PER_DEC1 - frac0x];
        if (((uint32)*buf) > DIG_MAX)
        {
            assert(0);
            return E_DEC_ERROR;
        }

        buf++;
    }

    decimal.sign = to_sign;
    decimal.intg = to_intg;
    decimal.frac = to_frac;
    decimal.buf = to_buf;
    decimal.len = to_buf_len;

    return E_DEC_OK;
}

double CValue::as_double() const
{
	double db = 0;

	if (is_null() || !is_valid()) return db;

	if (_type == MYSQL_TYPE_NEWDECIMAL)
	{
		// another one piece of shit of mysql sources. TODO: rewrite
		int precision = (int)(_metadata & 0xff);
		int scale = (int)(_metadata >> 8);
		const uint8_t* from = _storage;
        dec1 to_buf[512];
        decimal_t decimal;
        int ret = _init_decimal(decimal, to_buf, sizeof(to_buf), precision, scale, from);
        if (ret != E_DEC_OK) return std::numeric_limits<double>::max();
        decimal2double(&decimal, &db);
	}
	else
	{
        float tmp;
		if (_size == 4)
		{
            float4get(&tmp, _storage);
            db = tmp;
		}
		else if (_size == 8)
		{
            float8get(&db, _storage);
		}
	}
	
	return db;
}

uint64_t CValue::as_bit() const
{
	return as_uint64();
}

bool CValue::as_boolean() const
{
	return as_uint8() != 0;
}

#define mi_uint4korr(A) ((uint32) (((uint32) (((const uchar*) (A))[3])) +\
                                   (((uint32) (((const uchar*) (A))[2])) << 8) +\
                                   (((uint32) (((const uchar*) (A))[1])) << 16) +\
                                   (((uint32) (((const uchar*) (A))[0])) << 24)))


time_t CValue::as_timestamp() const 
{
	return _is_null || !is_valid() || _size != 4 ? 0 : (time_t)mi_uint4korr(_storage);
}

CValue::TDate CValue::as_date() const
{
	TDate d;
	if (_is_null || !is_valid() || _size < 3)
	{
		memset(&d, 0x00, sizeof(TDate));
		return d;
	}
	
	uint32_t dd = uint3korr(_storage);
	d.y = dd / (16L * 32L);
	d.m = dd / 32L % 16L;
	d.d = dd % 32L;
	
	return d;
}

CValue::TTime CValue::as_time() const
{
	TTime t;
	if (is_null() || !is_valid() || _size < 3)
	{
		memset(&t, 0x00, sizeof(TTime));
		return t;
	}
	
	uint32_t tt = uint3korr(_storage);
	t.h = tt / 10000;
	t.m = (tt % 10000) / 100;
	t.s = tt % 100;
	
	return t;
}


CValue::TDateTime CValue::as_datetime() const
{
	TDateTime datetime;
	
	if (is_null() || !is_valid() || _size < 8)
	{
		memset(&datetime, 0x00, sizeof(CValue::TDateTime));
		return datetime;
	}
	
	uint64_t dt = uint8korr(_storage);
	size_t d = dt / 1000000;
	size_t t = dt % 1000000;

	datetime.date.y = d / 10000;
	datetime.date.m = (d % 10000) / 100;
	datetime.date.d = d % 100;
	datetime.time.h = t / 10000;
	datetime.time.m = (t % 10000) / 100;
	datetime.time.s = t % 100;
	
	return datetime;
}

CValue::TYear CValue::as_year() const 
{
	if (is_null() || !is_valid()) return 0;
	
	return 1900 + *_storage;
}




// strings and blobs

const char* CValue::as_string(size_t* length) const
{
	if (is_null() || !is_valid())
	{
		*length = 0;
		return NULL;
	}
	
	const char* rc;
	switch (_type)
	{
		case MYSQL_TYPE_STRING:
		case MYSQL_TYPE_VARCHAR:
		case MYSQL_TYPE_BLOB:
		{
			*length = _size;
			rc = (const char*)_storage;
			break;
		}
		default:
		{
			*length = 0;
			rc = NULL;
		}
	}
	
	return rc;
}
void CValue::as_string(std::string& dst) const
{
	size_t len;
	const char* cstr = as_string(&len);
	if (cstr && len > 0)
	{
		dst.assign(cstr, len);
	}
	else
	{
		dst.clear();
	}
}

std::string CValue::as_string() const
{
    if (_type == MYSQL_TYPE_NEWDECIMAL)
    {
        int precision = (int)(_metadata & 0xff);
        int scale = (int)(_metadata >> 8);
        const uint8_t* from = _storage;
        dec1 to_buf[512];
        decimal_t decimal;
        int ret = _init_decimal(decimal, to_buf, sizeof(to_buf), precision, scale, from);
        if (ret != E_DEC_OK) return "";
        int str_size = decimal_string_size(&decimal);
        char str_buf[str_size];
        decimal2string(&decimal, str_buf, &str_size, precision+2, scale, '0');
        return std::string(str_buf);
    }
    else
    {
        std::string rc;
        as_string(rc);
        return rc;
    }
}

const uint8_t* CValue::as_blob(size_t* length) const
{
	if (is_null() || !is_valid())
	{
		*length = 0;
		return NULL;
	}
	
	*length = _size;
	return _storage;
}

uint64_t CValue::as_enum() const
{
	if (is_null() || !is_valid() || (_type != MYSQL_TYPE_ENUM && _type != MYSQL_TYPE_SET)) return 0;
	
	
	switch (_size)
	{
		case 1: return *_storage; 
        case 2: return uint2korr(_storage);
		case 3: return uint3korr(_storage);
		case 4: return uint4korr(_storage);
	}
		
	return 0;
}

void CValue::as_enum(const mysql::CColumnDesc& desc, std::string& result) const
{
	uint64_t pos = as_enum() - 1;
	
	if (desc.get_variants().size() > pos)
	{
		result = desc.get_variants()[pos];
	}
	else
	{
		result.clear();
	}
}

std::string CValue::as_enum(const mysql::CColumnDesc& desc) const
{
	std::string s;
	as_enum(desc, s);
	return s;
}

uint64_t CValue::as_set() const
{
	return as_enum();
}

void CValue::as_set(const mysql::CColumnDesc& desc, CValue::TSet& result) const
{
	uint64_t mask = as_enum(), pos = 0;
	
	result.clear();
	while (mask)
	{
		if (mask & 0x01)
		{
			if (desc.get_variants().size() > pos)
				result.push_back(desc.get_variants()[pos]);
			else
				return;
		}
		++pos;
		mask >>= 0x01;
	}
}

std::string CValue::as_set(const mysql::CColumnDesc& desc) const
{
	std::string s;
	CValue::TSet tset;
	as_set(desc, tset);
	for (CValue::TSet::const_iterator it = tset.begin(); it != tset.end(); ++it) 
	{
		s += *it + " ";
	}
	
	if (!s.empty())
	{
		s.erase(s.size()-1, 1);
	}	
	
	return s;
}

void CValue::as_json(Json::Value& value) const
{
    const uint8_t* p = _storage;
    uint8_t type = *p++;
    _read_jsonb_type(value, type, p);
}

struct ValueOrOffset
{
    uint8_t type;
    bool is_offset;
    Json::Value json_value;
    uint32_t offset;
};

size_t CValue::_read_jsonb_inline_or_offset(uint8_t& type, Json::Value& value, uint32_t& offset, bool& is_offset,
                                            const uint8_t* src, bool large) const
{
    const uint8_t* p = src;
    type = *p++;
    if (type == JSONB_TYPE_LITERAL || type == JSONB_TYPE_INT16 || type == JSONB_TYPE_UINT16)
    {
        is_offset = false;
        p += _read_jsonb_inlined(value, type, p);
    }
    else if ((type == JSONB_TYPE_INT32 || type == JSONB_TYPE_UINT32) && large)
    {
        is_offset = false;
        p += _read_jsonb_inlined(value, type, p);
    }
    else
    {
        is_offset = true;
        if (large)
        {
            offset = uint4korr(p);
            p += 4;
        }
        else
        {
            offset = uint2korr(p);
            p += 2;
        }
    }
    return p - src;
}

size_t CValue::_read_jsonb_inlined(Json::Value& value, uint8_t type, const uint8_t* src) const
{
    switch (type)
    {
    case JSONB_TYPE_LITERAL:
    {
        uint16_t v = uint2korr(src);
        if (v == JSONB_LITERAL_NULL)
        {
            value = Json::Value(Json::nullValue);
        }
        else if (v == JSONB_LITERAL_FALSE)
        {
            value = false;
        }
        else if (v == JSONB_LITERAL_TRUE)
        {
            value = true;
        }
        return 2;
    }
    case JSONB_TYPE_INT16:
        value = Json::Int(sint2korr(src));
        return 2;
    case JSONB_TYPE_UINT16:
        value = Json::Int(uint2korr(src));
        return 2;
    case JSONB_TYPE_INT32:
        value = Json::Int64(sint4korr(src));
        return 4;
    case JSONB_TYPE_UINT32:
        value = Json::UInt64(uint4korr(src));
        return 4;
    default:
        return 0;
    }
}

size_t CValue::_read_jsonb_string(Json::Value& value, const uint8_t* src) const
{
    size_t length = 0;
    uint8_t bits_read = 0;
    const uint8_t* p = src;
    do
    {
        length = length | ((*p & 0x7f) << bits_read);
        bits_read += 7;
    }
    while(*p++ != 0x80);
    const char* cp = (const char*)p;
    value = Json::Value(cp, cp + length);
    return (p - src) + length;
}

size_t CValue::_read_jsonb_array(Json::Value& value, const uint8_t* src, bool large) const
{
    uint32_t nelems;
    //uint32_t size;
    const uint8_t* p = src;
    if (large)
    {
        nelems = uint4korr(p);
        p += 4;
        //size = uint4korr(p);
        p += 4;
    }
    else
    {
        nelems = uint2korr(p);
        p += 2;
        //size = uint2korr(p);
        p += 2;
    }
    value = Json::Value(Json::arrayValue);
    std::vector<ValueOrOffset> values_or_offsets(nelems);
    for (uint32_t i = 0; i < nelems; i++)
    {
        ValueOrOffset& v = values_or_offsets[i];
        p += _read_jsonb_inline_or_offset(v.type, v.json_value, v.offset, v.is_offset, p, large);
    }
    for (uint32_t i = 0; i < nelems; i++)
    {
        ValueOrOffset& v = values_or_offsets[i];
        Json::Value elem;
        if (v.is_offset)
        {
            p += _read_jsonb_type(elem, EJSONBType(v.type), p);
        }
        else
        {
            elem = v.json_value;
        }
        value.append(elem);
    }
    return p - src;
}

size_t CValue::_read_jsonb_object(Json::Value& value, const uint8_t* src, bool large) const
{
    uint32_t nelems;
    //uint32_t size;
    const uint8_t* p = src;
    if (large)
    {
        nelems = uint4korr(p);
        p += 4;
        //size = uint4korr(p);
        p += 4;
    }
    else
    {
        nelems = uint2korr(p);
        p += 2;
        //size = uint2korr(p);
        p += 2;
    }
    std::vector<uint32_t> offsets(nelems);
    std::vector<uint32_t> key_sizes(nelems);
    for (uint32_t i = 0; i < nelems; i++)
    {
        if (large)
        {
            offsets[i] = uint4korr(p);
            p += 4;
        }
        else
        {
            offsets[i] = uint2korr(p);
            p += 2;
        }
        key_sizes[i] = uint2korr(p);
        p += 2;
    }
    std::vector<ValueOrOffset> values_or_offsets(nelems);
    for (uint32_t i = 0; i < nelems; i++)
    {
        ValueOrOffset& v = values_or_offsets[i];
        p += _read_jsonb_inline_or_offset(v.type, v.json_value, v.offset, v.is_offset, p, large);
    }
    std::vector<std::string> keys (nelems);
    for (uint32_t i = 0; i < nelems; i++)
    {
        keys[i] = std::string((char*)p, key_sizes[i]);
        p += key_sizes[i];
    }
    value = Json::Value();
    for (uint32_t i = 0; i < nelems; i++)
    {
        ValueOrOffset& v = values_or_offsets[i];
        if (v.is_offset)
        {
            p += _read_jsonb_type(value[keys[i]], EJSONBType(v.type), p);
        }
        else
        {
            value[keys[i]] = v.json_value;
        }
    }
    return p - src;
}

ssize_t CValue::_read_jsonb_type(Json::Value& value, uint8_t type, const uint8_t* src) const
{
    switch (type)
    {
        case JSONB_TYPE_SMALL_OBJECT:
            return _read_jsonb_object(value, src, false);
        case JSONB_TYPE_LARGE_OBJECT:
            return _read_jsonb_object(value, src, true);
        case JSONB_TYPE_SMALL_ARRAY:
            return _read_jsonb_array(value, src, false);
        case JSONB_TYPE_LARGE_ARRAY:
            return _read_jsonb_array(value, src, true);
        case JSONB_TYPE_LITERAL:
        {
            uint16_t v = uint2korr(src);
            if (v == JSONB_LITERAL_NULL)
            {
                value = Json::Value(Json::nullValue);
            }
            else if (v == JSONB_LITERAL_FALSE)
            {
                value = false;
            }
            else if (v == JSONB_LITERAL_TRUE)
            {
                value = true;
            }
            return 2;
        }
        case JSONB_TYPE_INT16:
            value = Json::Int(sint2korr(src));
            return 2;
        case JSONB_TYPE_UINT16:
            value = Json::Int(uint2korr(src));
            return 2;
        case JSONB_TYPE_INT32:
            value = Json::Int64(sint4korr(src));
            return 4;
        case JSONB_TYPE_UINT32:
            value = Json::UInt64(uint4korr(src));
            return 4;
        case JSONB_TYPE_INT64:
            value = Json::Int64(sint8korr(src));
            return 8;
        case JSONB_TYPE_UINT64:
            value = Json::UInt64(uint8korr(src));
            return 8;
        case JSONB_TYPE_DOUBLE:
        {
            double v;
            float8get(&v, src);
            return 8;
        }
        case JSONB_TYPE_STRING:
            return _read_jsonb_string(value, src);
        //case JSONB_TYPE_OPAQUE:
        default:
            return -1;
    }
}

std::ostream& operator<<(std::ostream& os, const CValue::TDate& d)
{
	char buf[20];
	sprintf(buf, "%d-%02d-%02d", d.y, d.m, d.d);
	os << buf;
	return os;
}

std::ostream& operator<<(std::ostream& os, const CValue::TTime& t)
{
	char buf[10];
	sprintf(buf, "%02d:%02d:%02d", t.h, t.m, t.s);
	os << buf;
	return os;
}

std::ostream& operator<<(std::ostream& os, const CValue::TDateTime& dt)
{
	os << dt.date << " " << dt.time;
	return os;
}

std::ostream& operator<<(std::ostream& os, const CValue& val)
{
	switch (val._type)
	{
		case MYSQL_TYPE_VARCHAR:
		case MYSQL_TYPE_VAR_STRING:
		case MYSQL_TYPE_STRING:
		case MYSQL_TYPE_BLOB:
		{
			std::string s;
			val.as_string(s);
			os << s;
			break;
		}
		
		case MYSQL_TYPE_ENUM:
		case MYSQL_TYPE_SET:
		{
			os << std::hex << std::setfill('0') << std::setw(8) <<  val.as_enum();
			break;
		}
		case MYSQL_TYPE_DECIMAL:
		case MYSQL_TYPE_TINY:
		case MYSQL_TYPE_SHORT:  
		case MYSQL_TYPE_LONG:
		case MYSQL_TYPE_LONGLONG:
		{
			os << val.as_int64();
			break;
		}
		
		case MYSQL_TYPE_FLOAT:
		{
			os << val.as_float();
			break;
		}
		case MYSQL_TYPE_DOUBLE:
		{
			os << val.as_double();
			break;
		}
		
		case MYSQL_TYPE_TIMESTAMP:
		case MYSQL_TYPE_TIMESTAMP2:
		{
			char buf[64];
			time_t t = val.as_timestamp();
			int n = ::sprintf(buf, "%s", ::ctime(&t));
			if (n > 1) buf[n - 1] = '\0';
			os << buf;
			break;
		}
		case MYSQL_TYPE_DATE:
		{
			os << val.as_date();
			break;
		}
		case MYSQL_TYPE_TIME:
		case MYSQL_TYPE_TIME2:
		{
			os << val.as_time();
			break;
		}
		case MYSQL_TYPE_DATETIME:
		case MYSQL_TYPE_DATETIME2:
		{
			os << val.as_datetime();
			break;
		}
		case MYSQL_TYPE_YEAR:
		{
			os << val.as_year();
			break;
		}
        case MYSQL_TYPE_JSON:
        {
            Json::Value json;
            Json::StreamWriterBuilder _json_builder;
            val.as_json(json);
            std::string s = Json::writeString(_json_builder, json);
            os << s;
            break;
        }
		default: {}
			
	}
	
	return os;
}



}

