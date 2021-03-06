/**
 * Copyright (c) 2013 Larisa Grigore<larisagrigore@gmail.com>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>

#include "utilsd.h"

extern int sysvd_debug;

void
sysvd_print(const char *fmt, ...)
{
	va_list ap;
	char format[1024];

	if(!sysvd_debug)
		return;

	snprintf(format, sizeof(format), "[sysvd %d] %s",
	    getpid(), fmt);
	va_start(ap, fmt);
//	vsyslog(LOG_DEBUG, format, ap);
	vfprintf(stderr, format, ap);
	va_end(ap);
}

void
sysvd_print_err(const char *fmt, ...)
{
	va_list ap;
	char format[1024];

	snprintf(format, sizeof(format), "[sysvd %d] error(%d): %s",
	    getpid(), errno, fmt);
	va_start(ap, fmt);
//	vsyslog(LOG_ERR, format, ap);
	vfprintf(stderr, format, ap);
	va_end(ap);
}
