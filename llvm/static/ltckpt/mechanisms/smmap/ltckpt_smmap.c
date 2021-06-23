#define LTCKPT_CHECKPOINT_METHOD smmap

#include "../../ltckpt_local.h"
LTCKPT_CHECKPOINT_METHOD_ONCE();
LTCKPT_DECLARE_EMPTY_STORE_HOOKS();

#include <common/util/util_def.h>
#include <smmap/smmap.h>
#include <pthread.h>

#ifdef LTCKPT_X86
#define STACK_SIZE (1024*1024*6)
#define SIZE_3G    (1024UL*1024*1024*3)
#define STACK_POS  (SIZE_3G/2-STACK_SIZE)
#define SHADOW_FROM ((void*)0)
#define SHADOW_TO ((void*)(SIZE_3G/2))
#define SHADOW_SIZE ((SIZE_3G/2))
#endif

#ifdef LTCKPT_X86_64
#define STACK_SIZE (1024*1024*6)
#define SHADOW_FROM ((void *)(0x7f0000000000))
#define SHADOW_SIZE          (0x010000000000)
#define SHADOW_TO  ((void*) 0x010000000000)
#define STACK_SIZE (1024*1024*6)
#define SIZE_3G    (1024UL*1024*1024*3)
#define STACK_POS  (0x7f0000000000-STACK_SIZE)
#endif

EARLY_INIT_CHANGE_STACK(STACK_POS, STACK_SIZE);

static void ltckpt_init_smmap();

LTCKPT_DECLARE_TOP_OF_THE_LOOP_HOOK()
{
	int ret;

	CTX_NEW_TOL_OR_RETURN();

	if (!CTX(initialized)) {
		ltckpt_init_smmap();
	}

	ret = smctl(SMMAP_SMCTL_CHECKPOINT, 0);
	if (ret < 0)
		ltckpt_panic("smctl failed: %d", ret);

	CTX_NEW_CHECKPOINT();
}

static void ltckpt_init_smmap()
{
	int ret;
	ltckpt_ctx_t *ctx;
	util_proc_maps_t maps;
	util_proc_maps_info_t info;
	char *addr = SHADOW_FROM;
	char *shadow_addr = SHADOW_TO;
	size_t size = SHADOW_SIZE;

	ctx = ltkcpt_ctx_get_buff(MIN_MMAP_ADDR, sizeof(ltckpt_ctx_t));
	assert(ctx);
	assert(sizeof(ltckpt_ctx_t) <= PAGE_SIZE);
	ltckpt_ctx_set(ctx);
	CTX(initialized) = 1;

	if (CTX(skip_mmap)) {
		ret = util_proc_maps_parse(getpid(), &maps);
		if (ret != 0) {
			ltckpt_panic("util_proc_maps_parse failed: %d\n", ret);
		}
		util_proc_maps_get_info(&maps, &info);
		addr = (char*) info.prog.vm_start;
		size = (char*) info.heap->vm_end - addr;
		util_proc_maps_destroy(&maps);
	}
	if ((char*)ctx >= addr) {
		size_t offset = ((char*)ctx)+2*PAGE_SIZE - addr;
		size -= offset;
		addr += offset;
	}
	ret = smmap(addr, shadow_addr, size);
	if (ret < 0) {
		ltckpt_panic("smmap(addr=%p, to=%p, size %zd) failed: %d errno:%d\n",
			addr, shadow_addr,size, ret, errno);
	}
}

LTCKPT_DECLARE_LATE_INIT_HOOK()
{
	if (!CTX(lazy_init)) {
		pthread_atfork(NULL,NULL, ltckpt_init_smmap);
		ltckpt_init_smmap();
	}
}

