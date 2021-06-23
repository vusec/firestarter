#!/bin/bash

set -e

# This script compiles a number of important packages and installs them
# in the autosetup.dir/install directory to avoid problems with bad versions
# or packages that have been broken by the distribution.
# However, a number of packages needs to be installed beforehand.
# The script has been tested on a clean Ubuntu 16.04.4 LTS Server system
# with the following packages installed:
#
# sudo apt-get install bison flex g++-4.8 gcc-4.8 gcc-multilib git \
#                      make python python3.6 libssl-dev libncurses5-dev \
#                      ltrace
# incoming variables expected:
# - LLVMBRANCH
# - LLVMVERSION
# - LLVMVERSIONCONF
# - PATHROOT

corecount="`grep '^processor' /proc/cpuinfo|wc -l`"

: ${CLEAN=0}
: ${JOBS="`expr "$corecount" "*" 2`"}

: ${PATHAUTOSETUP="$PATHROOT/autosetup.dir"}
: ${PATHAUTOPREFIX="$PATHAUTOSETUP/install"}
: ${PATHAUTOSRC="$PATHAUTOSETUP/src"}
: ${PATHAUTOOBJ="$PATHAUTOSETUP/obj"}
: ${PATHAUTOSTATE="$PATHAUTOSETUP/state"}
: ${PATHLLVM:="$PATHAUTOSRC/`uname -m`"}
: ${PATHLOG="$PATHROOT/autosetup-log.txt"}

: ${VERSIONAUTOCONF=autoconf-2.69}
: ${VERSIONAUTOMAKE=automake-1.15}
: ${VERSIONBASH=bash-4.3}
: ${VERSIONBINUTILS=binutils-2.27}
: ${VERSIONCMAKE=cmake-3.20.0}
: ${VERSIONCMAKEURL=v3.20}
: ${VERSIONCOREUTILS=coreutils-8.22}
: ${VERSIONKOMBIT=2.0.2}

PATHBINUTILS="$PATHAUTOSRC/$VERSIONBINUTILS"
MYPWD=`pwd`

usage()
(
	echo "$@" >&2

	echo "usage:"
	echo "  autosetup-firestarter.sh"
	echo ""
	echo "relevant environment variables:"
	echo "- CLEAN can be set to 1 to start with a clean slate"
	echo "- JOBS can be set to the number of concurrent processes make can use"
	echo "- PATHROOT can be set to the root of the repository (default: cwd)"
	exit 1
)

[ -f "$PATHROOT/autosetup.inc" ] || usage "ERROR: PATHROOT is incorrect"
[ "$LLVMBRANCH" != "" ] || usage "ERROR: LLVMBRANCH is not set"
[ "$LLVMVERSION" != "" ]|| usage "ERROR: LLVMVERSION is not set"
[ "$LLVMVERSIONCONF" != "" ]|| usage "ERROR: LLVMVERSIONCONF is not set"
[ "$#" -eq 0 ] || usage "ERROR: invalid number of arguments"

if [ "$CLEAN" -ne 0 ]; then
	echo "Cleaning up old state"
	rm -rf "$PATHAUTOSETUP"
fi

export PATH="$PATHAUTOPREFIX/bin:$PATH"

exec 5> "$PATHLOG"

run()
{
	echo -------------------------------------------------------------------------------- >&5
	echo "command:          $*"               >&5
	echo "\$PATH:            $PATH"            >&5
	echo "working dir:      $PWD"             >&5
	echo -------------------------------------------------------------------------------- >&5
	if "$@" >&5 2>&5; then
		echo "[done]" >&5
	else
		echo "Command '$*' failed in directory $PWD with exit code $?, please check $PATHLOG for details" >&2
		exit 1
	fi
}

echo "Creating directories"
run mkdir -p "$PATHAUTOSRC"
run mkdir -p "$PATHAUTOSTATE"

# build bash to override the system's default shell
echo "Building bash"
cd "$PATHAUTOSRC"
[ -f "$VERSIONBASH.tar.gz" ] || run wget "http://ftp.gnu.org/gnu/bash/$VERSIONBASH.tar.gz"
[ -d "$VERSIONBASH" ] || run tar xf "$VERSIONBASH.tar.gz"
cd "$VERSIONBASH"
[ -f Makefile ] || run ./configure --prefix="$PATHAUTOPREFIX"
run make -j"$JOBS"
run make install
[ -f "$PATHAUTOPREFIX/bin/sh" ] || ln -s "$PATHAUTOPREFIX/bin/bash" "$PATHAUTOPREFIX/bin/sh"

# build a sane version of coreutils
echo "Building coreutils"
cd "$PATHAUTOSRC"
[ -f "$VERSIONCOREUTILS.tar.xz" ] || run wget "http://ftp.gnu.org/gnu/coreutils/$VERSIONCOREUTILS.tar.xz"
[ -d "$VERSIONCOREUTILS" ] || run tar xf "$VERSIONCOREUTILS.tar.xz"
cd "$VERSIONCOREUTILS"
[ -f Makefile ] || run ./configure --prefix="$PATHAUTOPREFIX"
run make -j"$JOBS"
run make install

# build binutils to ensure we have gold
echo "Building binutils"
cd "$PATHAUTOSRC"
[ -f "$VERSIONBINUTILS.tar.bz2" ] || run wget "http://ftp.gnu.org/gnu/binutils/$VERSIONBINUTILS.tar.bz2"
[ -d "$VERSIONBINUTILS" ] || run tar xf "$VERSIONBINUTILS.tar.bz2"
cd "$PATHBINUTILS"
confopts="--enable-gold --enable-plugins --disable-werror"
[ -n "`gcc -print-sysroot`" ] && confopts="$confopts --with-sysroot" # match system setting to avoid 'this linker was not configured to use sysroots' error or failure to find libpthread.so
[ -f Makefile ] || run ./configure --prefix="$PATHAUTOPREFIX" $confopts
run make -j"$JOBS"
run make -j"$JOBS" all-gold
run make install
run rm "$PATHAUTOPREFIX/bin/ld"
run cp "$PATHAUTOPREFIX/bin/ld.gold" "$PATHAUTOPREFIX/bin/ld" # replace ld with gold

# build cmake
echo "Building cmake"
cd "$PATHAUTOSRC"
[ -f "$VERSIONCMAKE.tar.gz" ] || run wget "https://cmake.org/files/$VERSIONCMAKEURL/$VERSIONCMAKE.tar.gz"
[ -d "$VERSIONCMAKE" ] || run tar xf "$VERSIONCMAKE.tar.gz"
cd "$VERSIONCMAKE"
[ -f Makefile ] || run ./configure --prefix="$PATHAUTOPREFIX"
run make -j"$JOBS"
run make install

# build autoconf
echo "building autoconf"
cd "$PATHAUTOSRC"
[ -f "$VERSIONAUTOCONF.tar.gz" ] || run wget "http://ftp.gnu.org/gnu/autoconf/$VERSIONAUTOCONF.tar.gz"
[ -d "$VERSIONAUTOCONF" ] || run tar xf "$VERSIONAUTOCONF.tar.gz"
run mkdir -p "$PATHAUTOOBJ/$VERSIONAUTOCONF"
cd "$PATHAUTOOBJ/$VERSIONAUTOCONF"
[ -f Makefile ] || run "$PATHAUTOSRC/$VERSIONAUTOCONF/configure" --prefix="$PATHAUTOPREFIX"
run make -j"$JOBS"
run make install

# build automake
echo "building automake"
cd "$PATHAUTOSRC"
[ -f "$VERSIONAUTOMAKE.tar.gz" ] || run wget "http://ftp.gnu.org/gnu/automake/$VERSIONAUTOMAKE.tar.gz"
[ -d "$VERSIONAUTOMAKE" ] || run tar xf "$VERSIONAUTOMAKE.tar.gz"
run mkdir -p "$PATHAUTOOBJ/$VERSIONAUTOMAKE"
cd "$PATHAUTOOBJ/$VERSIONAUTOMAKE"
[ -f Makefile ] || run "$PATHAUTOSRC/$VERSIONAUTOMAKE/configure" --prefix="$PATHAUTOPREFIX"
run make -j"$JOBS"
run make install

# Setup kombit (to help define execution variations)
statefile="$PATHAUTOSTATE/kombit-installation"
if [ ! -f "$statefile" ]; then
	echo "Installing Kombit"
	run python3.6 -m pip install -i https://test.pypi.org/simple/ kombit-pkg-ksbhat==$VERSIONKOMBIT
	touch "$statefile"
fi