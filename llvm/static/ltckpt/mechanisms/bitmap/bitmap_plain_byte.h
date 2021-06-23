#ifndef LTCKPT_BITMAP_PLAIN_BYTE_H
#define LTCKPT_BITMAP_PLAIN_BYTE_H 1

#include "../../ltckpt_local.h"

#if BITMAP_TYPE == BITMAP_PLAIN_BYTE

#define LTCKPT_BITMAP_CLICK      16 /* how many bytes are one click */
#define LTCKPT_CLICKS_PER_BYTE   1
#define LTCKPT_BYTE_PER_WORD     4
#define LTCKPT_1ST_LEVEL_SIZE   ((LTCKPT_SHADOW_SPACE_SIZE / LTCKPT_BITMAP_CLICK) / LTCKPT_CLICKS_PER_BYTE)
#define LTCKPT_BITMAP_SIZE      (LTCKPT_1ST_LEVEL_SIZE)
#define LTCKPT_BITMAP_OFFSET    (PAGE_ROUND_DOWN((LTCKPT_SHADOW_SPACE_OFFSET - LTCKPT_BITMAP_SIZE)))


#define LTCKPT_MAX_PF (1024*1024)
extern unsigned int nr_pf;
extern ltckpt_va_t pages[LTCKPT_MAX_PF];



static inline int ltckpt_set_bit(void * a)
{
	uint8_t *p;
	ltckpt_va_t address = (ltckpt_va_t)a;
	address = address >> 4;
	p = (uint8_t *) ((address)  + LTCKPT_BITMAP_OFFSET);
	*p = 1;
	return 0;
}

#include <string.h>
#include <xmmintrin.h>
#include <sys/mman.h>

static inline void ltckpt_handle_bimap_word(uint32_t *w)
{
#if 1
	int j;
	uint32_t offset = (((ltckpt_va_t)w) -
			LTCKPT_BITMAP_OFFSET) *LTCKPT_BITMAP_CLICK;
	for (j=0;j<4;j++) {
		if (((char*)w)[j]) {
			ltckpt_va_t _data = offset + (j * LTCKPT_BITMAP_CLICK);
			__m128 *src = (__m128 *)_data;
			_data += LTCKPT_SHADOW_SPACE_OFFSET;
			__m128 *dst = (__m128 *)_data;
			*dst = *src;
		}
	}
#endif
}

#if 0
static inline void ltckpt_clear_bitmap()
{
#ifndef LTCKPT_DUMMY_CLEAR
	int i;
	for (i = 0; i < nr_pf; i++) {
		uint32_t *addr = (uint32_t *) pages[i];

		/* make sure we stop */
		addr[1023] |= 0x2;
		/* mark the end of the page */
		/* this doesn't interfere with the bits */
//		asm volatile ("cld\n\n");
		while (1) {
			/*fast forward?*/
#if 0
			while(0){};
			__m128 zero  = _mm_setzero_ps();
			__m128 chunk = _mm_load_ps((void *)addr);

			if (!((uint32_t)addr & 0xf))
			while(_mm_movemask_epi8(_mm_cmpeq_epi32(chunk,zero)) == 0xffff) {
				addr += 4;
				chunk = _mm_load_ps((void *)addr);
			}
#endif
#ifdef BITMAP_USE_REPEAT_SCAS
			asm volatile (
					"xor %%eax, %%eax        \n\t"
					"mov %1, %%edi           \n\t"
					"mov $1024, %%ecx        \n\t"
					"repe \nscasl            \n\t"
					"mov %%edi, %0           \n\t"
					: "=r" (addr)
					: "r" (addr)
					:"eax","ecx","edi"
					);
			addr--;
#else
			while(!*addr) {
				addr++;
			}
#endif
			if ((uint32_t)addr & 0xffc) {
				*addr &=~0x2;
				ltckpt_handle_bimap_word(addr);
				*addr = 0;
				break;
			}

			ltckpt_handle_bimap_word(addr);
			*addr = 0;
		}
		if (mprotect((void *)pages[i], 4096,PROT_READ)){
			ltckpt_panic("Could not protect bitmap");
		}
	}
	nr_pf=0;
#endif
}
#else

struct page_desc {
	ltckpt_va_t start;
	struct page_desc *next;
};
struct page_desc sorted_pages[1000];

static inline void ltckpt_clear_bitmap()
{
	int i;
	ltckpt_va_t region_start=0;
	struct page_desc p_li = {0, NULL};
	struct page_desc *act_p = &p_li;
	int entry=-1;

	for (i = 0; i < nr_pf; i++) {
		/* new entry */
		sorted_pages[++entry].start = pages[i];
		sorted_pages[entry].next = NULL;
		act_p = &p_li;

		while (act_p->next && act_p->next->start < pages[i]) {
			act_p = act_p->next;
		}

		sorted_pages[entry].next = act_p->next;
		act_p->next = &sorted_pages[entry];
	}

	act_p = p_li.next;

	while (act_p) {
		uint32_t *addr = (uint32_t *) act_p->start;

		if (region_start==0)
			region_start = act_p->start;

		/* make sure we stop */
		addr[1023] |= 0x2;
		/* mark the end of the page */
		/* this doesn't interfere with the bits */
		while (1) {
			/*fast forward?*/
#if 0
			while(0){};
			__m128 zero  = _mm_setzero_ps();
			__m128 chunk = _mm_load_ps((void *)addr);

			if (!((uint32_t)addr & 0xf))
			while(_mm_movemask_epi8(_mm_cmpeq_epi32(chunk,zero)) == 0xffff) {
				addr += 4;
				chunk = _mm_load_ps((void *)addr);
			}
			asm volatile (
					"xor %%eax, %%eax        \n\t"
					"mov %1, %%edi           \n\t"
					"mov $1024, %%ecx        \n\t"
					"repe \nscasl            \n\t"
					"mov %%edi, %0           \n\t"
					: "=r" (addr)
					: "r" (addr)
					:"eax","ecx","edi"
					);
			addr--;
#else
			while(!*addr) {
				addr++;
			}
#endif
			if ((uint32_t)addr & 0xffc) {
				*addr &=~0x2;
				ltckpt_handle_bimap_word(addr);
				*addr = 0;
				break;
			}

			ltckpt_handle_bimap_word(addr);
			*addr = 0;
		}
#define TRESHOLD (16*4096)
		if (!act_p->next || ( act_p->next->start > (act_p->start + TRESHOLD) )) {
			if ( mprotect((void *)region_start,
			 	 act_p->start-region_start + 4096,
				 PROT_READ) ){
				ltckpt_panic("Could not protect bitmap");
			}
			region_start = 0;
		}
		act_p = act_p->next;
	}
	nr_pf=0;
}

#endif


#endif

#endif
