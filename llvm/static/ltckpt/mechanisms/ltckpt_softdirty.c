#define LTCKPT_CHECKPOINT_METHOD softdirty

#include "../ltckpt_local.h"
#include <common/ut/uthash.h>
LTCKPT_CHECKPOINT_METHOD_ONCE();
LTCKPT_DECLARE_EMPTY_STORE_HOOKS();

#define SOFTDIRTY_MAX_PAGES     1000
#define PAGE_STAT_SIZE          50000
#define PAGE_NUM_HIST_SIZE      200000

#define SOFTDIRTY_STAT(S)          (softdirty ? softdirty->S : 0)

static void ltckpt_init_softdirty();

typedef struct pagestat_s {
	void     *addr;
	unsigned count;
	UT_hash_handle hh;
} pagestat_t;


typedef struct {
    char mem[SOFTDIRTY_MAX_PAGES*PAGE_SIZE];
	unsigned long num_mem_pages;
    util_proc_maps_t proc_maps;
    util_pagemap_t pagemap;
    
	/* for statistics */
	pagestat_t page_statistics_mem[PAGE_STAT_SIZE];
	unsigned long pagestat_pos;
	pagestat_t *page_statistics;
	unsigned long page_num_hist[PAGE_NUM_HIST_SIZE];
	unsigned long page_num_hist_len;
	int stats_enabled;
	
	ltckpt_ctx_t ctx;
} softdirty_t;


static softdirty_t *softdirty;


static int ltckpt_top_of_the_loop_cb(u64_t *entry, void *addr, void *cb_args);

static int ltckpt_top_of_the_loop_stat_cb(u64_t *entry, void *addr, void *cb_args)
{
	pagestat_t *stat = NULL; 
	HASH_FIND_PTR(softdirty->page_statistics, &addr, stat);
	if (!stat) {
		if ( softdirty->pagestat_pos >= PAGE_STAT_SIZE ) {
			ltckpt_panic("Out of statistics memory\n");
		}
		stat = &softdirty->page_statistics_mem[softdirty->pagestat_pos++];
		stat->count = 0;
		stat->addr  = addr;
		HASH_ADD_PTR(softdirty->page_statistics, addr, stat);
	}
	stat->count++;
	return ltckpt_top_of_the_loop_cb(entry, addr, cb_args);
}

LTCKPT_DECLARE_CTX_PRINT_HOOK() 
{
	int i;
	if (CTX(initialized)) {
		for ( i=0; i < softdirty->pagestat_pos ; i++) { 
			ltckpt_printf_force("CTX: STAT_PAGE_COUNT: %p = %d\n", 
					softdirty->page_statistics_mem[i].addr, 
					softdirty->page_statistics_mem[i].count
					);
		}
		for ( i=0; i < softdirty->page_num_hist_len; i++ ) {
			ltckpt_printf_force("CTX: STAT_PAGE_NUM: %d = %d\n",
					i, softdirty->page_num_hist[i]
					);
		}
	}
	ltckpt_ctx_print_default();
}

static int ltckpt_top_of_the_loop_cb(u64_t *entry, void *addr, void *cb_args)
{
	(void)(cb_args);

	ltckpt_printf("ltckpt: [ckpt=%lu] Saving dirty page @%p (0x%032llx)\n", CTX(num_checkpoints), addr, *entry);

	assert(softdirty->num_mem_pages < SOFTDIRTY_MAX_PAGES);
	memcpy(&softdirty->mem[softdirty->num_mem_pages*PAGE_SIZE], addr, PAGE_SIZE);
	softdirty->num_mem_pages++;
	CTX_INC(num_cows);

	return 0;
}

LTCKPT_DECLARE_TOP_OF_THE_LOOP_HOOK()
{
	int ret;

	CTX_NEW_TOL_OR_RETURN();

	if (!CTX(initialized)) {
		ltckpt_init_softdirty();
	}
	if (softdirty->stats_enabled) {
		softdirty->page_num_hist[softdirty->page_num_hist_len++] = softdirty->num_mem_pages;
		if (softdirty->page_num_hist_len >= PAGE_NUM_HIST_SIZE )
			softdirty->page_num_hist_len = 0;
	}
	softdirty->num_mem_pages = 0;
	
	if (softdirty->stats_enabled) {
		ret = util_pagemap_proc_walk(&softdirty->pagemap,
			&softdirty->proc_maps, PME_SOFT_DIRTY,
			ltckpt_top_of_the_loop_stat_cb, NULL);
	} else {
		ret = util_pagemap_proc_walk(&softdirty->pagemap,
			&softdirty->proc_maps, PME_SOFT_DIRTY,
			ltckpt_top_of_the_loop_cb, NULL);
	}

	if (ret < 0)
		ltckpt_panic("util_pagemap_proc_walk failed: %d %s", ret, strerror(errno));
	ret = util_pagemap_clear_refs(&softdirty->pagemap, CR_SOFTDIRTY);
	if (ret < 0)
		ltckpt_panic("util_pagemap_clear_refs failed: %d %s", ret, strerror(errno));

	CTX_NEW_CHECKPOINT();
}

int ltckpt_init_softdirty_cb(util_proc_maps_entry_t *entry, void *cb_args)
{
	(void)(cb_args);
	if (entry->vm_start == (unsigned long) softdirty) {
		/* Whitelist our own internal state. */
		return UTIL_PROC_MAPS_RET_CONTINUE;
	}
	if (!ltcpt_is_checkpointed_vma(entry)) {
		return UTIL_PROC_MAPS_RET_CONTINUE;
	}

	return UTIL_PROC_MAPS_RET_SAVE;
}


static void ltckpt_init_softdirty()
{
	int ret;
	char *buff;
	size_t buff_len;
	
	/* Map our entire internal state in 1 mmapped memory chunk. */
	buff_len = sizeof(softdirty_t)+UTIL_PAGEMMAP_DEFAULT_BUFF_SIZE;
	buff = ltkcpt_ctx_get_buff(MIN_MMAP_ADDR, buff_len);
	softdirty = (softdirty_t*) buff;
	buff += sizeof(softdirty_t);
	ltckpt_ctx_set(&softdirty->ctx);
	CTX(initialized) = 1;

	softdirty->stats_enabled = CTX(page_statistic_enabled);
	softdirty->pagestat_pos=0;
	softdirty->page_num_hist_len=0;
	softdirty->page_statistics=NULL;


	ret = util_proc_maps_parse_filter(getpid(), &softdirty->proc_maps,
		ltckpt_init_softdirty_cb, NULL);
	if (ret)
		ltckpt_panic("util_proc_maps_parse_filter failed: %d %s", ret, strerror(errno));
	ret = util_pagemap_init(getpid(), &softdirty->pagemap,
		buff, UTIL_PAGEMMAP_DEFAULT_BUFF_SIZE);
	if (ret)
		ltckpt_panic("util_pagemap_init failed: %d %s", ret, strerror(errno));
	ret = util_pagemap_clear_refs(&softdirty->pagemap, CR_SOFTDIRTY);
	if (ret < 0)
		ltckpt_panic("util_pagemap_clear_refs failed: %d %s", ret, strerror(errno));

	if (LTCKPT_IS_VERBOSE()) {
		ltckpt_printf("ltckpt: Using softdirty @%p, /proc/self/maps:\n", softdirty);
		util_proc_maps_print(&softdirty->proc_maps);
	}
}


LTCKPT_DECLARE_LATE_INIT_HOOK()
{
	if (!CTX(lazy_init) && CTX(checkpoint_interval)) {
		ltckpt_init_softdirty();
	}
}

