/* $Id$ */

#include <pthread.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
	void *p;

	(void)argc;
	(void)argv;
	p = pthread_setschedprio;
	exit(0);
}
