/*
 * usage:
 *	% cc -g -o watch.so -shared watch.c -lcurses
 *	% cc -g -o watch_test watch_test.c
 *	% ./watch_test
 */
#include <sys/param.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

void (*watch_untabify)(char *buf, int maxlen) = NULL;

#define ASSERT(_cond)							\
	if (!(_cond)) {							\
		fprintf(stderr, "ASSERT(%s) failed in %s() at %s:%d\n",	\
		    #_cond, __func__, __FILE__, __LINE__);		\
		abort();						\
	}

static void
untabify_test(void)
{
	char buf[80];

	strlcpy(buf, "\tOK", sizeof(buf));
	watch_untabify(buf, sizeof(buf));
	ASSERT(strcmp(buf, "        OK") == 0);

	strlcpy(buf, "    \tOK", sizeof(buf));
	watch_untabify(buf, sizeof(buf));
	ASSERT(strcmp(buf, "        OK") == 0);

	strlcpy(buf, "       \tOK", sizeof(buf));
	watch_untabify(buf, sizeof(buf));
	ASSERT(strcmp(buf, "        OK") == 0);

	memset(buf, 0xDD, sizeof(buf));
	strlcpy(buf, "\tOK", sizeof(buf));
	watch_untabify(buf, 8);
	ASSERT(strncmp(buf, "       ", 7) == 0);
	ASSERT((unsigned char)buf[8] == 0xDD);	/* don't overflow */
	ASSERT((unsigned char)buf[7] == '\0');	/* null terminate */

	memset(buf, 0xDD, sizeof(buf));
	strlcpy(buf, "\tOKOK", sizeof(buf));
	watch_untabify(buf, 11);
	ASSERT(strcmp(buf, "        OK") == 0);
	ASSERT((unsigned char)buf[11] == 0xDD);	/* don't overflow */
}

#define	TEST(_f)				\
	do {					\
		printf("%-20s .. ", #_f);	\
		_f();				\
		printf("ok\n");			\
	} while (0/*CONSTCOND*/)


int
main(int argc, char *argv[])
{
	void		*watch;
	const char	*objname = "./watch.so";

	argv++;
	argc--;

	if (argc == 1)
		objname = *argv;

	if ((watch = dlopen(objname, RTLD_NOW)) == NULL)
		errx(1, "dlopen(%s) failed", objname);

	watch_untabify = dlsym(watch, "untabify");
	if (watch_untabify == NULL)
		errx(1, "dlsum(, untabify) failed");

	TEST(untabify_test);

	exit(EXIT_SUCCESS);
}
