PROG=		iwatch
LDADD+=		-lcurses
DPADD+=		${LIBCURSES}

.include <bsd.prog.mk>
