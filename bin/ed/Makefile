# $FreeBSD: head/bin/ed/Makefile 298107 2016-04-16 07:45:30Z gjb $

PROG=	ed
SRCS=	buf.c cbc.c glbl.c io.c main.c re.c sub.c undo.c

LINKS=	${BINDIR}/ed ${BINDIR}/red
MLINKS=	ed.1 red.1

.if !defined(NO_CRYPT)
CFLAGS+=	-DDES -I${.CURDIR}/../../crypto/libressl/include
DPADD=		${LIBRECRYPTO}
LDADD=		-lprivate_crypto
LDFLAGS+=	${PRIVATELIB_LDFLAGS}
.endif

.include <bsd.prog.mk>
