#define LTCKPT_CHECKPOINT_METHOD tsx

#include <signal.h>
#include "../ltckpt_local.h"
LTCKPT_CHECKPOINT_METHOD_ONCE();
LTCKPT_DECLARE_EMPTY_STORE_HOOKS();

#include <immintrin.h>
#include <stdio.h>

#ifdef LTCKPT_WINDOW_PROFILING
    #include "../ltckpt_profiling.h"
#endif
#include "../../rcvry/rcvry_util.h"
#include "../../rcvry/rcvry_dispatch.h"
#include "../../rcvry/rcvry_actions.h"

#ifdef LTCKPT_LIBCALL_TIME_DIFF		// For perf-debugging only
#include "../../../include/common/util/time.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LTCKPT_TSX_MAX_TRIES
#define LTCKPT_TSX_MAX_TRIES 3
#endif

#ifndef LTCKPT_TSX_SUBSEQ_MAX_TRIES
/* By default, subsequent try-count shall remain the same */
#define LTCKPT_TSX_SUBSEQ_MAX_TRIES LTCKPT_TSX_MAX_TRIES
#endif

#ifdef LTCKPT_NOP_UNTIL_SIGNALLED
extern int ltckpt_tsx_disabled;
#endif

extern uint32_t rcvry_current_site_id;
extern uint32_t rcvry_libcall_gates[];
extern uint32_t rcvry_current_tx_type;

void ltckpt_switch_to_undolog()
{
#ifdef RCVRY_WINDOW_PROFILING
    rcvry_signal_action(SIGUSR1, NULL, NULL);
    return;
#endif
    ltckpt_printf("Switching to undolog based checkpointing.\n");
    RCVRY_CURR_TX_TYPE = LTCKPT_TYPE_UNDOLOG;
    rcvry_libcall_gates[rcvry_current_site_id] = LTCKPT_TYPE_UNDOLOG;
#ifdef RCVRY_LONGJMP
  longjmp(ltckpt_registers, 1);
#else
  ltckpt_asm_restore_registers(ltckpt_registers, 1);
#endif
}

//__attribute__((always_inline))
LTCKPT_DECLARE_TOP_OF_THE_LOOP_HOOK() {
#ifdef LTCKPT_WINDOW_PROFILING
	PROFILE_TOL(LTCKPT_TYPE_TSX, ltckpt_windows[site_id]);
#endif

#if 0
    ltckpt_printf("ltckpt site_id: %d in %s\n", rcvry_current_site_id, __func__);
#endif

#ifdef LTCKPT_NO_TSX_FOR_RCVRY_FAIL
    if (rcvry_info[rcvry_current_site_id].rcvry_type == RCVRY_TYPE_FAIL) {
        return;
    }
#endif

#ifdef LTCKPT_LIBCALL_TIME_DIFF		// For perf-debugging only
    static FILE *time_fptr = NULL;
    if (NULL == time_fptr) {
        char fname[100];
        sprintf(fname, "/tmp/ltckpt_time__%d.txt", getpid());
        time_fptr = fopen(fname, "w+");
    }
    static unsigned long long prev_tsc = 0, cur_tsc = 0;
    cur_tsc = util_time_tsc_read();
    fprintf(time_fptr, "[ltckpt_tsx] time diff: %llu\n", cur_tsc - prev_tsc);
    fflush(time_fptr);
    prev_tsc = cur_tsc;
    return;
#endif

//     int tries = 0;
//     static int max_tries = LTCKPT_TSX_MAX_TRIES;
//     static int is_subseq_retries = 0;
//     while (1) {
//         signed status = _xbegin();
//         if (status == _XBEGIN_STARTED) {
//             // successful start
// #ifdef LTCKPT_WINDOW_PROFILING
// 	PROFILE_TSX_BEGIN(ltckpt_windows[site_id]);
// #endif
//             break;
//         }
//         if (++tries == max_tries) {
//             // abort handler  TODO: fall-back to forking, currently leave w/o any protection
//             if (max_tries >= LTCKPT_TSX_MAX_TRIES) {
// 		if (0 == is_subseq_retries) {
//                 	max_tries = LTCKPT_TSX_SUBSEQ_MAX_TRIES; // reduce the tries
// 			is_subseq_retries = 1;
// 			continue;
// 		}
//             }
// 	    // no more retries
// #ifdef LTCKPT_WINDOW_PROFILING
//             PROFILE_TSX_FAIL(ltckpt_windows[site_id]);
// #endif
// 	    //raise(SIGSEGV);
//             break;
//         }
//     }
#ifdef LTCKPT_NOP_UNTIL_SIGNALLED
    if (ltckpt_tsx_disabled) {
        return;
    }
#endif

#ifdef RCVRY_AUTO_ADAPT
    rcvry_info[rcvry_current_site_id].num_tsx_hits++;
#endif

    int tries = LTCKPT_TSX_MAX_TRIES;
#if 0
    if (rcvry_child_state == RCVRY_CHILD_STARTED) {
        // Noop this for a child process that is testing the code piece.
        return;
    }
#endif
    while (1) {
        signed status = _xbegin();
        if (status == _XBEGIN_STARTED) {
            // successful start
#ifdef LTCKPT_WINDOW_PROFILING
	PROFILE_TSX_BEGIN(ltckpt_windows[site_id]);
#endif
            break;
        }
        if (--tries <= 0) {
            // unsuccessful #tries number of times.
#ifdef LTCKPT_WINDOW_PROFILING
            PROFILE_TSX_FAIL(ltckpt_windows[site_id]);
#endif

#ifdef RCVRY_AUTO_ADAPT
            rcvry_info[rcvry_current_site_id].num_tsx_fails++;
            rcvry_info[rcvry_current_site_id].sample_count 
                    = (rcvry_info[rcvry_current_site_id].sample_count + 1) % rcvry_auto_adapt_sample_size;
#endif
            /* When TSX fails, inform rcvry to handle appropriately */
#ifdef RCVRY_TSX_NO_SWITCHING
            ltckpt_switch_to_undolog();
#elif LTCKPT_NO_REG_RESTORATION
#else
            ltckpt_printf("[%s] informing rcvry via signal...\n", __func__);
            rcvry_signal_action(SIGUSR1, NULL, NULL);
#endif
            break;
        }
    }
#ifdef RCVRY_WINDOW_PROFILING
	rcvry_info[rcvry_current_site_id].rcvry_prof.num_hits_tsx++;
#endif
    return;
}

//__attribute__((always_inline))
LTCKPT_DECLARE_END_OF_WINDOW_HOOK() {
#if 0
    if (rcvry_child_state == RCVRY_CHILD_STARTED) {
        exit(RCVRY_CHILD_EXIT_OK);
    }
#endif
#ifdef LTCKPT_NO_TSX_FOR_RCVRY_FAIL
    if (rcvry_info[rcvry_current_site_id].rcvry_type == RCVRY_TYPE_FAIL) {
        return;
    }
#endif
    if (_xtest())  _xend();
#ifdef LTCKPT_WINDOW_PROFILING
	PROFILE_TOL(LTCKPT_TYPE_TSX_END, ltckpt_windows[site_id]);
	PROFILE_TSX_END(ltckpt_windows[site_id]);
#endif
#ifndef RCVRY_AUTO_ADAPT
#ifdef RCVRY_CKPT_DONT_REMEMBER
    if (LTCKPT_TYPE_INVALID == rcvry_info[rcvry_current_site_id].ltckpt_type) {
#ifdef RCVRY_CKPT_DEFAULT_TO_UNDOLOG
        RCVRY_CURR_TX_TYPE = LTCKPT_TYPE_UNDOLOG;
	    rcvry_libcall_gates[rcvry_current_site_id] = LTCKPT_TYPE_UNDOLOG;
#else
        RCVRY_CURR_TX_TYPE = LTCKPT_TYPE_TSX;
        rcvry_libcall_gates[rcvry_current_site_id] = LTCKPT_TYPE_TSX;
#endif
    }
#endif /* ends RCVRY_CKPT_DONT_REMEMBER */
#else /* Ifdef RCVRY_AUTO_ADAPT */
   // RCVRY_AUTO_ADAPT_CHECK_AND_SWITCH();
#endif
#ifdef RCVRY_DELAYED_FREE
    rcvry_ra_free_freelist();
#endif
}

#ifdef __cplusplus
}
#endif
