#ifndef _SMMAP_COMMON_H_
#define _SMMAP_COMMON_H_

/* Definitions. */
#define SMMAP_DEFAULT_MAX_PROCS             256
#define SMMAP_DEFAULT_MAX_MAPS               10
#define SMMAP_NUM_PRIV_PAGES                100
#define SMMAP_DEFAULT_DEBUG_VERBOSITY         1

#define SMMAP_CTL_PATH "/proc/sys/smmap/ctl"

typedef enum smmap_ctl_op_e {
    SMMAP_CTL_SMMAP,
    SMMAP_CTL_SMUNMAP,
    SMMAP_CTL_SMCTL,
    __NUM_SMMAP_CTL_OPS
} smmap_ctl_op_t;

typedef enum smmap_smctl_op_e {
    SMMAP_SMCTL_CHECKPOINT,
    SMMAP_SMCTL_ROLLBACK_DEFAULT,
    SMMAP_SMCTL_GET_STATS,
    SMMAP_SMCTL_CLEAR_STATS,
    __NUM_SMMAP_SMCTL_OPS
} smmap_smctl_op_t;

/* Control data structures. */
typedef struct __attribute__ ((__packed__)) smmap_ctl_smmap_s {
    void *addr;
    void *shadow_addr;
    unsigned long size;
} smmap_ctl_smmap_t;

typedef struct __attribute__ ((__packed__)) smmap_ctl_smunmap_s {
    void *addr;
} smmap_ctl_smunmap_t;

typedef struct __attribute__ ((__packed__)) smmap_ctl_smctl_s {
    smmap_smctl_op_t op;
    void *ptr;
} smmap_ctl_smctl_t;

typedef struct __attribute__ ((__packed__)) smmap_ctl_s {
    smmap_ctl_op_t op;
    union {
        smmap_ctl_smmap_t smmap;
        smmap_ctl_smunmap_t smunmap;
        smmap_ctl_smctl_t smctl;
    } u;
} smmap_ctl_t;

typedef struct smmap_stats_s {
    unsigned num_procs;
    unsigned num_maps;
    unsigned num_checkpoints;
    unsigned num_rollbacks;
    unsigned num_cows;
    unsigned num_faults;
    unsigned num_mergeable_pages;
    unsigned num_nonmergeable_pages;
    unsigned num_dirty_pages;
} smmap_stats_t;

#define SMMAP_STATS_PRINT(P, S) \
    P("STATS={ num_procs=%u, num_maps=%u, num_checkpoints=%u, num_rollbacks=%u, num_cows=%u, num_faults=%u, num_mergeable_pages=%u, num_nonmergeable_pages=%u, num_dirty_pages=%u }", \
    (S)->num_procs, (S)->num_maps, (S)->num_checkpoints, \
    (S)->num_rollbacks, (S)->num_cows, (S)->num_faults, \
    (S)->num_mergeable_pages, (S)->num_nonmergeable_pages, (S)->num_dirty_pages)

#endif /* _SMMAP_COMMON_H_ */

