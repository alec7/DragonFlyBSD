# $FreeBSD: src/usr.sbin/cdcontrol/Makefile,v 1.2.2.2 2002/03/15 22:34:25 mikeh Exp $

PROG= cdcontrol

DPADD=	${LIBEDIT} ${LIBNCURSES}
LDADD=	-lprivate_edit -lprivate_ncurses
LDFLAGS+=	${PRIVATELIB_LDFLAGS}

.include <bsd.prog.mk>
