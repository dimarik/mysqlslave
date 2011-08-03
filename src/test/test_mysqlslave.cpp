#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
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
	::pthread_kill(th_repl, 28);
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

void test_daemon::init(int argc, char** argv)
{
	char host[256], user[256], passwd[256], db[256], table[256];
	snprintf(host, 256, "localhost");
	user[0] = '\0';
	passwd[0] = '\0';
	db[0] = '\0';
	table[0] = '\0';
	int port = 3306;
	
	int c;
	while ((c = getopt(argc, argv, "h:u:p::")) != -1)
	{
		switch (c)
		{
			case 'h':
				snprintf(host, 256, "%s", optarg);
				break;
			case 'u':
				snprintf(user, 256, "%s", optarg);
				break;
			case 'p':
				char* buf;
				if (!optarg)
				{
					buf = getpass("Password:");
				}
				else
				{
					buf = optarg;
				}
				snprintf(passwd, 256, "%s", buf);
				break;
		}
	}

	connect_mysql_repl(host, user, passwd, port);

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

void test_daemon::connect_mysql_repl(const char* host, const char* user, const char* passwd, int port)
{
	set_connection_params(host, time(0), user, passwd, port);
	watch("test_mysqlslave_db", "test_table");
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
	if (strcasecmp(tbl.get_table_name(), "test_table") == 0)
	{
		for (mysql::CTable::TRows::const_iterator it = rows.begin(); it != rows.end(); ++it)
		{
			fprintf(stdout, "id: %u, number: %d, string: %s\n", (*it)["id"].as_uint32(), (*it)["number"].as_int32(), (*it)["string"].as_string().c_str());
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

