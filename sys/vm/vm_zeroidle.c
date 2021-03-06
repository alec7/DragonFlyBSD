/*
 * Copyright (c) 1994 John Dyson
 * Copyright (c) 2001,2016 Matt Dillon
 * Copyright (c) 2010,2016 The DragonFly Project
 *
 * All Rights Reserved.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Venkatesh Srinivas <me@endeavour.zapto.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/vmmeter.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
#include <sys/thread.h>
#include <sys/kthread.h>
#include <sys/unistd.h>
#include <vm/vm.h>
#include <vm/vm_page.h>
#include <cpu/lwbuf.h>

#include <sys/thread2.h>
#include <vm/vm_page2.h>

#if 0
/*
 * Remove this file in 2017.
 *
 * REMOVED -  Basically does not provide any performance benefit and instead
 *	      appears to cause a performance detriment.  I surmise the issue
 *	      is simply that it takes such an enormous amount of time to read
 *	      data from dynamic ram, what really matters for a page-fault is
 *	      not that the page is zerod but instead that its cache is hot.
 *
 *	      Zeroing the page during idle periods means the page is likely
 *	      to be cold in the cache when it actually gets used.  Zeroing the
 *	      page in-line with the VM-fault, on the other-hand, not only
 *	      ensures that the memory will be hot in the cache, the zeroing
 *	      operation itself does not actually have to read the dynamic ram,
 *	      it really only writes into the cache (for a 4K page), so the
 *	      page is already hot when the user program then accesses it.
 */

/*
 * Implement the pre-zeroed page mechanism.
 */
/* Number of bytes to zero between reschedule checks */
#define IDLEZERO_RUN	(64)

/* Maximum number of pages per second to zero */
#define NPAGES_RUN	(20000)

static int idlezero_enable = 1;
TUNABLE_INT("vm.idlezero_enable", &idlezero_enable);
SYSCTL_INT(_vm, OID_AUTO, idlezero_enable, CTLFLAG_RW, &idlezero_enable, 0,
	   "Allow the kernel to use idle CPU cycles to zero pages");
static int idlezero_rate = NPAGES_RUN;
SYSCTL_INT(_vm, OID_AUTO, idlezero_rate, CTLFLAG_RW, &idlezero_rate, 0,
	   "Maximum pages per second to zero");
static int idlezero_nocache = -1;
SYSCTL_INT(_vm, OID_AUTO, idlezero_nocache, CTLFLAG_RW, &idlezero_nocache, 0,
	   "Maximum pages per second to zero");

static ulong idlezero_count = 0;
SYSCTL_ULONG(_vm, OID_AUTO, idlezero_count, CTLFLAG_RD, &idlezero_count, 0,
	   "The number of physical pages prezeroed at idle time");

enum zeroidle_state {
	STATE_IDLE,
	STATE_GET_PAGE,
	STATE_ZERO_PAGE,
	STATE_RELEASE_PAGE
};

#define DEFAULT_SLEEP_TIME	(hz / 10)
#define LONG_SLEEP_TIME		(hz * 10)

/*
 * Attempt to maintain approximately 1/2 of our free pages in a
 * PG_ZERO'd state. Add some hysteresis to (attempt to) avoid
 * generally zeroing a page when the system is near steady-state.
 * Otherwise we might get 'flutter' during disk I/O / IPC or
 * fast sleeps. We also do not want to be continuously zeroing
 * pages because doing so may flush our L1 and L2 caches too much.
 *
 * Returns non-zero if pages should be zerod.
 */
static int
vm_page_zero_check(int *zero_countp, int *zero_statep)
{
	int base;
	int count;
	int nz;
	int nt;
	int i;

	*zero_countp = 0;
	if (idlezero_enable == 0)
		return (0);

	base = vm_get_pg_color(mycpu, NULL, 0) & PQ_L2_MASK;
	count = 16;
	while (count < PQ_L2_SIZE / ncpus)
		count <<= 1;
	if (base + count > PQ_L2_SIZE)
		count = PQ_L2_SIZE - base;

	for (i = nt = nz = 0; i < count; ++i) {
		struct vpgqueues *vpq = &vm_page_queues[PQ_FREE + base + i];
		nz += vpq->zero_count;
		nt += vpq->lcnt;
	}

	if (nt > 10) {
		*zero_countp = nz * 100 / nt;
	} else {
		*zero_countp = 100;
	}
	if (*zero_statep == 0) {
		/*
		 * Wait for the count to fall to LO before starting
		 * to zero pages.
		 */
		if (*zero_countp <= 50)
			*zero_statep = 1;
	} else {
		/*
		 * Once we are zeroing pages wait for the count to
		 * increase to HI before we stop zeroing pages.
		 */
		if (*zero_countp >= 90)
			*zero_statep = 0;
	}
	return (*zero_statep);
}

/*
 * vm_pagezero should sleep for a longer time when idlezero is disabled or
 * when there is an excess of zeroed pages.
 */
static int
vm_page_zero_time(int zero_count)
{
	if (idlezero_enable == 0)
		return (LONG_SLEEP_TIME);
	if (zero_count >= 90)
		return (LONG_SLEEP_TIME);
	return (DEFAULT_SLEEP_TIME);
}

/*
 * MPSAFE thread
 */
static void
vm_pagezero(void *arg)
{
	vm_page_t m = NULL;
	struct lwbuf *lwb = NULL;
	struct lwbuf lwb_cache;
	enum zeroidle_state state = STATE_IDLE;
	char *pg = NULL;
	int npages = 0;
	int sleep_time;	
	int i = 0;
	int cpu = (int)(intptr_t)arg;
	int zero_state = 0;

	/*
	 * Adjust thread parameters before entering our loop.  The thread
	 * is started with the MP lock held and with normal kernel thread
	 * priority.
	 *
	 * Also put us on the last cpu for now.
	 *
	 * For now leave the MP lock held, the VM routines cannot be called
	 * with it released until tokenization is finished.
	 */
	lwkt_setpri_self(TDPRI_IDLE_WORK);
	lwkt_setcpu_self(globaldata_find(cpu));
	sleep_time = DEFAULT_SLEEP_TIME;

	/*
	 * Loop forever
	 */
	for (;;) {
		int zero_count;

		switch(state) {
		case STATE_IDLE:
			/*
			 * Wait for work.
			 */
			tsleep(&zero_state, 0, "pgzero", sleep_time);
			if (vm_page_zero_check(&zero_count, &zero_state))
				npages = idlezero_rate / 10;
			sleep_time = vm_page_zero_time(zero_count);
			if (npages)
				state = STATE_GET_PAGE;	/* Fallthrough */
			break;
		case STATE_GET_PAGE:
			/*
			 * Acquire page to zero
			 */
			if (--npages == 0) {
				state = STATE_IDLE;
			} else {
				m = vm_page_free_fromq_fast();
				if (m == NULL) {
					state = STATE_IDLE;
				} else {
					state = STATE_ZERO_PAGE;
					lwb = lwbuf_alloc(m, &lwb_cache);
					pg = (char *)lwbuf_kva(lwb);
					i = 0;
				}
			}
			break;
		case STATE_ZERO_PAGE:
			/*
			 * Zero-out the page
			 */
			while (i < PAGE_SIZE) {
				if (idlezero_nocache == 1)
					bzeront(&pg[i], IDLEZERO_RUN);
				else
					bzero(&pg[i], IDLEZERO_RUN);
				i += IDLEZERO_RUN;
				lwkt_yield();
			}
			state = STATE_RELEASE_PAGE;
			break;
		case STATE_RELEASE_PAGE:
			lwbuf_free(lwb);
			vm_page_flag_set(m, PG_ZERO);
			vm_page_free_toq(m);
			state = STATE_GET_PAGE;
			++idlezero_count;	/* non-locked, SMP race ok */
			break;
		}
		lwkt_yield();
	}
}

static void
pagezero_start(void __unused *arg)
{
	struct thread *td;
	int i;

	if (idlezero_nocache < 0 && (cpu_mi_feature & CPU_MI_BZERONT))
		idlezero_nocache = 1;

	for (i = 0; i < ncpus; ++i) {
		kthread_create(vm_pagezero, (void *)(intptr_t)i,
			       &td, "pagezero %d", i);
	}
}

SYSINIT(pagezero, SI_SUB_KTHREAD_VM, SI_ORDER_ANY, pagezero_start, NULL);

#endif
