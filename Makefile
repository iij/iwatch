# This is for OpenBSD.  Please do ./configure and gmake for other OSs.

PROG=		iwatch
LDADD=		-lcurses
DPADD=		${LIBCURSES}
CFLAGS+=	-std=gnu99
CPPFLAGS+=	-DBSDMAKE -D_XOPEN_SOURCE_EXTENDED

CLEANFILES+=	iwatch_test iwatch.so

test:
	${CC} ${CFLAGS} ${CPPFLAGS} -O0 -g -fPIC -shared -o iwatch.so ${.CURDIR}/iwatch.c ${LDADD}
	${CC} ${CFLAGS} ${CPPFLAGS} -O0 -g -o iwatch_test ${.CURDIR}/iwatch_test.c
	./iwatch_test
	
.include <bsd.prog.mk>
