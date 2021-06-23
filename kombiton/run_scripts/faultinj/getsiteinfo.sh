#!/bin/bash


if [ $# -lt 2 ]; then
	echo "Usage: $0 <site_id> <.ll file>"
	exit 1
fi

SITE=$1
LLFILE=$2

if [ ! -f ${LLFILE} ]; then
	echo "Error: File not found: ${LLFILE}"
	exit 1
fi

grep -B2 "@rcvry_prof_count_rcvry_branch_hits(i32 ${SITE})" ${LLFILE} | head -1 | cut -d@ -f2 | cut -d'(' -f 1
