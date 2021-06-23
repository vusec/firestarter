ROOT=../../..

include $(ROOT)/Makefile.inc

MODULE = ltckpt

CFLAGS+=-D_GLIBCXX_USE_CXX11_ABI=0

ifeq ($(Plat),Minix)
LLVMGOLD_TARGET_ARCH=x86
endif

ifeq ($(LTCKPT_NO_WINCH), 1)
CFLAGS+= -DLTCKPT_NO_WINCH
endif

ifeq ($(LTCKPT_SET_PIC), 1)
CFLAGS+= -fpic
endif

ifneq ($(LLVMGOLD_TARGET_ARCH), x86_64)
ARCH=x86
CFLAGS+=-DLTCKPT_X86
else
CFLAGS+=-DLTCKPT_X86_64 -mrtm
ARCH=x64
endif

ifeq ($(LTCKPT_STKCHKLOG), 1)
CFLAGS+=-DLTCKPT_STKCHKLOG
endif

ifeq ($(LTRC_SUICIDE_ONLY_ONCE), 1)
CFLAGS+=-DLTRC_SUICIDE_ONLY_ONCE
endif

ifeq ($(LTRC_SUICIDE_WHEN_RECOVERABLE), 1) 
CFLAGS+=-DLTRC_SUICIDE_WHEN_RECOVERABLE
endif

ifeq ($(LTRC_REHANDLE_REQUEST), 1)
CFLAGS+=-DLTRC_REHANDLE_REQUEST
endif

ifeq ($(LTRC_NO_RECOVERY_ON_SVC_FI), 1)
CFLAGS+=-DLTRC_NO_RECOVERY_ON_SVC_FI
endif

ifeq ($(LTCKPT_O3_HACK), 1)
CFLAGS+=-DLTCKPT_O3_HACK
endif

ifeq ($(LTCKPT_TOGGLE_STATS), 1)
CFLAGS+=-DTOGGLE_ENABLE_STATS
endif

ifeq ($(LTCKPT_RECOVERY_HOOK_TRACE), 1)
CFLAGS+=-DLTCKPT_RECOVERY_HOOK_TRACE
endif

ifeq ($(LTCKPT_TEST_TSX_OV), 1)
CFLAGS+=-DLTCKPT_TEST_TSX_OV -mrtm
endif

ifeq ($(LTCKPT_REGISTERS), 1)
CFLAGS+=-DLTCKPT_REGISTERS
endif

ifeq ($(LTCKPT_SETJMP), 1)
CFLAGS+=-DLTCKPT_SETJMP
endif

ifeq ($(LTCKPT_WINDOW_PROFILING), 1)
CFLAGS+=-DLTCKPT_WINDOW_PROFILING
endif

ifeq ($(RCVRY_WINDOW_PROFILING), 1)
CFLAGS+=-DRCVRY_WINDOW_PROFILING
endif

ifeq ($(RCVRY_WINDOW_PROFILING_BBTRACING), 1)
CFLAGS+=-DRCVRY_WINDOW_PROFILING_BBTRACING
endif

ifeq ($(RCVRY_CKPT_DEFAULT_TO_UNDOLOG), 1)
CFLAGS+=-DRCVRY_CKPT_DEFAULT_TO_UNDOLOG
endif

ifeq ($(RCVRY_AUTO_ADAPT), 1)
CFLAGS+=-DRCVRY_AUTO_ADAPT
endif

ifeq ($(LTCKPT_LIBCALL_INTERVALS), 1)
CFLAGS+=-DLTCKPT_LIBCALL_INTERVALS
endif

ifdef LTCKPT_TSX_MAX_TRIES
CFLAGS+=-DLTCKPT_TSX_MAX_TRIES=$(LTCKPT_TSX_MAX_TRIES)
endif

ifdef LTCKPT_TSX_SUBSEQ_MAX_TRIES
CFLAGS+=-DLTCKPT_TSX_SUBSEQ_MAX_TRIES=$(LTCKPT_TSX_SUBSEQ_MAX_TRIES)
endif

ifeq ($(LTCKPT_LIBCALL_TIME_DIFF), 1)
CFLAGS+=-DLTCKPT_LIBCALL_TIME_DIFF
endif

ifeq ($(RCVRY_DYNAMIC_SWITCH), 1)
CFLAGS+=-DRCVRY_DYNAMIC_SWITCH
endif

ifeq ($(RCVRY_CKPT_DONT_REMEMBER), 1)
CFLAGS+=-DRCVRY_CKPT_DONT_REMEMBER
endif

ifeq ($(RCVRY_DELAYED_FREE), 1)
CFLAGS+=-DRCVRY_DELAYED_FREE
endif

ifeq ($(LTCKPT_EMPTY_UNDOLOG_HOOKS), 1)
CFLAGS+=-DLTCKPT_EMPTY_UNDOLOG_HOOKS
endif

ifeq ($(RCVRY_TSX_NO_SWITCHING), 1)
CFLAGS+=-DRCVRY_TSX_NO_SWITCHING
endif

ifeq ($(LTCKPT_NO_REG_RESTORATION), 1)
CFLAGS+=-DLTCKPT_NO_REG_RESTORATION
endif

ifeq ($(LTCKPT_NO_TSX_FOR_RCVRY_FAIL), 1)
CFLAGS+=-DLTCKPT_NO_TSX_FOR_RCVRY_FAIL
endif
 
CFLAGS+=-I$(DUNE_DIR)
CFLAGS+=-D_LARGEFILE64_SOURCE

SRCS = ltckpt_common.c                         \
       ltckpt_stat.c                          \
       ltckpt_aop.c                        \
       ltckpt_debug.c                        \
       ltckpt_ctx.c                        \
       ltckpt_recover.c			   \
       ltckpt_fi.c			   \
       ltckpt_signal.c			  \
       mechanisms/ltckpt_baseline.c        \
       mechanisms/ltckpt_writelog.c           \
       mechanisms/bitmap/ltckpt_bitmap.c      \
       mechanisms/bitmap/ltckpt_bitmap_init.c  \
       mechanisms/ltckpt_tsx.c

ifneq ($(Plat),Minix)
SRCS+= arch/$(ARCH)/ltckpt_common.c           \
       ltckpt_overrides.c                  \
       mechanisms/ltckpt_softdirty.c       \
       mechanisms/ltckpt_fork.c               \
       mechanisms/dune/ltckpt_dune.c          \
       mechanisms/mprotect/ltckpt_mprotect.c        \
       mechanisms/mprotect/mprotect_overrides.c     \
       mechanisms/smmap/ltckpt_smmap.c
endif

OBJS= $(SRCS:%.c=%.o)

BUILD_ON_X86_64=yes

USE_SECTIONIFY=YES
SECTIONIFY_DATA_SECTION_MAP=.*/ltckpt_data
SECTIONIFY_FUNCTION_SECTION_MAP=.*/ltckpt_functions

include $(ROOT)/llvm/static/Makefile.inc
