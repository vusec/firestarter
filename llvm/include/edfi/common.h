#ifndef _EDFI_COMMON_H
#define _EDFI_COMMON_H

#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#define EDFI_ENABLE_INJECTION_ON_START 1
#define EDFI_DISABLE_CTL_SRV 1

#ifndef PACKED
#define PACKED __attribute__((aligned(8),packed))
#endif

#define EDFI_DFLIB_PATH_MAX                     (512)
#define EDFI_USE_DYN_WRAPPER_FAULT_PROBS        0
#define EDFI_RAND_MAX                           32767
#define EDFI_CONTEXT_ADDRESS                    0x9FFFE000
#define EDFI_CANARY_VALUE                       0xFF0A0011

/* EDFI sections. */
#define EDFI_DATA_SECTION_START                 ((void*)&__start_edfi_data)
#define EDFI_DATA_SECTION_END                   ((void*)&__stop_edfi_data)
#define EDFI_DATA_RO_SECTION_START              ((void*)&__start_edfi_data_ro)
#define EDFI_DATA_RO_SECTION_END                ((void*)&__stop_edfi_data_ro)
#define EDFI_TEXT_SECTION_START                 ((void*)&__start_edfi_functions)
#define EDFI_TEXT_SECTION_END                   ((void*)&__stop_edfi_functions)
extern void* __start_edfi_data;
extern void* __stop_edfi_data;
extern void* __start_edfi_data_ro;
extern void* __stop_edfi_data_ro;
extern void* __start_edfi_functions;
extern void* __stop_edfi_functions;

/* this can be redefined to force pointer size to 32 or 64 bit */
#define POINTER(type) type*

/* EDFI context definitions. */
typedef unsigned long long exec_count;
typedef struct {
    POINTER(char) name;
    POINTER(int) bb_num_injected;
    POINTER(int) bb_num_candidates;
} fault_type_stats;

typedef struct {
    float fault_prob;
    unsigned long fault_prob_randmax;
    int min_fdp_interval;
    int faulty_bb_index;
    unsigned int min_fault_time_interval;
    unsigned int max_time;
    unsigned long long max_faults;
    unsigned int rand_seed;
    char dflib_path[EDFI_DFLIB_PATH_MAX];
} PACKED edfi_context_conf_t;

typedef struct {
    unsigned int canary_value1;
    int fault_fdp_count;
    unsigned long long fault_time;
    unsigned long long start_time;
    unsigned long long total_faults;
    POINTER(fault_type_stats) fault_type_stats;
    POINTER(exec_count) bb_num_executions; /* canaries in first and last elements */
    POINTER(int) bb_num_faults;
    unsigned int num_bbs;
    unsigned int num_fault_types;
    int no_svr_thread;
    POINTER(char) output_dir;
    int num_requests_on_start;
    int verbosity;
    edfi_context_conf_t c;
    unsigned int canary_value2;
} PACKED edfi_context_t;

#undef POINTER

enum edfi_context_t_fields {
    EDFI_CONTEXT_FIELD_CANARY_VALUE1,
    EDFI_CONTEXT_FIELD_FAULT_FDP_COUNT,
    EDFI_CONTEXT_FIELD_FAULT_TIME,
    EDFI_CONTEXT_FIELD_START_TIME,
    EDFI_CONTEXT_FIELD_TOTAL_FAULTS,
    EDFI_CONTEXT_FIELD_FAULT_TYPE_STATS,
    EDFI_CONTEXT_FIELD_BB_NUM_EXECUTIONS,
    EDFI_CONTEXT_FIELD_BB_NUM_FAULTS,
    EDFI_CONTEXT_FIELD_NUM_BBS,
    EDFI_CONTEXT_FIELD_NUM_FAULT_TYPES,
    EDFI_CONTEXT_FIELD_NO_SVR_THREAD,
    EDFI_CONTEXT_FIELD_OUTPUT_DIR,
    EDFI_CONTEXT_FIELD_NUM_REQUESTS_ON_START,
    EDFI_CONTEXT_FIELD_VERBOSITY,
    EDFI_CONTEXT_FIELD_C,
    EDFI_CONTEXT_FIELD_CANARY_VALUE2
};

#define EDFI_PRINT_CONTEXT_CONF(PF, C)                                         \
    do {                                                                       \
        (PF)("fault_prob=%f\n", (C)->c.fault_prob);                            \
        (PF)("fault_prob_randmax=%ul\n", (C)->c.fault_prob_randmax);           \
        (PF)("min_fdp_interval=%d\n", (C)->c.min_fdp_interval);                \
        (PF)("faulty_bb_index=%d\n", (C)->c.faulty_bb_index);                  \
        (PF)("min_fault_time_interval=%u\n", (C)->c.min_fault_time_interval);  \
        (PF)("max_time=%u\n", (C)->c.max_time);                                \
        (PF)("max_faults=%llu\n", (C)->c.max_faults);                            \
        (PF)("rand_seed=%u\n", (C)->c.rand_seed);                              \
        (PF)("dflib_path=%s\n", (C)->c.dflib_path);                            \
    } while(0)

#define EDFI_PRINT_CONTEXT(PF, C)                                              \
    do {                                                                       \
        (PF)("\n[edfi-context]\n");                                            \
        (PF)("module_name=%s\n", edfi_module_name);                            \
        (PF)("faultinjection_enabled=%d\n", edfi_faultinjection_enabled);      \
        (PF)("address=0x%08x\n", (unsigned int) (C));                          \
        (PF)("fault_fdp_count=%d\n", (C)->fault_fdp_count);                    \
        (PF)("fault_time=%llu\n", (C)->fault_time);                            \
        (PF)("start_time=%llu\n", (C)->start_time);                            \
        (PF)("fault_type_stats=0x%08x\n",                                      \
            (unsigned int) (C)->fault_type_stats);                             \
        (PF)("bb_num_executions=0x%08x\n",                                     \
            (unsigned int) (C)->bb_num_executions);                            \
        (PF)("num_bbs=%u\n", (C)->num_bbs);                                    \
        (PF)("num_fault_types=%u\n", (C)->num_fault_types);                    \
        (PF)("no_svr_thread=%d\n", (C)->no_svr_thread);                        \
        (PF)("output_dir=%s\n", (C)->output_dir);                              \
        (PF)("num_requests_on_start=%d\n", (C)->num_requests_on_start);        \
        (PF)("verbosity=%d\n", (C)->verbosity);                                \
        EDFI_PRINT_CONTEXT_CONF(PF, C);                                        \
    } while(0)

#define EDFI_IS_OK_CONTEXT(C) \
    ( (C)->canary_value1 == EDFI_CANARY_VALUE && (C)->canary_value2 == EDFI_CANARY_VALUE )

#define EDFI_CHECK_CONTEXT(C)                                                  \
    do {                                                                       \
        if(!EDFI_IS_OK_CONTEXT(C)) {                                           \
           edfi_printf("Bad context!\n");                                      \
           assert(0);                                                          \
           abort();                                                            \
        }                                                                      \
    } while(0)

/* EDFI ctl definitions. */
enum edfi_ctl_cmd {
    EDFI_CTL_CMD_STOP,
    EDFI_CTL_CMD_START,
    EDFI_CTL_CMD_TEST,
    EDFI_CTL_CMD_STAT,
    EDFI_CTL_CMD_UPDATE,
    __EDFI_CTL_NUM_CMDS
};

typedef struct {
    enum edfi_ctl_cmd cmd;
    int status;
    size_t context_size;
    size_t params_size;
    edfi_context_t *context;
    char *params;
} PACKED edfi_cmd_data_t;

#define EDFI_CTL_PATH                       "edfi.ctl"
#define EDFI_OUTPUT_FILE                    "edfi.out"
#define EDFI_STAT_FILE                      "edfi.stat"
#define EDFI_OUTPUT_DIR                     "/tmp"
#define UNIX_PATH_MAX                       108
#define EDFI_USE_PROC_OUTPUT_FILES          1

/* EDFI stats definitions. */
#define EDFI_STATS_HEADER_FMT               "\n[%s]\n"
#define EDFI_STATS_UL_FMT                   "%s = %lu\n"
#define EDFI_STATS_ULL_FMT                  "%s = %llu\n"
#define EDFI_STATS_LD_FMT                   "%s = %.4Lf\n"
#define EDFI_STATS_S_FMT	            "%s = %s\n"

#define EDFI_STATS_SECTION_FAULTS           (1 << 0)
#define EDFI_STATS_SECTION_CANDIDATES       (1 << 1)
#define EDFI_STATS_SECTION_PROB             (1 << 2)
#define EDFI_STATS_SECTION_DEBUG            (1 << 3)

#define EDFI_STATS_SECTION_FAULTS_NAME      "edfi-faults"
#define EDFI_STATS_SECTION_CANDIDATES_NAME  "edfi-candidates"
#define EDFI_STATS_SECTION_PROB_NAME        "edfi-prob"
#define EDFI_STATS_SECTION_DEBUG_NAME       "edfi-debug"
#define EDFI_STATS_SECTION_PER_BB_NAME      "edfi-per-bb"
#define EDFI_STATS_SECTION_HYPERMEMLIKE_STATS "edfi-hypermemlike-stats"

#endif

