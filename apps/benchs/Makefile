install: all

all: httpd sqld

httpd sqld:
	echo "*** Configuring $@ benchmarks..."
	make -C $@/ all

.PHONY : httpd sqld
