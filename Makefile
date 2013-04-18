PROG= iwatch
CFLAGS+= -g -Wall
LDADD+=	-lcurses -ltermcap
DPADD+=	${LIBCURSES} ${LIBTERMCAP}

MKMAN=	no

.include <bsd.prog.mk>
