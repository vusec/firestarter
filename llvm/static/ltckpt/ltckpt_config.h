#ifndef LTCKPT_LTCKPT_CONFIG_H
#define LTCKPT_LTCKPT_CONFIG_H

#include <stdio.h>

/**
 * This is the configuration file for ltckpt.
 * You can configure here which scheme should be used for
 * taking checkpoints.  Further the common attributes for the
 * different techniques can be configured here (e.g. region size)
 **/


/**
 * Enable debugging output
 **/
#define LTCKPT_CFG_DEBUG 0

/**
 * Enable statistics
 **/
#ifndef LTCKPT_CFG_STATS_ENABLED
#define LTCKPT_CFG_STATS_ENABLED 0
#endif

#if LTCKPT_CFG_DEBUG == 1
#define DEBUG(x) x
#define ltckpt_debug_print(...) do { \
	ltckpt_printf("%s: ", __func__); \
	ltckpt_printf(__VA_ARGS__);     \
} while(0)

#ifndef __MINIX
#define ltckpt_debug_func() do { \
	ltckpt_printf("Calling: %s()\n", __func__); \
} while(0)
#else
#define ltckpt_debug_func()
#endif

#else
#define DEBUG(x)
#define ltckpt_debug_print(fmt, ...)
#define ltckpt_debug_func()
#endif

#ifndef LTCKPT_RESTART_DEBUG
#define LTCKPT_RESTART_DEBUG 1
#endif

#ifndef CHECKPOINT_REGION_SIZE_LOG2
#define CHECKPOINT_REGION_SIZE_LOG2 4
#endif

#if CHECKPOINT_REGION_SIZE_LOG2 < 3
#warning checkpoint region size is smaller then 8 byte. This might lead to problems with 8 byte wide stores
#endif

#endif
