#include <pthread.h>
#include <mysqlslave/logparser.h>

class test_daemon : public mysql::CLogParser
{
	int do_terminate;

	MYSQL DB;
	pthread_t th_repl;
	void connect_mysql_repl();

	int on_insert(const mysql::CTable& tbl, const mysql::CTable::TRows& rows);
	int on_update(const mysql::CTable& tbl, const mysql::CTable::TRows& rows, const mysql::CTable::TRows& old_rows);
	int on_delete(const mysql::CTable& tbl, const mysql::CTable::TRows& rows);

public:
	test_daemon();
	virtual ~test_daemon() throw();

	void replication_thread_proc();
	void terminate();
	void init();
	void run();
};

