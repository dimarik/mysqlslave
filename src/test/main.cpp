#include "test_mysqlslave.hpp"

int main()
{
	test_daemon d;
	d.init();
	d.run();

	return 0;
}
