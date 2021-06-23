#ifndef RCVRY_ACTIONS_H
#define RCVRY_ACTIONS_H

extern volatile void *rcvry_return_value;
extern volatile void *rcvry_return_addr;
extern uint32_t rcvry_num_delayed_free_calls;

int rcvry_ra_hw_tsx();
int rcvry_ra_noop();
int rcvry_ra_fail();
int rcvry_ra_set_ret_errno();
int rcvry_ra_undo_malloc();
int rcvry_ra_undo_memalign();
int rcvry_ra_close_socket();
int rcvry_ra_close();
int rcvry_ra_dlclose();
int rcvry_ra_undo_fork();
int rcvry_ra_undo_addrinfo();
int rcvry_ra_undo_mkdir();
int rcvry_ra_undo_mmap();
int rcvry_ra_undo_pthread_mutex_init();
int rcvry_ra_undo_pthread_mutex_lock();
void rcvry_ra_delayed_free(void *ptr);
void rcvry_ra_free_freelist();

#endif
