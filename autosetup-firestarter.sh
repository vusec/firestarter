#!/bin/bash

set -e

: ${PATHROOT:="$PWD"}
if [ ! -f "$PATHROOT/autosetup.inc" ]; then
	echo "Please execute from the root of the repository or set PATHROOT" >&2
	exit 1
fi

LLVMBRANCH=RELEASE_40/final
LLVMVERSION=4.0
LLVMVERSIONCONF=4.0

source "$PATHROOT/autosetup.inc"

# store settings
(
echo "export PATH=\"$PATHAUTOPREFIX/bin:\$PATH\""
echo "PATHTOOLS=\"$PATHAUTOPREFIX\""
echo "export PYTHONPATH=\"\$PYTHONPATH:${PATHROOT}\""
) > "$PATHROOT/apps/autosetup-paths.inc"

# build app
echo "Setting up FIRestarter"
cd "$PATHROOT/apps"
export JOBS
scripts/setup_firestarter.sh "$PATHROOT" || (
	echo "ERROR: see $PATHROOT/firestarter.setup.log for details" >&2
	exit 1
)

