#define LTCKPT_CHECKPOINT_METHOD dune

#include "../../ltckpt_local.h"
LTCKPT_CHECKPOINT_METHOD_ONCE();

#ifndef HAVE_LIBDUNE
LTCKPT_DECLARE_UNSUPPORTED("ltckpt_dune: libdune not found!");

#else

LTCKPT_DECLARE_EMPTY_STORE_HOOKS();

#define DSMMAP_ALLOW_WEAK 1
#include <libdune/dsmmap.h>
#include <limits.h>
#include <stdlib.h>
#include <pthread.h>

void ltckpt_init_dsmmap();

LTCKPT_DECLARE_TOP_OF_THE_LOOP_HOOK()
{
	if (dsmmap_available()) {
		CTX(num_cows) += DSMMAP_STAT(num_cows);
		DSMMAP_STAT(num_cows) = 0;
	}
	CTX_NEW_TOL_OR_RETURN();

	if (!CTX(initialized)) {
		ltckpt_init_dsmmap();
	}

	dsmctl(DSMMAP_DSMCTL_CHECKPOINT, NULL);
	CTX_NEW_CHECKPOINT();
}

int ltckpt_dsmmap_printf(const char *fmt, ...)
{
	va_list args;
	char buf[1024];

	va_start(args, fmt);

	vsnprintf(buf, 1023, fmt, args);

	ltckpt_printf_force("%s", buf);
	return 0;
}

void ltckpt_init_dsmmap()
{
	int ret;
	util_proc_maps_t maps;
	util_proc_maps_info_t info;
	unsigned long start, size;
	void *min_mmap_addr = MIN_MMAP_ADDR;
	ltckpt_ctx_t *ctx = ltkcpt_ctx_get_buff(min_mmap_addr,
		sizeof(ltckpt_ctx_t));
	assert(ctx);
	assert(sizeof(ltckpt_ctx_t) <= PAGE_SIZE);
	ltckpt_ctx_set(ctx);
	CTX(initialized) = 1;

	dsmmap_set_debug_enabled(LTCKPT_IS_VERBOSE());
	dsmmap_set_printf(ltckpt_dsmmap_printf);
	ret = dsmmap_init();
	assert(ret == 0);

	if (CTX(skip_mmap)) {
		ret = util_proc_maps_parse(getpid(), &maps);
		if (ret != 0) {
			ltckpt_panic("util_proc_maps_parse failed: %d\n", ret);
		}
		util_proc_maps_get_info(&maps, &info);
		start = info.prog.vm_start;
		size = info.heap->vm_end - start;
		util_proc_maps_destroy(&maps);
	}
	else {
		start = ((unsigned long) ctx) + PAGE_SIZE*2;
		size = ULONG_MAX-start;
	}
	ret = dsmmap((void*) start, size);
	assert(ret == 0);
}

LTCKPT_DECLARE_LATE_INIT_HOOK()
{
	if (!dsmmap_available()) {
		/* Libdune not linked in. */
		ltckpt_printf_force("ltckpt_dune: libdune not linked in.");
		CTX(checkpoint_interval) = 0;
		return;
	}
	if (!CTX(lazy_init) && CTX(checkpoint_interval)) {
		ltckpt_init_dsmmap();
	}
}

#endif
