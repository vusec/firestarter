#ifndef LTCKPT_PROFILING_H
#define LTCKPT_PROFILING_H
#include <stdio.h>

typedef struct {
	ltckpt_type_t	type;	
	uint64_t g_num_hits;
	uint64_t c_num_retries;		/* Num automatic retries (esp. TSX) */
	uint64_t g_num_retries_max;	/* peak num of retries (esp. TSX) */
	uint64_t g_num_fails;		/* Num failed transactions (TSX) */
	uint64_t c_num_stores; 		/* number of stores in the current window */
	uint64_t g_num_stores; 
	uint64_t c_memcpy_size; 	/* size of memcpy in the current window */
	uint64_t g_memcpy_size;
	uint64_t g_num_stores_max, g_memcpy_size_max;
	uint64_t g_undolog_size_max;
        double g_undolog_size_avg;
} ltckpt_window_t;

#define PROFILE_TOL(T, W)	\
	W.type = T;		\
	W.g_num_hits++;		\
	W.c_num_stores = 0;	\
	W.c_memcpy_size = 0;	\
	W.c_num_retries = 0;

#define PROFILE_TSX_BEGIN(W)	\
	W.c_num_retries++;	\
	if (W.c_num_retries > W.g_num_retries_max)	\
		W.g_num_retries_max = W.c_num_retries;

#define PROFILE_TSX_END(W)	;

#define PROFILE_TSX_FAIL(W)	\
	W.g_num_fails++;

#define PROFILE_STORE(W)	\
	W.c_num_stores++;	\
	W.g_num_stores++;	\
	if (W.c_num_stores > W.g_num_stores_max)		\
        	W.g_num_stores_max = W.c_num_stores;		\

#define PROFILE_MEMCPY(W, S)	\
	W.c_memcpy_size += S;	\
	W.g_memcpy_size += S;	\
	if (W.c_memcpy_size > W.g_memcpy_size_max)		\
        	W.g_memcpy_size_max = W.c_memcpy_size;		\

#define PROFILE_UNDOLOG_SIZE(W, S)					\
	W.g_undolog_size_avg += (S - W.g_undolog_size_avg)/W.g_num_hits;\
	if (S > W.g_undolog_size_max)					\
		W.g_undolog_size_max = S;

#define LTCKPT_MAX_PROFILING_WINDOWS	4096

#define LTCKPT_PROFILING_OUTFILE	"/tmp/ltckpt_profile_dump.txt"
#define ltckpt_fscribe(...)  \
              if (NULL != ltckpt_log_fptr) {                                      \
                      fprintf(ltckpt_log_fptr, __VA_ARGS__);                      \
                      fflush(ltckpt_log_fptr);                                    \
              }

extern uint32_t ltckpt_current_site_id;
extern ltckpt_window_t ltckpt_windows[LTCKPT_MAX_PROFILING_WINDOWS];
extern FILE *ltckpt_log_fptr;

void ltckpt_windows_dump();
#endif

