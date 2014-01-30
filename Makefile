PROG=		watch
LDADD+=		-lcurses
DPADD+=		${LIBCURSES}


test:
	${LINK.c} -O0 -g -fPIC -shared -o watch.so ${.CURDIR}/watch.c ${LDADD}
	${LINK.c} -O0 -g -o watch_test ${.CURDIR}/watch_test.c
	./watch_test
	
.include <bsd.prog.mk>
