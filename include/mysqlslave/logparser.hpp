#ifndef LOGPARSER_H_
#define LOGPARSER_H_

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <string>

#include "logevent.hpp"
#include "database.hpp"
#include "exceptions.hpp"
#include <map>

namespace mysql {

class CLogParser 
{
public:
	typedef std::map<std::string, CDatabase, CCaseIgnorer> TDatabases;

public:
	CLogParser();
	virtual ~CLogParser() throw();

	void set_connection_params(const char* host, uint32_t slave_id, const char* user, const char* passwd, int port = 0);
	void set_binlog_position(const char* fname, uint32_t pos, uint16_t flags = 0);
	
	void prepare();
	void dispatch_events();
	void stop_event_loop();
	
	volatile int dispatch() const { return _dispatch; }

public:
	void watch(const std::string& db_name, const std::string& table_name);

protected:
	virtual int on_insert(const CTable& table, const CTable::TRows& newrows) = 0;
	virtual int on_update(const CTable& table, const CTable::TRows& newrows, const CTable::TRows& oldrows) = 0;
	virtual int on_delete(const CTable& table, const CTable::TRows& newrows) = 0;
	
protected:
	virtual void pre_reconnect(MYSQL* mysql) {}
	virtual void post_reconnect(MYSQL* mysql) {}
	
protected:
	void connect();
	void reconnect();
	void disconnect();
	void get_binlog_format();
	void build_db_structure();
	void get_last_binlog_position();

	void request_binlog_dump();

protected:
	TDatabases _databases;
	MYSQL _mysql;
	std::string _host;
	uint32_t _slave_id;
	int _port;
	std::string _user;
	std::string _passwd;
	CFormatDescriptionLogEvent _fmt;
	std::string _binlog_name;
	uint32_t _binlog_pos;
	uint32_t _binlog_flags;
	bool _is_connected;
	
private:
	volatile int _dispatch;
};

}

#endif /* LOGPARSER_H_ */

