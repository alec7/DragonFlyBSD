# $FreeBSD: src/tools/regression/lib/msun/Makefile,v 1.21 2013/05/31 00:27:55 svnexp Exp $

TESTS=	test-cexp test-conj test-csqrt test-ctrig \
	test-exponential test-fenv test-fma \
	test-fmaxmin test-ilogb test-invtrig test-invctrig \
	test-logarithm test-lrint \
	test-lround test-nan test-nearbyint test-next test-rem test-trig
CFLAGS+= -O0 -lm

.PHONY: tests
tests: ${TESTS}
	for p in ${TESTS}; do ${.OBJDIR}/$$p; done

.PHONY: clean
clean:
	-rm -f ${TESTS}
