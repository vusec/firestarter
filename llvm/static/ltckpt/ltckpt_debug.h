#ifndef __MINIX

#define lt_kputs(X)
#define lt_kpucs(X)
#define lt_malloc(X)
#define lt_free(X)
#define lt_putn(X)
#define lt_putx(X)
#define lt_printf(...)

#define lt_assert(X)
#else
void lt_printf(const char *fmt, ...)
     __attribute__((__format__(__printf__,1,2)));
void *lt_malloc(int size);
void lt_free(void *);

/* we can't call many functions. */
#define STRINGIT2(l) #l
#define STRINGIT(l) STRINGIT2(l)
#define lt_assert(condition) { if(!(condition)) { lt_printf("assert failed in " __FILE__ ": " STRINGIT(__LINE__) ": " #condition "\n"); ((void(*)(void))((__LINE__ % 4096)))(); } } /* jump to PC indicating lineno */

#endif

