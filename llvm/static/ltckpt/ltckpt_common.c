#include "ltckpt_common.h"

#ifdef __MINIX
#include <minix/sef.h>
#endif 

#ifdef LTCKPT_WINDOW_PROFILING
#include "ltckpt_profiling.h"
uint32_t ltckpt_current_site_id = 0;
ltckpt_window_t ltckpt_windows[LTCKPT_MAX_PROFILING_WINDOWS];   // per-site window profiles
FILE *ltckpt_log_fptr = NULL;

void ltckpt_windows_dump()
{
    ltckpt_printf("Dumping ltckpt profiling data.\n");
    if (NULL == ltckpt_log_fptr) {
	ltckpt_log_fptr = fopen(LTCKPT_PROFILING_OUTFILE, "w+");
    }
    char wtype_names[5][10] = { "INVALID", "UNDOLOG", "TSX", "TSX_END", ""};

    ltckpt_fscribe("site_id,type,hits,fails,max_retries,,num_stores,max_stores,memcpy_size,memcpy_max_size,undolog_size_avg,undolog_size_max\n");
    for (unsigned i=0; i < LTCKPT_MAX_PROFILING_WINDOWS; i++) {
	ltckpt_window_t *p = &ltckpt_windows[i];
        ltckpt_fscribe("%4d,%8s,%8lu,%8lu,%8lu,,%8lu,%8lu,%8lu,%16lu,%16.4f,%16lu\n",
			i, wtype_names[p->type], p->g_num_hits, p->g_num_fails, p->g_num_retries_max,
			p->g_num_stores, p->g_num_stores_max,
			p->g_memcpy_size, p->g_memcpy_size_max,
			p->g_undolog_size_avg, p->g_undolog_size_max);
    }
    fflush(ltckpt_log_fptr);
}
#endif

void ltckpt_recovery_cleanup()
{
#ifdef __MINIX
	int replyable = 0;
	message msg;
	endpoint_t src;

	if(sef_get_current_message(&msg, &replyable)) {
		if(replyable) {
			src = msg.m_source;
			memset(&msg, ~0, sizeof(msg));
			/* kindof sure that the requester is still there */
			ipc_send(src, &msg); 
		}
	}
#endif
}
ltckpt_checkpoint_conf_t ltckpt_confs[] = LTKCPT_CHECKPOINT_CONFS_INITIALIZER;
volatile ltckpt_checkpoint_conf_t* ltckpt_conf;

/*
 * Configuration. Only called in instrumented programs.
 */
void LTCKPT_NOINLINE LTCKPT_INSTFUNCT ltckpt_conf_setup()
{
	/* Filled by instrumentation. Called at preinit time. */
	if (!ltckpt_conf) {
		ltckpt_conf_setup_from_hook("baseline");
	}
}

void LTCKPT_NOINLINE LTCKPT_INSTFUNCT ltckpt_conf_setup_from_hook(const char *name)
{
	int i = 0;

	// ltckpt_conf could already be set by constructor to default value
	while (ltckpt_confs[i].name) {
		if (!strcmp(name, ltckpt_confs[i].name)) {
			ltckpt_conf = &ltckpt_confs[i];
			break;
		}
		i++;
	}
	assert(ltckpt_conf);
}

/*
 * Output-handling functions.
 */
#ifndef __MINIX

#include <pthread.h>

static void ltckpt_output_atfork_child();

static void ltckpt_output_init()
{
	util_output_conf_t *output_conf = &CTX(output_conf);

	output_conf->id = getpid();
	output_conf->file = _UTIL_OUTPUT_NAME_TO_FILE("ltckpt");
	output_conf->level = LTCKPT_OUTPUT_BASIC;
	util_output_from_env(output_conf);
	util_output_init(output_conf);
	pthread_atfork(NULL, NULL, ltckpt_output_atfork_child);
}

static void ltckpt_output_atfork_child()
{
	util_output_conf_t *output_conf = &CTX(output_conf);

        util_output_close_child(output_conf);
        ltckpt_output_init();
}
#else
#define ltckpt_output_init()
#endif

/*
 * Init functions.
 */
void ltckpt_common_early_init()
{
	extern void ltckpt_ctx_init();
	static int early_init_done = 0;

	if (!early_init_done) {
		ltckpt_ctx_init();
		early_init_done = 1;
	}
}

void __attribute__((constructor)) ltckpt_common_late_init()
{
	/* Get configuration from the pass if early init did not do that already. */
	ltckpt_conf_setup();

	ltckpt_common_early_init();
	ltckpt_output_init();

	if (CONF(late_init_hook)) {
		ltckpt_debug_print("calling mechanisms init_hook\n");
		CONF(late_init_hook)();
	} else {
		ltckpt_debug_print("no mechanism specific late init function defined\n");
	}
}

extern int inside_trusted_compute_base;

int ltckpt_restart(void *arg)
{
	inside_trusted_compute_base = 1;
	if (CONF(restart_hook)) {
		ltckpt_debug_print("calling mechanism-specific restart hook\n");
		return CONF(restart_hook)(arg);
	}

	ltckpt_debug_print("no mechanism-specific restart hook defined\n");
	return ENOENT;
}

__thread jmp_buf ltckpt_registers;
__thread jmp_buf *ltckpt_registers_ptr = NULL;

void ltckpt_asm_save_registers_type(jmp_buf *buf, int val)
{

}

int ltckpt_setjmp_save_registers_type(jmp_buf buf)
{
	return 0;
}

LTCKPT_ALWAYSINLINE void ltckpt_save_registers()
{
#ifdef LTCKPT_REGISTERS
    CTX_SAVE_REGISTERS(ltckpt_registers);
#endif
}

LTCKPT_ALWAYSINLINE void ltckpt_restore_registers()
{
#ifdef LTCKPT_REGISTERS
    CTX_RESTORE_REGISTERS(ltckpt_registers);
#endif
}

int ltckpt_mechanism_enabled()
{
	return ltckpt_conf && strcmp(CONF(name), "baseline");
}

int ltckpt_is_mapped(void *addr)
{
	int is_mapped;
	size_t len = 4096;

	ltckpt_va_t ret = ltckpt_mmap(LTCKPT_PTR_TO_VA(addr), len,
		PROT_WRITE | PROT_READ,
		LTCKPT_MAP_PRIVATE | LTCKPT_MAP_NORESERVE);
	is_mapped = (ret != LTCKPT_PTR_TO_VA(addr));
	if (ret != LTCKPT_MAP_FAILED) {
		ltckpt_munmap(ret, len);
	}

	return is_mapped;
}

/*
 * Memory management functions.
 */
/* mmap_wrapper */
ltckpt_va_t ltckpt_mmap(ltckpt_va_t addr, size_t size, int prot, unsigned int flags)
{
	int mmap_flags = 0;
	int mmap_prot  = 0;
	void *mmap_ret;

	ltckpt_va_t ret;

	if (flags & LTCKPT_MAP_FIXED) {
		mmap_flags |= MAP_FIXED;
	}

	if (flags & LTCKPT_MAP_POPULATE) {
#ifdef __MINIX
		mmap_flags |= MAP_PREALLOC;
#else
		mmap_flags |= MAP_POPULATE;
#endif
	}

	if (flags & LTCKPT_MAP_PRIVATE) {
		mmap_flags |= MAP_PRIVATE;
	}

	if (flags & LTCKPT_MAP_NORESERVE) {
		mmap_flags |= MAP_NORESERVE;
	}

	if (flags & LTCKPT_MAP_STACK) {
		mmap_flags |= MAP_STACK;
	}

	mmap_flags |= MAP_ANONYMOUS;

	if (prot & LTCKPT_PROT_R) {
		mmap_prot |= PROT_READ;
	}

	if (prot & LTCKPT_PROT_W) {
		mmap_prot |= PROT_WRITE;
	}

	if (prot & LTCKPT_PROT_X) {
		mmap_prot |= PROT_EXEC;
	}

	mmap_ret = mmap(LTCKPT_VA_TO_PTR(addr), size, mmap_prot,
		mmap_flags, -1,0);

	ltckpt_debug_print("mmap(%p, 0x%lx, 0x%x, 0x%x, 0, 0) = %p\n",
		LTCKPT_VA_TO_PTR(addr), (unsigned long) size, mmap_prot,
		mmap_flags, mmap_ret);

	if (mmap_ret == MAP_FAILED) {
		ret = LTCKPT_MAP_FAILED;
	} else {
		ret = LTCKPT_PTR_TO_VA(mmap_ret);
	}
	return ret;
}

int ltckpt_munmap(ltckpt_va_t start, size_t size)
{
	return munmap(LTCKPT_VA_TO_PTR(start), size);
}

#ifndef __MINIX
int ltckpt_mprotect(ltckpt_va_t start, size_t size, int prot)
{
	int mp_prot = 0;

	if (prot & LTCKPT_PROT_R) {
		mp_prot |= PROT_READ;
	}

	if (prot & LTCKPT_PROT_W) {
		mp_prot |= PROT_WRITE;
	}

	if (prot & LTCKPT_PROT_X) {
		mp_prot |= PROT_EXEC;
	}

	return mprotect(LTCKPT_VA_TO_PTR(start), size, mp_prot);
}

int ltcpt_is_checkpointed_vma(util_proc_maps_entry_t *entry)
{
	if (UTIL_PROC_MAPS_ENTRY_IS_STACK(entry)
		|| UTIL_PROC_MAPS_ENTRY_IS_TLS(entry)
		|| entry->w != 'w') {
		return 0;
	}
	if (UTIL_PROC_MAPS_ENTRY_NAME_CONTAINS(entry, "libltckpt")) {
		return 0;
	}
	if (CTX(skip_mmap)) {
		if (!UTIL_PROC_MAPS_ENTRY_IS_HEAP(entry)
			&& !UTIL_PROC_MAPS_ENTRY_IS_PROG_DATA(entry)) {
			return 0;
		}
	}

	return 1;
}
#endif

