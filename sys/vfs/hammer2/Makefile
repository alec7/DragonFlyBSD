# Makefile for hammer2 vfs
#
#
.PATH: ${.CURDIR} ${.CURDIR}/zlib ${.CURDIR}/xxhash

CFLAGS+= -DINVARIANTS -DSMP
CFLAGS+= -DXXH_NAMESPACE=h2_
KMOD=	hammer2
SRCS=	hammer2_vfsops.c hammer2_vnops.c hammer2_xops.c hammer2_inode.c
SRCS+=	hammer2_strategy.c
SRCS+=	hammer2_chain.c hammer2_flush.c hammer2_freemap.c hammer2_cluster.c
SRCS+=	hammer2_ioctl.c hammer2_msgops.c hammer2_subr.c hammer2_bulkfree.c
SRCS+=  hammer2_lz4.c hammer2_io.c hammer2_iocom.c hammer2_admin.c
SRCS+=	hammer2_synchro.c
SRCS+=  hammer2_zlib_adler32.c hammer2_zlib_deflate.c
SRCS+=  hammer2_zlib_inffast.c hammer2_zlib_inflate.c
SRCS+=  hammer2_zlib_inftrees.c hammer2_zlib_trees.c
SRCS+=  hammer2_zlib_zutil.c
SRCS+=	xxhash.c

.include <bsd.kmod.mk>
