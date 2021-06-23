#ifndef LTCKPT_LTCKPT_TYPES_H
#define LTCKPT_LTCKPT_TYPES_H 1

#include <stdint.h>

#define LTCKPT_ALIGNED(x) __attribute__((aligned(16)));

#define LTCKPT_WEAK __attribute__((weak))
#define LTCKPT_INSTFUNCT __attribute__((used))
#define LTCKPT_NOINLINE __attribute__((noinline))
#define LTCKPT_ALWAYSINLINE __attribute__((always_inline))

typedef uint8_t  ltckpt_u8_t;
typedef uint16_t ltckpt_u16_t;
typedef uint32_t ltckpt_u32_t;
typedef uint64_t ltckpt_u64_t;

typedef struct ltckpt_m128_s { 
	ltckpt_u8_t data[16];
} ltckpt_m128_t __attribute__((aligned(16)));

typedef uintptr_t ltckpt_va_t;

#define LTCKPT_VA_TO_PTR(x) ((void*)x)
#define LTCKPT_PTR_TO_VA(x) ((ltckpt_va_t)x)

typedef struct ltckpt_early_init_data_s {
	int (*main) (int, char * *, char * *);
	int argc;
	char * * ubp_av;
	void (*init) (void);
	void (*fini) (void);
	void (*rtld_fini) (void);
	void * libc_start_main;
	void * stack_end;
} ltckpt_early_init_data_t;

typedef enum ltckpt_type_e {
        LTCKPT_TYPE_INVALID = 0,
        LTCKPT_TYPE_UNDOLOG,
        LTCKPT_TYPE_TSX,
        LTCKPT_TYPE_TSX_END
} ltckpt_type_t;

#endif
