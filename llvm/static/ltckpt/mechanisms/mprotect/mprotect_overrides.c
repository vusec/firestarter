#define _GNU_SOURCE

#define LTCKPT_WRAPPER_METHOD mprotect

#include "../../ltckpt_local.h"

#include <sys/select.h>
#include <unistd.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>

/* Workarounds for EFAULT-prone system calls using copy_to_user(). */
volatile char ltckpt_check_write_sink;
#define LTCKPT_CHECK_WRITE(B) do { \
	if (B) {\
		ltckpt_check_write_sink = *((char*)(B)); \
		*((char*)(B)) = ltckpt_check_write_sink; \
	} \
} while(0)

#define LTCKPT_CHECK_BUFF(B, S) do { \
	int __i; \
	char *__b; \
	if ((B) && (S) > 0) { \
		for (__i=0;__i<(S);__i+=PAGE_SIZE) { \
			__b = &((char*)(B))[__i]; \
			LTCKPT_CHECK_WRITE(__b); \
		} \
		__b = &((char*)(B))[(S)-1]; \
		LTCKPT_CHECK_WRITE(__b); \
	} \
} while(0)

/* epoll_wait() */
LTCKPT_WRAPPER(int, epoll_wait,
	LTCKPT_CONCAT(int epfd, struct epoll_event *events, int maxevents,
		int timeout),
	LTCKPT_CONCAT(epfd, events, maxevents, timeout),

	LTCKPT_CHECK_WRITE(events);
)

/* poll() */
LTCKPT_WRAPPER(int, poll,
	LTCKPT_CONCAT(struct pollfd *fds, nfds_t nfds, int timeout),
	LTCKPT_CONCAT(fds, nfds, timeout),

	LTCKPT_CHECK_WRITE(fds);
)

/* select() */
LTCKPT_WRAPPER(int, select,
	LTCKPT_CONCAT(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
		struct timeval *timeout),
	LTCKPT_CONCAT(nfds, readfds, writefds, exceptfds, timeout),

	LTCKPT_CHECK_WRITE(readfds);
	LTCKPT_CHECK_WRITE(writefds);
	LTCKPT_CHECK_WRITE(exceptfds);
)

/* read() */
LTCKPT_WRAPPER(ssize_t, read,
	LTCKPT_CONCAT(int fd, void *buf, size_t count),
	LTCKPT_CONCAT(fd, buf, count),

	LTCKPT_CHECK_BUFF(buf, count);
)

/* accept() */
LTCKPT_WRAPPER(int, accept,
	LTCKPT_CONCAT(int sockfd, struct sockaddr *addr, socklen_t *addrlen),
	LTCKPT_CONCAT(sockfd, addr, addrlen),

	LTCKPT_CHECK_WRITE(addr);
	LTCKPT_CHECK_WRITE(addrlen);
)

/* accept4() */
LTCKPT_WRAPPER(int, accept4,
	LTCKPT_CONCAT(int sockfd, struct sockaddr *addr,
		socklen_t *addrlen, int flags),
	LTCKPT_CONCAT(sockfd, addr, addrlen, flags),

	LTCKPT_CHECK_WRITE(addr);
	LTCKPT_CHECK_WRITE(addrlen);
)

/* recv() */
LTCKPT_WRAPPER(ssize_t, recv,
	LTCKPT_CONCAT(int sockfd, void *buf, size_t len, int flags),
	LTCKPT_CONCAT(sockfd, buf, len, flags),

	LTCKPT_CHECK_BUFF(buf, len);
)

/* recvfrom() */
LTCKPT_WRAPPER(ssize_t, recvfrom,
	LTCKPT_CONCAT(int sockfd, void *buf, size_t len, int flags,
		struct sockaddr *src_addr, socklen_t *addrlen),
	LTCKPT_CONCAT(sockfd, buf, len, flags, src_addr, addrlen),

	LTCKPT_CHECK_BUFF(buf, len);
	LTCKPT_CHECK_WRITE(src_addr);
	LTCKPT_CHECK_WRITE(addrlen);
)

/* recvmsg() */
LTCKPT_WRAPPER(ssize_t, recvmsg,
	LTCKPT_CONCAT(int sockfd, struct msghdr *msg, int flags),
	LTCKPT_CONCAT(sockfd, msg, flags),

	if (msg) {
		int i;
		LTCKPT_CHECK_BUFF(msg->msg_control, msg->msg_controllen);
		for (i=0;i<msg->msg_iovlen;i++) {
			struct iovec *iov = &msg->msg_iov[i];
			LTCKPT_CHECK_BUFF(iov->iov_base, iov->iov_len);
		}
		LTCKPT_CHECK_WRITE(msg);
	}
)

/* semop() */
LTCKPT_WRAPPER(int, semop,
	LTCKPT_CONCAT(int semid, struct sembuf *sops, size_t nsops),
	LTCKPT_CONCAT(semid, sops, nsops),

	LTCKPT_CHECK_WRITE(sops);
)


/* stat() */
int stat(const char *path, struct stat *buf)
{
	LTCKPT_CHECK_WRITE(buf);
	return __xstat(_STAT_VER, path, buf);
}
LTCKPT_WRAPPER(int, __xstat,
	LTCKPT_CONCAT(int __ver, const char *path, struct stat *buf),
	LTCKPT_CONCAT(__ver, path, buf),

	LTCKPT_CHECK_WRITE(buf);
)

/* fstat() */
int fstat(int fd, struct stat *buf)
{
	LTCKPT_CHECK_WRITE(buf);
	return __fxstat(_STAT_VER, fd, buf);
}
LTCKPT_WRAPPER(int, __fxstat,
	LTCKPT_CONCAT(int __ver, int fd, struct stat *buf),
	LTCKPT_CONCAT(__ver, fd, buf),

	LTCKPT_CHECK_WRITE(buf);
)

/* lstat() */
int lstat(const char *path, struct stat *buf)
{
	LTCKPT_CHECK_WRITE(buf);
	return __lxstat(_STAT_VER, path, buf);
}
LTCKPT_WRAPPER(int, __lxstat,
	LTCKPT_CONCAT(int __ver, const char *path, struct stat *buf),
	LTCKPT_CONCAT(__ver, path, buf),

	LTCKPT_CHECK_WRITE(buf);
)

