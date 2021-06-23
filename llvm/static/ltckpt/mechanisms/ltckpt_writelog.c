#define LTCKPT_CHECKPOINT_METHOD undolog
#include "../ltckpt_local.h"
#include "../ltckpt_recover.h"
LTCKPT_CHECKPOINT_METHOD_ONCE();

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <setjmp.h>

#ifdef LTCKPT_TEST_TSX_OV
#include <immintrin.h>
#endif 

#ifdef LTCKPT_WINDOW_PROFILING
#include "../ltckpt_profiling.h"
#endif
#include "../../rcvry/rcvry_util.h"
#include "../../rcvry/rcvry_dispatch.h"
#include "../../rcvry/rcvry_actions.h"

#ifndef __MINIX
#define WRITELOG_START       0
#define WRITELOG_FLAGS       0
#ifdef LTCKPT_X86_64
#define WRITELOG_MAXLEN      (1024*1024*1024*4L*10)
#else  /* !LTCKPT_X86_64 */
#define WRITELOG_MAXLEN      (1024*1024*128L)
#endif /* LTCKPT_X86_64 */
#define LTCKPT_RESTARTING()  0
#define LTCKPT_IS_VM()       0
#define LTCKPT_IS_RS()       0

#else /* __MINIX */
#include <minix/sef.h>
#include <minix/sysutil.h>
#include <minix/endpoint.h>
#include <minix/syslib.h>
#define WRITELOG_START       (1536*1024*1024)
#define WRITELOG_FLAGS       LTCKPT_MAP_FIXED
#define WRITELOG_MAXLEN      (1024*1024*2)

static endpoint_t i_am_ep;
static char i_am_name[8];


extern __attribute__((weak)) int   vm_is_hole(uint32_t addr, int pages);
extern __attribute__((weak)) void *vm_allocpages_at(uint32_t addr, size_t size);
extern __attribute__((weak)) int copy_rs_start(void *, ...);
#define LTCKPT_IS_VM() (vm_is_hole)
#define LTCKPT_IS_RS() (copy_rs_start)
#endif /* __MINIX */


#define WRITELOG_GRANULARITY         (8) /* the sizeof writelog entries in bytes */
#define WRITELOG_BYTES          (WRITELOG_MAXLEN*(WRITELOG_GRANULARITY+sizeof(void*)))

#ifdef WRITELOG_PER_THREAD

static __thread char *wl_data = NULL;
static __thread char *wl_stkchk = NULL;
static __thread unsigned long wl_position=0, wl_stkchk_pos=0;
static __thread sigjmp_buf ltckpt_registers = NULL;

LTCKPT_DECLARE_ATPTHREAD_CREATE_CHILD_HOOK()
{
	ltckpt_init_writelog();
}

#else

static char *wl_data = NULL;
static char __attribute__((unused)) *wl_stkchk = NULL;
volatile void *ltckpt_fp_addr1 = NULL, *ltckpt_fp_addr2 = NULL;
static unsigned long __attribute((unused)) wl_position_high_watermark, wl_stkchk_pos;
static unsigned long wl_position;

#endif

#ifdef LTCKPT_ALWAYS_ON
#define LTCKPT_WRITELOG_ALWAYS_ON LTCKPT_ALWAYS_ON
#endif

#ifndef LTCKPT_WRITELOG_ALWAYS_ON
#define LTCKPT_WRITELOG_ALWAYS_ON 0
#endif

#ifdef __MINIX
#if LTCKPT_WRITELOG_ALWAYS_ON
static const int ltckpt_writelog_enabled = 1;
#else
static int ltckpt_writelog_enabled = 0;
#endif
#endif


typedef struct region_t {
	char data[WRITELOG_GRANULARITY];
} region_t;

#ifdef __MINIX
static int LTCKPT_RESTARTING() 
{
    int r;
    int priv_flags;
    int init_flags;

    r = sys_whoami(&i_am_ep, i_am_name, 8,
            &priv_flags, &init_flags);

	lt_printf("restarting: wl_data: %x.\n", (unsigned int) wl_data);

    if (ltckpt_is_mapped((void*)WRITELOG_START)) {
	lt_printf("yes mapped: wl_data: %x.\n", (unsigned int) wl_data);
	wl_data = (char *) WRITELOG_START; /* ??? - this should probably be assigned already? */
        return 1;
    }

	lt_printf("not mapped: wl_data: %x.\n", (unsigned int) wl_data);

    /* we have to special case RS here */
    if ( r != OK) {
        panic("LTCKPT_RESTARTING: sys_whoami failed: %d\n", r);
    }

    return ((priv_flags & ROOT_SYS_PROC) && i_am_ep != RS_PROC_NR);
}
#endif

static inline void ltckpt_write_wl(void * addr)
{
	/* align address to region boundary */
	addr = (void *)(LTCKPT_PTR_TO_VA(addr) & ~(WRITELOG_GRANULARITY-1));
	char *wl_data_start = (char *) LTCKPT_PTR_TO_VA(wl_data);
	char *wl_data_end = (char *) LTCKPT_PTR_TO_VA(wl_data) + WRITELOG_BYTES;
	lt_assert(wl_data);

#ifdef __MINIX
	if (!sa_window__is_open) {
		return;
	}
#endif

	if ( (char *) LTCKPT_PTR_TO_VA(addr) >= wl_data_start &&
			(char *) LTCKPT_PTR_TO_VA(addr) < wl_data_end)
	{
		lt_panic("Try to write into writelog!");
	}

	void **addr_slot  =    (void **) &wl_data[wl_position];
	region_t  *region = (region_t *) &(wl_data[wl_position + sizeof(void*)]);

	*addr_slot   = addr;
	*region      = *((region_t*) addr);

	wl_position +=  (sizeof(void*) + sizeof(region_t));

	// A guard page is already being set right after the writelog area to
	// prevent overflowing

#ifdef __MINIX
	if (wl_position >= WRITELOG_BYTES)
	{
		lt_printf("Writelog overflow at position: %lx.\n", wl_position);
		lt_printf("Closing window\n");
		sa_window__is_open = 0;
		g_recovery_bitmask |= LTCKPT_RECOVERY_MASK_FAIL_STOP;
		if (!g_recovery_bitmask_reason) {
			g_recovery_bitmask_reason = "ltckpt recovery: window closed: writelog full";
		}
	}
	ltckpt_debug_print("%lx of %lx\n", wl_position, WRITELOG_BYTES);
#endif
}

#ifdef LTCKPT_STKCHKLOG
static void inline ltckpt_stkcpy()
{
	void *ret = NULL;
	if (ltckpt_fp_addr2 <= ltckpt_fp_addr1) {
		lt_printf("[error] ltckpt_stkcpy: fp2 (0x%x) <= fp1 (0x%x).\n", ltckpt_fp_addr2, ltckpt_fp_addr1);
		ltckpt_fp_addr1 = ltckpt_fp_addr2 = NULL;
		return;
	}

	ltckpt_va_t cpy_size = (ltckpt_fp_addr2 - ltckpt_fp_addr1);

	printf("ltckpt_stkcpy: frameptr1: %x, frameptr2:%x, cpysize: %d\n", ltckpt_fp_addr1, ltckpt_fp_addr2, cpy_size);
	ret = memcpy(&(wl_stkchk[wl_stkchk_pos]), ltckpt_fp_addr1, cpy_size);
	if (NULL == ret) {
		printf("ltckpt_stkcpy: memcpy() from: %x to: %x (pos:%d) failed.\n", ltckpt_fp_addr1, &(wl_stkchk[wl_stkchk_pos]), wl_stkchk_pos);
		lt_panic("ltckpt_stkcpy->memcpy() failed.\n");
	}
	wl_stkchk_pos += (cpy_size / WRITELOG_GRANULARITY);
	return;
}

static void inline ltckpt_stkchklog()
{
	if (NULL == ltckpt_fp_addr1) {
		printf("ltckpt_stkchklog: not checking.\n");
		return;
	}
	for (unsigned i=0; i < wl_stkchk_pos; i++) {
		// Note: This is not portable intentionally, for fast dev purpose.
		uint64_t *logged = (uint64_t*) &(wl_stkchk[i]);
		uint64_t *present = (uint64_t*) &(ltckpt_fp_addr1[i]);
		if (*((uint64_t*)logged) != *((uint64_t*)present)) {
			printf("ltckpt_stkchklog: at addr: %x, expected: %x, present: %x\n", present, *((uint64_t*) logged), *((uint64_t*) present));
		}
	}
	printf("ltckpt_stkchklog: finished.\n");
	return;
}

#endif

LTCKPT_DECLARE_STORE_HOOK()
{
#ifdef LTCKPT_EMPTY_UNDOLOG_HOOKS
return;
#endif
#ifdef __MINIX
#if LTCKPT_WRITELOG_ALWAYS_ON == 0
	if (ltckpt_writelog_enabled)
#endif
#endif
		ltckpt_write_wl(addr);
#ifdef LTCKPT_WINDOW_PROFILING
	PROFILE_STORE(ltckpt_windows[ltckpt_current_site_id]);
#endif
}

LTCKPT_DECLARE_MEMCPY_HOOK()
{
#ifdef __MINIX
	if (!ltckpt_writelog_enabled)
		return;
#endif
	char *end = addr + size;
	region_t *reg = (region_t *)(((ltckpt_va_t)(addr)) & ~0x3);
	while ((char *)reg < end) {
		ltckpt_write_wl(reg);
		reg++;
	}
#ifdef LTCKPT_WINDOW_PROFILING
	PROFILE_MEMCPY(ltckpt_windows[ltckpt_current_site_id], size);
#endif
}

LTCKPT_DECLARE_TOP_OF_THE_LOOP_HOOK()
{
#ifdef LTCKPT_EMPTY_UNDOLOG_HOOKS
return;
#endif
	CTX_NEW_LOG_SIZE(wl_position);
	CTX_NEW_TOL_OR_RETURN();

#ifdef __MINIX
	if(wl_position > wl_position_high_watermark) {
		if(!i_am_name[0]) {
			int priv_flags;
    			int init_flags;
			lt_printf("ltckpt: retrying retrieving self nmae\n");
			sys_whoami(&i_am_ep, i_am_name, 8,
		            &priv_flags, &init_flags);
		}

		wl_position_high_watermark = wl_position;
		lt_printf("ltckpt: %s:%d: undolog high watermark: %ld kB (%ld bytes)\n", i_am_name, i_am_ep, wl_position_high_watermark/1024, wl_position_high_watermark);
	}
#endif
#ifdef LTCKPT_WINDOW_PROFILING
	if (0 < wl_position) {
		PROFILE_UNDOLOG_SIZE(ltckpt_windows[ltckpt_current_site_id], wl_position);
	}
#endif
	wl_position=0;

#ifdef __MINIX
#if !LTCKPT_WRITELOG_ALWAYS_ON
	ltckpt_writelog_enabled = 1;
#endif
#endif

	CTX_NEW_CHECKPOINT();

#ifdef LTCKPT_TEST_TSX_OV		
/* _xbegin() and _xend() immediately to just check TSX initialization overhead */
	while(1) { 
		signed status = _xbegin();
		if (status == _XBEGIN_STARTED) {
			break; // successful start
		}
		// No retries;
		lt_printf("TSX aborted. Not retrying.\n");
		break;
	}
	if (_xtest())	_xend();
#endif
#ifdef LTCKPT_WINDOW_PROFILING
	ltckpt_current_site_id = site_id;
	PROFILE_TOL(LTCKPT_TYPE_UNDOLOG, ltckpt_windows[site_id]);
#endif
#ifdef RCVRY_WINDOW_PROFILING
	rcvry_info[rcvry_current_site_id].rcvry_prof.num_hits_ckpt++;
#endif
#ifdef LTCKPT_STKCHKLOG
	ltckpt_stkcpy();
#endif
}

LTCKPT_DECLARE_END_OF_WINDOW_HOOK() {
#ifdef RCVRY_CKPT_DONT_REMEMBER
	if (LTCKPT_TYPE_INVALID == rcvry_info[rcvry_current_site_id].ltckpt_type) {
#ifdef RCVRY_CKPT_DEFAULT_TO_UNDOLOG
		RCVRY_CURR_TX_TYPE = LTCKPT_TYPE_UNDOLOG;
		rcvry_libcall_gates[rcvry_current_site_id] = LTCKPT_TYPE_UNDOLOG;
#else
		RCVRY_CURR_TX_TYPE = LTCKPT_TYPE_TSX;
		rcvry_libcall_gates[rcvry_current_site_id] = LTCKPT_TYPE_TSX;
#endif
	}
#endif
#ifdef RCVRY_AUTO_ADAPT
    RCVRY_AUTO_ADAPT_CHECK_AND_SWITCH();
#endif
#ifdef RCVRY_DELAYED_FREE
	rcvry_ra_free_freelist();
#endif
#ifdef LTCKPT_STKCHKLOG
	wl_stkchk_pos = 0;
	ltckpt_fp_addr1 = NULL;
	ltckpt_fp_addr2 = NULL;
#endif
}

static void  ltckpt_init_writelog()
{
	unsigned long flags = LTCKPT_MAP_PRIVATE | LTCKPT_MAP_NORESERVE | WRITELOG_FLAGS;
#ifdef __MINIX
	if (LTCKPT_IS_RS()) {
		/* rs may not pagefault during VM recovery */
		printf("preallocating log for RS\n");
		flags |= LTCKPT_MAP_POPULATE;
	}
#endif
	lt_printf("%s", "doing mmap..\n");
	ltckpt_va_t ret =  ltckpt_mmap(LTCKPT_PTR_TO_VA(WRITELOG_START),
	                 	       WRITELOG_BYTES,
	                 	       LTCKPT_PROT_W | LTCKPT_PROT_R, flags);
	if (ret == LTCKPT_MAP_FAILED) {
		ltckpt_panic("%s", "ltckpt: could not allocate writelog data\n");
	}

#ifdef __MINIX
	if (ret != LTCKPT_PTR_TO_VA(WRITELOG_START)) {
		ltckpt_panic("%s", "ltckpt: wrong address returned?!\n");
	}
#endif

	lt_printf("writelog: %x.\n", ret);
	wl_data = LTCKPT_VA_TO_PTR(ret);

	ret = PAGE_SIZE * (((ret + WRITELOG_BYTES) / PAGE_SIZE) + 1);
	ret = ltckpt_mmap(LTCKPT_PTR_TO_VA(ret),
			  PAGE_SIZE,
			  LTCKPT_PROT_N, flags);
	if (ret == LTCKPT_MAP_FAILED) {
		ltckpt_panic("%s", "ltckpt: could not allocate writelog guard page\n");
	}
	ltckpt_registers_ptr = &ltckpt_registers;

#ifdef LTCKPT_STKCHKLOG
	// allocate two pages for stkchklog and then followed by one more guard page.
	ret = ret + PAGE_SIZE;
	ret = ltckpt_mmap(LTCKPT_PTR_TO_VA(ret),
                          PAGE_SIZE * 8,
                          LTCKPT_PROT_W | LTCKPT_PROT_R, flags);
	if (ret == LTCKPT_MAP_FAILED) {
                ltckpt_panic("%s", "ltckpt: could not allocate stkchklog page\n");
        }
	lt_printf("[for debugging] stkchklog: %x.\n", ret);
	wl_stkchk = LTCKPT_VA_TO_PTR(ret);
	wl_stkchk_pos = 0;
	ret = ret + PAGE_SIZE * 2;

	ret = ltckpt_mmap(LTCKPT_PTR_TO_VA(ret),
                          PAGE_SIZE,
                          LTCKPT_PROT_N, flags);
	if (ret == LTCKPT_MAP_FAILED) {
                ltckpt_panic("%s", "ltckpt: could not allocate stkchklog guard page\n");
        }
	lt_printf("[for debugging] stkchklog guard page: %x.\n", ret);
#endif
}

LTCKPT_DECLARE_LATE_INIT_HOOK()
{
	if (LTCKPT_IS_VM()) {
		ltckpt_debug_print("ltckpt: VM detected. Skipping generic late init hook.\n");
		return;
	}

	if (LTCKPT_RESTARTING()) {
		ltckpt_debug_print("ltckpt: We are restarting now, skipping late init hook\n");
		return;
	}
	ltckpt_init_writelog();
}

#ifdef __MINIX
void ltckpt_undolog_vm_late_init()
{
	void *ret;
	printf("ltckpt: running vm late init WML: %d entries, 0x%x bytes\n",
			WRITELOG_MAXLEN, WRITELOG_BYTES);
	if (!vm_is_hole(WRITELOG_START, 1)) {
		lt_printf("ltckpt: We are restarting now, skipping late init hook\n");
		return;
	}
	if ( (ret = vm_allocpages_at(WRITELOG_START, WRITELOG_BYTES)) != NULL ) {
		wl_data = ret;
	} else {
		lt_panic("ltckpt: vm: could not allocate writelog");
	}
}
#endif

extern int inside_trusted_compute_base, have_handled_message;

static int ltckpt_overlaps(const void *p1, size_t s1, const void *p2, size_t s2) {
	const char *ps1 = p1, *pe1 = ps1 + s1;
	const char *ps2 = p2, *pe2 = ps2 + s2;
	return (ps1 <= ps2 && pe1 > ps2) || (ps2 <= ps1 && pe2 > ps1);
}

static int ltckpt_can_restore(const void *addr) {
	return !ltckpt_overlaps(addr, sizeof(region_t), wl_data, WRITELOG_BYTES) &&
		!ltckpt_overlaps(addr, sizeof(region_t), &wl_data, sizeof(wl_data)) &&
		!ltckpt_overlaps(addr, sizeof(region_t), &wl_position, sizeof(wl_position))
#ifdef __MINIX
		&& !ltckpt_overlaps(addr, sizeof(region_t), &ltckpt_writelog_enabled, sizeof(ltckpt_writelog_enabled))
#endif
		;
}

#ifdef __MINIX
LTCKPT_DECLARE_RESTART_HOOK()
{
	void *addr, *datamax;
	char buf[1024], *p = buf, *pend = buf + sizeof(buf);
	region_t *region;
	unsigned long entry_size = sizeof(void*) + sizeof(region_t);
	unsigned long i, num_entries, stack_entries;
	int r;

	printf("%s:%d: suicide replyable: %d\n", __FILE__, __LINE__, ltckpt_is_message_replyable());
	hypermem_log("ltckpt recovery: start: writelog");
	printf("%s:%d: have handled message: %d\n", __FILE__, __LINE__, have_handled_message);
	printf("%s:%d: in tcb: %d\n", __FILE__, __LINE__, inside_trusted_compute_base);

	inside_trusted_compute_base = 1;

	printf("%s:%d: in tcb: %d\n", __FILE__, __LINE__, inside_trusted_compute_base);
	printf("%s:%d: have handled message: %d\n", __FILE__, __LINE__, have_handled_message);

#ifdef __MINIX
	/* Disable temporarily, due to the instrumented printf() below. */
	sef_init_info_t *info = (sef_init_info_t *) arg;

#if LTCKPT_RESTART_DEBUG
	printf("ltckpt_restart: disabling the log in the old process\n");
#endif
	ltckpt_writelog_enabled=0;
	if((r = sys_safecopyto(info->old_endpoint, SEF_STATE_TRANSFER_GID, (vir_bytes) &ltckpt_writelog_enabled,
			(vir_bytes) &ltckpt_writelog_enabled, sizeof(ltckpt_writelog_enabled))) != OK) {
			hypermem_log("ltckpt recovery: failed: writelog: sys_safecopyto failed");
			printf("sef_copy_state_region: sys_safecopyto failed\n");
			return r;
	}
	ltckpt_writelog_enabled=0;

#if LTCKPT_RESTART_DEBUG
	printf("ltckpt_restart: performing identity state transfer\n");
#endif

	r = sef_cb_init_identity_state_transfer(SEF_INIT_RESTART, info);
	if(r != OK) {
		hypermem_log("ltckpt recovery: failed: writelog: identity state transfer failed");
		printf("ltckpt_restart: identity state transfer failed: %d\n", r);
		return r;
	}
#endif
	num_entries = wl_position/entry_size;

#if LTCKPT_RESTART_DEBUG
	printf("ltckpt_restart: about to restore up to %lu log entries\n", num_entries);
#endif

	/* Walk the log in reverse order and restore entries. */
	i=0;
	stack_entries=0;
	assert(wl_position % entry_size == 0);
	assert(num_entries == wl_position/entry_size);
	datamax = (char*)&num_entries - 4096;
	while(wl_position >= entry_size) {
		i++;
		wl_position -= entry_size;
		inside_trusted_compute_base = 1;
		if((r = sys_safecopyfrom(info->old_endpoint, SEF_STATE_TRANSFER_GID, (vir_bytes) &wl_data[wl_position],
			(vir_bytes) &addr, sizeof(addr))) != OK) {
			hypermem_log("ltckpt recovery: failed: writelog: sys_safecopyfrom addr failed");
			printf("sef_copy_state_region: sys_safecopyfrom addr failed\n");
			return r;
		}
		if (!ltckpt_can_restore(addr)) {
			printf("sef_copy_state_region: cannot restore address 0x%p from write log\n", addr);
			continue;
		}

		region = (region_t *) &wl_data[wl_position + sizeof(void*)];
		if (addr > datamax) {
			stack_entries++;
			continue;
		}
#if LTCKPT_RESTART_DEBUG && 0
		printf("ltckpt_restart: restoring entry %lu @0x%08lx \n",
			i, (unsigned long)addr);
		printf("ltckpt_restart: region at %p \n", region);
#endif
		if (addr) {
			inside_trusted_compute_base = 1;
			if((r = sys_safecopyfrom(info->old_endpoint, SEF_STATE_TRANSFER_GID, (vir_bytes) region,
				(vir_bytes) addr, sizeof(region_t))) != OK) {
				hypermem_log("ltckpt recovery: failed: writelog: sys_safecopyfrom region failed");
				printf("sef_copy_state_region: sys_safecopyfrom region failed\n");
				return r;
			}
		}
	} // while ends
	assert(i == num_entries);
	assert(wl_position == 0);

#if LTCKPT_RESTART_DEBUG
	printf("ltckpt_restart: restored %lu log entries (%lu stack entries skipped)\n",
		num_entries-stack_entries, stack_entries);
#endif
	//ltckpt_recovery_cleanup();

	inside_trusted_compute_base = 1;
    printf("%s:%d: suicide replyable: %d\n", __FILE__, __LINE__, ltckpt_is_message_replyable());
	printf("%s:%d: have handled message: %d\n", __FILE__, __LINE__, have_handled_message);
	printf("%s:%d: in tcb: %d\n", __FILE__, __LINE__, inside_trusted_compute_base);

	p = hypermem_cat(p, pend, "ltckpt recovery: success: writelog: ");
	p = hypermem_cat_num(p, pend, num_entries - stack_entries);
	p = hypermem_cat(p, pend, " log entries, ");
	p = hypermem_cat_num(p, pend, stack_entries);
	p = hypermem_cat(p, pend, " stack entries");
	hypermem_log(buf);

	return 0;
}
#else

LTCKPT_DECLARE_RESTART_HOOK()
{
	void *addr=NULL, *datamax=NULL;
	// char buf[1024], *p = buf, *pend = buf + sizeof(buf);
	region_t *region;
	unsigned long entry_size = sizeof(void*) + sizeof(region_t);	// addr + its content
	unsigned long i, num_entries, stack_entries;

	printf("ltckpt_restart: start: writelog\n");

	num_entries = wl_position/entry_size;
#if LTCKPT_RESTART_DEBUG
	printf("ltckpt_restart: about to restore up to %lu log entries\n", num_entries);
#endif

	/* Walk the log in reverse order and restore entries. */
	i=0;
	stack_entries=0;
	assert(wl_position % entry_size == 0);
	assert(num_entries == wl_position/entry_size);
	datamax = (char*)&num_entries - 4096;
	while(wl_position >= entry_size) {
			i++;
			wl_position -= entry_size;
		// Get the addr part
		if(NULL == memcpy(&addr, &wl_data[wl_position], sizeof(addr))) {
			printf("region restoration: memcpy() from addr failed.\n");
			return LTCKPT_FAIL;
		}
		if (!ltckpt_can_restore(addr)) {
			printf("region restoration: cannot restore address 0x%p from write log.\n", addr);
			continue;
		}

		region = (region_t *) &wl_data[wl_position + sizeof(void*)];
		if (addr > datamax) {
			stack_entries++;
			/* We must copy even stack entries too! (for libcall based recovery)*/
#ifndef LTCKPT_LIBCALL_INTERVALS
            continue;
#endif
    	}
#if LTCKPT_RESTART_DEBUG && 0
		printf("ltckpt_restart: restoring entry %lu @0x%08lx \n",
				i, (unsigned long)addr);
		printf("ltckpt_restart: region at %p \n", region);
#endif
		if (addr) {
			if (NULL == memcpy(addr, region, sizeof(region_t))) {
				printf("region restoration: memcpy() region failed.\n");
				return LTCKPT_FAIL;
			}
		}
	} // while ends
	lt_printf("i: %lu, num_entries: %lu", i, num_entries);
#ifndef LTCKPT_STKCHKLOG
	assert(i == num_entries); // This doesnt hold if we are copy stackentries too.
#endif
	assert(wl_position == 0);

#if LTCKPT_RESTART_DEBUG
#ifndef LTCKPT_LIBCALL_INTERVALS
    printf("ltckpt_restart: restored %lu log entries (%lu stack entries skipped)\n",
            num_entries-stack_entries, stack_entries);
#else
    printf("ltckpt_restart: restored %lu log entries (%lu stack entries copied)\n",
            num_entries-stack_entries, stack_entries);
#endif
#endif
#ifdef LTCKPT_STKCHKLOG
	ltckpt_stkchklog();
#endif
	return 0;
}
#endif