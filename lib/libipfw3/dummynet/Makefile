SRCDIR=	${.CURDIR}/

LIB=    ipfw3dummynet
SRCS=	ipfw3_dummynet.c

INCS=	ipfw3_dummynet.h


WARNS?=	2

LFLAGS+=
YFLAGS+=
CFLAGS+=-DHAVE_CONFIG_H -I${.CURDIR} -I${.OBJDIR}
CFLAGS+=-D_U_="__attribute__((unused))" -DYYBISON
CFLAGS+=-DHAVE_STRUCT_ETHER_ADDR -DHAVE_NET_PFVAR_H
.if !defined(NOINET6)
CFLAGS+=-DINET6
.endif

SHLIB_MAJOR=3

#
# Magic to grab sources out of src/contrib
#
DUMMYNET_DISTDIR?=${SRCDIR}
CFLAGS+=-I${DUMMYNET_DISTDIR}
.PATH:	${DUMMYNET_DISTDIR}

.include <bsd.lib.mk>
