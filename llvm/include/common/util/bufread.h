#ifndef __BUFREAD_H__
#define __BUFREAD_H__

#include <unistd.h>
#include <string.h>
#include "safefuncs.h"

/* We implement our own fgets()-like implementation since the libc fopen()
 * implementation uses malloc and other calls which can be intercepted by other
 * instrumentation (e.g address sanitizer) and cause issues. */

typedef struct {
    char *start;
    char *end;
    char *ptr;  // set this to 'end' on before reading
} read_buffer_t;

static inline long bufread(int fd, void *dst, size_t dstsize, read_buffer_t *buf) {
    long ret = 0, len = 0, new_data_was_read = 0;
    char *p;

    if (buf->ptr <= buf->start || buf->ptr > buf->end) {
        return -1; // ptr out of bounds
    }

    do {
        new_data_was_read = 0;
        if (buf->ptr != buf->end && *buf->ptr == '\0') {
            buf->ptr++; // Don't choke on \0 bytes in file
            break;
        }
        if (buf->ptr != buf->end) {
            // Return next data up until (including) newline
            p = strchrnul(buf->ptr, '\n');
            if (*p == '\n') p++;
            len = (long)p - (long)buf->ptr;
            if (ret + len >= dstsize - 1) len = dstsize - ret - 1;
            strncpy(dst, buf->ptr, len);
            buf->ptr = p;
            dst += len;
            ret += len;
            if (ret == dstsize - 1 || *(buf->ptr - 1) == '\n') {
                break;
            }
        }
        if (buf->ptr == buf->end) {
            // when at the end, fill io buffer with new data and continue
            memset(buf->start, 0, buf->end - buf->start + 1);
            char *readptr = buf->start;
            long to_read = (long)buf->end - (long)buf->start;
            while (to_read > 0) {
                long bytesread = 0;
                do {
                    bytesread = read(fd, readptr, to_read);
                } while (bytesread == -1 && errno == EINTR);
                if (bytesread <= -1) return -1; // Error
                if (bytesread == 0) break; // EOF
                to_read -= bytesread;
                readptr += bytesread;
            }
            buf->ptr = buf->start;
            new_data_was_read = 1; // new data. Iterate!
        }
    } while (new_data_was_read);

    return ret;
}

#endif
