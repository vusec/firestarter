#ifndef LTCKPT_BITMAP_PLAIN_EPOCH_H
#define LTCKPT_BITMAP_PLAIN_EPOCH_H 1

#if BITMAP_TYPE == BITMAP_PLAIN_EPOCH

#define LTCKPT_BITMAP_CLICK    128 /* how many bytes are one click */
#define LTCKPT_CLICKS_PER_BYTE     8
#define LTCKPT_1ST_LEVEL_SIZE   ((LTCKPT_SHADOW_SPACE_SIZE / LTCKPT_BITMAP_CLICK))
#define LTCKPT_BITMAP_SIZE      (LTCKPT_1ST_LEVEL_SIZE)

#ifdef LTCKPT_X86
#define LTCKPT_BITMAP_START    (PAGE_ROUND_DOWN((LTCKPT_SHADOW_SPACE_OFFSET - LTCKPT_BITMAP_SIZE)))
#ifndef LTCKPT_PER_THREAD_BITMAPS
#define LTCKPT_BITMAP_OFFSET    LTCKPT_BITMAP_START
#else
#define LTCKPT_BITMAP_OFFSET    _th_bitmap_offset
#endif
#define LTCKPT_ADDR_TO_BITMAP(x)  (((x) / LTCKPT_BITMAP_CLICK)  + LTCKPT_BITMAP_OFFSET)
#endif


#ifdef LTCKPT_X86_64
#define LTCKPT_BITMAP_START    0x1000000000 //(LTCKPT_SHADOW_SPACE_SIZE + LTCKPT_SHADOW_SPACE_OFFSET)
#ifndef LTCKPT_PER_THREAD_BITMAPS
#define LTCKPT_BITMAP_OFFSET    LTCKPT_BITMAP_START
#else
#define LTCKPT_BITMAP_OFFSET    _th_bitmap_offset
#endif
#define LTCKPT_BITMAPS_END LTCKPT_SHADOW_SPACE_OFFSET
#define LTCKPT_ADDR_TO_BITMAP(x)  (((x & (LTCKPT_SHADOW_SPACE_SIZE-1)) / LTCKPT_BITMAP_CLICK)  + LTCKPT_BITMAP_OFFSET)
#endif

#if LTCKPT_BITMAP_CLICK == 16
#define _ltckpt_save_region(X) ltckpt_save_16_region(X)
#else
#define _ltckpt_save_region(X) _ltckpt_save_sized_region(X)
#endif

#include "../../ltckpt_types.h"

extern uint8_t ltckpt_epoch;

static __attribute__((noinline)) __attribute__((used)) void ltckpt_save_16_region(ltckpt_va_t address)
{
	uint8_t *p;
	p = (uint8_t *) LTCKPT_ADDR_TO_BITMAP(address) ;
#ifdef LTCKPT_BITMAP_SYNCHRONIZE
	uint8_t old= *p;
	if (old != ltckpt_epoch) {
		address = address&(~15);
		ltckpt_m128_t *dst = (ltckpt_m128_t *) ltckpt_primary_to_shadow((void*)(address));
		ltckpt_m128_t *src = (ltckpt_m128_t *) (address);
		asm volatile (" movaps (%0), %%xmm15":: "r" (src));
		ltckpt_debug_print("%p -> %p\n", LTCKPT_VA_TO_PTR(address), dst);
		//	*p = ltckpt_epoch;
		if (__sync_bool_compare_and_swap(p, old, ltckpt_epoch)) {
			asm volatile (" movaps %%xmm15, (%0)":: "r" (dst));
		}
	}
#else
	address = address&(~15);
	ltckpt_m128_t *dst = (ltckpt_m128_t *) ltckpt_primary_to_shadow((void*)(address));
	ltckpt_m128_t *src = (ltckpt_m128_t *) (address);
	ltckpt_debug_print("%p -> %p\n", LTCKPT_VA_TO_PTR(address), dst);
	*p = ltckpt_epoch;
	*dst = *src;
#endif
}


static __attribute__((noinline)) __attribute__((used)) void _ltckpt_save_sized_region(ltckpt_va_t address)
{
	uint8_t *p = (uint8_t *) LTCKPT_ADDR_TO_BITMAP(address);
	uint8_t old= *p;
	if (old != ltckpt_epoch) {
		ltckpt_m128_t *dst = (ltckpt_m128_t *) ltckpt_primary_to_shadow((void*)(address&(~(LTCKPT_BITMAP_CLICK-1))));
		ltckpt_m128_t *src = (ltckpt_m128_t *) (address&(~(LTCKPT_BITMAP_CLICK-1)));
#ifndef LTCKPT_BITMAP_SYNCHRONIZE
		memcpy(dst, src, LTCKPT_BITMAP_CLICK);
		*p=ltckpt_epoch;
#else
		char saved_bytes[LTCKPT_BITMAP_CLICK] __attribute__((aligned(16)));
		memcpy(saved_bytes, src, LTCKPT_BITMAP_CLICK);
		ltckpt_debug_print("%p -> %p\n", LTCKPT_VA_TO_PTR(address), dst);
		if (__sync_bool_compare_and_swap(p, old, ltckpt_epoch)) {
			memcpy(dst, saved_bytes, LTCKPT_BITMAP_CLICK);
		}
#endif
	}
}


static inline int ltckpt_set_bit(void * a)
{
	uint8_t *p;
	ltckpt_va_t address = LTCKPT_PTR_TO_VA(a);
	p = (uint8_t *) LTCKPT_ADDR_TO_BITMAP(address) ;
#ifndef __MINIX
	ltckpt_debug_print("bitmap_position for %p at %p\n",a ,p);
#endif
	if (ltckpt_epoch != *p ) {
		_ltckpt_save_region(address);
	}
	return 1;
}


static inline void ltckpt_clear_bitmap()
{
}
#endif

#endif
