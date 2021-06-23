#ifndef LTCKPT_BITMAP_PLAIN_H
#define LTCKPT_BITMAP_PLAIN_H 1

#if BITMAP_TYPE == BITMAP_PLAIN
#define LTCKPT_BITMAP_CLICK        8 /* how many bytes per bit in the bitmask */
#define LTCKPT_CLICKS_PER_BYTE     8
#define LTCKPT_BYTE_PER_WORD     4
#define LTCKPT_CLICKS_PER_WORD  (LTCKPT_CLICKS_PER_BYTE*LTCKPT_BYTE_PER_WORD)

#define LTCKPT_NTH_BIT_MASK        (LTCKPT_CLICKS_PER_BYTE-1)
#define LTCKPT_BITMAP_SIZE         (LTCKPT_SHADOW_SPACE_SIZE / (LTCKPT_BITMAP_CLICK * 8))
#define LTCKPT_BITMAP_OFFSET       (LTCKPT_SHADOW_SPACE_OFFSET - LTCKPT_BITMAP_SIZE) /* dirty bits offset */


/* set the bit in the bitmap if not
 * already set and return the previous state. */
static inline int ltckpt_set_bit(void *a)
{
	uint32_t address = (uint32_t) a;
	uint8_t * p;
	int ret;
	address = address / LTCKPT_BITMAP_CLICK;
	unsigned bit = (address & (LTCKPT_NTH_BIT_MASK));
	uint32_t offset = address / LTCKPT_CLICKS_PER_BYTE;

	p   =	(uint8_t*)(LTCKPT_BITMAP_OFFSET + offset);
	ret = *p &  (1 << bit);
	*p |= (1 << bit);

	return ret;
}
#endif

#endif
