#ifndef __SAFEFUNCS_H__
#define __SAFEFUNCS_H__

#ifdef USE_SAFEFUNCS

#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>

/*
 Include this to ensure that other instrumentation does not interpose calls to these functions.
 We use function pointers loaded from the dynamic libraries to ensure noone can interpose them.
 */


  // #ifdef DEFINE_SAFEFUNCS
  //   rettype (* # F # )( # args # ) = 0;
  // #else
  //   extern rettype (* # F # )( # args # );
  // #endif
  // DEFINE_SAFE_WRAPPER(

#ifdef DEFINE_SAFEFUNCS
#undef DEFINE_SAFEFUNCS
void *(*__safe_memset_fptr)(void *s, int c, size_t n) = 0;
void *(*__safe_memcpy_fptr)(void *dest, const void *src, size_t n) = 0;
int (*__safe_memcmp_fptr)(const void *s1, const void *s2, size_t n) = 0;
char *(*__safe_strncpy_fptr)(char *dest, const char *src, size_t n) = 0;
int (*__safe_vsscanf_fptr)(const char *str, const char *format, va_list ap) = 0;
int (*__safe_strcmp_fptr)(const char *s1, const char *s2) = 0;
size_t (*__safe_strlen_fptr)(const char *s) = 0;
int (*__safe_fflush_fptr)(FILE *stream) = 0;
int (*__safe_vsprintf_fptr)(char *str, const char *format, va_list ap) = 0;
ssize_t (*__safe_read_fptr)(int fd, void *buf, size_t count) = 0;
ssize_t (*__safe_pread_fptr)(int fd, void *buf, size_t count, off_t offset) = 0;
ssize_t (*__safe_write_fptr)(int fd, const void *buf, size_t count) = 0;
ssize_t (*__safe_pwrite_fptr)(int fd, const void *buf, size_t count, off_t offset) = 0;
void *(*__safe_mmap_fptr)(void *addr, size_t length, int prot, int flags, int fd, off_t offset) = 0;
#else
extern void *(*__safe_memset_fptr)(void *s, int c, size_t n);
extern void *(*__safe_memcpy_fptr)(void *dest, const void *src, size_t n);
extern int (*__safe_memcmp_fptr)(const void *s1, const void *s2, size_t n);
extern char *(*__safe_strncpy_fptr)(char *dest, const char *src, size_t n);
extern int (*__safe_vsscanf_fptr)(const char *str, const char *format, va_list ap);
extern int (*__safe_strcmp_fptr)(const char *s1, const char *s2);
extern size_t (*__safe_strlen_fptr)(const char *s);
extern int (*__safe_fflush_fptr)(FILE *stream);
extern int (*__safe_vsprintf_fptr)(char *str, const char *format, va_list ap);
extern ssize_t (*__safe_read_fptr)(int fd, void *buf, size_t count);
extern ssize_t (*__safe_pread_fptr)(int fd, void *buf, size_t count, off_t offset);
extern ssize_t (*__safe_write_fptr)(int fd, const void *buf, size_t count);
extern ssize_t (*__safe_pwrite_fptr)(int fd, const void *buf, size_t count, off_t offset);
extern void *(*__safe_mmap_fptr)(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
#endif



#define LOAD_SAFE_FUNC(F) __safe_ ## F ## _fptr = dlsym(RTLD_NEXT, #F )
static inline void init_safe_funcs() {
  LOAD_SAFE_FUNC(memset);
  LOAD_SAFE_FUNC(memcpy);
  LOAD_SAFE_FUNC(memcmp);
  LOAD_SAFE_FUNC(strncpy);
  LOAD_SAFE_FUNC(strcmp);
  LOAD_SAFE_FUNC(strlen);
  LOAD_SAFE_FUNC(vsscanf);
  LOAD_SAFE_FUNC(fflush);

  LOAD_SAFE_FUNC(vsprintf);
  LOAD_SAFE_FUNC(read);
  LOAD_SAFE_FUNC(pread);
  LOAD_SAFE_FUNC(write);
  LOAD_SAFE_FUNC(pwrite);
  //LOAD_SAFE_FUNC(realloc);

  LOAD_SAFE_FUNC(mmap);
}

static inline void *__safe_memset(void *s, int c, size_t n) {
    return __safe_memset_fptr(s, c, n);
}

static inline void *__safe_memcpy(void *dest, const void *src, size_t n) {
    return __safe_memcpy_fptr(dest, src, n);
}

static inline int __safe_memcmp(const void *s1, const void *s2, size_t n) {
    return __safe_memcmp_fptr(s1, s2, n);
}

static inline char *__safe_strncpy(char *dest, const char *src, size_t n) {
    return __safe_strncpy_fptr(dest, src, n);
}

static inline int __safe_sscanf(const char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = __safe_vsscanf_fptr(str, format, args);
    va_end(args);
    return ret;
}

static inline int __safe_strcmp(const char *s1, const char *s2) {
    return __safe_strcmp_fptr(s1, s2);
}

static inline size_t __safe_strlen(const char *s) {
    return __safe_strlen_fptr(s);
}

static inline int __safe_fflush(FILE *stream) {
    return __safe_fflush_fptr(stream);
}

static inline int __safe_sprintf(char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = __safe_vsprintf_fptr(str, format, args);
    va_end(args);
    return ret;
}

static inline ssize_t __safe_read(int fd, void *buf, size_t count) {
    return __safe_read_fptr(fd, buf, count);
}

static inline ssize_t __safe_pread(int fd, void *buf, size_t count, off_t offset) {
    return __safe_pread_fptr(fd, buf, count, offset);
}

static inline ssize_t __safe_write(int fd, const void *buf, size_t count) {
    return __safe_write_fptr(fd, buf, count);
}

static inline ssize_t __safe_pwrite(int fd, const void *buf, size_t count, off_t offset) {
    return __safe_pwrite_fptr(fd, buf, count, offset);
}

static inline void *__safe_mmap(void *addr, size_t length, int prot, int flags,
                                int fd, off_t offset) {
    return __safe_mmap_fptr(addr, length, prot, flags, fd, offset);
}


#define memset __safe_memset
#define memcpy __safe_memcpy
#define memcmp __safe_memcmp
#define strncpy __safe_strncpy
#define sscanf __safe_sscanf
#define strcmp __safe_strcmp
#define strlen __safe_strlen
#define fflush __safe_fflush

#define sprintf __safe_sprintf
#define read __safe_read
#define pread __safe_pread
#define write __safe_write
#define pwrite __safe_pwrite
//#define mmap __safe_mmap

//#define realloc __safe_realloc

#endif
#endif

// __interceptor_backtrace
// __interceptor_backtrace_symbols
// __interceptor_free
// __interceptor_getenv
// __interceptor_memcpy  DONE
// __interceptor_memmove
// __interceptor_mmap    DONE
// __interceptor_pread   DONE
// __interceptor_pread64
// __interceptor_pwrite  DONE
// __interceptor_read    DONE
// __interceptor_readlink
// __interceptor_realloc
// __interceptor_snprintf
// __interceptor_sprintf DONE
// __interceptor_strlen  DONE
// __interceptor_strstr
// __interceptor_strtol
// __interceptor_vfprintf
// __interceptor_waitpid
// __interceptor_write   DONE

