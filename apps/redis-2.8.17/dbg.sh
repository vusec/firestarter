#!/bin/bash

MYPWD=`pwd`
FILE_ALL_TESTS=${MYPWD}/tests/all_tests.txt
FILE_TCL_TEMPLATE=${MYPWD}/tests/test_helper.tcl.tpl
FILE_TEST_TCL=${MYPWD}/tests/test_helper.tcl
FILE_RESULT_OK=${MYPWD}/ok.txt
FILE_RESULT_NOK=${MYPWD}/not_ok.txt

set -e

init()
{
	for p in `pgrep tclsh`; do
		kill -9 $p || true
	done
	rm /tmp/rcvry_* || true
}


for t in `grep -F -x -v -f  $FILE_RESULT_OK $FILE_ALL_TESTS`; do
	echo "Attempting test: $t"
	tr=`echo $t | sed 's/\//\\\\\//g'`
	echo $tr
	cat ${FILE_TCL_TEMPLATE} | sed "s/TPLTEST/${tr}/" > $FILE_TEST_TCL
	init
	CLIENT_CP=1 BENCH_TYPE=2 ./clientctl bench || true
	echo
	echo
	echo "#######################"
	echo "Was it a success? [y/n]"
	echo "#######################"
	read success
	if [ "$success" == "y" ] || [ "$success" == "Y" ]; then
		echo "    $t" >> ${FILE_RESULT_OK}
	else
		echo "    $t" >> ${FILE_RESULT_NOK}
	fi
	echo
done
