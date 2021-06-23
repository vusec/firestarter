#ifndef _SMMAP_H_
#define _SMMAP_H_

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

#include <smmap/smmap_common.h>

int smmap_ctl_fd __attribute__ ((weak));

static inline int __smmap_init()
{
    int fd;
    while ((fd=open(SMMAP_CTL_PATH, O_WRONLY)) < 0 && errno == EINTR);
    if (fd < 0) {
        printf("__smmap_init: open failed, smmap.ko not installed?\n");
        smmap_ctl_fd = fd;
        return fd;
    }
    if (!__sync_bool_compare_and_swap(&smmap_ctl_fd, 0, fd)) {
        close(fd);
    }
    return fd;
}

static inline int __smmap_ctl(smmap_ctl_t *ctl)
{
    int ret;

    if (!smmap_ctl_fd) {
        __smmap_init();
    }
    while((ret = write(smmap_ctl_fd, ctl, sizeof(smmap_ctl_t))) < 0 && errno ==EINTR);
    if (ret != sizeof(smmap_ctl_t)) {
        return ret != 0 ? ret : EINTR;
    }
    return 0;
}

static inline int smmap(void *addr, void *shadow_addr, size_t size)
{
    smmap_ctl_t ctl = { SMMAP_CTL_SMMAP,
        { .smmap = { addr, shadow_addr, size } }
    };
    return __smmap_ctl(&ctl);
}

static inline int smunmap(void *addr)
{
    smmap_ctl_t ctl = { SMMAP_CTL_SMUNMAP,
        { .smunmap = { addr } }
    };
    return __smmap_ctl(&ctl);
}

static int inline smctl(smmap_smctl_op_t op, void *ptr)
{
    smmap_ctl_t ctl = { SMMAP_CTL_SMCTL,
        { .smctl = { op, ptr } }
    };
    return __smmap_ctl(&ctl);
}

#endif /* _SMMAP_COMMON_H_ */

