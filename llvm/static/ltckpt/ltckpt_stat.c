#include <stdlib.h>
#include <stdio.h>
#include "ltckpt_config.h"
#include "ltckpt_stat.h"
#include "ltckpt_types.h"
#if LTCKPT_CFG_STATS_ENABLED
typedef struct ltckpt_stats {
	ltckpt_va_t addr;
	uint32_t count;
	struct ltckpt_stats *next;
} ltckpt_stat_t;

static ltckpt_stat_t *stats = NULL;

static struct ltckpt_stats * ltckpt_stat_find(ltckpt_va_t addr)
{
	ltckpt_stat_t *stat= stats;
	while (stat) {
		if (stat->addr == addr) {
			return stat;
		}
		stat = stat->next;
	}
	return NULL;
}

void ltckpt_stat_add(void* addr)
{
	ltckpt_stat_t *stat;
	if( !(stat = ltckpt_stat_find(LTCKPT_PTR_TO_VA(addr))) ) {
		stat = malloc(sizeof(ltckpt_stat_t));
		stat->addr = LTCKPT_PTR_TO_VA(addr);
		stat->count = 1;
		stat->next = stats;
		stats = stat;
	} else {
		++stat->count;
	}
}


void ltckpt_stat_clear()
{
	ltckpt_stat_t *stat = stats;
	while(stat) {
		ltckpt_stat_t *next = stat->next;
		free(stat);
		stat = next;
	}
	stats=NULL;
}

void __attribute__((used)) ltckpt_stat_dump()
{
	static int iteration = 0;
	ltckpt_stat_t *stat = stats;
	while(stat) {
		ltckpt_printf_force("%d 0x%p %d\n", iteration,
			LTCKPT_VA_TO_PTR(stat->addr), stat->count);
		stat = stat->next;
	}
	iteration++;
}

void ltckpt_stat_init()
{
}
#endif
