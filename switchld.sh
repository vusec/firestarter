#!/bin/bash

: ${SUFFIX=}

if [ ! -f /usr/bin/ld.gold${SUFFIX} ] || [ ! -f /usr/bin/ld.bfd${SUFFIX} ]; then
	echo "ld.gold or ld.bfd are missing. Aborting"
	exit 1
fi

arg=$1

if [ "$arg" == "gold" ]; then
rm /usr/bin/ld
ln -s /usr/bin/ld.gold${SUFFIX} /usr/bin/ld
else
rm /usr/bin/ld
ln -s /usr/bin/ld.bfd${SUFFIX} /usr/bin/ld
fi

stat -c%N /usr/bin/ld
