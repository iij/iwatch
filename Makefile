PROG=		iwatch
LDADD+=		-lcurses -ltermcap
DPADD+=		${LIBCURSES} ${LIBTERMCAP}

.include <bsd.prog.mk>
