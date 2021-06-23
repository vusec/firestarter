
default:	build

clean::
	rm -rf Makefile objs

build:
	$(MAKE) -f objs/Makefile

install: build
	$(MAKE) -f objs/Makefile install

modules:
	$(MAKE) -f objs/Makefile modules

upgrade:
	/mnt/hdd/koustubha/repos/firestarter/bugs/nginx-1.11.0--bug1263/install/sbin/nginx -t

	kill -USR2 `cat /mnt/hdd/koustubha/repos/firestarter/bugs/nginx-1.11.0--bug1263/install/logs/nginx.pid`
	sleep 1
	test -f /mnt/hdd/koustubha/repos/firestarter/bugs/nginx-1.11.0--bug1263/install/logs/nginx.pid.oldbin

	kill -QUIT `cat /mnt/hdd/koustubha/repos/firestarter/bugs/nginx-1.11.0--bug1263/install/logs/nginx.pid.oldbin`

clean::
	cp Makefile.configure Makefile && cp -r objs.configure objs
