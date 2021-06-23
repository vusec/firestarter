#ifndef LTCKPT_CTX_H
#define LTCKPT_CTX_H

#include <syslog.h>
#include <unistd.h>

typedef struct ltckpt_ctx_s {
	int lazy_init;
	int skip_mmap;
	int initialized;
	int atexit_dump;
	int exited;
	int allow_prctl;
	int checkpoint_interval;
	int pgfault_nesting_level;
	int page_statistic_enabled;
	util_output_conf_t output_conf;
	const char *approach;
	unsigned long errors;
	unsigned long num_cows;
	unsigned long num_tols;
	unsigned long num_checkpoints;
	unsigned long max_log_size;
	unsigned long num_aop_funcs;
	unsigned long num_aop_tols;
} ltckpt_ctx_t;

extern ltckpt_ctx_t *ltckpt_ctx;

#define CTX(X)       (ltckpt_ctx->X)
#define CTX_INC(X)   CTX(X)++
#define CTX_CLEAR(X) CTX(X)=0

#define CTX_LOG_FMT \
	"CTX: { pid=%d, approach=%s, errors=%lu, num_cows=%lu, num_tols=%lu, num_checkpoints=%lu, max_log_size=%lu, num_aop_funcs=%lu, num_aop_tols=%lu }\n"

#define CTX_LOG_ARGS \
	(int) getpid(), CTX(approach), CTX(errors), CTX(num_cows), CTX(num_tols), \
	CTX(num_checkpoints), CTX(max_log_size), CTX(num_aop_funcs), \
	CTX(num_aop_tols)

#define CTX_NEW_TOL_OR_RETURN() do { \
	if (!CTX(checkpoint_interval) || (CTX(num_tols) % CTX(checkpoint_interval) > 0)) { \
		CTX_INC(num_tols); \
		return; \
	} \
	CTX_INC(num_tols); \
} while(0)

#define CTX_NEW_CHECKPOINT() do { \
	CTX_INC(num_checkpoints); \
} while(0)

#define CTX_NEW_LOG_SIZE(LS) do { \
	if (LS > CTX(max_log_size)) { \
		CTX(max_log_size) = LS; \
	} \
} while(0)

#ifdef LTCKPT_SETJMP
#define CTX_SAVE_REGISTERS(JB) do {	\
	if (0 != sigsetjmp(JB, 1)) {	\
	; 				\
	}				\
} while (0)

#define CTX_RESTORE_REGISTERS(JB) do {	\
	siglongjmp(JB, 1);		\
} while (0)

#else /* LTCKPT_SETJMP */
#define CTX_SAVE_REGISTERS(JB) do { 		\
	ltckpt_asm_save_registers(JB, 1); 	\
} while (0)

#define CTX_RESTORE_REGISTERS(JB) do { 		\
        ltckpt_asm_restore_registers(JB, 1); 	\
} while (0)
#endif

/*
 * Sample usage:
 * ./clientctl dumpcp pids
 * ./clientctl bench
 * ./clientctl dumpcp pids
 */
void ltckpt_ctx_clear() __attribute__((used));
void ltckpt_ctx_print() __attribute__((used));
void ltckpt_ctx_clear_default();
void ltckpt_ctx_print_default();

void ltckpt_ctx_set(ltckpt_ctx_t *ctx);
void* ltkcpt_ctx_get_buff(void *addr, size_t len);

#endif /* LTCKPT_CTX_H */

