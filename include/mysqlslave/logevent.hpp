#ifndef LOGEVENT_H_
#define LOGEVENT_H_

#ifdef VDEBUG
#define VDEBUG_CHUNK(x) x;
#else
#define VDEBUG_CHUNK(x) 
#endif

// copy-paste from mysql sources.
#include <my_global.h>
#include <mysql.h>

#undef HAVE_STPCPY
#undef max
#undef min
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define LOG_EVENT_OFFSET 4
#define ST_SERVER_VER_LEN 50
#define LOG_EVENT_HEADER_LEN 19     /* the fixed header length */
#define OLD_HEADER_LEN       13     /* the fixed header length in 3.23 */
#define LOG_EVENT_MINIMAL_HEADER_LEN 19
/* event-specific post-header sizes */
// where 3.23, 4.x and 5.0 agree
#define QUERY_HEADER_MINIMAL_LEN     (4 + 4 + 1 + 2)
// where 5.0 differs: 2 for len of N-bytes vars.
#define QUERY_HEADER_LEN     (QUERY_HEADER_MINIMAL_LEN + 2)
#define STOP_HEADER_LEN      0
#define LOAD_HEADER_LEN      (4 + 4 + 4 + 1 +1 + 4)
#define SLAVE_HEADER_LEN     0
#define START_V3_HEADER_LEN     (2 + ST_SERVER_VER_LEN + 4)
#define ROTATE_HEADER_LEN    8 // this is FROZEN (the Rotate post-header is frozen)
#define INTVAR_HEADER_LEN      0
#define CREATE_FILE_HEADER_LEN 4
#define APPEND_BLOCK_HEADER_LEN 4
#define EXEC_LOAD_HEADER_LEN   4
#define DELETE_FILE_HEADER_LEN 4
#define NEW_LOAD_HEADER_LEN    LOAD_HEADER_LEN
#define RAND_HEADER_LEN        0
#define USER_VAR_HEADER_LEN    0
#define FORMAT_DESCRIPTION_HEADER_LEN (START_V3_HEADER_LEN+1+LOG_EVENT_TYPES)
#define XID_HEADER_LEN         0
#define BEGIN_LOAD_QUERY_HEADER_LEN APPEND_BLOCK_HEADER_LEN
#define ROWS_HEADER_LEN        8
#define TABLE_MAP_HEADER_LEN   8
#define EXECUTE_LOAD_QUERY_EXTRA_HEADER_LEN (4 + 4 + 4 + 1)
#define EXECUTE_LOAD_QUERY_HEADER_LEN  (QUERY_HEADER_LEN + EXECUTE_LOAD_QUERY_EXTRA_HEADER_LEN)
#define INCIDENT_HEADER_LEN    2
/*
  Max number of possible extra bytes in a replication event compared to a
  packet (i.e. a query) sent from client to master;
  First, an auxiliary log_event status vars estimation:
*/
#define MAX_SIZE_LOG_EVENT_STATUS (1 + 4          /* type, flags2 */   + \
                                   1 + 8          /* type, sql_mode */ + \
                                   1 + 1 + 255    /* type, length, catalog */ + \
                                   1 + 4          /* type, auto_increment */ + \
                                   1 + 6          /* type, charset */ + \
                                   1 + 1 + 255    /* type, length, time_zone */ + \
                                   1 + 2          /* type, lc_time_names_number */ + \
                                   1 + 2          /* type, charset_database_number */ + \
                                   1 + 8          /* type, table_map_for_update */ + \
                                   1 + 4          /* type, master_data_written */ + \
                                   1 + 16 + 1 + 60/* type, user_len, user, host_len, host */)
#define MAX_LOG_EVENT_HEADER   ( /* in order of Query_log_event::write */ \
  LOG_EVENT_HEADER_LEN + /* write_header */ \
  QUERY_HEADER_LEN     + /* write_data */   \
  EXECUTE_LOAD_QUERY_EXTRA_HEADER_LEN + /*write_post_header_for_derived */ \
  MAX_SIZE_LOG_EVENT_STATUS + /* status */ \
  NAME_LEN + 1)

/*
   Event header offsets;
   these point to places inside the fixed header.
*/
#define EVENT_TYPE_OFFSET    4
#define SERVER_ID_OFFSET     5
#define EVENT_LEN_OFFSET     9
#define LOG_POS_OFFSET       13
#define FLAGS_OFFSET         17

/* query event post-header */
#define Q_THREAD_ID_OFFSET	0
#define Q_EXEC_TIME_OFFSET	4
#define Q_DB_LEN_OFFSET		8
#define Q_ERR_CODE_OFFSET	9
#define Q_STATUS_VARS_LEN_OFFSET 11
#define Q_DATA_OFFSET		QUERY_HEADER_LEN

/* Intvar event data */
#define I_TYPE_OFFSET        0
#define I_VAL_OFFSET         1

/* Rotate event post-header */
#define R_POS_OFFSET       0
#define R_IDENT_OFFSET     8

/* TM = "Table Map" */
#define TM_MAPID_OFFSET    0
#define TM_FLAGS_OFFSET    6

/* RW = "RoWs" */
#define RW_MAPID_OFFSET    0
#define RW_FLAGS_OFFSET    6

/* 4 bytes which all binlogs should begin with */
#define BINLOG_MAGIC        "\xfe\x62\x69\x6e"

/* Artificial events are created arbitarily and not written to binary log */
#define LOG_EVENT_ARTIFICIAL_F 0x20

namespace mysql {

enum Log_event_type
{
	// Every time you update this enum (when you add a type), you have to fix CFormatDescriptionLogEvent
	UNKNOWN_EVENT = 0,
	START_EVENT_V3 = 1,
	QUERY_EVENT = 2,
	STOP_EVENT = 3,
	ROTATE_EVENT = 4,
	INTVAR_EVENT = 5,
	LOAD_EVENT = 6,
	SLAVE_EVENT = 7,
	CREATE_FILE_EVENT = 8,
	APPEND_BLOCK_EVENT = 9,
	EXEC_LOAD_EVENT = 10,
	DELETE_FILE_EVENT = 11,
	/*
	NEW_LOAD_EVENT is like LOAD_EVENT except that it has a longer
	sql_ex, allowing multibyte TERMINATED BY etc; both types share the
	same class (Load_log_event)
	*/
	NEW_LOAD_EVENT = 12,
	RAND_EVENT = 13,
	USER_VAR_EVENT = 14,
	FORMAT_DESCRIPTION_EVENT = 15,
	XID_EVENT = 16,
	BEGIN_LOAD_QUERY_EVENT = 17,
	EXECUTE_LOAD_QUERY_EVENT = 18,

	TABLE_MAP_EVENT = 19,

	// These event numbers were used for 5.1.0 to 5.1.15 and are therefore obsolete 
	PRE_GA_WRITE_ROWS_EVENT = 20,
	PRE_GA_UPDATE_ROWS_EVENT = 21,
	PRE_GA_DELETE_ROWS_EVENT = 22,

	// These event numbers are used from 5.1.16 and forward 
	WRITE_ROWS_EVENT = 23,
	UPDATE_ROWS_EVENT = 24,
	DELETE_ROWS_EVENT = 25,

	// Something out of the ordinary happened on the master
	INCIDENT_EVENT = 26,

    /*
    HEARTBEAT_EVENT = 27,

    IGNORABLE_EVENT = 28,
    ROWS_QUERY_EVENT = 29,

    // Version 2 of the Row events
    WRITE_ROWS_V2_EVENT = 30,
    UPDATE_ROWS_V2_EVENT = 31,
    DELETE_ROWS_V2_EVENT = 32,

    GTID_EVENT = 33,
    ANONYMOUS_GTID_EVENT = 34,

    PREVIOUS_GTIDS_EVENT = 35,

    TRANSACTION_CONTEXT_EVENT = 36,

    VIEW_CHANGE_EVENT = 37,

    // Prepared XA transaction terminal event similar to Xid
    XA_PREPARE_EVENT = 38,
    */

	/*
	Add new events here - right above this comment!
	Existing events (except ENUM_END_EVENT) should never change their numbers
	*/
	ENUM_END_EVENT /* end marker */
};

#define LOG_EVENT_TYPES (ENUM_END_EVENT-1)

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * CLogEvent
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

class CFormatDescriptionLogEvent;

class CLogEvent
{
public:
	CLogEvent();
	virtual ~CLogEvent() throw();

	virtual Log_event_type get_type_code() const = 0;
	virtual const char* get_type_code_str() const = 0;
	virtual bool is_valid() const = 0;

	virtual int tune(uint8_t* data, size_t size, const CFormatDescriptionLogEvent& fmt);
	
public:
	time_t _when;
	uint32_t _server_id;
	uint32_t _data_written;
	uint64_t _log_pos;
	uint16_t _flags;
        uint32_t _checksum;

VDEBUG_CHUNK (
public:
	virtual void dump(FILE* stream) const
	{
		char buf[64];
		char* when = ctime_r(&_when, buf);
		when[strlen(buf) - 1] = '\0';
		//char *ctime_r(const time_t *timep, char *buf);
		fprintf(stream, "#%d %s: %s, log pos %llu\n", 
				get_type_code(), get_type_code_str(), when, (unsigned long long)_log_pos);
	}
)
	

};


/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * CFormatDescriptionLogEvent
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
class CFormatDescriptionLogEvent : public CLogEvent
{
public:
	CFormatDescriptionLogEvent();

	virtual Log_event_type get_type_code() const { return FORMAT_DESCRIPTION_EVENT; }
	virtual const char* get_type_code_str() const { return "format description event"; }
	virtual bool is_valid() const;
	
	bool is_supported(uint32_t event_type) { return event_type <= LOG_EVENT_TYPES; }
	
        int tune(uint8_t binlog_ver, const char* server_ver, bool use_checksum);
	
public:
	uint8_t _binlog_version;
	char _server_version[ST_SERVER_VER_LEN];
	uint8_t _common_header_len;
	uint8_t _post_header_len[LOG_EVENT_TYPES];
        bool _use_checksum;
};

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * CRotateLogEvent
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */
class CRotateLogEvent : public CLogEvent
{
public:
	CRotateLogEvent() : _position(0), _new_log(NULL), _len(0)  { }
	virtual Log_event_type get_type_code() const { return ROTATE_EVENT; };
	virtual const char* get_type_code_str() const { return "rotate event"; };
	virtual bool is_valid() const { return true; }
	
	virtual int tune(uint8_t *data, size_t size, const CFormatDescriptionLogEvent& fmt);
	
	const uint8_t* get_log_name(size_t *len) const { if (len) *len=_len; return _new_log; }
	const uint8_t* get_log_name() const { return _new_log; }
	size_t get_log_name_len() const { return _len; }
	uint64_t get_log_pos() const { return _position; }
	
protected:
	uint64_t _position;
	uint8_t* _new_log;
	size_t _len;
	
VDEBUG_CHUNK (	
public: 
	virtual void dump(FILE *stream) const
	{
		CLogEvent::dump(stream);
		char buf[256];
		memcpy(buf, _new_log, _len);
		buf[_len] = '\0';
		fprintf(stream, "%s:%llu\n", buf, (unsigned long long)_position);
	}
)
	
};

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * CUnhandledLogEvent
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

class CUnhandledLogEvent : public CLogEvent
{
public:
	virtual Log_event_type get_type_code() const { return (Log_event_type)_event_number; }
	virtual const char* get_type_code_str() const { return "unhandled event"; }
	virtual bool is_valid() const { return 1; }
	virtual int tune(uint8_t *data, size_t size, const CFormatDescriptionLogEvent& fmt) 
	{
		CLogEvent::tune(data, size, fmt);
		_event_number = data[EVENT_TYPE_OFFSET];
		return 0;
	}

protected:
	uint8_t	_event_number;
};

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * CIntvarLogEvent
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

class CIntvarLogEvent : public CLogEvent
{
public: 
	enum Int_event_type
	{
		INVALID_INT_EVENT = 0, 
		LAST_INSERT_ID_EVENT = 1, 
		INSERT_ID_EVENT = 2
	};

public:
	virtual Log_event_type get_type_code() const { return INTVAR_EVENT; }
	virtual const char* get_type_code_str() const
	{ 
		if( _type == LAST_INSERT_ID_EVENT )
		{
			return "intvar event LAST_INSERT_ID";
		}
		else if( _type == INSERT_ID_EVENT )
		{
			return "intvar event INSERT_ID";
		}
		else
		{
			return "intvar event unknown type";
		}
	}

	virtual bool is_valid() const { return 1; }
	virtual int tune(uint8_t *data, size_t size, const CFormatDescriptionLogEvent& fmt) ;

protected:
	uint8_t _type;
	uint64_t _val;

};


/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * CQueryLogEvent
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

class CQueryLogEvent : public CLogEvent
{
public:
	virtual Log_event_type get_type_code() const { return QUERY_EVENT; }
	virtual const char* get_type_code_str() const { return "query event"; }
	virtual bool is_valid() const { return 1; }
	virtual int tune(uint8_t *data, size_t size, const CFormatDescriptionLogEvent& fmt);
	
public:
	uint32_t _q_exec_time;
	uint32_t _q_len;
	uint32_t _db_len;
	uint16_t _error_code;
	uint16_t _status_vars_len;
	char _query[1024];

VDEBUG_CHUNK (
public:
	virtual void dump(FILE *stream) const
	{
		CLogEvent::dump(stream);
		fprintf(stream, "query '%s' with error_code %d, exec time: %ds\n", _query, (int)_error_code, (int)_q_exec_time);
	}
)

};

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * CTableMapLogEvent
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

class CTableMapLogEvent : public CLogEvent
{
public:
    CTableMapLogEvent();
//	CTableMapLogEvent(uint8_t *data, size_t size, const CFormatDescriptionLogEvent *fmt);
	virtual ~CTableMapLogEvent() throw();

	static const char* get_database_name(uint8_t *data, size_t size, const CFormatDescriptionLogEvent& fmt);
	static const char* get_table_name(uint8_t *data, size_t size, const CFormatDescriptionLogEvent& fmt);
	static uint64_t	get_table_id(uint8_t *data, size_t size, const CFormatDescriptionLogEvent& fmt);
	
	virtual Log_event_type get_type_code() const { return TABLE_MAP_EVENT; }
	virtual const char* get_type_code_str() const { return "table map event"; }
	virtual bool is_valid() const
	{
		return 
		_db_name[0] && 
		_table_name[0] && 
		_column_count && 
		_column_types && 
		(_metadata_length ? _metadata != NULL : true);
	}
	
	virtual int tune(uint8_t *data, size_t size, const CFormatDescriptionLogEvent& fmt);
	
	const char * get_database_name() const;
	const char * get_table_name() const;
	int get_column_count() const;

public:
	uint64_t _table_id;

protected:
	uint8_t *_data;
	size_t _size;
	char _db_name[255];
	char _table_name[255];
	uint64_t _column_count;
	uint8_t *_column_types;
	uint64_t _metadata_length;
	uint8_t *_metadata;
	uint64_t _bit_null_fields;
	
};

/*
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * CRowLogEvent
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 */

class CRowLogEvent : public CLogEvent
{
public:
	CRowLogEvent()
		: _valid(0)
	{
	}
	
	virtual Log_event_type get_type_code() const { return (Log_event_type)_type; }
	virtual const char* get_type_code_str() const
	{
		if( _type == WRITE_ROWS_EVENT )
		{
			return "WRITE_ROWS_EVENT";
		}
		else if( _type == DELETE_ROWS_EVENT )
		{
			return "DELETE_ROWS_EVENT";
		}
		else if( _type == UPDATE_ROWS_EVENT )
		{
			return "UPDATE_ROWS_EVENT";
		}
		else
		{
			return "unknown rows event";
		}
	}
	virtual bool is_valid() const { return _valid; }
	
	virtual int tune(uint8_t *data, size_t size, const CFormatDescriptionLogEvent& fmt);
	
	const uint8_t* rows_data() const { return _data; }
	const size_t rows_len() const { return _len; }
	uint64_t used_columns_mask() const { return _used_columns_mask & 0x00FFFFFFFFFFFFFFFFLL; }
	uint64_t used_columns_1bit_count() const { return _used_columns_mask >> (64-8); }
	uint64_t used_columns_afterimage_mask() const { return _used_columns_afterimage_mask & 0x00FFFFFFFFFFFFFFFFLL; }
	uint64_t used_columns_afterimage_1bit_count() const { return _used_columns_afterimage_mask >> (64-8); }
	
	uint64_t build_column_mask(const uint8_t **ptr, size_t *len, uint64_t n);
	void update_n1bits(uint64_t *mask);
	
public:
	uint32_t _type;
	uint16_t _row_flags;
	uint64_t _table_id;
	uint64_t _ncolumns;
	
	const uint8_t *_rows;
protected:
	// в самый старший байт пишем число установленных бит
	uint64_t _used_columns_mask;
	// в самый старший байт пишем число установленных бит
	uint64_t _used_columns_afterimage_mask; // for UPDATE_ROWS_EVENT only
	const uint8_t *_data;
	size_t _len;
	int _valid;
	
VDEBUG_CHUNK (
public:
	virtual void dump(FILE *stream) const
	{
		CLogEvent::dump(stream);
		fprintf(stream, 
				"valid: %d, rowslen %d, rowflags %d, table_id %d, ncolumns %d, ucm %X (%d bits)",
				_valid, 
				(int)_len,
				(int)_row_flags,
				(int)_table_id, 
				(int)_ncolumns, 
				(int)used_columns_mask(),
				(int)used_columns_1bit_count());
		if( _type == UPDATE_ROWS_EVENT )
			fprintf(stream, ", ucam %X (%d bits)", 
				(int)used_columns_afterimage_mask(),
				(int)used_columns_afterimage_1bit_count());
		fprintf(stream, "\n");
	}
)

};

}

#endif /* LOGEVENT_H_ */

