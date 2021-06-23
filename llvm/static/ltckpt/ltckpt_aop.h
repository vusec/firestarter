#ifndef LTCKPT_AOP_H
#define LTCKPT_AOP_H

#define uthash_malloc(sz) lt_malloc(sz)
#define uthash_free(ptr,sz) lt_free(ptr)

#include <common/ut/uthash.h>

typedef unsigned long long wstat_t;
typedef struct {
    wstat_t bb_tcb;
    wstat_t bb_in;
    wstat_t bb_out;
    wstat_t end;        /* number of window closings */
    wstat_t end_k;
    wstat_t end_s;
    wstat_t end_n;

    /* time out of window because of closings by this context */
    wstat_t ended_cumulative;
} wprof_pol_t;

typedef struct {
    wprof_pol_t pes;
    wprof_pol_t dfa;
    wprof_pol_t opt;
    wprof_pol_t rwindow;  // recovery window based accounting
    char summary[200];
} wprof_set_t;

#define MAXFUNCS (10*800)       /* combinations of callers + functions so it can add up */
#define FUNCNAME 100

typedef struct {
    wprof_set_t glo;
    struct wprof_scope {
       char funcname[FUNCNAME];
       wprof_set_t stats;
       UT_hash_handle hh;
    } funcs[MAXFUNCS];
    struct wprof_scope *index;  /* hash table of funcs[] */
    int wprof_next_free;        /* first unused entry in funcs[] */
} wprof_t;

void ltckpt_wprof_pol_print(const char* name, wprof_pol_t *pol);
#endif 
