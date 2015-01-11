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
#include <wchar.h>

void (*watch_untabify)(wchar_t *buf, int maxlen) = NULL;

#define ASSERT(_cond)							\
	if (!(_cond)) {							\
		fprintf(stderr, "ASSERT(%s) failed in %s() at %s:%d\n",	\
		    #_cond, __func__, __FILE__, __LINE__);		\
		abort();						\
	}

static void
untabify_test(void)
{
	wchar_t buf[80];

	wcslcpy(buf, L"\tOK", sizeof(buf) / sizeof(wchar_t));
	watch_untabify(buf, sizeof(buf));
	ASSERT(wcscmp(buf, L"        OK") == 0);

	wcslcpy(buf, L"    \tOK", sizeof(buf) / sizeof(wchar_t));
	watch_untabify(buf, sizeof(buf));
	ASSERT(wcscmp(buf, L"        OK") == 0);

	wcslcpy(buf, L"       \tOK", sizeof(buf) / sizeof(wchar_t));
	watch_untabify(buf, sizeof(buf));
	ASSERT(wcscmp(buf, L"        OK") == 0);

	memset(buf, 0xDD, sizeof(buf));
	wcslcpy(buf, L"\tOK", sizeof(buf) / sizeof(wchar_t));
	watch_untabify(buf, 8 * sizeof(wchar_t));
	ASSERT(wcsncmp(buf, L"       ", 7) == 0);
	ASSERT(*((u_char *)&buf[8]) == 0xdd)	/* don't overflow */
	ASSERT(buf[7] == L'\0');		/* null terminate */

	memset(buf, 0xDD, sizeof(buf));
	wcslcpy(buf, L"\tOKOK", sizeof(buf) / sizeof(wchar_t));
	watch_untabify(buf, 11 * sizeof(wchar_t));
	ASSERT(wcscmp(buf, L"        OK") == 0);
	ASSERT(*((u_char *)&buf[11]) == 0xdd)	/* don't overflow */
}

static void
untabify_test2(void)
{
	wchar_t buf[80];

	wcslcpy(buf, L"\t\u4e86\u89e3", sizeof(buf) / sizeof(wchar_t));
	watch_untabify(buf, sizeof(buf));
	ASSERT(wcscmp(buf, L"        \u4e86\u89e3") == 0);

	wcslcpy(buf, L"  \t\u4e86\u89e3", sizeof(buf) / sizeof(wchar_t));
	watch_untabify(buf, sizeof(buf));
	ASSERT(wcscmp(buf, L"        \u4e86\u89e3") == 0);

	wcslcpy(buf, L"     \t\u4e86\u89e3", sizeof(buf) / sizeof(wchar_t));
	watch_untabify(buf, sizeof(buf));
	ASSERT(wcscmp(buf, L"        \u4e86\u89e3") == 0);
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
	const char	*objname = "./iwatch.so";

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
	TEST(untabify_test2);

	exit(EXIT_SUCCESS);
}
