#ifndef LTCKPT_BITMAP_H
#define	LTCKPT_BITMAP_H
#include "../../ltckpt_common.h"

/*
 * we use the compat mem layout of minux. It squeezes every thing
 * but the shared libraries under 1GB. The split we are going to do
 * is now the following:
 *    32 bit                             64-bit
 *                                
 *                             
 * 0 GB _____                    0x7ff..  --------
 *      |                                 | stack
 *      |                                 |  
 *      |                                 | text
 *      |                                 | bss
 *      |STACK                            | mmap
 *      |------                      -4gb |-------
 *      \BITMAP                           | (...)
 * 1.5 GB _____                           | (...)
 *      \                                 .  
 *      \ Shadow                          . 
 *      | Space                           . 
 *      \                                 |-------
 *      \                                 | bitmaps
 *      \                            4gb  |--------
 *      \                                 | shadow 
 * 3 GB ______                            |--------
 *
 */

extern ltckpt_va_t __thread _th_bitmap_offset;

#ifdef LTCKPT_X86
#define LTCKPT_SHADOW_SPACE_SIZE   (1536*1024*1024) /* 1.5 GB */
#define LTCKPT_SHADOW_SPACE_OFFSET (1536*1024*1024) /* shadow space starts at 1.5G */
#define LTCKPT_SHADOW_SPACE_START  (LTCKPT_SHADOW_SPACE_OFFSET)
static inline void* ltckpt_primary_to_shadow(void* a) {
	return (void*) (LTCKPT_PTR_TO_VA(a) + LTCKPT_SHADOW_SPACE_START);
}

static inline void* ltckpt_shadow_to_primary(void* a) {
	char* x = a;
	return (void*) (LTCKPT_PTR_TO_VA(x) + LTCKPT_SHADOW_SPACE_START);
}
#endif

#ifdef LTCKPT_X86_64
#define LTCKPT_SHADOW_SPACE_SIZE    ((ltckpt_va_t) 0x1000000000) 
#define LTCKPT_SHADOW_SPACE_OFFSET  ((ltckpt_va_t) 0x2000000000)
#define LTCKPT_SHADOW_SPACE_START   (LTCKPT_SHADOW_SPACE_OFFSET)

static inline void* ltckpt_primary_to_shadow(void* a) {
	return (void*) ((LTCKPT_PTR_TO_VA(a) & (LTCKPT_SHADOW_SPACE_SIZE - 1)) + LTCKPT_SHADOW_SPACE_OFFSET);
}

static inline void* ltckpt_shadow_to_primary(void* a) {
	return (void*) (LTCKPT_PTR_TO_VA(a) & (LTCKPT_SHADOW_SPACE_SIZE - 1));
}

#endif
void ltckpt_allocate_bitmap();
void ltckpt_allocate_shadow_space();

struct region_s;

static struct region_s * ltckpt_addr_to_region(void * addr);

#define LTCKPT_DIFF_SCHEME 1
#define LTCKPT_SYNC_SCHEME 2

#ifndef LTCKPT_SCHEME
#ifndef __MINIX
#define LTCKPT_SCHEME LTCKPT_SYNC_SCHEME
#else
#define LTCKPT_SCHEME LTCKPT_DIFF_SCHEME
#endif
#endif

#define BITMAP_PLAIN 1   /* keep dirty information in a plain bitmap */
#define BITMAP_TREE  2
#define BITMAP_TREE_BYTE 3
#define BITMAP_PLAIN_BYTE 4   /* keep dirty information in a plain bitmap */
#define BITMAP_PLAIN_EPOCH 5
#define BITMAP_PLAIN_SMMAP 6

#ifndef BITMAP_TYPE
#define BITMAP_TYPE BITMAP_PLAIN_EPOCH
#endif


#if LTCKPT_SCHEME == LTCKPT_SYNC_SCHEME && BITMAP_TYPE == BITMAP_PLAIN
#error Can't use sync scheme with plain bitmap!
#endif

#include "bitmap_plain_epoch.h"
#include "bitmap_plain.h"
#include "bitmap_tree.h"
#include "bitmap_tree_byte.h"
#include "bitmap_plain_byte.h"
#include "bitmap_smmap.h"


typedef struct region_s{
	char byte[LTCKPT_BITMAP_CLICK];
} region_t;


static inline void ltckpt_save_region(region_t *region)
{
	region_t * shadow_region = ltckpt_primary_to_shadow(region);
	*shadow_region = * region;
}


static inline void ltckpt_save_region_for_address(void *addr) {
	ltckpt_save_region(ltckpt_addr_to_region(addr));
}


static inline region_t * ltckpt_addr_to_region(void * addr)
{
	return (region_t *) (((ltckpt_va_t)addr) & ~(LTCKPT_BITMAP_CLICK - 1));
}

#ifndef LTCKPT_BITMAP_START
#define LTCKPT_BITMAP_START (LTCKPT_BITMAP_OFFSET)
#endif
#define LTCKPT_STACK_SIZE   (6*1024*1024)
#define LTCKPT_STACK_START  (PAGE_ROUND_DOWN(LTCKPT_BITMAP_OFFSET) - LTCKPT_STACK_SIZE)
#define LTCKPT_STACK_END    (PAGE_ROUND_DOWN(LTCKPT_BITMAP_OFFSET))

#endif
