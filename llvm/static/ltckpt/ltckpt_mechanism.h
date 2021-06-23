#ifndef LTCKPT_LTCKPT_MECHANISM_H
#define LTCKPT_LTCKPT_MECHANISM_H

#include <stdlib.h>

struct ltckpt_checkpoint_conf_s;

/* Global definitions. */
#define LTCKPT_HOOK(M, H) LTCKPT_TOKENPASTE(H, M)

#define CONF(X) ltckpt_conf->X

extern volatile struct ltckpt_checkpoint_conf_s* ltckpt_conf;

void ltckpt_conf_setup();
void ltckpt_conf_setup_from_hook(const char *name);

/* The possible checkpoint methods (add new methods here). */
#ifndef __MINIX
typedef enum ltckpt_checkpoint_method_e {
	LTCKPT_CHECKPOINT_METHOD_BASELINE = 0,
	LTCKPT_CHECKPOINT_METHOD_BITMAP,
	LTCKPT_CHECKPOINT_METHOD_FORK,
	LTCKPT_CHECKPOINT_METHOD_WRITELOG,
	LTCKPT_CHECKPOINT_METHOD_SMMAP,
	LTCKPT_CHECKPOINT_METHOD_SOFTDIRTY,
	LTCKPT_CHECKPOINT_METHOD_MPROTECT,
	LTCKPT_CHECKPOINT_METHOD_DUNE,
	__NUM_LTCKPT_CHECKPOINT_METHODS
} ltckpt_checkpoint_method_t;

#define LTKCPT_CHECKPOINT_CONFS_INITIALIZER { \
	{ __CONF(baseline)    }, \
	{ __CONF(bitmap)      }, \
	{ __CONF(fork)        }, \
	{ __CONF(undolog)     }, \
	{ __CONF(smmap)       }, \
	{ __CONF(softdirty)   }, \
	{ __CONF(mprotect)    }, \
	{ __CONF(dune)        }, \
	{} \
}

#define LTKCPT_DEFINE_CHECKPOINT_HOOKS() \
	__HOOK(baseline); \
	__HOOK(bitmap); \
	__HOOK(fork); \
	__HOOK(undolog); \
	__HOOK(smmap); \
	__HOOK(softdirty); \
	__HOOK(mprotect); \
	__HOOK(dune);
#else
typedef enum ltckpt_checkpoint_method_e {
        LTCKPT_CHECKPOINT_METHOD_BASELINE = 0,
        LTCKPT_CHECKPOINT_METHOD_BITMAP,
        LTCKPT_CHECKPOINT_METHOD_WRITELOG,
        __NUM_LTCKPT_CHECKPOINT_METHODS
} ltckpt_checkpoint_method_t;

#define LTKCPT_CHECKPOINT_CONFS_INITIALIZER { \
        { __CONF(baseline)    }, \
        { __CONF(bitmap)      }, \
        { __CONF(undolog)     }, \
        {} \
}

#define LTKCPT_DEFINE_CHECKPOINT_HOOKS() \
        __HOOK(baseline); \
        __HOOK(bitmap); \
        __HOOK(undolog);
#endif

/* The possible checkpoint hooks (add new hooks here). */

typedef struct ltckpt_checkpoint_conf_s {
	char *name;
	ltckpt_checkpoint_method_t method;
	void (*top_of_the_loop_hook)();
	void (*store_hook)(void *addr);
        void (*memcpy_hook)(char *addr, size_t size);
	void (*early_init_hook)(ltckpt_early_init_data_t *init_data);
	void (*late_init_hook)();
	int (*restart_hook)(void *arg);
	void (*before_exit_hook)(int status);
	void (*ctx_print_hook)();
	void (*ctx_clear_hook)();
	void (*atpthread_create_child)();
} ltckpt_checkpoint_conf_t;

/* Hooks handled by the instrumentation (all required). */
#define LTCKPT_DECLARE_CONF_SETUP_HOOK() void LTCKPT_INSTFUNCT LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_conf_setup)()
#ifdef LTCKPT_WINDOW_PROFILING
    #define LTCKPT_DECLARE_TOP_OF_THE_LOOP_HOOK() void LTCKPT_INSTFUNCT LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_top_of_the_loop)(unsigned site_id)
    #define LTCKPT_DECLARE_STORE_HOOK() void LTCKPT_INSTFUNCT LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_store_hook)(void *addr, unsigned site_id)
    #define LTCKPT_DECLARE_MEMCPY_HOOK() void LTCKPT_INSTFUNCT LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_memcpy_hook)(char *addr, size_t size, unsigned site_id)
    #define LTCKPT_DECLARE_END_OF_WINDOW_HOOK() void LTCKPT_INSTFUNCT LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_end_of_window)(unsigned site_id)
#else /* LTCKPT_WINDOW_PROFILING */
    #define LTCKPT_DECLARE_TOP_OF_THE_LOOP_HOOK() void LTCKPT_INSTFUNCT LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_top_of_the_loop)()
    #define LTCKPT_DECLARE_STORE_HOOK() void LTCKPT_INSTFUNCT LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_store_hook)(void *addr)
    #define LTCKPT_DECLARE_MEMCPY_HOOK() void LTCKPT_INSTFUNCT LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_memcpy_hook)(char *addr, size_t size)
    #define LTCKPT_DECLARE_END_OF_WINDOW_HOOK() void LTCKPT_INSTFUNCT LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_end_of_window)()
#endif

/* Hooks handled by the static library (all optional). */
#define LTCKPT_DECLARE_EARLY_INIT_HOOK() void LTCKPT_INSTFUNCT LTCKPT_NOINLINE LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_early_init_hook)(ltckpt_early_init_data_t *data)
#define LTCKPT_DECLARE_LATE_INIT_HOOK() void LTCKPT_INSTFUNCT LTCKPT_NOINLINE LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_late_init_hook)()
#define LTCKPT_DECLARE_RESTART_HOOK() int LTCKPT_INSTFUNCT LTCKPT_NOINLINE LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_restart_hook)(void *arg)
#define LTCKPT_DECLARE_BEFORE_EXIT_HOOK() void LTCKPT_INSTFUNCT LTCKPT_NOINLINE LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_before_exit_hook)(int status)
#define LTCKPT_DECLARE_CTX_PRINT_HOOK() void LTCKPT_INSTFUNCT LTCKPT_NOINLINE LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_ctx_print_hook)()
#define LTCKPT_DECLARE_CTX_CLEAR_HOOK() void LTCKPT_INSTFUNCT LTCKPT_NOINLINE LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_ctx_clear_hook)()
#define LTCKPT_DECLARE_ATPTHREAD_CREATE_CHILD_HOOK() void LTCKPT_INSTFUNCT LTCKPT_NOINLINE LTCKPT_HOOK(LTCKPT_CHECKPOINT_METHOD, ltckpt_atpthread_create_child_hook)()

#define LTCKPT_CHECKPOINT_METHOD_ONCE() \
	LTCKPT_DECLARE_CONF_SETUP_HOOK() { ltckpt_conf_setup_from_hook(LTCKPT_STRINGIFY(LTCKPT_CHECKPOINT_METHOD)); }

#define LTCKPT_DECLARE_EMPTY_STORE_HOOKS() \
	LTCKPT_ALWAYSINLINE LTCKPT_DECLARE_STORE_HOOK() {} \
	LTCKPT_ALWAYSINLINE LTCKPT_DECLARE_MEMCPY_HOOK() {}

#define LTCKPT_DECLARE_BASELINE_HOOKS() \
	LTCKPT_DECLARE_EMPTY_STORE_HOOKS(); \
	LTCKPT_ALWAYSINLINE LTCKPT_DECLARE_TOP_OF_THE_LOOP_HOOK() {} \

#define LTCKPT_DECLARE_UNSUPPORTED(M) \
	LTCKPT_DECLARE_BASELINE_HOOKS(); \
	LTCKPT_DECLARE_LATE_INIT_HOOK() { ltckpt_panic("%s", M); }

#ifdef LTCKPT_WINDOW_PROFILING
#define __HOOK(M) \
	void LTCKPT_HOOK(M, ltckpt_conf_setup)(); \
	void LTCKPT_HOOK(M, ltckpt_top_of_the_loop)(unsigned site_id); \
	void LTCKPT_HOOK(M, ltckpt_store_hook)(void *addr, unsigned site_id); \
	void LTCKPT_HOOK(M, ltckpt_memcpy_hook)(char *addr, size_t size, unsigned site_id); \
	\
	void LTCKPT_WEAK LTCKPT_HOOK(M, ltckpt_early_init_hook)(ltckpt_early_init_data_t *init_data); \
	void LTCKPT_WEAK LTCKPT_HOOK(M, ltckpt_late_init_hook)(); \
	int LTCKPT_WEAK LTCKPT_HOOK(M, ltckpt_restart_hook)(void *arg); \
	void LTCKPT_WEAK LTCKPT_HOOK(M, ltckpt_before_exit_hook)(int status); \
	void LTCKPT_WEAK LTCKPT_HOOK(M, ltckpt_ctx_print_hook)(); \
	void LTCKPT_WEAK LTCKPT_HOOK(M, ltckpt_ctx_clear_hook)(); \
	void LTCKPT_WEAK LTCKPT_HOOK(M, ltckpt_atpthread_create_child_hook)() \

#else /* LTKCPT_WINDOW_PROFILING */
#define __HOOK(M) \
	void LTCKPT_HOOK(M, ltckpt_conf_setup)(); \
	void LTCKPT_HOOK(M, ltckpt_top_of_the_loop)(); \
	void LTCKPT_HOOK(M, ltckpt_store_hook)(void *addr); \
	void LTCKPT_HOOK(M, ltckpt_memcpy_hook)(char *addr, size_t size); \
	\
	void LTCKPT_WEAK LTCKPT_HOOK(M, ltckpt_early_init_hook)(ltckpt_early_init_data_t *init_data); \
	void LTCKPT_WEAK LTCKPT_HOOK(M, ltckpt_late_init_hook)(); \
	int LTCKPT_WEAK LTCKPT_HOOK(M, ltckpt_restart_hook)(void *arg); \
	void LTCKPT_WEAK LTCKPT_HOOK(M, ltckpt_before_exit_hook)(int status); \
	void LTCKPT_WEAK LTCKPT_HOOK(M, ltckpt_ctx_print_hook)(); \
	void LTCKPT_WEAK LTCKPT_HOOK(M, ltckpt_ctx_clear_hook)(); \
	void LTCKPT_WEAK LTCKPT_HOOK(M, ltckpt_atpthread_create_child_hook)() \

#endif

#define __CONF(M) \
	.name = #M, \
	.top_of_the_loop_hook = &LTCKPT_HOOK(M, ltckpt_top_of_the_loop), \
	.store_hook = &LTCKPT_HOOK(M, ltckpt_store_hook), \
	.memcpy_hook = &LTCKPT_HOOK(M, ltckpt_memcpy_hook), \
	\
	.early_init_hook = &LTCKPT_HOOK(M, ltckpt_early_init_hook), \
	.late_init_hook = &LTCKPT_HOOK(M, ltckpt_late_init_hook), \
	.restart_hook = &LTCKPT_HOOK(M, ltckpt_restart_hook), \
	.before_exit_hook = &LTCKPT_HOOK(M, ltckpt_before_exit_hook), \
	.ctx_print_hook = &LTCKPT_HOOK(M, ltckpt_ctx_print_hook), \
	.ctx_clear_hook = &LTCKPT_HOOK(M, ltckpt_ctx_clear_hook), \
	.atpthread_create_child = &LTCKPT_HOOK(M, ltckpt_atpthread_create_child_hook)

/*
 * Include all the hooks definitions.
 */
LTKCPT_DEFINE_CHECKPOINT_HOOKS();

#endif

