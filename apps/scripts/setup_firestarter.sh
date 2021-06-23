#!/bin/bash

set -e

# : ${BUILDVARS:="-V CPPFLAGS=-DUSR_STACKTOP=0x50000000 -V DBG=-g"}
: ${JOBS:=16}

if [ $# -ne 1 ]
then
   echo "Usage: "
   echo "$0 <path to llvm-apps repository (root of app-recovery repo)> "
   exit 1
fi

LROOT="$1"
LPASSES="${LROOT}/llvm/passes"
LSTATIC="${LROOT}/llvm/static"

[ ! -f "$LROOT/autosetup-paths.inc" ] || source "$LROOT/autosetup-paths.inc"

: ${LOGFILE="${LROOT}/firestarter.setup.log"}
MYPWD=`pwd`

echo "Building FIRestarter's LLVM artifacts" | tee -a ${LOGFILE}
cd ${LROOT}
make -C "$LROOT/llvm/passes" install >> "$LOGFILE" 2>&1
make -C "$LROOT/llvm/static" install >> "$LOGFILE" 2>&1
make -C "$LROOT/llvm/sharedlib" install >> "$LOGFILE" 2>&1
echo "                                       [ done ] " > ${LOGFILE} 2>&1
cd ${MYPWD}

# Configure a server app, say, nginx
cd ${LROOT}
echo -n > $LOGFILE
if [ ! -d "$LROOT/.tmp" ]; then 
	mkdir -p "$LROOT/.tmp"
fi
echo "Configuring Nginx server application" | tee -a ${LOGFILE}
cd `find . -name "nginx-*"`
./configure.llvm $CONFIGOPTS >> ${LOGFILE} 2>&1
echo "                                       [ done ] " > ${LOGFILE} 2>&1
cd ${MYPWD}
echo "Completed FIRestarter setup." | tee -a ${LOGFILE}