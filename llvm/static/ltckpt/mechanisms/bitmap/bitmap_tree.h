#ifndef LTCKPT_BITMAP_TREE_H
#define LTCKPT_BITMAP_TREE_H 1

#if BITMAP_TYPE == BITMAP_TREE

#define LTCKPT_BITMAP_CLICK      16 /* how many bytes are one click */
#define LTCKPT_CLICKS_PER_BYTE   8
#define LTCKPT_BYTE_PER_WORD     4
#define LTCKPT_CLICKS_PER_WORD  (LTCKPT_CLICKS_PER_BYTE*LTCKPT_BYTE_PER_WORD)
#define LTCKPT_NTH_BIT_MASK     (LTCKPT_CLICKS_PER_BYTE-1)

#define LTCKPT_1ST_LEVEL_SIZE   ((LTCKPT_SHADOW_SPACE_SIZE / LTCKPT_BITMAP_CLICK) / LTCKPT_CLICKS_PER_BYTE)
#define LTCKPT_2ND_LEVEL_SIZE   (LTCKPT_1ST_LEVEL_SIZE/ LTCKPT_BITMAP_CLICK / LTCKPT_CLICKS_PER_BYTE)
#define LTCKPT_3RD_LEVEL_SIZE   (LTCKPT_2ND_LEVEL_SIZE/ LTCKPT_BITMAP_CLICK / LTCKPT_CLICKS_PER_BYTE)
#define LTCKPT_BITMAP_SIZE      (LTCKPT_3RD_LEVEL_SIZE + LTCKPT_2ND_LEVEL_SIZE + LTCKPT_1ST_LEVEL_SIZE)
#define LTCKPT_BITMAP_OFFSET    (PAGE_ROUND_DOWN((LTCKPT_SHADOW_SPACE_OFFSET - LTCKPT_BITMAP_SIZE)))

#define LTCKPT_2ND_LEVEL_OFFSET (LTCKPT_BITMAP_OFFSET    + LTCKPT_1ST_LEVEL_SIZE)
#define LTCKPT_3RD_LEVEL_OFFSET (LTCKPT_2ND_LEVEL_OFFSET + LTCKPT_2ND_LEVEL_SIZE)

#if 0
static inline int ltckpt_set_bit(void * address)
{
	/* 3rd Level */
	uint32_t a = (uint32_t)address;
	uint32_t x;
	uint32_t *p;

	a = a >> 4;
	x = a & 0x1f;
	a = a >> 5;
	p = (uint32_t*) a + LTCKPT_BITMAP_OFFSET;
	*p |= (1 << x);

	a = a >> 4;
	x = a & 0x1f;
	a = a >> 5;
	p = (uint32_t*) a +  LTCKPT_2ND_LEVEL_OFFSET;
	*p |= (1 << x);

	a = a >> 4;
	x = a & 0x1f;
	a = a >> 5;
	p = (uint32_t*) a + LTCKPT_3RD_LEVEL_OFFSET;
	*p |= (1 << x);
}

#else
static inline int ltckpt_set_bit(void * a)
{
	/* 3rd Level */
	uint32_t address = (uint32_t)a;
	uint8_t * p;

	int ret = 0;
	address = address / LTCKPT_BITMAP_CLICK;
	unsigned bit = (address & (LTCKPT_NTH_BIT_MASK));
	uint32_t offset = address / LTCKPT_CLICKS_PER_BYTE;
	p   =	(uint8_t*)(LTCKPT_BITMAP_OFFSET + offset);
	*p |= (1 << bit);

	/* 2nd Level */
	address = offset/ LTCKPT_BITMAP_CLICK;
	bit = (address & (LTCKPT_NTH_BIT_MASK));
	offset = address / LTCKPT_CLICKS_PER_BYTE;
	p = (uint8_t*)(LTCKPT_2ND_LEVEL_OFFSET + offset);
	*p |= (1 << bit);

	/* 1st Level */
	address = offset / LTCKPT_BITMAP_CLICK;
	bit = (address & (LTCKPT_NTH_BIT_MASK));
	offset = address / LTCKPT_CLICKS_PER_BYTE;
	p = (uint8_t*)(LTCKPT_3RD_LEVEL_OFFSET + offset);

#if LTCKPT_SCHEME == LTCKPT_DIFF_SCHEME
	ret = *p & (1<<bit);
#endif
	*p |= (1 << bit);
	return ret;
}
#endif
#include <string.h>
#include <xmmintrin.h>


static inline void ltckpt_check_first_level(uint32_t _fl)
{
#if LTCKPT_SCHEME == LTCKPT_SYNC_SCHEME
	uint32_t * fl = (uint32_t *) _fl;
	int i;
	for ( i= 0; i < 4; i++ ) {
		uint32_t set = -1;
		while ( fl[i] ) {
			set++;
			if ( (fl[i]&1) ) {
				uint32_t _data =  (((uint32_t)&fl[i]) -  LTCKPT_BITMAP_OFFSET) * LTCKPT_CLICKS_PER_BYTE * LTCKPT_BITMAP_CLICK;
#if 1
				_data += (set * LTCKPT_BITMAP_CLICK);
				__m128 *src = (__m128 *)_data;
				_data += LTCKPT_SHADOW_SPACE_OFFSET;
				__m128 *dst = (__m128 *)_data;
				*dst = *src;
#else
				_data += (set * LTCKPT_BITMAP_CLICK);
				uint32_t  *src = (uint32_t *)_data;
				_data += LTCKPT_SHADOW_SPACE_OFFSET;
				uint32_t *dst = (uint32_t *)_data;
				dst[0] = src[0];
				dst[1] = src[1];
				dst[2] = src[2];
				dst[3] = src[3];
#endif
			}
			fl[i] = fl[i] >> 1;
		}
	}
#endif

#if LTCKPT_SCHEME == LTCKPT_DIFF_SCHEME
	__m128 zero = _mm_setzero_ps();
	__m128 *fl = (__m128 *) _fl;
	*fl= zero;
#endif
}

static inline void ltckpt_check_second_level(uint32_t _sl)
{
	uint32_t *sl = (uint32_t *)_sl;
	int i;
	for (i = 0; i < 4; i++ ) {
		uint32_t set = -1;
		while ( sl[i] ) {
			set++;
			if ( (sl[i] & 1) ) {
				uint32_t _fl =  (((uint32_t)&sl[i]) - LTCKPT_2ND_LEVEL_OFFSET)
					* LTCKPT_CLICKS_PER_BYTE * LTCKPT_BITMAP_CLICK;
				_fl += (set * LTCKPT_BITMAP_CLICK);
				_fl += LTCKPT_BITMAP_OFFSET;
				ltckpt_check_first_level(_fl);
			}
			sl[i] = sl[i] >> 1;
		}
	}
}

static inline void ltckpt_clear_bitmap()
{
#ifndef LTCKPT_DUMMY_CLEAR
	uint32_t *tl;
	for ( tl = (uint32_t *) LTCKPT_3RD_LEVEL_OFFSET;
			(uint32_t)tl < (LTCKPT_3RD_LEVEL_OFFSET + LTCKPT_3RD_LEVEL_SIZE);
			tl++) {
		uint32_t set = -1;
		while ( *tl ) {
			set++;
			if ((*tl&1) )  {
				uint32_t _sl = (((uint32_t)tl) - LTCKPT_3RD_LEVEL_OFFSET)
					* LTCKPT_CLICKS_PER_BYTE * LTCKPT_BITMAP_CLICK;
				_sl += (set * LTCKPT_BITMAP_CLICK);
				_sl += LTCKPT_2ND_LEVEL_OFFSET;
				ltckpt_check_second_level(_sl);
			}
			*tl = *tl >> 1;
		}
	}
#endif
}
#endif

#endif
