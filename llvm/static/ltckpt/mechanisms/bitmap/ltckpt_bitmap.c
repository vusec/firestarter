#define LTCKPT_CHECKPOINT_METHOD bitmap

#include "../../ltckpt_local.h"
LTCKPT_CHECKPOINT_METHOD_ONCE();

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <smmap/smmap.h>
#include "bitmap.h"

uint8_t ltckpt_epoch=1;

#ifdef LTCKPT_ALWAYS_ON
#define LTCKPT_BITMAP_ALWAYS_ON LTCKPT_ALWAYS_ON
#endif

#ifndef LTCKPT_BITMAP_ALWAYS_ON
#ifdef __MINIX
#define LTCKPT_BITMAP_ALWAYS_ON 0
#else
#define LTCKPT_BITMAP_ALWAYS_ON 1
#endif
#endif

#if LTCKPT_BITMAP_ALWAYS_ON
static const int ltckpt_bitmap_enabled = 1;
#else
static int ltckpt_bitmap_enabled = 0;
#endif

/* Ok rewrite: assumption the region that corresponds to bit is bigger
 * then 8 byte. Like that we can use one function to rule all stores.
 * one assumption here is that stores are always aligned to their width.
 */
LTCKPT_DECLARE_STORE_HOOK()
{
	if (!ltckpt_bitmap_enabled)
		return;

	ltckpt_stat_add(addr);
#ifndef __MINIX
	ltckpt_debug_print("store to %p\n", addr);
#endif

#if LTCKPT_SCHEME == LTCKPT_SYNC_SCHEME
	ltckpt_set_bit(addr);
#endif

#if LTCKPT_SCHEME == LTCKPT_DIFF_SCHEME
	int bitset = ltckpt_set_bit(addr);
	if(!bitset){
		ltckpt_save_region_for_address(addr);
	}
#endif

}

/* I guess we can't really make assumptions about alignment here.
 */
LTCKPT_DECLARE_MEMCPY_HOOK()
{
	char *end;
	region_t *reg;

	if (!ltckpt_bitmap_enabled)
		return;

	end = addr + size;
	reg = ltckpt_addr_to_region(addr);
	while ((char *)reg < end) {
		ltckpt_stat_add(reg);
		if(ltckpt_set_bit(reg)) {
#if LTCKPT_SCHEME == LTCKPT_DIFF_SCHEME
			ltckpt_save_region(reg);
#endif
		}
		reg++;
	}
}

LTCKPT_DECLARE_TOP_OF_THE_LOOP_HOOK()
{
	static int first_time = 1;
	ltckpt_stat_dump();
	CTX_NEW_TOL_OR_RETURN();

	/* we do that to reduce the RSS again */
	if (first_time) {
 		ltckpt_allocate_bitmap();
		ltckpt_allocate_shadow_space();
		first_time = 0;
	}

#if BITMAP_TYPE == BITMAP_SMMAP
	int ret = smctl(SMMAP_SMCTL_ROLLBACK_DEFAULT, 0);
	if (ret <0)
		ltckpt_panic("smctl failed\n");
#endif

#if LTCKPT_SCHEME == LTCKPT_DIFF_SCHEME
	ltckpt_allocate_bitmap();
#else
#if BITMAP_TYPE == BITMAP_PLAIN_EPOCH
	if(++ltckpt_epoch==0) {
		ltckpt_allocate_bitmap();
		ltckpt_epoch = 1;
	}
#endif
	ltckpt_clear_bitmap();
#endif

#if !LTCKPT_BITMAP_ALWAYS_ON
	ltckpt_bitmap_enabled = 1;
#endif
	CTX_NEW_CHECKPOINT();
}

#ifdef LTCKPT_PER_THREAD_BITMAPS
/*
 * TODO: we do not free bitmaps for now...
 *       (but we should)
 */
ltckpt_va_t __thread _th_bitmap_offset= LTCKPT_BITMAP_START;

static ltckpt_va_t bitmap_end      = LTCKPT_BITMAPS_END;
static ltckpt_va_t current_bitmap  = LTCKPT_BITMAP_START;

static inline ltckpt_va_t fetch_and_inc(ltckpt_va_t * variable, ltckpt_va_t value){
	asm volatile(
			"lock; xaddq %%rax, (%2)\n\t"
			:"=a" (value)                   //output
			: "a" (value), "m" (variable)  //input
			:"memory" );
	return value;
}

LTCKPT_DECLARE_ATPTHREAD_CREATE_CHILD_HOOK()
{
	/*
	 * we allocate bitmaps from the bottom to the top this means
	 */
	_th_bitmap_offset = ltckpt_fetch_and_inc(current_bitmap, LTCKPT_BITMAP_SIZE);
	if (_th_bitmap_offset + LTCKPT_BITMAP_SIZE > bitmap_end) {
		ltckpt_panic("ran out of bitmaps\n");
	}

}

#endif

LTCKPT_DECLARE_LATE_INIT_HOOK()
{
	extern void ltckpt_bitmap_late_init_hook();
	ltckpt_bitmap_late_init_hook();
}

#ifdef __MINIX
#include <minix/sef.h>
#include <minix/vm.h>

/* XXX: Should be called at tol time. */
void ltckpt_sync_state()
{
#if 0
    struct vm_region_info vmri[MAX_VRI_COUNT];
    vir_bytes next;
    int count, i, r;
    r=0;
    
    count = 0;
    next = 0;
    do {
        r = vm_info_region(SELF, vmri, MAX_VRI_COUNT, &next);
        if(r<0) {
            printf("vm_info_region returns negative result %d; errno %d: %s\n", r, errno, strerror(errno));
            exit(1);
        }
        for(i=0; i<r; i++) {
#if LTCKPT_SYNC_DEBUG
            char seg='S';
            printf("%c %08lx-%08lx %c%c%c %c\n",
	    		seg, vmri[i].vri_addr,
    			vmri[i].vri_addr + vmri[i].vri_length,
			    (vmri[i].vri_prot & PROT_READ) ? 'r' : '-',
		    	(vmri[i].vri_prot & PROT_WRITE) ? 'w' : '-',
	    		(vmri[i].vri_prot & PROT_EXEC) ? 'x' : '-',
    			(vmri[i].vri_flags & MAP_IPC_SHARED) ? 's' : 'p');
#endif
            if(vmri[i].vri_prot & PROT_WRITE && vmri[i].vri_addr<LTCKPT_BITMAP_START) {
#if LTCKPT_SYNC_DEBUG
                printf("copying write segment %08lx to %08lx; size %d bytes\n", vmri[i].vri_addr, vmri[i].vri_addr+LTCKPT_SHADOW_SPACE_OFFSET, vmri[i].vri_length);
#endif
                memcpy((void *) vmri[i].vri_addr+LTCKPT_SHADOW_SPACE_OFFSET,
                        (void*) vmri[i].vri_addr, vmri[i].vri_length);
            }
	    	count++;
        }
    } while(r == MAX_VRI_COUNT);
#endif
}

uintptr_t ltckpt_get_primary_from_bitmap(void *a, int bitmap_offset, int bit_num, int *region_size) {
    uintptr_t address = (uintptr_t) a;
    address = (address - bitmap_offset) * LTCKPT_BITMAP_CLICK * LTCKPT_CLICKS_PER_BYTE + (LTCKPT_CLICKS_PER_BYTE * bit_num);
    *region_size = LTCKPT_CLICKS_PER_BYTE;
    return address;
}

int ltckpt_get_offset() {
   return LTCKPT_SHADOW_SPACE_OFFSET;
}

/* XXX: Should be rewritten by:
  - Using bitmap and shadow space already mapped in the current address space.
  - Skipping stack entries.
  - Temporarily disabling checkpointing (ltckpt_bitmap_enabled=0) after identity state transfer.
  - Isolating MINIX-specific code at the beginning.
 */
LTCKPT_DECLARE_RESTART_HOOK()
{
#if 0
    sef_init_info_t *info = (sef_init_info_t *)arg;

#if LTCKPT_RESTART_DEBUG

#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
    (byte & 0x80 ? 1 : 0), \
    (byte & 0x40 ? 1 : 0), \
    (byte & 0x20 ? 1 : 0), \
    (byte & 0x10 ? 1 : 0), \
    (byte & 0x08 ? 1 : 0), \
    (byte & 0x04 ? 1 : 0), \
    (byte & 0x02 ? 1 : 0), \
    (byte & 0x01 ? 1 : 0)

    printf("ltckpt_restart: starting cb restart procedure\n");
#endif

#if LTCKPT_RESTART_DEBUG
    printf("ltckpt_restart: Using %s scheme\n", LTCKPT_SCHEME == LTCKPT_DIFF_SCHEME ? "diff" : "sync");
#endif

    /* restore previous data from primary or shadow space, depending on the scheme used*/
    info->copy_flags = SEF_COPY_OLD_TO_NEW;
    if (LTCKPT_SCHEME == LTCKPT_SYNC_SCHEME)
        info->copy_flags |= SEF_COPY_SRC_OFFSET;
#if LTCKPT_RESTART_DEBUG
    printf("ltckpt_restart: restoring %s segments\n", (info->copy_flags & SEF_COPY_SRC_OFFSET) ? "shadow space" : "data");
#endif
    int r = sef_cb_init_identity_state_transfer(SEF_INIT_RESTART, info);
    if(r != OK) {
        printf("ltckpt_restart: indentity state transfer failed: %d\n", r);
        return r;
    }

    /* retrieve old bitmap */
    int bitmap_size = LTCKPT_BITMAP_SIZE;
    uintptr_t bitmap_address = LTCKPT_BITMAP_START;
    uintptr_t temp_bitmap;
    if(LTCKPT_SCHEME == LTCKPT_DIFF_SCHEME) {
        /* in temporary area with diff scheme */
#if LTCKPT_RESTART_DEBUG
        printf("ltckpt_restart: mmapping temporary bitmap...\n");
#endif
        temp_bitmap = (uintptr_t) mmap((void*) 0, bitmap_size, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANON, -1, 0);
        if((void*)temp_bitmap == (void*)-1) {
            printf("ltckpt_restart: mmap of temporary bitmap failed. errno %d: %s", errno, strerror(errno));
            return ENOMEM;
        }
    } else /* or over the current bitmap with sync scheme */
        temp_bitmap = bitmap_address;

    info->copy_flags = 0;
    /* in any case, we need to copy from the memory region of the old program image */
    info->copy_flags |= SEF_COPY_OLD_TO_NEW;
#if LTCKPT_RESTART_DEBUG
    printf("ltckpt_restart: restoring bitmap segments in %s area\n", (temp_bitmap == bitmap_address) ? "current bitmap" : "temporary bitmap");
#endif
    r = sef_copy_state_region(info, bitmap_address, bitmap_size, temp_bitmap);
    if(r != OK) {
        printf("ltckpt_restart: failed to retrieve old bitmap: %d\n", r);
        return r;
    }

    /* with sync scheme, we're done. */
    if(LTCKPT_SCHEME == LTCKPT_SYNC_SCHEME) {
        printf("ltckpt_restart: state restored\n");
        return OK;
    }

#if LTCKPT_RESTART_DEBUG
    printf("ltckpt_restart: Scanning bitmap...\n");
#endif
    /* with diff scheme, we need to copy chunks from the shadow to the primary to restore the state */
    int i = 0;
    info->copy_flags |= SEF_COPY_SRC_OFFSET;
    char *it = (char *) temp_bitmap;
    for( ; it<(char*)(temp_bitmap+bitmap_size); ++it) {
        if(*it) {
           for(i=0; i<8; ++i) {
                char tmp = *it;
                if((tmp>>i)&1){
#if LTCKPT_RESTART_DEBUG
                    printf("ltckpt_restart: 1 in bitmap found. bitmap address %08lx, offset %d, bitmask "BYTETOBINARYPATTERN"\n",
                        (unsigned long)it, it-(char*)temp_bitmap, BYTETOBINARY(*it));
#endif
                    int region_size = 0;
                    uintptr_t addr = ltckpt_get_primary_from_bitmap(it, temp_bitmap, i, &region_size);
                    r = sef_copy_state_region(info, addr, region_size, addr);
                    if (r != OK) {
                       printf("ltckpt_restart: sef_copy_state_region @0x%08lx, len=%d failed\n",
                           (unsigned long)addr, region_size);
                       return r;
                    }
                }
            }
        }
    }

    /*copy over temp bitmap*/
#if LTCKPT_RESTART_DEBUG
    printf("ltckpt_restart: bitmap regions restore completed\n");
    printf("ltckpt_restart: restoring bitmap from temporary image: source %08lx, destination %08lx, size %d\n",
        (unsigned long)temp_bitmap, (unsigned long)bitmap_address, bitmap_size);
#endif
    memcpy((void*)bitmap_address, (void*)temp_bitmap, bitmap_size);
#if LTCKPT_RESTART_DEBUG
    printf("ltckpt_restart: state restored\n");
#endif
#endif
    return OK;
}
#endif

