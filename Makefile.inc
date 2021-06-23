TMPDIR:=$(ROOT)/.tmp

COMMON_CMD:='ROOT=$(ROOT) bash $(ROOT)/script.inc; cat $(TMPDIR)/common.merged.inc | sed "s/\$$(/\$$(shell /g" | sed "s/{/(/g" | sed "s/}/)/g" | sed "s/^\. /include /g" | sed "s/\"//g"'
COMMON_INC=$(shell eval $(COMMON_CMD) > $(TMPDIR)/Makefile.common.inc && echo $(TMPDIR)/Makefile.common.inc)
include $(COMMON_INC)

Plat=Linux

ifeq ($(HAVE_LLVM),1)
LLVM_VERSION=$(shell $(LLVMPREFIX)/bin/llvm-config --version | cut -d"." -f1,2 | sed "s/[^0-9]//g")
else
LLVM_VERSION=0
endif

LLVMGOLD_TARGET_ARCH := $(shell $(LLVMGOLD_CC) $(LLVMGOLD_CFLAGS) -v 2>&1| sed "s/-.*//g" | awk '$$1=="Target:"{printf "%s",$$2}')

export Plat

