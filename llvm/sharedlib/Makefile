ROOT=../..

include ${ROOT}/Makefile.inc

DIRS := ltckpt

all: $(patsubst %,build_%,$(DIRS))

$(patsubst %,build_%,$(DIRS)): DIR = $(subst build_,,$@)
$(patsubst %,build_%,$(DIRS)):
	@echo Building $(DIR) shared library...
	$(MAKE) -C $(DIR) install

clean:
	$(foreach DIR, $(DIRS), $(MAKE) -C $(DIR) distclean;)

distclean: clean

install: all