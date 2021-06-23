#ifndef LTCKPT_LTCKPT_STAT_H
#define LTCKPT_LTCKPT_STAT_H 1

#include "ltckpt_common.h"
#include "ltckpt_config.h"
#include <stdio.h>

#if LTCKPT_CFG_STATS_ENABLED

#ifndef LTCKPT_STAT_LOGFILE
#define LTCKPT_STAT_LOGFILE "/tmp/ltckpt.out"
#endif

#warning LTCKPT STATISTICS ENABLED --- DON'T USE FOR PERFORMANCE BENCHMARKING

void ltckpt_stat_add(void* r_addr);

void ltckpt_stat_clear();

void ltckpt_stat_dump();

void ltckpt_stat_init();

#else

#define ltckpt_stat_add(x)
#define ltckpt_stat_clear()
#define ltckpt_stat_dump()
#define ltckpt_stat_init()
#endif

#endif
