#include "test_mysqlslave.hpp"

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		fprintf(stdout, "Usage %s [OPTIONS]\n", argv[0]);
		fprintf(stdout, "\
-h		host\n\
-u		user\n\
-p		password\n\
");
		return 0;
	}
	test_daemon d;
	d.init(argc, argv);
	d.run();

	return 0;
}
