#include <time.h>
#include <signal.h>
#include "test_mysqlslave.hpp"

static void* __replication_thread_proc(void* ptr)
{
	test_daemon* _d = (test_daemon*)(ptr);
	_d->replication_thread_proc();
	return 0;
}

test_daemon::test_daemon()
	: do_terminate(0)
	, th_repl(0)
{
}

test_daemon::~test_daemon() throw()
{
	stop_event_loop();
	::pthread_kill(th_repl, SIGTERM);

	if (th_repl)
	{
		pthread_join(th_repl, 0);
		fprintf(stdout, "th_repl joined\n");
	}
}

void test_daemon::terminate()
{
	do_terminate = 1;
}

static test_daemon* test_daemon_ptr = 0;

void signal_handler(int sig)
{
	signal(sig, SIG_IGN);
	if (!test_daemon_ptr) return;
	switch (sig)
	{
		case SIGHUP:
		case SIGUSR1:
		case SIGINT:
		case SIGTERM:
		case SIGSTOP:
			fprintf(stdout, "signal %d received, going to terminate\n", sig);
			test_daemon_ptr->terminate();
			break;
		case SIGSEGV:
		case SIGABRT:
			fprintf(stdout, "signal %d received, fatal terminate\n", sig);
			exit(0);
		default:
			fprintf(stdout, "signal %d received, ignoring\n", sig);
			break;
	}
}

void test_daemon::init()
{
	test_daemon_ptr = this;
	for (int sig = 1; sig < 32; sig++)
	{
		if (SIGKILL == sig || SIGSTOP == sig) continue;
		if (SIG_ERR == signal(sig, signal_handler))
		{
			fprintf(stderr, "can't set handler for %d signal\n", sig);
			exit(-1);
		}
	}
}

void test_daemon::run()
{
	connect_mysql_repl();

	int rc = pthread_create(&th_repl, 0, &__replication_thread_proc, this);
	if (rc)
	{
		fprintf(stderr, "can't create thread, error %d\n", rc);
	}
	fprintf(stdout, "th_repl created\n");

	while (!do_terminate)
	{
		sleep(1);
	}
}

void test_daemon::connect_mysql_repl()
{
	set_connection_params("localhost", time(0), "repl", "qwerty", 3306);
	watch("test", "mysqlslave");
	prepare();
}

void test_daemon::replication_thread_proc()
{
	fprintf(stdout, "th_repl started\n");

	while (dispatch())
	{
		try
		{
			dispatch_events();
		}
		catch (const std::exception& e)
		{
			fprintf(stderr, "catch exception: %s\n", e.what());
			sleep(1);
		}
	}

	fprintf(stdout, "th_repl finished\n");
}

int test_daemon::on_insert(const mysql::CTable& tbl, const mysql::CTable::TRows& rows)
{
fprintf(stdout, "INSERT\n");
	if (strcasecmp(tbl.get_table_name(), "mysqlslave") == 0)
	{
		for (mysql::CTable::TRows::const_iterator it = rows.begin(); it != rows.end(); ++it)
		{
			fprintf(stdout, "id: %d\nnumber: %d\nword: %s\n", (*it)["id"].as_int32(), (*it)["number"].as_int32(), (*it)["word"].as_string().c_str());
		}
	}

	return 0;
}

int test_daemon::on_update(const mysql::CTable& tbl, const mysql::CTable::TRows& rows, const mysql::CTable::TRows& old_rows)
{
fprintf(stdout, "UPDATE\n");
	return 0;
}

int test_daemon::on_delete(const mysql::CTable& tbl, const mysql::CTable::TRows& rows)
{
fprintf(stdout, "DELETE\n");
	return 0;
}

