#ifndef LTCKPT_BITMAP_TREE_BYTE_H
#define LTCKPT_BITMAP_TREE_BYTE_H 1

#if BITMAP_TYPE == BITMAP_TREE_BYTE

#define LTCKPT_BITMAP_CLICK      16 /* how many bytes are one click */
#define LTCKPT_CLICKS_PER_BYTE   1

#define LTCKPT_1ST_LEVEL_SIZE   ((LTCKPT_SHADOW_SPACE_SIZE / LTCKPT_BITMAP_CLICK) / LTCKPT_CLICKS_PER_BYTE)
#define LTCKPT_2ND_LEVEL_SIZE   (LTCKPT_1ST_LEVEL_SIZE / LTCKPT_BITMAP_CLICK / LTCKPT_CLICKS_PER_BYTE)
#define LTCKPT_3RD_LEVEL_SIZE   (LTCKPT_2ND_LEVEL_SIZE / LTCKPT_BITMAP_CLICK / LTCKPT_CLICKS_PER_BYTE)
#define LTCKPT_BITMAP_SIZE      (LTCKPT_3RD_LEVEL_SIZE + LTCKPT_2ND_LEVEL_SIZE + LTCKPT_1ST_LEVEL_SIZE)
#define LTCKPT_BITMAP_OFFSET    (PAGE_ROUND_DOWN((LTCKPT_SHADOW_SPACE_OFFSET - LTCKPT_BITMAP_SIZE)))

#define LTCKPT_2ND_LEVEL_OFFSET (LTCKPT_BITMAP_OFFSET    + LTCKPT_1ST_LEVEL_SIZE)
#define LTCKPT_3RD_LEVEL_OFFSET (LTCKPT_2ND_LEVEL_OFFSET + LTCKPT_2ND_LEVEL_SIZE)

#include <string.h>
#include <xmmintrin.h>

static inline int ltckpt_set_bit(void * a)
{
	uint8_t *p;
	uint32_t address = (uint32_t)a;


	address = address >> 4;
	p = (uint8_t *) ((address)  + LTCKPT_BITMAP_OFFSET);
	*p = 1;

	address = address >> 4;
	p = (uint8_t *) ((address)  + LTCKPT_2ND_LEVEL_OFFSET);
	*p = 1;

	address = address >> 4;
	p = (uint8_t *) ((address) + LTCKPT_3RD_LEVEL_OFFSET);
	*p = 1;
	return 0;
}

static inline void ltckpt_check_first_level(uint32_t _fl)
{
#if LTCKPT_SCHEME == LTCKPT_SYNC_SCHEME
	uint32_t * fl = (uint32_t *) _fl;
	int i;
	for ( i= 0; i < 16; i++ ) {
		uint32_t set = -1;
		while ( fl[i] ) {
			set++;
			if ( (fl[i]&1) ) {
				uint32_t _data =  (((uint32_t)&fl[i]) -  LTCKPT_BITMAP_OFFSET) * LTCKPT_CLICKS_PER_BYTE * LTCKPT_BITMAP_CLICK;
				_data += (set * LTCKPT_BITMAP_CLICK);
				__m128 *src = (__m128 *)_data;
				_data += LTCKPT_SHADOW_SPACE_OFFSET;
				__m128 *dst = (__m128 *)_data;
				*dst = *src;
			}
			fl[i] = fl[i] >> 8;
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
	for (i = 0; i < 16; i++ ) {
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
			sl[i] = sl[i] >> 8;
		}
	}
}


static inline void ltckpt_clear_bitmap()
{
#ifndef LTKPCT_DUMMY_CLEAR
	uint8_t *tl;
	for ( tl = (uint8_t *) LTCKPT_3RD_LEVEL_OFFSET;
			(uint32_t)tl < (LTCKPT_3RD_LEVEL_OFFSET + LTCKPT_3RD_LEVEL_SIZE);
			tl++) {
		if ( *tl ) {
			uint32_t _sl = ((uint32_t)tl  - LTCKPT_3RD_LEVEL_OFFSET) *16;
			_sl += LTCKPT_2ND_LEVEL_OFFSET;
			ltckpt_check_second_level(_sl);
		}
	}
#endif
}
#endif


#endif 
