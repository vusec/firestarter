ROOT=../../..
include $(ROOT)/Makefile.inc

ifndef PERF_PATH
$(error PERF_PATH is not set. Please set in your common overrides.)
endif

all:

clean:

install: $(INSTALL_DIR)/perf

$(INSTALL_DIR)/perf:
	cp $(PERF_PATH) $@

distclean:
	rm -f $(INSTALL_DIR)/perf


