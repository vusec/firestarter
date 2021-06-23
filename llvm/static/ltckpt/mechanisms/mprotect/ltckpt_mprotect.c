#define LTCKPT_CHECKPOINT_METHOD mprotect

#include "../../ltckpt_local.h"
LTCKPT_CHECKPOINT_METHOD_ONCE();
LTCKPT_DECLARE_EMPTY_STORE_HOOKS();

#include <signal.h>
#include <sys/syscall.h>

#define MPROTECT_MAX_PAGES     1000

#define LTCKPT_MPROTECT_SAFE(B) do { \
	int initialized = CTX(initialized); \
	if (initialized) \
		ltckpt_mprotect_vmas(1, 1); \
	B \
	if (initialized) \
		ltckpt_mprotect_vmas(0, 1); \
} while(0)

static void ltckpt_init_mpr();

typedef struct mpr_page_s {
	void *addr;
	void *mem;
} mpr_page_t;

typedef struct {
	mpr_page_t pages[MPROTECT_MAX_PAGES];
	unsigned long num_pages;

	mpr_page_t iter_pages[MPROTECT_MAX_PAGES];
	unsigned long num_iter_pages;

	char mem[MPROTECT_MAX_PAGES*PAGE_SIZE];
	unsigned long num_mem_pages;

	util_proc_maps_t proc_maps;
	ltckpt_ctx_t ctx;
} mpr_t;

static mpr_t *mpr;

static void ltckpt_mem_flush()
{
	mpr->num_mem_pages = 0;
}

void ltckpt_page_list_add(mpr_page_t *page)
{
	mpr_page_t *new_page;
	char *new_mem;

	assert(mpr->num_pages < MPROTECT_MAX_PAGES);
	new_page = &mpr->pages[mpr->num_pages++];
	new_page->addr = page->addr;

	assert(mpr->num_mem_pages < MPROTECT_MAX_PAGES);
	new_mem = &mpr->mem[mpr->num_mem_pages*PAGE_SIZE];
	new_page->mem = new_mem;
	memcpy(new_page->mem, (void*)new_page->addr, PAGE_SIZE);
	mpr->num_mem_pages++;
}

void ltckpt_page_list_clear_and_iter(mpr_page_t **iter)
{
	*iter = mpr->iter_pages;
	mpr->num_iter_pages = mpr->num_pages;
	memcpy(mpr->iter_pages, mpr->pages, mpr->num_iter_pages*sizeof(mpr_page_t));
	mpr->num_pages = 0;
}

int ltckpt_page_list_iter_next(mpr_page_t *page, mpr_page_t **iter)
{
	int index = *iter - mpr->iter_pages;

	if (index >= mpr->num_iter_pages) {
		return 0;
	}

	*page = mpr->iter_pages[index];
	(*iter)++;
	return 1;
}

static inline int ltckpt_mprotect_mem(void *addr, size_t len, int writable)
{
	int ret, prot;

	prot = writable ? PROT_READ|PROT_WRITE : PROT_READ;
	ret = syscall(SYS_mprotect, addr, len, prot);
	if (ret) {
		ltckpt_panic("mprotect failed: %d (err=%d, addr=%p, len=%lu)",
			ret, errno, addr, (unsigned long) len);
	}
	return ret;
}

static void ltckpt_mprotect_vma(util_proc_maps_entry_t *entry, int writable,
	int quiet)
{
	int ret;
	void *addr;
	size_t len;

	if (!entry->s) {
		/* We need to skip this VMA. */
		return;
	}
	addr = (void*) entry->vm_start;
	len = entry->vm_end - entry->vm_start;

	ret = ltckpt_mprotect_mem(addr, len, writable);
	if (ret) {
		entry->s = 0;
		if (!quiet) {
			ltckpt_printf_error("ERROR: mprotect failed: %d (err=%d, addr=%p, len=%lu), skipping VMA...\n",
				ret, errno, addr, (unsigned long) len);
		}
	}
}

static void ltckpt_mprotect_vmas(int writable, int quiet)
{
	int i;

	for (i=0;i<mpr->proc_maps.num_entries;i++) {
	    util_proc_maps_entry_t *entry = &mpr->proc_maps.entries[i];
	    ltckpt_mprotect_vma(entry, writable, quiet);
	}
}

static void ltckpt_sighandler(int sig, siginfo_t *si, void *unused)
{
	char *addr;
	mpr_page_t page;

	CTX(pgfault_nesting_level)++;
	/* XXX: Check that the faulting address is valid. */
	addr = (char*) si->si_addr;
	addr -= (((unsigned long) addr) % PAGE_SIZE);
	page.addr = addr;
	ltckpt_page_list_add(&page);

	CTX_INC(num_cows);
	ltckpt_mprotect_mem(addr, PAGE_SIZE, 1);

	if (CTX(pgfault_nesting_level) == 1) {
		ltckpt_printf("ltckpt: [ckpt=%lu] COW @%p\n",
			CTX(num_checkpoints), page.addr);
	}
	CTX(pgfault_nesting_level)--;
}

static inline void ltckpt_checkpoint()
{
	mpr_page_t page;
	mpr_page_t *iter;

	ltckpt_page_list_clear_and_iter(&iter);
	while (ltckpt_page_list_iter_next(&page, &iter)) {
		ltckpt_mprotect_mem(page.addr, PAGE_SIZE, 0);
	}
	ltckpt_mem_flush();
}

LTCKPT_DECLARE_TOP_OF_THE_LOOP_HOOK()
{
	CTX_NEW_TOL_OR_RETURN();

	if (!CTX(initialized)) {
		ltckpt_init_mpr();
	}

	ltckpt_checkpoint();

	CTX_NEW_CHECKPOINT();
}

int ltckpt_init_mpr_cb(util_proc_maps_entry_t *entry, void *cb_args)
{
	(void)(cb_args);
	if (entry->vm_start == (unsigned long) mpr) {
		/* Whitelist our own internal state. */
		return UTIL_PROC_MAPS_RET_CONTINUE;
	}
	if (!ltcpt_is_checkpointed_vma(entry)) {
		return UTIL_PROC_MAPS_RET_CONTINUE;
	}

	return UTIL_PROC_MAPS_RET_SAVE;
}

static void ltckpt_init_mpr()
{
	int ret;
	size_t buff_len;
	struct sigaction sa;
	sigset_t sigset;

	/* Map our entire internal state in 1 mmapped memory chunk. */
	buff_len = sizeof(mpr_t);
	mpr = (mpr_t*) ltkcpt_ctx_get_buff(MIN_MMAP_ADDR, buff_len);
	ltckpt_ctx_set(&mpr->ctx);
	CTX(initialized) = 1;

	ret = util_proc_maps_parse_filter(getpid(), &mpr->proc_maps,
		ltckpt_init_mpr_cb, NULL);
	if (ret)
		ltckpt_panic("util_proc_maps_parse_filter failed: %d", ret);

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGSEGV);
	ret = pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
	if (ret)
		ltckpt_panic("pthread_sigmask failed: %d", ret);
	sa.sa_flags = SA_SIGINFO|SA_NODEFER|SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = ltckpt_sighandler;
	ret = sigaction(SIGSEGV, &sa, NULL);
	if (ret)
		ltckpt_panic("sigaction failed: %d", ret);

	if (LTCKPT_IS_VERBOSE()) {
		ltckpt_printf("ltckpt: Using mpr @%p, /proc/self/maps:\n", mpr);
		util_proc_maps_print(&mpr->proc_maps);
		ltckpt_printf("\nltckpt: Original /proc/self/maps was:\n");
		util_proc_maps_dump(getpid());
	}

	ltckpt_mprotect_vmas(0, 0);
}

LTCKPT_DECLARE_LATE_INIT_HOOK()
{
	if (!CTX(lazy_init) && CTX(checkpoint_interval)) {
		ltckpt_init_mpr();
	}
}

LTCKPT_DECLARE_CTX_PRINT_HOOK()
{
	LTCKPT_MPROTECT_SAFE(
		ltckpt_ctx_print_default();
	);
}

