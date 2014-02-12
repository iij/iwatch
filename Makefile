PROG=		watch
LDADD+=		-lncursesw
DPADD+=		${LIBCURSES}
CPPFLAGS+=	-DBSDMAKE -D_XOPEN_SOURCE_EXTENDED

.ifdef .FreeBSD
CFLAGS+=	${CPPFLAGS}
.else
CFLAGS+=	-std=c99
.endif
CLEANFILES+=	watch_test watch.so

test:
	${CC} ${CFLAGS} ${CPPFLAGS} -O0 -g -fPIC -shared -o watch.so ${.CURDIR}/watch.c ${LDADD}
	${CC} ${CFLAGS} ${CPPFLAGS} -O0 -g -o watch_test ${.CURDIR}/watch_test.c
	./watch_test
	
.include <bsd.prog.mk>
