/* Description: Recovery actions
 * All recovery actions are defined here
 *
 * Author : Koustubha Bhat
 * Date	  : 8-March-2017
 * Vrije Universiteit, Amsterdam.
 */

#include <dlfcn.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "rcvry_common.h"
#include "rcvry_actions.h"
#include "rcvry_util.h"
#include "../ltckpt/ltckpt_common.h"

#define RCVRY_RA_PROLOGUE(RA_TYPE, ARGC, ARGV)	({			\
	rcvry_print_debug("[site_id:%d] %s: initiating recovery.\n", 	\
	rcvry_current_site_id, ra_names[RA_TYPE]);			\
	assert(rcvry_info[rcvry_current_site_id].argc == ARGC 		\
	       && "rcvry_action: argc mismatch");			\
	ARGV = &(rcvry_info[rcvry_current_site_id].argv);		\
	})

#define RCVRY_RA_EPILOGUE(RA_TYPE, RET)		({			\
	rcvry_return_value = (void *) RET;				\
        operate_libcall_gate();						\
	rcvry_print_debug("%s: successful (ret: %d).\n",		\
			   ra_names[RA_TYPE], rcvry_return_value);	\
    (*(int*)rcvry_return_addr) = (int)rcvry_return_value;   \
	return RCVRY_E_OK;						\
	})

#define RCVRY_RA_EPILOGUE_ERRNO(RA_TYPE, RET, ERRNO)	({		\
	errno = ERRNO;							\
	RCVRY_RA_EPILOGUE(RA_TYPE, RET);				\
	})

volatile void *rcvry_return_value=NULL;
volatile void *rcvry_return_addr=NULL;

static char *ra_names[__NUM_RCVRY_TYPES];
extern char *rcvry_dispatched_action;

static void __attribute__((always_inline))
operate_libcall_gate()
{
#ifdef RCVRY_CLOSE_GATE_AFTER
    rcvry_libcall_gates[rcvry_current_site_id] = RCVRY_LIBCALL_GATE_CLOSE;
#else
    rcvry_libcall_gates[rcvry_current_site_id] = rcvry_current_tx_type;
#endif
}

void __attribute__((constructor)) rcvry_map_actions()
{
    // First initialize all the gates to OPEN
    for (int i=0; i < RCVRY_MAX_LIBCALL_SITES; i++)
        if (LTCKPT_TYPE_INVALID != rcvry_info[i].ltckpt_type) {
            rcvry_libcall_gates[i] = rcvry_info[i].ltckpt_type;
        } else {
#ifdef RCVRY_CKPT_DEFAULT_TO_UNDOLOG
            rcvry_libcall_gates[i] = LTCKPT_TYPE_UNDOLOG;
#else
            rcvry_libcall_gates[i] = LTCKPT_TYPE_TSX;
#endif
        }

#if TEST_ALTERNATE_TX   // Alternate gates set to ltckpt
    for (unsigned i=0; i < RCVRY_MAX_LIBCALL_SITES; i+=2) {
        rcvry_libcall_gates[i] = LTCKPT_TYPE_UNDOLOG;
    }
#endif

    ra_names[RCVRY_TYPE_DEFAULT] = "RCVRY_TYPE_DEFAULT";
    rcvry_actions_map[RCVRY_TYPE_DEFAULT] = rcvry_ra_fail;

    ra_names[RCVRY_TYPE_NOOP] = "RCVRY_TYPE_NOOP";
    rcvry_actions_map[RCVRY_TYPE_NOOP] = rcvry_ra_noop;

    ra_names[RCVRY_TYPE_FAIL] = "RCVRY_TYPE_FAIL";
    rcvry_actions_map[RCVRY_TYPE_FAIL] = rcvry_ra_fail;
    
    ra_names[RCVRY_TYPE_SET_RET_ERRNO] = "RCVRY_TYPE_SET_RET_ERRNO";
    rcvry_actions_map[RCVRY_TYPE_SET_RET_ERRNO] = rcvry_ra_set_ret_errno;

    ra_names[RCVRY_TYPE_SET_RET_ERRNO_EACCES] = "RCVRY_TYPE_SET_RET_ERRNO_EACCES";
    rcvry_actions_map[RCVRY_TYPE_SET_RET_ERRNO_EACCES] = rcvry_ra_set_ret_errno;
    ra_names[RCVRY_TYPE_SET_RET_ERRNO_ENOMEM] = "RCVRY_TYPE_SET_RET_ERRNO_ENOMEM";
    rcvry_actions_map[RCVRY_TYPE_SET_RET_ERRNO_ENOMEM] = rcvry_ra_set_ret_errno;
    ra_names[RCVRY_TYPE_SET_RET_ERRNO_ENOSPC] = "RCVRY_TYPE_SET_RET_ERRNO_ENOSPC";
    rcvry_actions_map[RCVRY_TYPE_SET_RET_ERRNO_ENOSPC] = rcvry_ra_set_ret_errno;
    ra_names[RCVRY_TYPE_SET_RET_ERRNO_EBADF] = "RCVRY_TYPE_SET_RET_ERRNO_EBADF";
    rcvry_actions_map[RCVRY_TYPE_SET_RET_ERRNO_EBADF] = rcvry_ra_set_ret_errno;
    ra_names[RCVRY_TYPE_SET_RET_ERRNO_EINVAL] = "RCVRY_TYPE_SET_RET_ERRNO_EINVAL";
    rcvry_actions_map[RCVRY_TYPE_SET_RET_ERRNO_EINVAL] = rcvry_ra_set_ret_errno;
    ra_names[RCVRY_TYPE_SET_RET_ERRNO_EFAULT] = "RCVRY_TYPE_SET_RET_ERRNO_EFAULT";
    rcvry_actions_map[RCVRY_TYPE_SET_RET_ERRNO_EFAULT] = rcvry_ra_set_ret_errno;
    ra_names[RCVRY_TYPE_SET_RET_ERRNO_EMFILE] = "RCVRY_TYPE_SET_RET_ERRNO_EMFILE";
    rcvry_actions_map[RCVRY_TYPE_SET_RET_ERRNO_EMFILE] = rcvry_ra_set_ret_errno;
    ra_names[RCVRY_TYPE_SET_RET_ERRNO_ZSTREAM_ERROR] = "RCVRY_TYPE_SET_RET_ERRNO_ZSTREAM_ERROR";
    rcvry_actions_map[RCVRY_TYPE_SET_RET_ERRNO_ZSTREAM_ERROR] = rcvry_ra_set_ret_errno;
    ra_names[RCVRY_TYPE_SET_RET_NULL_ERRNO_ENOMEM] = "RCVRY_TYPE_SET_RET_NULL_ERRNO_ENOMEM";
    rcvry_actions_map[RCVRY_TYPE_SET_RET_NULL_ERRNO_ENOMEM] = rcvry_ra_set_ret_errno;
    ra_names[RCVRY_TYPE_SET_RET_NULL_ERRNO_EBADF] = "RCVRY_TYPE_SET_RET_NULL_ERRNO_EBADF";
    rcvry_actions_map[RCVRY_TYPE_SET_RET_NULL_ERRNO_EBADF] = rcvry_ra_set_ret_errno;
    
    ra_names[RCVRY_TYPE_HW_TSX] = "RCVRY_TYPE_HW_TSX";
    rcvry_actions_map[RCVRY_TYPE_HW_TSX] = rcvry_ra_noop;

    ra_names[RCVRY_TYPE_UNDO_MALLOC] = "RCVRY_TYPE_UNDO_MALLOC";
    rcvry_actions_map[RCVRY_TYPE_UNDO_MALLOC] = rcvry_ra_undo_malloc;

    ra_names[RCVRY_TYPE_UNDO_MEMALIGN] = "RCVRY_TYPE_UNDO_MEMALIGN";
    rcvry_actions_map[RCVRY_TYPE_UNDO_MEMALIGN] = rcvry_ra_undo_memalign;

    ra_names[RCVRY_TYPE_UNDO_REALLOC] = "RCVRY_TYPE_UNDO_REALLOC";
    rcvry_actions_map[RCVRY_TYPE_UNDO_REALLOC] = rcvry_ra_fail;

    ra_names[RCVRY_TYPE_UNDO_SETLOCALE] = "RCVRY_TYPE_UNDO_SETLOCALE";
    rcvry_actions_map[RCVRY_TYPE_UNDO_SETLOCALE] = rcvry_ra_set_ret_errno; // returns NULL

    ra_names[RCVRY_TYPE_CLOSE_SOCKET] = "RCVRY_TYPE_CLOSE_SOCKET";
    rcvry_actions_map[RCVRY_TYPE_CLOSE_SOCKET] = rcvry_ra_close_socket;

    ra_names[RCVRY_TYPE_CLOSE_RET_NULL] = "RCVRY_TYPE_CLOSE_RET_NULL";
    rcvry_actions_map[RCVRY_TYPE_CLOSE_RET_NULL] = rcvry_ra_close;

    ra_names[RCVRY_TYPE_CLOSE] = "RCVRY_TYPE_CLOSE";
    rcvry_actions_map[RCVRY_TYPE_CLOSE] = rcvry_ra_close;

    ra_names[RCVRY_TYPE_DLCLOSE] = "RCVRY_TYPE_DLCLOSE";
    rcvry_actions_map[RCVRY_TYPE_DLCLOSE] = rcvry_ra_dlclose;

    ra_names[RCVRY_TYPE_UNDO_FORK] = "RCVRY_TYPE_UNDO_FORK";
    rcvry_actions_map[RCVRY_TYPE_UNDO_FORK] = rcvry_ra_undo_fork;

    ra_names[RCVRY_TYPE_UNDO_ADDRINFO] = "RCVRY_TYPE_UNDO_ADDRINFO";
    rcvry_actions_map[RCVRY_TYPE_UNDO_ADDRINFO] = rcvry_ra_undo_addrinfo;

    ra_names[RCVRY_TYPE_UNDO_MKDIR] = "RCVRY_TYPE_UNDO_MKDIR";
    rcvry_actions_map[RCVRY_TYPE_UNDO_MKDIR] = rcvry_ra_undo_mkdir;

    ra_names[RCVRY_TYPE_UNDO_MMAP] = "RCVRY_TYPE_UNDO_MMAP";
    rcvry_actions_map[RCVRY_TYPE_UNDO_MMAP] = rcvry_ra_undo_mmap;

    ra_names[RCVRY_TYPE_UNDO_PTHRDMUTX_INIT] = "RCVRY_TYPE_UNDO_PTHRDMUTX_INIT";
    rcvry_actions_map[RCVRY_TYPE_UNDO_PTHRDMUTX_INIT] = rcvry_ra_undo_pthread_mutex_init;

    ra_names[RCVRY_TYPE_UNDO_PTHRDMUTX_LOCK] = "RCVRY_TYPE_UNDO_PTHRDMUTX_LOCK";
    rcvry_actions_map[RCVRY_TYPE_UNDO_PTHRDMUTX_LOCK] = rcvry_ra_undo_pthread_mutex_lock;

    return;
}

int rcvry_check_swfault_exists()
{
    void __attribute__((unused)) **argv;
    int __attribute__((unused)) set_ret, set_errno;
    int c_pid, c_status;

    // The testing child should never come here.
    if (RCVRY_CHILD_STARTED == rcvry_child_state) {
        exit(RCVRY_CHILD_ERROR);
    }

    // 1. fork() to find out if there is a s/w fault, or is it just transient.
    rcvry_child_state = RCVRY_CHILD_INVALID;
    c_pid = rcvry_fork();
    if (c_pid == 0) {		/* child process */
	    rcvry_child_state = RCVRY_CHILD_STARTED;
	    return RCVRY_TSX_RA_SW_OK;	// Allow the child to retry the same, until the next libcall
    } else if (c_pid > 0) {	/* parent process */
	    rcvry_print_info("%s: Waiting for child process: %d\n", __func__, c_pid);
	    if (-1 == waitpid(c_pid, &c_status, __WCLONE)) {
	        rcvry_print_error("%s: waitpid() failed.\n", __func__);
	        return RCVRY_E_FAIL;
	    }
    } else {
	    rcvry_print_error("%s: fork() failed.\n", __func__);
	    return RCVRY_E_FAIL;
    }

    // 2. steer to recovery path if s/w fault
    if (WIFSIGNALED(c_status)) {
        // Child must have been signalled. Let's steer towards error-path
	    rcvry_print_error("%s: swfault-checking child terminated by signal: %d\n.",
                           __func__, WTERMSIG(c_status));
	    return RCVRY_TSX_RA_SW_FAULT;
    }
    if (WIFEXITED(c_status)) {
        rcvry_print_info("%d Child (pid: %d) exit status: %d\n", getpid(),
                          c_pid, WEXITSTATUS(c_status));
	    if (RCVRY_CHILD_EXIT_OK == WEXITSTATUS(c_status)) {
	        return RCVRY_TSX_RA_SW_OK;	// Allow the parent to retry the same
        } else if (RCVRY_CHILD_ERROR == WEXITSTATUS(c_status)) {
	        rcvry_print_error("%s: swfault-checking child exited with an error.\n", __func__);
	        return RCVRY_TSX_RA_SW_FAULT;	// Something bad happened. Cannot conclusively determine.
        }
    }

    rcvry_print_error("%s: Unknown error occured in child that tests TSX window.", __func__);
    return RCVRY_TSX_RA_SW_FAULT;
}

int rcvry_ra_noop()
{
#ifdef RCVRY_WINDOW_PROFILING
      rcvry_info[rcvry_current_site_id].rcvry_prof.num_rcvry_ra__noop++;
#endif
    operate_libcall_gate();
    rcvry_print_debug("%s: successful.\n", ra_names[RCVRY_TYPE_NOOP]);

    RCVRY_RA_EPILOGUE(RCVRY_TYPE_NOOP, -1);
    return RCVRY_E_OK;
}

int rcvry_ra_fail()
{
#ifdef RCVRY_WINDOW_PROFILING
      rcvry_info[rcvry_current_site_id].rcvry_prof.num_rcvry_ra__fail++;
#endif
    operate_libcall_gate();
    rcvry_print_debug("%s: successful.\n", ra_names[RCVRY_TYPE_FAIL]);
    return RCVRY_E_FAIL;

}

int rcvry_ra_set_ret_errno()
{
    void **argv;

#ifdef RCVRY_WINDOW_PROFILING
      rcvry_info[rcvry_current_site_id].rcvry_prof.num_rcvry_ra__set_ret_errno++;
#endif
    RCVRY_RA_PROLOGUE(RCVRY_TYPE_SET_RET_ERRNO, 2, argv);
    int ret = (int) argv[0];
    RCVRY_RA_EPILOGUE_ERRNO(RCVRY_TYPE_SET_RET_ERRNO, ret, EACCES);
    return RCVRY_E_OK;
}

int rcvry_ra_undo_malloc()
{
    void **argv;

#ifdef RCVRY_WINDOW_PROFILING
      rcvry_info[rcvry_current_site_id].rcvry_prof.num_rcvry_ra__undo_malloc++;
#endif
    RCVRY_RA_PROLOGUE(RCVRY_TYPE_UNDO_MALLOC, 1, argv);
    rcvry_print_debug("[%s] argv[0] (malloc-ed pointer) is AT: %p\n", __func__, argv[0]);

    if (NULL != argv[0]) {
        void **ptr = (void **)(argv[0]);    // We need to dereference the retVal
        rcvry_print_debug("[%s] *argv[0] (malloc-ed pointer) is: %p\n", __func__, *ptr);
        if (NULL != *ptr) {
            free(*ptr);
        }
    }

    RCVRY_RA_EPILOGUE_ERRNO(RCVRY_TYPE_UNDO_MALLOC, NULL, ENOMEM);
    return RCVRY_E_OK;
}

int rcvry_ra_undo_memalign()
{
    void **argv;

#ifdef RCVRY_WINDOW_PROFILING
      rcvry_info[rcvry_current_site_id].rcvry_prof.num_rcvry_ra__undo_memalign++;
#endif
    RCVRY_RA_PROLOGUE(RCVRY_TYPE_UNDO_MEMALIGN, 1, argv);
    rcvry_print_debug("[%s] argv[0] (memalign-ed pointer) is AT: %p\n", __func__, argv[0]);

    if (NULL != argv[0]) {
        void **ptr = (void **)(argv[0]);    // We need to dereference the argument
        rcvry_print_debug("[%s] *argv[0] (memaligned-ed pointer) is: %p\n", __func__, *ptr);
        if (NULL != *ptr) {
            free(*ptr);
        }
    }

    RCVRY_RA_EPILOGUE(RCVRY_TYPE_UNDO_MEMALIGN, ENOMEM);
    return RCVRY_E_OK;
}

int rcvry_ra_undo_realloc()
{
    void **argv;

#ifdef RCVRY_WINDOW_PROFILING
      rcvry_info[rcvry_current_site_id].rcvry_prof.num_rcvry_ra__undo_memalign++;
#endif
    RCVRY_RA_PROLOGUE(RCVRY_TYPE_UNDO_REALLOC, 2, argv);
    rcvry_print_debug("[%s] argv[0] (realloc return value) is AT: %p\n", __func__, argv[0]);
    rcvry_print_debug("[%s] argv[1] (realloc() ptr) is AT: %p\n", __func__, argv[1]);

    if (NULL != argv[0]) {
        void *ptr = (void *)(argv[1]);
        if (NULL != ptr) {
            ptr = argv[0]; // set actual returned value to ptr (TODO: This is not really sufficient when we consider pointer copies lying around)
        }
    }

    RCVRY_RA_EPILOGUE(RCVRY_TYPE_UNDO_MEMALIGN, NULL);
    return RCVRY_E_OK;
}

int rcvry_ra_close_socket()
{
    void **argv;
    int fd, set_errno;

#ifdef RCVRY_WINDOW_PROFILING
      rcvry_info[rcvry_current_site_id].rcvry_prof.num_rcvry_ra__close_socket++;
#endif
    RCVRY_RA_PROLOGUE(RCVRY_TYPE_CLOSE_SOCKET, 3, argv);

    fd = (int)(argv[0]);
    set_errno = (int) argv[1];
    rcvry_print_info("[%s] argv[0] (fd) is %d\n", __func__, fd);
    rcvry_print_info("[%s] argv[1] (errno) is %d\n", __func__, set_errno);

    assert(fd > 0 && "Invalid socket descriptor");
    if (-1 == close(fd)) {
	    rcvry_print_error("%s: close() socket failed.\n", ra_names[RCVRY_TYPE_CLOSE_SOCKET]);
	    return RCVRY_E_FAIL;
    }

    rcvry_print_debug("%s: rcvry_return_addr: %p, rcvry_return_value: %d\n", __func__, (int*)rcvry_return_addr, (int)rcvry_return_value);
    RCVRY_RA_EPILOGUE_ERRNO(RCVRY_TYPE_CLOSE_SOCKET, -1, set_errno);
    return RCVRY_E_OK;
}

int rcvry_ra_close()
{
    void **argv;
    int fd, set_retval, set_errno;

#ifdef RCVRY_WINDOW_PROFILING
      rcvry_info[rcvry_current_site_id].rcvry_prof.num_rcvry_ra__close++;
#endif
    RCVRY_RA_PROLOGUE(RCVRY_TYPE_CLOSE, 3, argv);

    fd = (int)((*(int *)argv[0]) & 0xFFFFFFFF);  // We need to dereference the retVal
    set_retval = (int)(argv[1]);
    set_errno = (int)(argv[2]);
    rcvry_print_info("[%s] argv[0] (fd) is %d\n", __func__, fd);
    rcvry_print_info("[%s] argv[0] (ret) is %d\n", __func__, set_retval);
    rcvry_print_info("[%s] argv[1] (errno) is %d\n", __func__, set_errno);

    //assert(fd > 0 && "Invalid descriptor");
    if ((fd > 0) && (-1 == close(fd))) {
	rcvry_print_error("%s: close() failed.\n", ra_names[RCVRY_TYPE_CLOSE]);
	return RCVRY_E_FAIL;
    }

    RCVRY_RA_EPILOGUE_ERRNO(RCVRY_TYPE_CLOSE, set_retval, set_errno);
    return RCVRY_E_OK;
}

int rcvry_ra_dlclose()
{
    void **argv;

#ifdef RCVRY_WINDOW_PROFILING
      rcvry_info[rcvry_current_site_id].rcvry_prof.num_rcvry_ra__dlclose++;
#endif
    RCVRY_RA_PROLOGUE(RCVRY_TYPE_DLCLOSE, 1, argv);
    rcvry_print_info("[%s] argv[0] (handle) is %p\n", __func__, argv[0]);

    assert(NULL != argv[0]);
    // We need to dereference the retVal location received via argv[0]
    if (0 != dlclose(*(void **)argv[0])) {
	rcvry_print_error("%s: dlclose() failed.\n", ra_names[RCVRY_TYPE_DLCLOSE]);
	return RCVRY_E_FAIL;
    }

    RCVRY_RA_EPILOGUE(RCVRY_TYPE_DLCLOSE, -1);
    return RCVRY_E_OK;
}

int rcvry_ra_undo_fork()
{
    void **argv;
    int ret, status, c_id;

#ifdef RCVRY_WINDOW_PROFILING
      rcvry_info[rcvry_current_site_id].rcvry_prof.num_rcvry_ra__undo_fork++;
#endif
    RCVRY_RA_PROLOGUE(RCVRY_TYPE_UNDO_FORK, 1, argv);

    ret = (int)(*(int *)argv[0]);    // We need to dereference the retVal received here.
    rcvry_print_info("[%s] argv[0] (ret) is %d\n", __func__, ret);

    if (0 == ret) { 		/* child process */
	exit(0);
    } else if ( ret > 0 ) {	/* parent process */ 
	c_id = waitpid(ret, &status, 0);
	if (-1 == c_id) {
	    rcvry_print_error("%s: waitpid() failed (%d).\n", ra_names[RCVRY_TYPE_UNDO_FORK], c_id);
	    return RCVRY_E_FAIL;
        }
    } else {
	rcvry_print_error("%s: already in error-path of fork() libcall.", ra_names[RCVRY_TYPE_UNDO_FORK]);
	return RCVRY_E_FAIL;
    }

    RCVRY_RA_EPILOGUE_ERRNO(RCVRY_TYPE_UNDO_FORK, -1, ENOMEM);
    return RCVRY_E_OK;
}

int rcvry_ra_undo_addrinfo()
{
    void **argv;
    struct addrinfo **res;
    int set_ret;

#ifdef RCVRY_WINDOW_PROFILING
      rcvry_info[rcvry_current_site_id].rcvry_prof.num_rcvry_ra__undo_addrinfo++;
#endif
    RCVRY_RA_PROLOGUE(RCVRY_TYPE_UNDO_ADDRINFO, 2, argv);
 
    res = (struct addrinfo **)(argv[0]);
    set_ret = (int)(argv[1]);
    rcvry_print_info("[%s] argv[0] (res) is %p\n", __func__, res);
    rcvry_print_info("[%s] argv[1] (ret) is %d\n", __func__, set_ret);

    if (NULL != res) {
	freeaddrinfo(*res);
    }

    RCVRY_RA_EPILOGUE(RCVRY_TYPE_UNDO_ADDRINFO, set_ret);
    return RCVRY_E_OK;
}

int rcvry_ra_undo_mkdir()
{
    void **argv;
    char *path = NULL;
    int set_errno;

#ifdef RCVRY_WINDOW_PROFILING
      rcvry_info[rcvry_current_site_id].rcvry_prof.num_rcvry_ra__undo_mkdir++;
#endif
    RCVRY_RA_PROLOGUE(RCVRY_TYPE_UNDO_MKDIR, 2, argv);

    path = (char *)(argv[0]);
    set_errno = (int)(argv[1]);
    rcvry_print_info("[%s] argv[0] (path) is %s\n", __func__, path);
    rcvry_print_info("[%s] argv[1] (errno) is %d\n", __func__, errno);

	if (0 != rmdir(path)) {
	    rcvry_print_error("%s: rmdir(%s) failed.\n", ra_names[RCVRY_TYPE_UNDO_MKDIR], path);
	    return RCVRY_E_FAIL;
	}

    RCVRY_RA_EPILOGUE_ERRNO(RCVRY_TYPE_UNDO_MKDIR, -1, set_errno);
    return RCVRY_E_OK;
}

int rcvry_ra_undo_mmap()
{
    void **argv;
    int len;
    int set_errno;

#ifdef RCVRY_WINDOW_PROFILING
      rcvry_info[rcvry_current_site_id].rcvry_prof.num_rcvry_ra__undo_mmap++;
#endif
    RCVRY_RA_PROLOGUE(RCVRY_TYPE_UNDO_MMAP, 3, argv);

    len = (int)argv[1];
    set_errno = (int)(argv[2]);
    rcvry_print_info("[%s] argv[0] (len) is %d\n", __func__, len);
    rcvry_print_info("[%s] argv[0] (errno) is %d\n", __func__, set_errno);

    if (-1 == munmap(argv[0], len)) {
	rcvry_print_debug("%s: munmap() failed.\n", ra_names[RCVRY_TYPE_UNDO_MMAP]);
	return RCVRY_E_FAIL;
    }

    RCVRY_RA_EPILOGUE_ERRNO(RCVRY_TYPE_UNDO_MMAP, -1, set_errno);
    return RCVRY_E_OK;
}

int rcvry_ra_undo_pthread_mutex_init()
{
    void *mutex, **argv;
    int set_ret;

#ifdef RCVRY_WINDOW_PROFILING
      rcvry_info[rcvry_current_site_id].rcvry_prof.num_rcvry_ra__undo_pthreadmtx_init++;
#endif
    RCVRY_RA_PROLOGUE(RCVRY_TYPE_UNDO_PTHRDMUTX_INIT, 2, argv);
    mutex = (pthread_mutex_t *)(argv[0]);
    set_ret = (int)(argv[1]);
    rcvry_print_info("[%s] argv[0] (mutex) is %p\n", __func__, mutex);
    rcvry_print_info("[%s] argv[1] (ret) is %d\n", __func__, set_ret);

    if (-1 == pthread_mutex_destroy(mutex)) {
	rcvry_print_debug("%s: pthread_mutex_destroy() failed.\n", ra_names[RCVRY_TYPE_UNDO_PTHRDMUTX_INIT]);
	return RCVRY_E_FAIL;
    }

    RCVRY_RA_EPILOGUE(RCVRY_TYPE_UNDO_PTHRDMUTX_INIT, set_ret);
    return RCVRY_E_OK;
}

int rcvry_ra_undo_pthread_mutex_lock()
{
    void *mutex, **argv;
    int set_ret;

#ifdef RCVRY_WINDOW_PROFILING
      rcvry_info[rcvry_current_site_id].rcvry_prof.num_rcvry_ra__undo_pthreadmtx_lock++;
#endif
    RCVRY_RA_PROLOGUE(RCVRY_TYPE_UNDO_PTHRDMUTX_LOCK, 2, argv);
    mutex = (pthread_mutex_t *)(argv[0]);
    set_ret = (int)(argv[1]);
    rcvry_print_info("[%s] argv[0] (mutex) is %p\n", __func__, mutex);
    rcvry_print_info("[%s] argv[1] (ret) is %d\n", __func__, set_ret);

    if (-1 == pthread_mutex_unlock(mutex)) {
	rcvry_print_debug("%s: pthread_mutex_unlock() failed.\n", ra_names[RCVRY_TYPE_UNDO_PTHRDMUTX_LOCK]);
	return RCVRY_E_FAIL;
    }

    RCVRY_RA_EPILOGUE(RCVRY_TYPE_UNDO_PTHRDMUTX_LOCK, set_ret);
    return RCVRY_E_OK;
}

#define RCVRY_FREE_LIST_SIZE    16384
static void *freeList[RCVRY_FREE_LIST_SIZE];
static int freeListPos = -1;
uint32_t rcvry_num_delayed_free_calls = 0;

void rcvry_ra_delayed_free(void *ptr)
{
    assert(++freeListPos < RCVRY_FREE_LIST_SIZE);
    freeList[freeListPos] = ptr;
    rcvry_num_delayed_free_calls++;
}

void rcvry_ra_free_freelist()
{
    for (int i=0; i < freeListPos; i++) {
        free(freeList[i]);
    }
    freeListPos = -1;
}
