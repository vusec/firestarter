#ifndef _EDFI_STATFILE_H
#define _EDFI_STATFILE_H

#include <stdint.h>

/*
 * file structure:
 *   struct edfi_stats_header header;
 *   char fault_names[header.num_fault_types][EDFI_STATS_FAULT_NAME_LEN];
 *   uint64_t bb_num_executions[header.num_bbs];
 *   int bb_num_candidates[header.num_fault_types][header.num_bbs];
 */

#define EDFI_STATS_MAGIC 0x74536445
#define EDFI_STATS_FAULT_NAME_LEN 64

struct edfi_stats_header {
	uint32_t magic;
	uint32_t num_bbs;
	uint32_t num_fault_types;
	uint32_t fault_name_len;
};

#endif

