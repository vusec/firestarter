#ifndef RCVRY_DISPATCH_H
#define RCVRY_DISPATCH_H

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include "../ltckpt/ltckpt_types.h"

#define RCVRY_MAX_LIBCALL_SITES		18196
#define RCVRY_LIBCALL_GATE_OPEN		1	// Take normal code path
#define RCVRY_LIBCALL_GATE_CLOSE	0	// Nudge into error-recovery code path
#define RCVRY_ACTIONS_MAX_ARGS		6	// Actually used max: 3
#define RCVRY_PROFILING_MAX_END_SITES	9	// Max unique sites that end a libcall interval
						// that we care to record.

#ifdef RCVRY_DYNAMIC_SWITCH
#define RCVRY_CURR_TX_TYPE		(rcvry_current_tx_type)
#else
#define RCVRY_CURR_TX_TYPE		(rinfo->ltckpt_type)
#endif

#ifdef RCVRY_AUTO_ADAPT
extern uint32_t rcvry_auto_adapt_sample_size, rcvry_auto_adapt_threshold, rcvry_auto_adapt_scale;

#define RCVRY_AUTO_ADAPT_CHECK_AND_SWITCH()                     {           \
    if (rcvry_libcall_gates[rcvry_current_site_id] != LTCKPT_TYPE_UNDOLOG) {	\
    if (rcvry_info[rcvry_current_site_id].num_tsx_hits > 0) {               \
        if (rcvry_info[rcvry_current_site_id].sample_count == 0) {              \
                if (((rcvry_info[rcvry_current_site_id].num_tsx_fails * 100 * rcvry_auto_adapt_scale)    \
                    / rcvry_info[rcvry_current_site_id].num_tsx_hits)           \
                    >= rcvry_auto_adapt_threshold) {                            \
                    RCVRY_CURR_TX_TYPE = LTCKPT_TYPE_UNDOLOG;                   \
                    rcvry_libcall_gates[rcvry_current_site_id] = LTCKPT_TYPE_UNDOLOG;   \
                }                                                                       \
                rcvry_info[rcvry_current_site_id].num_tsx_hits = 0;                     \
                rcvry_info[rcvry_current_site_id].num_tsx_fails = 0;                    \
        }	                                                                \
    }	                                                                        \
    }	                                                                        \
  }

#endif

typedef enum rcvry_type_e {
    RCVRY_TYPE_INVALID = 0,
    RCVRY_TYPE_DEFAULT = 1,
    RCVRY_TYPE_NOOP,
    RCVRY_TYPE_SET_RET_ERRNO,
    RCVRY_TYPE_SET_RET_ERRNO_EACCES,
    RCVRY_TYPE_SET_RET_ERRNO_ENOMEM,
    RCVRY_TYPE_SET_RET_ERRNO_ENOSPC,
    RCVRY_TYPE_SET_RET_ERRNO_EBADF,
    RCVRY_TYPE_SET_RET_ERRNO_EINVAL,
    RCVRY_TYPE_SET_RET_ERRNO_EFAULT,
    RCVRY_TYPE_SET_RET_ERRNO_EMFILE,
    RCVRY_TYPE_SET_RET_ERRNO_ZSTREAM_ERROR,
    RCVRY_TYPE_SET_RET_NULL_ERRNO,
    RCVRY_TYPE_SET_RET_NULL_ERRNO_ENOMEM,
    RCVRY_TYPE_SET_RET_NULL_ERRNO_EBADF,
    RCVRY_TYPE_HW_TSX,
    RCVRY_TYPE_UNDO_MALLOC,
    RCVRY_TYPE_UNDO_MEMALIGN,
    RCVRY_TYPE_UNDO_REALLOC,
    RCVRY_TYPE_UNDO_SETLOCALE,
    RCVRY_TYPE_CLOSE_SOCKET,
    RCVRY_TYPE_CLOSE_RET_NULL,
    RCVRY_TYPE_CLOSE,
    RCVRY_TYPE_DLCLOSE,
    RCVRY_TYPE_UNDO_FORK,
    RCVRY_TYPE_UNDO_ADDRINFO,
    RCVRY_TYPE_UNDO_MKDIR,
    RCVRY_TYPE_UNDO_MMAP,
    RCVRY_TYPE_UNDO_PTHRDMUTX_INIT,
    RCVRY_TYPE_UNDO_PTHRDMUTX_LOCK,
    RCVRY_TYPE_CKPT_BUFF_PREAD,
    RCVRY_TYPE_FAIL,
    __NUM_RCVRY_TYPES
} rcvry_type_t;

#ifdef RCVRY_WINDOW_PROFILING
/* We try not to touch this structure from instrumentations. We directly use this in
 * the hooks that are defined in this static library
 */
typedef struct rcvry_prof_s {
    uint32_t num_hits;			/* Total number of hits */
    uint32_t num_hits_tsx;      /* Number of hits thru TSX protected code segments */
    uint32_t num_hits_ckpt;     /* Number of hits via LTCKPT protected code segments */

    uint32_t num_rcvry_success__switchtxtype; /* (A) Num of successful recovery by switching to s/w ckpt*/
    uint32_t num_rcvry_success__errpath;    /* (B) Num of successful recovery leading towards error paths */
    uint32_t num_rcvry_successes;	/* (A + B) Total num of successful recovery */
    // uint32_t end_sites[RCVRY_PROFILING_MAX_END_SITES];	/* Uniq sites that end this recovery window */

    uint32_t num_hits_rcvry_branch;	/* Number of hits of the rcvry inducing branch (at the libcall-site) */
    uint32_t libcalls__protected;	/* Number of libcalls hit with protected recovery windows */
    uint32_t libcalls__skipped_nonfaultable;	/* Number of libcalls hit without recovery windows coz nonfaultable libcalls */
    uint32_t libcalls__skipped_nousers;	/* Number of libcalls hit without recovery windows coz lack of error handling */

    uint32_t num_rcvry_ra__noop;		/* Num of times tsx rcvry was NOOPed */
    uint32_t num_rcvry_ra__fail;
    uint32_t num_rcvry_ra__set_ret_errno;
    uint32_t num_rcvry_ra__undo_malloc;
    uint32_t num_rcvry_ra__undo_memalign;
    uint32_t num_rcvry_ra__undo_realloc;
    uint32_t num_rcvry_ra__close_socket;
    uint32_t num_rcvry_ra__close;
    uint32_t num_rcvry_ra__dlclose;
    uint32_t num_rcvry_ra__undo_fork;
    uint32_t num_rcvry_ra__undo_addrinfo;
    uint32_t num_rcvry_ra__undo_mkdir;
    uint32_t num_rcvry_ra__undo_mmap;
    uint32_t num_rcvry_ra__undo_pthreadmtx_init;
    uint32_t num_rcvry_ra__undo_pthreadmtx_lock;
    uint32_t num_rcvry_ra__delayed_free;

#ifdef RCVRY_WINDOW_PROFILING_BBTRACING
    #define RCVRY_BBTRACING_MAXSIZE	16384
    uint32_t bbs[RCVRY_BBTRACING_MAXSIZE];   /* Unique bbs encountered in this window at runtime. */
    uint32_t num_bbs;               /* Number of unique bbs encountered in this window at runtime. */
#endif
} rcvry_prof_t;
#endif

typedef struct rcvry_fi_s {
    uint32_t fault_probability;
    uint32_t num_hits;
} rcvry_fi_t;

typedef int (*rcvry_action_t)();

typedef struct rcvry_info_s {
    ltckpt_type_t ltckpt_type;
    rcvry_type_t rcvry_type;
    int argc;
    void *argv[RCVRY_ACTIONS_MAX_ARGS];

#ifdef RCVRY_WINDOW_PROFILING
    rcvry_prof_t rcvry_prof;
#endif
    rcvry_fi_t rcvry_fi;
#ifdef RCVRY_AUTO_ADAPT
    uint32_t sample_count, num_tsx_hits, num_tsx_fails;
#endif
} rcvry_info_t;

extern uint32_t rcvry_current_site_id;
extern uint32_t rcvry_libcall_gates[RCVRY_MAX_LIBCALL_SITES];
extern rcvry_info_t rcvry_info[RCVRY_MAX_LIBCALL_SITES];
extern rcvry_action_t rcvry_actions_map[__NUM_RCVRY_TYPES];
extern char *rcvry_type_strs[];
extern uint32_t rcvry_just_noop_for_tsx;
extern uint32_t rcvry_current_tx_type;
extern rcvry_info_t *rinfo;
#ifdef RCVRY_PRINT_TO_FILE
extern FILE *rcvry_fprint_fptr;
#endif

int rcvry_dispatch(void *info);
void rcvry_prof_at_newproc();
void rcvry_prof_at_exit();
void rcvry_prof_dump();
#ifdef RCVRY_WINDOW_PROFILING_BBTRACING
void rcvry_prof_bbtrace_dump();
#endif
#ifdef RCVRY_AUTO_ADAPT
void rcvry_auto_adapt_init();
#endif

#endif
