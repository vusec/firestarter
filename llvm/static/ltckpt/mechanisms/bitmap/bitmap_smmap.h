#ifndef LTCKPT_BITMAP_PLAIN_SMMAP_H
#define LTCKPT_BITMAP_PLAIN_SMMAP_H 1
#include <smmap/smmap.h>
#include "../../ltckpt_local.h"

#if BITMAP_TYPE == BITMAP_PLAIN_SMMAP

#define LTCKPT_BITMAP_CLICK      16 /* how many bytes are one click */
#define LTCKPT_CLICKS_PER_BYTE   1
#define LTCKPT_BYTE_PER_WORD     4
#define LTCKPT_BITMAP_SIZE      (LTCKPT_SHADOW_SPACE_SIZE)
#define LTCKPT_BITMAP_OFFSET    0x40000000000


#define LTCKPT_ADDR_TO_BITMAP(x)  (((x & (LTCKPT_SHADOW_SPACE_SIZE-1)))  + LTCKPT_BITMAP_OFFSET)

static int inline ltckpt_set_bit(void *a)
{
	ltckpt_va_t addr = LTCKPT_PTR_TO_VA(a);
	unsigned long * bitmap_pos = LTCKPT_VA_TO_PTR(LTCKPT_ADDR_TO_BITMAP(addr));
	*bitmap_pos = 1;
	return 1;
}

static inline void ltckpt_clear_bitmap()
{
}

#endif
#endif
