#include <stdio.h>
#include <iostream>
#include <sys/syscall.h>
#include <pthread.h>
#include <signal.h>
#include "./../../mysqlslave/logparser.h"
#include "./../../mysqlslave/exceptions.hpp"

class sample_binlog_reader : public mysql::CLogParser
{
public:
	
	void replication_thread_proc()
	{
		while (dispatch())
		{
			try
			{
				dispatch_events();
			}
			catch (std::exception& e)
			{
				std::cout << e.what() << std::endl;
				::sleep(1);
			}
		}
	}
	virtual int on_insert(const mysql::CTable &table, const mysql::CTable::TRows &rows)
	{
		std::string s;
		if (!strcasecmp(table.get_table_name(), "strings")) 
		{
			dump_strings(table, rows);
		}
		else if (!strcasecmp(table.get_table_name(), "psites")) 
		{
			dump_psites(rows);
		}
		return 0;
		
	}
	virtual int on_update(const mysql::CTable &table, const mysql::CTable::TRows &newrows, const mysql::CTable::TRows &oldrows)
	{
		if (!strcasecmp(table.get_table_name(), "strings")) 
		{
			dump_strings(table, newrows);
			dump_strings(table, oldrows);
		}
		else if (!strcasecmp(table.get_table_name(), "psites"))
		{
			dump_psites(newrows);
			dump_psites(oldrows);
		}
		
		return 0;
	}
	virtual int on_delete(const mysql::CTable &table, const mysql::CTable::TRows &rows)
	{
		if (!strcasecmp(table.get_table_name(), "strings")) 
		{
			dump_strings(table, rows);;
		}
		else if (!strcasecmp(table.get_table_name(), "strings")) 
		{
			dump_psites(rows);;
		}
		return 0;
	}
private:
	
	void dump_psites(const mysql::CTable::TRows &rows)
	{
		std::string s;
		for(mysql::CTable::TRows::const_iterator it = rows.begin(); it != rows.end(); ++it )
		{
			(*it)["psite_id"].as_string(s);
			std::cout << "psite_id: " << s << std::endl;
		}
	}
	
	
	void dump_strings(const mysql::CTable& tbl, const mysql::CTable::TRows &rows)
	{
		std::string s;
		//uint64_t i;
		//const uint8_t* data;
		size_t size;
		
		const mysql::CColumnDesc* colDesc;
		for(mysql::CTable::TRows::const_iterator it = rows.begin(); it != rows.end(); ++it )
		{
			const mysql::CValue& v00_primary = (*it)[0];
			const mysql::CValue& v01_char = (*it)[1];
			const mysql::CValue& v02_varchar = (*it)[2];
			const mysql::CValue& v03_text = (*it)[3];
			const mysql::CValue& v04_binary = (*it)[4];
			const mysql::CValue& v05_varbinary = (*it)[5];
			const mysql::CValue& v06_enum = (*it)[6];
			const mysql::CValue& v07_set = (*it)[7];
			
			/*i = */v00_primary.as_int64();
			s = v01_char.as_string();
			s = v02_varchar.as_string();
			s = v03_text.as_string();
			/*data = */v04_binary.as_blob(&size);
			/*data = */v05_varbinary.as_blob(&size);
			/*i = */v06_enum.as_enum();
			/*i = */v07_set.as_set();
			
			std::cout << "primary:\t" << v00_primary << std::endl;
			std::cout << "char(20):\t" << v01_char << std::endl;
			std::cout << "varchar(50):\t" << v02_varchar << std::endl;
			std::cout << "text:\t" << v03_text << std::endl;
			std::cout << "binary(100):\t" << v04_binary << std::endl;
			std::cout << "varbinary(100):\t" << v05_varbinary << std::endl;
			
			colDesc = tbl.find_column("_ENUM");
			std::cout << "enum:\t" << v06_enum;
			if (colDesc) std::cout << ": " << v06_enum.as_enum(*colDesc);
			std::cout << std::endl;
			
			colDesc = tbl.find_column("_SET");
			std::cout << "set:\t" << v07_set;
			if (colDesc) std::cout << ": " << v07_set.as_set(*colDesc);
			std::cout << std::endl;
		}
	}



};
//query 'ALTER TABLE `strings` CHANGE `_ENUM` `_ENUM` ENUM('yes','no','fuckyou', 'preved', 'medved', 'shlyapa') CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL' with error_code 0, exec time: 0s
static void *__replication_thread_proc(void *ptr)
{
	sample_binlog_reader* _m = (sample_binlog_reader*)ptr;
	_m->replication_thread_proc();
	return 0;
}


#define MYSQL_HOST "192.168.3.101"
#define MYSQL_USER "testy"
#define MYSQL_PWD "testy"
#define MYSQL_BINLOG_NAME "mysql_binary_log.000023"
//#define MYSQL_BINLOG_POS 4
#define MYSQL_BINLOG_POS 1705
#define LOCAL_BINLOG_READER_ID 1


sample_binlog_reader g_binlog_reader;

void signal_install(int signum, void (*)(int signum));
void signal_handler(int signum);

static void *__replication_thread_proc(void *ptr);



int main() 
{
	 
	signal_install(SIGTERM, signal_handler);
	signal_install(SIGINT, signal_handler);
	
	g_binlog_reader.set_connection_params(MYSQL_HOST, LOCAL_BINLOG_READER_ID, MYSQL_USER, MYSQL_PWD);
//	g_binlog_reader.set_binlog_position(MYSQL_BINLOG_NAME,MYSQL_BINLOG_POS);

//	g_binlog_reader.watch("test", "psites_properties");
	g_binlog_reader.watch("test", "psites");
	
	try
	{
		g_binlog_reader.prepare();
	}
	catch (std::exception& e)
	{
		std::cout << e.what() << std::endl;
		::exit(-1);
	}
	
/*	pthread_t th_repl;
	::pthread_create(&th_repl, 0, &__replication_thread_proc, &g_binlog_reader);
	while (g_binlog_reader.dispatch()) ::pause();
	::pthread_kill(th_repl, SIGTERM);
	pthread_join(th_repl, 0);*/
	__replication_thread_proc(&g_binlog_reader);
	
	return 0;
}

void signal_install(int signum, void (*handler)(int signum))
{
	struct sigaction new_action, old_action;
	new_action.sa_handler = handler;
	::sigemptyset (&new_action.sa_mask);
	new_action.sa_flags = 0;
	::sigaction(signum, NULL, &old_action);
	if (old_action.sa_handler != SIG_IGN) ::sigaction(signum, &new_action, NULL);
}


void signal_handler(int signum)
{
	// ::fprintf(stderr, "%d incoming to %ld\n", signum, ::syscall(SYS_gettid));
	if (signum==SIGTERM || signum==SIGINT) 
	{
		g_binlog_reader.stop_event_loop();
	}
}

