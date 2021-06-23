#ifndef _EDFI_CTL_SERVER_H
#define _EDFI_CTL_SERVER_H

#include <edfi/common.h>
#include <edfi/df/df.h>
#include <string.h>

/* CTL server API. */
int edfi_ctl_process_request(void *ctl_request);

void edfi_print_stacktrace();
unsigned long edfi_get_dynamic_fault_id();

void edfi_printf(char* fmt, ...);
void edfi_srand(unsigned int seed);
int edfi_rand();
unsigned long long edfi_getcurrtime_ns(void);
int edfi_load_dflib(const char* path);
void *edfi_context_realloc(void);

static inline unsigned long edfi_hash(unsigned long hash, const char *s)
{
    for (; *s != '\0'; s++) {
        hash = *s + 31 * hash;
    }

    return hash;
}

static inline unsigned long edfi_get_static_fault_id(const char* file, int line)
{
    char buff[32];
    unsigned long hash = 0;
    memset(buff, 0, sizeof(buff));
    memcpy(buff, &line, sizeof(int));
    hash = edfi_hash(hash, file);
    hash = edfi_hash(hash, buff);

    return hash;
}

#endif

