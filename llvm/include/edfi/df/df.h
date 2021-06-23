#ifndef _EDFI_DF_H
#define _EDFI_DF_H

#include <edfi/common.h>
#include <edfi/ctl/server.h>
#include <stdint.h>

/* EDFI context definitions. */
extern edfi_context_t edfi_context_buff;
extern edfi_context_t *edfi_context;

/* DF handlers. */
typedef void (*edfi_onstart_t)(char *params);
typedef int (*edfi_onfdp_t)(int bb_index);
typedef void (*edfi_onfault_t)(int bb_index);
typedef void (*edfi_onstop_t)(void);

extern edfi_onstart_t edfi_onstart_p;
extern edfi_onfdp_t edfi_onfdp_p;
extern edfi_onfault_t edfi_onfault_p;
extern edfi_onstop_t edfi_onstop_p;

/* DF default handlers. */
void edfi_onstart_default(char *params);
/*int  edfi_onfdp_default(const char *file, int line);*/
int  edfi_onfdp_default(int bb_index);
/*void edfi_onfault_default(const char *file, int line, int num_fault_types, ...);*/
void edfi_onfault_default(int bb_index);
void edfi_onstop_default();

/* DF handler helpers. */
int edfi_onfdp_min_fdp_interval();
int edfi_onfdp_min_fault_time_interval();
int edfi_onfdp_fault_prob();
int edfi_onfdp_bb_index(int bb_index);
void edfi_onfdp_min_fdp_interval_update();
void edfi_onfdp_min_fault_time_interval_update();
int edfi_onfdp_max_time();

/* DF API. */
int edfi_start(edfi_context_t *context, char *params);
void edfi_stop();
void edfi_test();
int edfi_stat(edfi_context_t *context);
void edfi_print_stats();
int edfi_update_context(edfi_context_t *context);
int edfi_process_cmd(edfi_cmd_data_t *data);

#if defined(EDFI_BB_TRACING_FOR_RCVRY) || defined(RCVRY_WINDOW_PROFILING_BBTRACING)
/* DF bbtracing API */
#define EDFI_BB_TRACING_HASHLIST_SIZE   409600	// 64000
#define EDFI_BB_TRACING_HISTORY_SIZE    16384	// 8192

extern uint32_t edfi_bbtrace_hashlist[EDFI_BB_TRACING_HASHLIST_SIZE];
extern uint32_t edfi_bbtrace_history[EDFI_BB_TRACING_HISTORY_SIZE];
extern uint32_t edfi_bbtrace_next_index, edfi_bbtrace_num_bbs, edfi_bbtrace_is_startup;
void edfi_trace_bb(unsigned bb_id);
#endif

#endif

