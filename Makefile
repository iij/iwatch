PROG=		watch
LDADD+=		-lcurses
DPADD+=		${LIBCURSES}

.include <bsd.prog.mk>
