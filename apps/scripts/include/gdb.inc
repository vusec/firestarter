#!/bin/bash

set -o errexit
set -o errtrace

function gdb_get_init_args {
    echo $(find ../../llvm/static ../../llvm/static/common ../../llvm/shared ../../llvm/shared/common -type d -printf 'dir %p\\n')
    cat ../scripts/include/gdbinit.common
}

function gdb_exit_usage {
    echo "Usage: gdb [action ...]"
    echo " - Available actions: "
    echo "     <pid>|[$0 pid action] [command|dump_sentries|dump_dsentries]"
    echo "     core"

    exit 1
}

function __gdb_run {
    local bin=$1
    local pid=$2
    shift; shift
    local gdb_opts="--command=_gdb_cmds"
    echo -e "$(gdb_get_init_args)" > _gdb_cmds
    if [ $# -ne 0 ]; then
        echo -e "\n$*\n" >> _gdb_cmds
        gdb_opts="--batch $gdb_opts"
    fi
    eval $SUDO_PREFIX gdb $gdb_opts $bin $pid
    rm -f _gdb_cmds
}

function gdb_run {
    local pid=$1
    if ./serverctl $1 &> /dev/null; then
        local bin=`./serverctl bin`
    else
        local bin=`ps -eo pid,args | grep "^[ ]*$pid" | awk '{ print $2; }'`
    fi
    echo " ***** Running: gdb_run $bin $*"
    __gdb_run $bin $*
    echo ""
}

function gdb_dump_sentries {
    echo -e "$(gdb_get_init_args)" > _gdb_cmds
    echo -e "\ncall magic_print_sentries()\n" >> _gdb_cmds
    eval $SUDO_PREFIX gdb --batch --command=_gdb_cmds $*
    rm -f _gdb_cmds
}

function gdb_dump_dsentries {
    echo -e "$(gdb_get_init_args)" > _gdb_cmds
    echo -e "\ncall magic_print_dsentries()\n" >> _gdb_cmds
    eval $SUDO_PREFIX gdb --batch --command=_gdb_cmds $*
    rm -f _gdb_cmds
}

function gdb_action {
    local mode=$1
    shift

    SUDO_PREFIX=sudo
    case "$mode" in
      '')
        gdb_exit_usage
        ;;
      'core')
        __gdb_run "$($0 bin) $($0 core)"
        ;;
      *)
        for pid in $( eval $0 $mode )
        do
            gdb_run $pid $*
        done
        ;;
    esac
}

