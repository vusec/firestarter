#define LTCKPT_CHECKPOINT_METHOD bitmap
#define _GNU_SOURCE
#include <dlfcn.h>
#include <elf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "../../ltckpt_local.h"
#include "bitmap.h"

#ifndef __MINIX
#include <pthread.h>

#ifdef LTCKPT_X86
EARLY_INIT_CHANGE_STACK(LTCKPT_STACK_START, LTCKPT_STACK_SIZE)
#endif
#define LTCKPT_RESTARTING() 0
#else
#define LTCKPT_RESTARTING() ltckpt_is_mapped((void*)LTCKPT_SHADOW_SPACE_START)
#endif

/*
 * This file contains all the stuff necessary for initialization of 
 * our checkpointing library.
 */

void ltckpt_allocate_bitmap()
{
	ltckpt_debug_func();
	ltckpt_va_t ret = ltckpt_mmap(LTCKPT_BITMAP_START, LTCKPT_BITMAP_SIZE,
		PROT_WRITE | PROT_READ,
		LTCKPT_MAP_FIXED | LTCKPT_MAP_PRIVATE |	LTCKPT_MAP_NORESERVE);

	if (ret == LTCKPT_MAP_FAILED) {
		ltckpt_panic("%s", "Could not allocate memory for bitmap.\n");
	}

#if BITMAP_TYPE == BITMAP_PLAIN_SMMAP
	int smmap_ret;
 	smmap_ret = smmap((void*) LTCKPT_BITMAP_START,
	      (void *)(LTCKPT_BITMAP_START+LTCKPT_BITMAP_SIZE),
		  LTCKPT_BITMAP_SIZE);
	if (smmap_ret < 0) {
		ltckpt_panic("%s", "smmap failed");
	}
#endif
}


/* allocate the shadow space */
void ltckpt_allocate_shadow_space()
{
	ltckpt_debug_func();
	ltckpt_va_t ret = ltckpt_mmap(LTCKPT_SHADOW_SPACE_START,
		LTCKPT_SHADOW_SPACE_SIZE,
		PROT_WRITE | PROT_READ,
		LTCKPT_MAP_FIXED | LTCKPT_MAP_PRIVATE |	LTCKPT_MAP_NORESERVE);

	if (ret == LTCKPT_MAP_FAILED) {
		ltckpt_panic("%s", "Could not allocate memory for shadow space.\n");
	}

}


static void ltckpt_clean_space()
{
	ltckpt_debug_func();
	ltckpt_munmap(LTCKPT_BITMAP_START, LTCKPT_BITMAP_SIZE+LTCKPT_SHADOW_SPACE_SIZE);
}

#if (BITMAP_TYPE == BITMAP_PLAIN_BYTE && LTCKPT_SCHEME != LTCKPT_DIFF_SCHEME) 
unsigned int nr_pf;
ltckpt_va_t pages[LTCKPT_MAX_PF];
#include <signal.h>
#include <ucontext.h>
void ltckpt_sigsegvhandler(int signo, siginfo_t *si, void *context)
{
	ltckpt_debug_func();
	pages[nr_pf] = (uint32_t) PAGE_ROUND_DOWN((uint32_t)si->si_addr);
	if(!pages[nr_pf] || mprotect((void*)pages[nr_pf], 4096,PROT_EXEC|PROT_READ|PROT_WRITE)) {
		struct ucontext *u = (struct ucontext *)context;
#ifdef REG_EIP
#define PC REG_EIP
#else
#define PC REG_RIP
#endif
		unsigned char *pc = (unsigned char *)u->uc_mcontext.gregs[PC];
		ltckpt_panic("error: %d IP: %p ADDR: %p\n",errno, pc, si->si_addr);
	}
	if(++nr_pf == LTCKPT_MAX_PF) {
		ltckpt_panic("out of mem");
	}
}

struct sigaction sick_action = {
		.sa_sigaction = ltckpt_sigsegvhandler,
		.sa_flags = SA_SIGINFO
};
#endif

#ifndef __MINIX
static void ltckpt_bitmap_fork_prepare() {
	madvise((void *)LTCKPT_BITMAP_START, LTCKPT_BITMAP_SIZE, MADV_DONTNEED);
	madvise((void *)LTCKPT_SHADOW_SPACE_START, LTCKPT_SHADOW_SPACE_SIZE, MADV_DONTNEED);
}
#endif

/**
 * Allocate the shadow space and the bitmap
 */
void ltckpt_bitmap_late_init_hook()
{
	if (LTCKPT_RESTARTING()) {
		ltckpt_debug_print("ltckpt: We are restarting now, skipping late init hook\n");
		return;
	}
	ltckpt_debug_func();
	ltckpt_clean_space();
	ltckpt_allocate_bitmap();
	ltckpt_allocate_shadow_space();
	ltckpt_stat_init();
#if (BITMAP_TYPE == BITMAP_PLAIN_BYTE && LTCKPT_SCHEME != LTCKPT_DIFF_SCHEME)
	/* mark the end of the pages (for the scanning code)*/
	ltckpt_mprotect(LTCKPT_BITMAP_OFFSET, LTCKPT_BITMAP_SIZE, LTCKPT_PROT_R);
	sigaction(SIGSEGV, &sick_action, NULL);
#endif
#ifndef __MINIX
	pthread_atfork(ltckpt_bitmap_fork_prepare,NULL,NULL);
#endif
}

