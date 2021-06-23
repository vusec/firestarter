#ifndef LTCKPT_LTCKPT_COMMON_H
#define LTCKPT_LTCKPT_COMMON_H 1

#define __LTCKPT_STRINGIFY(X) #X
#define LTCKPT_STRINGIFY(X) __LTCKPT_STRINGIFY(X)
#define __LTCKPT_TOKENPASTE(x, y) x ## _ ## y
#define LTCKPT_TOKENPASTE(x, y) __LTCKPT_TOKENPASTE(x, y)
#define __LTCKPT_TOKENPASTE2(x, y, z) x ## y ## z
#define LTCKPT_TOKENPASTE2(x, y, z) __LTCKPT_TOKENPASTE2(x, y, z)
#define LTCKPT_CONCAT(...) __VA_ARGS__

#include "ltckpt_types.h"
#include "ltckpt_mechanism.h"
#include "ltckpt_debug.h"
#include "../../sharedlib/ltckpt/ltckpt_regs.h"

#include <dlfcn.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>

#ifndef __MINIX
#include <sys/prctl.h>
#else
#define MAP_ANONYMOUS MAP_ANON
#define fdatasync(x)
#define accept4(a, b, c, d) 0
#endif

#define PAGE_ROUND_DOWN(x) ((ltckpt_va_t)x & ~(4096-1))

/* wrapper to mmap t*/
#define LTCKPT_MAP_FIXED     (1<<0)
#define LTCKPT_MAP_PRIVATE   (1<<1)
#define LTCKPT_MAP_NORESERVE (1<<2)
#define LTCKPT_MAP_STACK     (1<<3)
#define LTCKPT_MAP_POPULATE  (1<<4)

#define LTCKPT_PROT_N  (0)
#define LTCKPT_PROT_R  (1<<0)
#define LTCKPT_PROT_W  (1<<1)
#define LTCKPT_PROT_X  (1<<2)

#define LTCKPT_OK	0
#define LTCKPT_FAIL	-1
#define LTCKPT_MAP_FAILED ((ltckpt_va_t)-1)

#include "ltckpt_config.h"

/*
 * Output-related definitions.
 */
#define LTCKPT_OUTPUT_BASIC 1
#define LTCKPT_OUTPUT_INFO  2

#define LTCKPT_HANG_ON_ASSERT 0

#define LTCKPT_IS_VERBOSE() \
	(CTX(output_conf).level >= LTCKPT_OUTPUT_INFO)

#ifdef __MINIX
#define ltckpt_printf_level(L, ...) printf(__VA_ARGS__)
#define ltckpt_printf_error(...) printf(__VA_ARGS__)
#else
#define ltckpt_printf_level(L, ...) \
	printf(__VA_ARGS__)
//	util_output_printf_level(L, &CTX(output_conf), __VA_ARGS__)

#define ltckpt_printf_error(...)  do { \
        fprintf(stderr, __VA_ARGS__); \
        ltckpt_printf_force(__VA_ARGS__); \
} while(0)
#endif

#ifndef LTCKPT_O3_HACK
#define ltckpt_printf(...) \
	ltckpt_printf_level(LTCKPT_OUTPUT_INFO, __VA_ARGS__)
#else
#define ltckpt_printf(...)  
#endif

#define ltckpt_printf_force(...)  do { \
	ltckpt_printf_level(LTCKPT_OUTPUT_BASIC, __VA_ARGS__); \
} while(0)

#define lt_panic(m) do { \
	lt_printf("%s:%d: panic: %s\n", __FILE__, __LINE__, (m)); \
	lt_assert(0);	\
} while (0)

#ifndef LTCKPT_O3_HACK
#define ltckpt_panic(...) do { \
	ltckpt_printf_error("ERROR: %s: !!PANIC!! ", __func__); \
	ltckpt_printf_error(__VA_ARGS__);        \
	ltckpt_printf_error("%s", " \n");               \
	abort();                            \
} while (0)
#else
#define ltckpt_panic(...) abort();
#endif

#define _UTIL_PRINTF ltckpt_printf

#include <common/util/output.h>

#include "ltckpt_ctx.h"

#ifndef __MINIX
#include <common/util/proc_maps.h>
#include <common/util/pagemap.h>
#endif
#include <common/util/env.h>

/*
 * Wrapper-related definitions used for glibc overrides.
 */
#ifndef LTCKPT_ENABLE_WRAPPERS
#define LTCKPT_ENABLE_WRAPPERS 1
#endif

#if LTCKPT_ENABLE_WRAPPERS

#ifdef LTCKPT_WRAPPER_METHOD

#define LTCKPT_WREAL(F) LTCKPT_TOKENPASTE2(ltckpt_real_, LTCKPT_WRAPPER_METHOD, F)
#define LTCKPT_WINIT(F) LTCKPT_TOKENPASTE2(ltckpt_init_, LTCKPT_WRAPPER_METHOD, F)
#define LTCKPT_WSYM(F)  LTCKPT_TOKENPASTE2(LTCKPT_WRAPPER_METHOD, _wrap_, F)

#else

#define LTCKPT_WREAL(F) ltckpt_real_ ## F
#define LTCKPT_WINIT(F) ltckpt_init_ ## F
#define LTCKPT_WSYM(F)  F

#endif

#define LTCKPT_WRAPPER(RET, F, ARGS, _ARGS, BODY) \
	RET (*LTCKPT_WREAL(F))(ARGS); \
	void __attribute__((constructor)) LTCKPT_WINIT(F)() { \
		LTCKPT_WREAL(F) = dlsym(RTLD_NEXT, #F); \
		assert(LTCKPT_WREAL(F)); \
	} \
	RET LTCKPT_WSYM(F)(ARGS) \
	{ \
		BODY \
		return LTCKPT_WREAL(F)(_ARGS); \
	}
#define LTCKPT_NORET_WRAPPER(RET, F, ARGS, _ARGS, BODY) \
	RET (*LTCKPT_WREAL(F))(ARGS); \
	void __attribute__((constructor)) LTCKPT_WINIT(F)() { \
		LTCKPT_WREAL(F) = dlsym(RTLD_NEXT, #F); \
		assert(LTCKPT_WREAL(F)); \
	} \
	RET LTCKPT_WSYM(F)(ARGS) \
	{ \
		BODY \
		LTCKPT_WREAL(F)(_ARGS); \
		__builtin_unreachable(); \
	}
#else
#define LTCKPT_WRAPPER(RET, F, ARGS, _ARGS, BODY)
#define LTCKPT_NORET_WRAPPER(RET, F, ARGS, _ARGS, BODY)
#endif

#include <setjmp.h>
extern __thread jmp_buf ltckpt_registers;
extern __thread jmp_buf *ltckpt_registers_ptr;
void __attribute__((weak)) ltckpt_asm_save_registers_type(jmp_buf *buf, int val);  // just so, we get the functype in ltCkptPass.cpp
int __attribute__((weak)) ltckpt_setjmp_save_registers_type(jmp_buf buf);

/**
 * A wrapper to mmap.
 * Flags can be a bitise combination of LTKCPT_MAP_FIXED,
 * LTCKPT_MAP_PRIVATE and LTCKPT_MAP_NORESERVE, which should
 * have the same effect as there Linux counterpart, if supported
 * by the underlying OS.
 */
ltckpt_va_t ltckpt_mmap(ltckpt_va_t addr, size_t size, int proto, unsigned int flags);
int ltckpt_munmap(ltckpt_va_t start, size_t size);
int ltckpt_mprotect(ltckpt_va_t start, size_t size, int prot);


/****************************************************************************
*     Init helper functions                                                 *
****************************************************************************/

/**
 * Common constructors
 **/
void ltckpt_common_early_init();
void ltckpt_common_late_init();

/**
 * Common utility functions.
 **/
#ifndef __MINIX
int ltcpt_is_checkpointed_vma(util_proc_maps_entry_t *entry);
#endif
int ltckpt_restart(void *arg);
int ltckpt_mechanism_enabled(void);
int ltckpt_is_mapped(void *addr);
void ltckpt_recovery_cleanup();
void ltckpt_save_registers();
void ltckpt_restore_registers();

/**
 * Allocate a memory area suitable as a new stack for the process
 **/
ltckpt_va_t ltckpt_allocate_stack(ltckpt_va_t start, size_t size);

/**
 * This functions sets up a new stack using the information
 * contained in the old one. It returns the new stack address
 **/
ltckpt_va_t
ltckpt_setup_stack(ltckpt_early_init_data_t *eid,
                   ltckpt_va_t new_stack_top);

/**
 * Switches to a new stack and continiues execution from the address
 * specified in entry
 **/
void ltckpt_start(ltckpt_early_init_data_t *eid, ltckpt_va_t newstack);

/* A little helper macro for the early init hook */

/**
 * This function intercepts libc's __libc_start_main. This is the right place
 * to move our stack. We could also reserve the memory of the shadow space
 * but currently this is done in the constructors
 **/
#define EARLY_INIT_CHANGE_STACK(A, S)                                      \
LTCKPT_DECLARE_EARLY_INIT_HOOK() \
{                                                                          \
	ltckpt_va_t new_stack; \
	ltckpt_va_t volatile newstack; \
	ltckpt_common_early_init(); \
	new_stack = ltckpt_allocate_stack( (A),  (S)  ); \
	newstack = ltckpt_setup_stack(data, new_stack+S); \
	ltckpt_start(data, newstack);                                                \
}                                                                          \


#endif

