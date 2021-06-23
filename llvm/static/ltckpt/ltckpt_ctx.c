#include "ltckpt_local.h"

ltckpt_ctx_t ltckpt_ctx_buff;
ltckpt_ctx_t *ltckpt_ctx = &ltckpt_ctx_buff;

void ltckpt_ctx_clear_default()
{
	CTX_CLEAR(errors);
	CTX_CLEAR(num_cows);
	CTX_CLEAR(num_checkpoints);
	CTX_CLEAR(num_aop_funcs);
	CTX_CLEAR(num_aop_tols);
}

void ltckpt_ctx_print_default()
{
	ltckpt_printf_force(CTX_LOG_FMT, CTX_LOG_ARGS);
}

void ltckpt_ctx_clear()
{
	if (CONF(ctx_clear_hook)) {
		CONF(ctx_clear_hook)();
		return;
	}

	ltckpt_ctx_clear_default();
}

void ltckpt_ctx_print()
{
	if (CONF(ctx_print_hook)) {
		CONF(ctx_print_hook)();
		return;
	}

	ltckpt_ctx_print_default();
}

void ltckpt_ctx_set(ltckpt_ctx_t *ctx)
{
	memcpy(ctx, ltckpt_ctx, sizeof(ltckpt_ctx_t));
	ltckpt_ctx = ctx;
}

void ltckpt_ctx_init()
{
	CTX(lazy_init) = util_env_parse_int("CP_LINIT", 1);
	CTX(skip_mmap) = util_env_parse_int("CP_NOMMAP", 1);
	CTX(atexit_dump) = util_env_parse_int("ATEXIT_DUMP", 0);
#ifdef LTCKPT_X86_64 
	/* Workaround: don't allow prctl on 64 bit by default */
	CTX(allow_prctl) = util_env_parse_int("CP_PRCTL", 0);
#else
	CTX(allow_prctl) = util_env_parse_int("CP_PRCTL", 1);
#endif
	CTX(checkpoint_interval) = util_env_parse_int("CP_INTERVAL", 1); /* 0 disables checkpointing. */
	CTX(page_statistic_enabled) = util_env_parse_int("PAGESTAT", 0);

	CTX(approach) = CONF(name);
}

void* ltkcpt_ctx_get_buff(void *addr, size_t len)
{
	char *buff;
	int flags = MAP_PRIVATE|MAP_ANON;

	/* Used to allocate the checkpointing context (and state) in a single
	 * mmapped chunk. We need this for whitelisting purposes.
	 * The 2 guard pages below are used to prevent VMA merging and
	 * allow parsing /proc/self/maps correctly.
	 */
	if (addr) {
		flags |= MAP_FIXED;
	}
	if (len % PAGE_SIZE) {
		len = len - (len % PAGE_SIZE) + PAGE_SIZE;
	}
	buff = mmap(addr, len+(2*PAGE_SIZE), PROT_READ|PROT_WRITE, flags, 0, 0);
	if (buff == MAP_FAILED) {
		return NULL;
	}
	mmap(buff, PAGE_SIZE, PROT_NONE, MAP_PRIVATE|MAP_ANON|MAP_FIXED, 0, 0);
	buff += PAGE_SIZE;
	mmap(buff+len, PAGE_SIZE, PROT_NONE, MAP_PRIVATE|MAP_ANON|MAP_FIXED, 0, 0);

	return buff;
}

