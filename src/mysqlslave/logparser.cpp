#include <mysqlslave/logparser.h>

namespace mysql {

CLogParser::CLogParser()
	: _slave_id(0)
	, _port(0)
	, _binlog_pos(0)
	, _binlog_flags(0)
	, _is_connected(false)
	, _dispatch(1)
{
	mysql_init(&_mysql);
}

CLogParser::~CLogParser() throw()
{
	disconnect();
}

void CLogParser::connect()
{
	if (!mysql_init(&_mysql) )
	{
		throw CException("mysql_init() failed");
	}

	if (mysql_real_connect(&_mysql, _host.c_str(), _user.c_str(), _passwd.c_str(), 0, _port, 0, 0) == 0 )
	{
		throw CException("mysql_real_connect() to '%s:%d' with user '%s' failed: %s",
			_host.c_str(), _port ? _port : 3306, _user.c_str(), mysql_error(&_mysql));
	}
		
	_is_connected = true;
}

void CLogParser::reconnect()
{
	pre_reconnect(&_mysql);
	disconnect();
	connect();
	post_reconnect(&_mysql);
	request_binlog_dump();
}

void CLogParser::disconnect()
{
	mysql_close(&_mysql);
	_is_connected = false;
}


void CLogParser::set_connection_params(const char* host, uint32_t slave_id, const char* user, const char* passwd, int port)
{
	_host = host;
	_user = user;
	_passwd = passwd;
	_port = port;
	_slave_id = slave_id;
}

void CLogParser::set_binlog_position(const char* fname, uint32_t pos, uint16_t flags)
{
	_binlog_name = fname;
	_binlog_pos	= pos;
	_binlog_flags = flags;
}


void CLogParser::get_binlog_format()
{
	MYSQL_RES* res = 0;
	MYSQL_ROW row;
	const char* version;

	if (mysql_query(&_mysql, "SELECT VERSION()") || 
		!( res = mysql_store_result(&_mysql) ) ||
		!( row = mysql_fetch_row(res) ) || 
		!( version = row[0] )
	)
	{
		if (res)
		{
			mysql_free_result(res);
		}
		throw CException("could not get server version: '%s'", mysql_error(&_mysql));
	}

	if (*version != '5')
	{
		if (res)
		{
			mysql_free_result(res);
		}
		throw CException("invalid server version '%s'", version);
	}
	
	if (_fmt.tune(4, version) != 0)
	{
		throw std::runtime_error("cannot tune format log description");
	}
}


void CLogParser::request_binlog_dump()
{
	unsigned char buf[1024];
	
	if (_binlog_name.empty() || !_binlog_pos)
	{
		throw CException("binlog name or position is empty");
	}

	int4store(buf, _binlog_pos);
	int2store(buf + 4, 0); // flags
	int4store(buf + 6, _slave_id); 
	memcpy(buf + 10, _binlog_name.c_str(), _binlog_name.length());
	if (simple_command(&_mysql, COM_BINLOG_DUMP, (const unsigned char*)buf, _binlog_name.length() + 10, 1) )
	{
		throw CException("binlog dump request from '%s:%d' failed", _binlog_name.c_str(), _binlog_pos);
	}
}


void CLogParser::prepare()
{
	connect();
	get_binlog_format();
	build_db_structure();
	request_binlog_dump();
}


void CLogParser::dispatch_events()
{
	unsigned long len;
	CRotateLogEvent event_rotate;
	CQueryLogEvent event_query;
	CRowLogEvent event_row;
	CUnhandledLogEvent event_unhandled;
	uint32_t event_type;
	uint8_t* buf;
	TDatabases::iterator it_dbs;
	CTable* tbl = 0;
	
	do
	{
		while (_dispatch && !_is_connected) { reconnect(); sleep(1); }
		
		len = cli_safe_read(&_mysql);
		
		if (!_dispatch) return;
		
		if (len == packet_error || (long)len < 1)
		{
			_is_connected = false;
			throw CException("Error reading packet from server: %s ( server_errno=%d)", 
							 mysql_error(&_mysql), mysql_errno(&_mysql));
		}
		
		if (len < 8 && _mysql.net.read_pos[0] == 254)
		{
			_is_connected = false;
			throw CException("Resv end packet from server,  apparent master shutdown: %s", mysql_error(&_mysql));
		}
		
		buf = _mysql.net.read_pos;
		
		buf++; len--;

		if (len < EVENT_LEN_OFFSET || (uint32_t) len != uint4korr(buf + EVENT_LEN_OFFSET))
		{
			throw CLogEventException((CLogEvent*)NULL, "event sanity check failed");
		}

		event_type = buf[EVENT_TYPE_OFFSET];
		if (!_fmt.is_supported(event_type) && event_type != FORMAT_DESCRIPTION_EVENT )
		{
			throw CLogEventException(event_type, "not supported");
		}
		
		switch (event_type)
		{
		case ROTATE_EVENT:
		{
			if (event_rotate.tune(buf, len, _fmt) == 0)
			{
				_binlog_name.assign((const char*)event_rotate.get_log_name(), event_rotate.get_log_name_len());
				_binlog_pos = event_rotate.get_log_pos();
				VDEBUG_CHUNK(event_rotate.dump(stderr);)
			}
			else
			{
				throw CLogEventException(&event_rotate, "tuning failed");
			}
			break;
		}
		case FORMAT_DESCRIPTION_EVENT:
		{
			// ничего не делаем, главное, не выставить позицию
			break;
		}
		case QUERY_EVENT:
		{
			if (event_query.tune(buf, len, _fmt) == 0)
			{
				VDEBUG_CHUNK(event_query.dump(stderr);)
				_binlog_pos = event_query._log_pos;
			}
			break;
		}		
		case TABLE_MAP_EVENT:
		{
			it_dbs = _databases.find(CTableMapLogEvent::get_database_name(buf, len, _fmt));
			if (it_dbs != _databases.end())
			{
				tbl = it_dbs->second.get_table(CTableMapLogEvent::get_table_name(buf, len, _fmt));
			}
			else
			{
				tbl = NULL;
			}
			
			if (tbl != NULL)
			{
				if (tbl->tune(buf, len, _fmt) != 0 )
				{
					tbl = NULL;
					throw CLogEventException(tbl, "tuning failed");
				}
				VDEBUG_CHUNK( else tbl->dump(stderr);)
			}
			break;
		}
		case WRITE_ROWS_EVENT:
		{
			if (tbl )
			{
				if (event_row.tune(buf, len, _fmt) == 0 )
				{
					VDEBUG_CHUNK(event_row.dump(stderr);)
					tbl->update(event_row);
					on_insert(*tbl, tbl->get_rows());
				}
				else
				{
					throw CLogEventException(&event_row, "tuning failed");
				}
			}
			break;
		}
		case UPDATE_ROWS_EVENT:
		{
			if (tbl )
			{
				if (event_row.tune(buf, len, _fmt) == 0 )
				{
					VDEBUG_CHUNK(event_row.dump(stderr);)
					tbl->update(event_row);
					on_update(*tbl, tbl->get_new_rows(), tbl->get_rows());
				}
				else
				{
					throw CLogEventException(&event_row, "tuning failed");
				}
			}
			break;
		}	
		case DELETE_ROWS_EVENT:
		{
			if (tbl )
			{
				if (event_row.tune(buf, len, _fmt) == 0 )
				{
					VDEBUG_CHUNK(event_row.dump(stderr);)
					tbl->update(event_row);
					on_delete(*tbl, tbl->get_rows());
				}
				else
				{
					throw CLogEventException(&event_row, "tuning failed");
				}
			}
			break;
		}
		default:
		{
			if (event_unhandled.tune(buf, len, _fmt) == 0)
			{
				_binlog_pos = event_unhandled._log_pos;
				VDEBUG_CHUNK(event_unhandled.dump(stderr);)
			}
			else
			{
				throw CLogEventException(&event_unhandled, "rows event tuning failed");
			}
		}
		}
		
	}
	while (dispatch());

	disconnect();
}

void CLogParser::stop_event_loop() 
{
	_dispatch = 0;
	::close(_mysql.net.fd);
}


void CLogParser::watch(const std::string& db_name, const std::string& table_name)
{
	if (db_name.empty() || table_name.empty()) return;

	_databases[db_name];
	TDatabases::iterator it_dbs = _databases.find(db_name);

	CDatabase& db = it_dbs->second;

	if (!db.get_table(table_name))
	{
		CTable tbl;
		db.set_table(table_name, tbl);
	}
}


void CLogParser::build_db_structure()
{
	MYSQL_RES* res_db = 0;
	MYSQL_RES* res_tbl = 0;
	MYSQL_RES* res_column = 0;
	MYSQL_ROW row;
	
	TDatabases::iterator it_dbs;
	
	if (_databases.empty())
	{
		throw std::runtime_error("build_db_structure() failed: no databases filtered");
	}
	
	if ((res_db = mysql_list_dbs(&_mysql, NULL)) == NULL )
	{
		throw CException("build_db_structure() call to 'show databases' failed: %s", mysql_error(&_mysql));
		return;
	}
	
	while ( (row = mysql_fetch_row(res_db)) != NULL )
	{
		if ((it_dbs = _databases.find(row[0])) != _databases.end())
		{
			if (mysql_select_db(&_mysql, row[0]) || (res_tbl = mysql_list_tables(&_mysql, NULL)) == NULL)
			{
				if (res_db)
				{
					mysql_free_result(res_db);
				}
				if (res_tbl)
				{
					mysql_free_result(res_tbl);
				}
				throw CException("build_db_structure() call to 'mysql_list_tables()' failed: %s", mysql_error(&_mysql));
				return;
			}
			
			while ( (row = mysql_fetch_row(res_tbl)) != NULL )
			{
				CTable* tbl = it_dbs->second.get_table(row[0]);
				if (tbl)
				{
					char query[256];
					sprintf(query, "SHOW COLUMNS FROM %s",row[0]);
					if (mysql_query(&_mysql, query) != 0 || (res_column = mysql_store_result(&_mysql)) == NULL)
					{
						throw CException("build_db_structure() call to '%s' failed: %s", query, mysql_error(&_mysql));
						if (res_db)
						{
							mysql_free_result(res_db);
						}
						if (res_tbl)
						{
							mysql_free_result(res_tbl);
						}
						if (res_column)
						{
							mysql_free_result(res_tbl);
						}
						return;
					}
					
					size_t pos = 0;
					while ((row = mysql_fetch_row(res_column)) != NULL) 
					{
						tbl->add_column(row[0], pos++, row[1]);
					}
					mysql_free_result(res_column);
					res_column = NULL;
				}
			}
			mysql_free_result(res_tbl);
			res_tbl = NULL;
		}
	}
	
	if (_binlog_name.empty() || !_binlog_pos)
	{
		get_last_binlog_position();
	}
	
	if (res_db)
	{
		mysql_free_result(res_db);
	}
	if (res_tbl)
	{
		mysql_free_result(res_tbl);
	}
	if (res_column)
	{
		mysql_free_result(res_tbl);
	}
}

void CLogParser::get_last_binlog_position()
{
	MYSQL_RES* res = 0;
	MYSQL_ROW row;
	
	if (mysql_query(&_mysql, "SHOW MASTER STATUS") || (res = mysql_store_result(&_mysql)) == NULL)
	{
		throw CException("get_last_binlog_position() call to 'show master status' failed: %s", mysql_error(&_mysql));
		if (res)
		{
			mysql_free_result(res);
		}
		return;
	}
	
	if ((row = mysql_fetch_row(res)) == NULL)
	{
		throw CException("get_last_binlog_position() 'show master status' returns 0 rows: %s", mysql_error(&_mysql));
		mysql_free_result(res);
		return;
	}
	
	if (row[0] && row[0][0] && row[1])
	{
		_binlog_name = row[0];
		_binlog_pos = (uint32_t)atoll(row[1]);
	}
	else
	{
		throw CException("get_last_binlog_position() returns invalid row due to '%s'", mysql_error(&_mysql));
		mysql_free_result(res);
		return;
	}
}

}

