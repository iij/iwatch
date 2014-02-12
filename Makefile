# This is for OpenBSD.  Please do ./configure and gmake for other OSs.

PROG=		watch
LDADD=		-lcurses
DPADD=		${LIBCURSES}
CFLAGS+=	-std=gnu99
CPPFLAGS+=	-DBSDMAKE -D_XOPEN_SOURCE_EXTENDED

CLEANFILES+=	watch_test watch.so

test:
	${CC} ${CFLAGS} ${CPPFLAGS} -O0 -g -fPIC -shared -o watch.so ${.CURDIR}/watch.c ${LDADD}
	${CC} ${CFLAGS} ${CPPFLAGS} -O0 -g -o watch_test ${.CURDIR}/watch_test.c
	./watch_test
	
.include <bsd.prog.mk>
