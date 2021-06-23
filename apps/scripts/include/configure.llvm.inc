#!/bin/bash

set -o errexit

. ${ROOT}/script.inc

function build_llvm_tool {
	if [ $HAVE_LLVM -eq 0 ]; then
	    tool="/usr/bin/$1"
	else
	    tool="$LLVMGOLD_PREFIX/bin/llvm-$1"
	fi
	echo $tool
}

function build_llvm_libs {
	local LIBS=""
	for a in $*
	do
		LIBS="$LIBS `find $INSTALL_DIR -maxdepth 1 -name ${a}\*.bcc | xargs`"
	done
	echo $LIBS
}

function build_llvm_app_settings {
	if [ -f settings.llvm/$1/$2 ]; then
	    . ./settings.llvm/$1/$2
	fi
}

function build_llvm_check_lto_disable_opt {
	LLVMGOLD_HAS_LTO_DISABLE_OPT=1
	TMP_FILE=`mktemp --tmpdir configure.llvm.inc.XXXXXX`
	echo "int main() { return 0; }" > ${TMP_FILE}.c
	$LLVMGOLD_CC $LLVMGOLD_LDFLAGS -Wl,-plugin-opt=-disable-opt -o ${TMP_FILE}.out ${TMP_FILE}.c &> /dev/null || LLVMGOLD_HAS_LTO_DISABLE_OPT=0
	rm -f ${TMP_FILE}*
}

LLVMGOLD_HAS_LTO_DISABLE_OPT=${LLVMGOLD_HAS_LTO_DISABLE_OPT:0}
if [ $LLVMGOLD_HAS_LTO_DISABLE_OPT -ne 1 ]; then
	build_llvm_check_lto_disable_opt
fi
if [ $LLVMGOLD_HAS_LTO_DISABLE_OPT -eq 1 ]; then
	LLVMGOLD_LDFLAGS="$LLVMGOLD_LDFLAGS -Wl,-plugin-opt=-disable-opt"
elif [ $HAVE_LLVM -eq 1 ]; then
    if [ ${LLVM_VERSION} -lt 33 ]; then
	echo "*****"
	echo "***** Warning: You're running an unpatched old version of LLVM (<3.3). Please follow README.llvm to patch your LLVM installation."
	echo "*****"
    fi
fi
