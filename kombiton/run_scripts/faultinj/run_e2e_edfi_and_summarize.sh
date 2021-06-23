#!/bin/bash


###############################################################################
## Settings
###############################################################################

# Script-internal settings
MYPWD=`pwd`
: ${KOMBIT="python /home/koustubha/.local/lib/python2.7/site-packages/kombit/core.py"}
: ${KOMBIT_CFGDIR_FAULTINJ="$HOMEKB/repos/apprecovery/kombit/configs/faultinj"}

: ${TIME_TO_RECOVER=0}

TIME_TO_RECOVER_SUFFIX=
if [ ${TIME_TO_RECOVER} -eq 1 ]; then
    TIME_TO_RECOVER_SUFFIX=_timetorecover
    KOMBIT_CFGDIR_FAULTINJ="${KOMBIT_CFGDIR_FAULTINJ}/../timetorecover"
fi

CFG_NULLPTR_SUFFIX=.libcall_hybrid__edfi_bbtracing_injectedfault_nullptrref__testsuite${TIME_TO_RECOVER_SUFFIX}
CFG_REALISTIC_SUFFIX=.libcall_hybrid__edfi_bbtracing_injectedfault_several__testsuite${TIME_TO_RECOVER_SUFFIX}
RUNCOUNT=

# EDFI Target settings
: ${TARGET_PREFIX=ngx}
: ${TARGET_ROOT_PDIR=${HOMEKB}/repos/apprecovery/apps}
: ${KOMBIT_EXPT_BBTRACE_CFG=${TARGET_PREFIX}.libcall_hybrid__edfi_bbtracing__testsuite${TIME_TO_RECOVER_SUFFIX}.cfg}
: ${KOMBIT_EXPT_REINSTRUMENT_CFG=${TARGET_PREFIX}${CFG_NULLPTR_SUFFIX}.cfg}
: ${FAULT_TYPE=0}	# 0 : NULLPTR, 1: REALISTIC
: ${DIR_CKPT=${MYPWD}/ckpt_edfi_bbtrace}
: ${LOG_DIR=${MYPWD}/LOGS/FAULT_TYPE_${FAULT_TYPE}/${TARGET_PREFIX}${TIME_TO_RECOVER_SUFFIX}}

TARGET_LL_FILE=
case $TARGET_PREFIX in
"ngx")
	TARGET_LL_FILE=${TARGET_ROOT_PDIR}/nginx-0.8.54/objs/nginx.bcl.ll
	;;
"httpd")
	TARGET_LL_FILE=${TARGET_ROOT_PDIR}/httpd-2.2.23/httpd.bcl.ll
	;;
"lighttpd")
	TARGET_LL_FILE=${TARGET_ROOT_PDIR}/lighttpd-1.4.45/src/lighttpd.bcl.ll
	;;
"postgres")
	TARGET_LL_FILE=${TARGET_ROOT_PDIR}/postgresql-9.0.10/src/backend/postgres.bcl.ll
	;;
"redis")
	TARGET_LL_FILE=${TARGET_ROOT_PDIR}/redis-2.8.17/src/redis-server.bcl.ll
	;;
*)
	echo "Wrong target prefix provided."
	exit 1
	;;
esac

set -e

# Script execution settings
: ${SKIP_BBTRACE_GEN=0}
: ${BBTRACE_EXEC_PHASE=""}
: ${BBTRACE_ELIMCRITPATH=0}
: ${START_FAULT_INDEX=0}
: ${LIMIT_NUM_SITES=0}
: ${INTERACTIVE=0}
: ${RCVRY_BBTRACE_APPEND=0}
: ${RM_RCVRY_LOG_FILE=0}
: ${EXECUTE_ONLY=0}
DIR_EDFI_SERVER_CLIENT=${TARGET_ROOT_PDIR}/../llvm/static/edfi/platform/unix_apps/ctl/server
DIR_EDFI_DF=${TARGET_ROOT_PDIR}/../llvm/static/edfi/df
RCVRY_BBTRACE_JSON="/tmp/rcvry_bbtrace_dump.json"
RCVRY_MERGED_BBTRACE_JSON="./rcvry_bbtrace_dump.merged.json"
RCVRY_LOG_FILE="/tmp/rcvry.log"

[ -d ${LOG_DIR} ] || mkdir -p ${LOG_DIR}
[ -f ${LOG_DIR}/.RUNCOUNT ] || echo 1 > ${LOG_DIR}/.RUNCOUNT
RUNCOUNT=$(cat ${LOG_DIR}/.RUNCOUNT)
echo `expr ${RUNCOUNT} + 1` > ${LOG_DIR}/.RUNCOUNT

LOG_DIR=${LOG_DIR}/${RUNCOUNT}
LOG_FILE="${LOG_DIR}/edfi_run.log"
LOG_TRACE_INSTRUMENT="${LOG_DIR}/edfi_trace_instrument.log"
LOG_TRACE_RUN="/tmp/edfi_trace_run.log"
LOG_FAULT_REINSTRUMENT="${LOG_DIR}/edfi_fault_reinstrument.log" # Per site log
LOG_FAULT_RUN="${LOG_DIR}/edfi_fault_run.log"				    # Per site log
LOG_FAULTRUN_DIFF="${LOG_DIR}/edfi_fault_run_diff.log"		    # Per site log
LOG_FAULT_RESULT_DETAILS="${LOG_DIR}/edfi_fault_result.log"		# Per site log
LOG_SUMMARY="${LOG_DIR}/edfi_summary.csv"		        	    # Per site log
# LOG_ALL_REINSTRUMENT="${LOG_DIR}/edfi_reinstrument.all.log"
# LOG_TEST_RESULTS="${LOG_DIR}/edfi_testresult.log"				# Per site log

###############################################################################
## Script elements
###############################################################################

NUM_INJECTED_FAULTS=0

rebuild_edfi_bins()
{
    if [ "${TARGET_PREFIX}" == "httpd" ]; then
        LTCKPT_SET_PIC=1 LTCKPT_NO_WINCH=0 make clean install -C ${DIR_EDFI_SERVER_CLIENT}
        LTCKPT_SET_PIC=1 LTCKPT_NO_WINCH=0 make clean install -C ${DIR_EDFI_DF}
    elif [ "${TARGET_PREFIX}" == "postgres" ]; then
        LTCKPT_SET_PIC=1 LTCKPT_NO_WINCH=1 make clean install -C ${DIR_EDFI_SERVER_CLIENT}
        LTCKPT_SET_PIC=1 LTCKPT_NO_WINCH=1 make clean install -C ${DIR_EDFI_DF}
    else
        LTCKPT_SET_PIC=0 LTCKPT_NO_WINCH=0 make clean install -C ${DIR_EDFI_SERVER_CLIENT}
        LTCKPT_SET_PIC=0 LTCKPT_NO_WINCH=0 make clean install -C ${DIR_EDFI_DF}
    fi
    cd ${MYPWD}
}

bbtrace_instrument()
{
    cd ${KOMBIT_CFGDIR_FAULTINJ}
    echo -n "Instrumenting BBTracing using kombit cfg: ${KOMBIT_EXPT_BBTRACE_CFG} ..." | tee -a ${LOG_FILE}
    ${KOMBIT} ${KOMBIT_EXPT_BBTRACE_CFG}  instrument > ${LOG_TRACE_INSTRUMENT} 2>&1
    echo "  [done]" | tee -a ${LOG_TRACE_INSTRUMENT} | tee -a ${LOG_FILE}
    echo "" | tee -a ${LOG_TRACE_INSTRUMENT} | tee -a ${LOG_FILE}
    cd $MYPWD
}

bbtrace_gen()
{
    if [ "${BBTRACE_EXEC_PHASE}" == "instrument" ]; then
        return
    fi

    cd ${KOMBIT_CFGDIR_FAULTINJ}
    rm ${RCVRY_BBTRACE_JSON} 2>/dev/null || true
    echo -n "Executing BBTracing using kombit cfg: ${KOMBIT_EXPT_BBTRACE_CFG} ..." | tee -a ${LOG_TRACE_INSTRUMENT} | tee -a ${LOG_FILE}
    ${KOMBIT} ${KOMBIT_EXPT_BBTRACE_CFG}  execute 2>&1 | tee ${LOG_TRACE_RUN}
    cp ${LOG_TRACE_RUN} ${LOG_DIR}
    echo "  [done]" | tee -a ${LOG_TRACE_INSTRUMENT} | tee -a ${LOG_FILE}
    echo "" | tee -a ${LOG_TRACE_INSTRUMENT} | tee -a ${LOG_FILE}
    cd $MYPWD
}

bbtrace_merge()
{
    cd ${KOMBIT_CFGDIR_FAULTINJ}
    echo "Merging all bbtraces..." | tee -a ${LOG_TRACE_INSTRUMENT} | tee -a ${LOG_FILE}
    python $MYPWD/merge_bbtraces.py 2>&1 | tee -a ${LOG_TRACE_INSTRUMENT} | tee -a ${LOG_FILE}
    cp ${RCVRY_MERGED_BBTRACE_JSON} ${RCVRY_BBTRACE_JSON}
    echo "  [done]" | tee -a ${LOG_TRACE_INSTRUMENT} | tee -a ${LOG_FILE}
    echo "" | tee -a ${LOG_TRACE_INSTRUMENT} | tee -a ${LOG_FILE}
    cd $MYPWD
    check_bbtrace_exists
	ckpt_bbtrace_and_bitcode
}

check_bbtrace_exists()
{
    if [ ! -f ${RCVRY_BBTRACE_JSON} ] && [ 0 -ne `find /tmp/ "rcvry_bbtrace*" | grep "rcvry_bbtrace*"` ]; then
        echo "ERROR: Cannot find the site-basicblock map json file." | tee -a ${LOG_TRACE_INSTRUMENT} | tee -a ${LOG_FILE} && exit 1
    fi
}

ckpt_bbtrace_and_bitcode()
{
    [ -d ${DIR_CKPT} ] || mkdir -p ${DIR_CKPT}
    cp ${RCVRY_BBTRACE_JSON} ${DIR_CKPT}/	|| true
	cp ./rcvry_startup_dump.merged.json ${DIR_CKPT}/ || true
    cp ${TARGET_LL_FILE%.bcl.ll}.bcl ${DIR_CKPT}/
    cp ${TARGET_LL_FILE} ${DIR_CKPT}/
    echo "Ckpt-ed bbtrace and bitcode files." | tee -a ${LOG_TRACE_INSTRUMENT} | tee -a ${LOG_FILE}
}

force_kill_servers()
{
    for p in `pidof nginx`; do kill -9 $p || true; done
    for p in `pidof httpd`; do kill -9 $p || true; done
    for p in `pidof lighttpd`; do kill -9 $p || true; done
    for p in `pidof postgres`; do kill -9 $p || true; done
}

get_num_fault_sites()
{
python -c \
"import json
with open(\"${RCVRY_BBTRACE_JSON}\") as jfile:
    trace = json.load(jfile)
print len(trace)"
}

get_fault_site_info_from_ll()
{
    grep -B 5000 "\!EDFI_FI " ${TARGET_LL_FILE} | grep "^define.*@.*" | grep -v "@bbclone" | tail -1
    # We expect to find only one that is relevant.
#    grep "\!EDFI_FI " ${TARGET_LL_FILE} | while read l;
#    do
#	grep -B 5000 $l ${TARGET_LL_FILE} | grep "^define.*@.*" | grep -v "@bbclone" | tail -1
#    done
}

siteid2libcallname()
{
    local siteid=$1
    lname=`grep -B 50 "store.*i32 ${siteid},.*rcvry_current_site_id" ${TARGET_LL_FILE} | grep HYBPREP_LIBCALL | cut -d@ -f2 | cut -d'(' -f 1 | tail -1`
    echo $lname
}

siteid2funcname()
{
    local siteid=$1
    fname=""
    n_lines=50
    while [ "$fname" == "" ]
    do
        fname=`grep -B ${n_lines} "store.*i32 ${siteid},.*rcvry_current_site_id" ${TARGET_LL_FILE} | grep "^define.*@" | cut -d@ -f2 | cut -d'(' -f 1 | tail -1`
        n_lines=`expr $n_lines + 100`
    done
    echo $fname
}

get_fault_site_info()
{
    local testfile=$1
    site_ids=`grep -o "^[rcvry site:[0-9]*].*Received signal" $testfile | cut -d: -f 2 | cut -d']' -f 1`
    for s in ${site_ids};
    do
        lname=`siteid2libcallname $s`
	    fname=`siteid2funcname $s`
	    echo "site_id:$s libcall: $lname() in function: $fname()" 
	    grep -A 8 "^[rcvry site:$s].*Received signal" $testfile
	    echo
    done
}

get_fault_window_id()
{
   local siteid=$1
   grep "window_id selected:" ${LOG_FAULT_REINSTRUMENT%.log}.site_$siteid.log | cut -d':' -f 2 | cut -d'(' -f 1 | tr -d ' '
}

get_fault_bb_id()
{
    local siteid=$1
    grep "picked BB-ID:" ${LOG_FAULT_REINSTRUMENT%.log}.site_$siteid.log | cut -d':' -f 2 | tr -d ' '
}

enable_fault_at_site()
{
    local i=$1
    cd ${KOMBIT_CFGDIR_FAULTINJ}
    echo "FAULT_SITE_INDEX: $i" | tee ${LOG_FAULT_REINSTRUMENT%.log}.site_$i.log | tee -a ${LOG_FILE}
    FAULT_SITE_INDEX=$i ${KOMBIT} ${KOMBIT_EXPT_REINSTRUMENT_CFG} reinstrument 2>&1 | tee -a ${LOG_FAULT_REINSTRUMENT%.log}.site_$i.log 2>&1
    grep -B 1 "picked BB-ID:" ${LOG_FAULT_REINSTRUMENT%.log}.site_$i.log | tee -a ${LOG_FILE}

	if [ ${FAULT_TYPE} -eq 0 ]; then
	    NUM_INJECTED_FAULTS=`grep -A 1 "\[edfi\-faults\]" ${LOG_FAULT_REINSTRUMENT%.log}.site_$i.log | tail -1 | cut -d= -f 2 |  tr -d ' '`
	else
	    NUM_INJECTED_FAULTS=`grep -A 10 "\[edfi\-faults\]" ${LOG_FAULT_REINSTRUMENT%.log}.site_$i.log | tail -10 | cut -d= -f 2 |  tr -d ' ' | grep -v "0" | awk '{ sum+=$1 } END { print sum }'`
		grep -A 10 "\[edfi\-faults\]" ${LOG_FAULT_REINSTRUMENT%.log}.site_$i.log 2>&1 | tail -11 | tee -a ${LOG_FAULT_REINSTRUMENT%.log}.site_$i.log
        grep -A 10 "\[edfi\-prob\]" ${LOG_FAULT_REINSTRUMENT%.log}.site_$i.log 2>&1 | tail -11 | tee -a ${LOG_FAULT_REINSTRUMENT%.log}.site_$i.log
	fi
    if [ "0" == "${NUM_INJECTED_FAULTS}" ]; then
        echo "ERROR: No faults injected." | tee -a ${LOG_FILE}
    else
        echo "Number of faults injected: ${NUM_INJECTED_FAULTS}" | tee -a ${LOG_FILE}
    fi
    cd $MYPWD
}

execute_fault_at_site()
{
    local i=$1
    cd ${KOMBIT_CFGDIR_FAULTINJ}
    echo "Executing FAULT_SITE_INDEX: $i" | tee -a ${LOG_FAULT_RESULT_DETAILS%.log}.site_$i.log | tee -a ${LOG_FILE} > /dev/null
    FAULT_SITE_INDEX=$i ${KOMBIT} ${KOMBIT_EXPT_REINSTRUMENT_CFG} execute || true;
    cd $MYPWD
}

get_rcvry_action()
{
    local siteid=$1
    grep "Recovery_action_applied: " ${LOG_FAULT_RUN%.log}.site_$siteid.log | head -n 1 | cut -d':' -f 2
}

get_faultrun_diff()
{
    local siteid=$1
    diff ${LOG_FAULT_RUN%.log}.site_$siteid.log ${LOG_TRACE_RUN}
}

get_fault_types()
{
    local siteid=$1
    grep -A 10 "\[edfi\-faults\]" ${LOG_FAULT_REINSTRUMENT%.log}.site_$siteid.log | tail -10 | grep -v "= 0$"
}

is_infinite_rcvry()
{
    local siteid=$1
    grep "POSSIBLE INFINITE RECOVERY ATTEMPTS" ${LOG_FAULT_RUN%.log}.site_$siteid.log > /dev/null
    if [ 0 -eq $? ]; then
        echo 1
        return
    else
        echo 0
        return
    fi
}

is_crash()
{
    local siteid=$1
    grep "core dumped" ${LOG_FAULT_RUN%.log}.site_$siteid.log
    if [ 0 -eq $? ]; then
        echo "crash"
        return
    fi
    echo "no-crash"
}

summarize()
{
    local siteid=$1
    # site_id, window_id, bb_id, rcvry_action, Result
    window_id=`get_fault_window_id $siteid`
    bb_id=`get_fault_bb_id $siteid`
    ra=`get_rcvry_action $siteid`
    crash=`is_crash $siteid`
    inf_rcvry=`is_infinite_rcvry $siteid`

    echo "site_id, window_id, bb_id, rcvry_action, crash?, infinite_rcvry?"
    echo $siteid, $window_id, $bb_id, $ra, $crash, $inf_rcvry 
    echo "" 

    echo "Fault types:"
    get_fault_types $siteid 
    echo ""
    echo "Fault site and effect site:"
    cat ${LOG_FAULT_RESULT_DETAILS%.log}.site_$siteid.log
    echo ""
    echo "Diff: Fault run vs Trace run" 
    get_faultrun_diff $siteid
    return
}

main()
{
    [ -d ${LOG_DIR} ] || mkdir -p ${LOG_DIR}

    rebuild_edfi_bins

    # BBTrace
    if [ 0 == ${SKIP_BBTRACE_GEN} ]; then
		
        if [ "${BBTRACE_EXEC_PHASE}" != "execute" ]; then
        	bbtrace_instrument
		fi
        bbtrace_gen
        if [ 1 == ${RCVRY_BBTRACE_APPEND} ]; then
			echo "Going to run bbtrace_merge"
		    bbtrace_merge
        fi
	    ckpt_bbtrace_and_bitcode
	fi

	if [ "${INTERACTIVE}" == "1" ]; then
	    echo "Press any key to continue..." && read x
	fi

    if [ 1 == ${RM_RCVRY_LOG_FILE} ]; then
	    rm ${RCVRY_LOG_FILE} 2>/dev/null || true
    fi
    check_bbtrace_exists
    echo "Continuing further... to inject faults" | tee -a ${LOG_FILE}
    echo "" | tee -a ${LOG_FILE}

	echo "Disabling bbtrace dump..." | tee -a ${LOG_FILE}
	export DISABLE_BBTRACE_DUMP=1

    NUM_FAULT_SITES=`get_num_fault_sites`

    # Enable faults and execute one by one.
    N_FAULTS=${NUM_FAULT_SITES}
    if [ 0 != ${LIMIT_NUM_SITES} ];then
        N_FAULTS=${LIMIT_NUM_SITES}
    fi
    echo "N_FAULTS: ${N_FAULTS}" | tee -a ${LOG_FILE}

	if [ ${FAULT_TYPE} -eq 0 ]; then
		KOMBIT_EXPT_REINSTRUMENT_CFG=${TARGET_PREFIX}${CFG_NULLPTR_SUFFIX}.cfg
	else
		KOMBIT_EXPT_REINSTRUMENT_CFG=${TARGET_PREFIX}${CFG_REALISTIC_SUFFIX}.cfg
	fi

    for i in $(seq ${START_FAULT_INDEX} ${N_FAULTS})
    do
        force_kill_servers
        if [ ${EXECUTE_ONLY} -ne 1 ]; then
            enable_fault_at_site $i
	   	    if [ "0" == "${NUM_INJECTED_FAULTS}" ]; then
                echo "Fault index: $i: Skipping because no faults injected." | tee ${LOG_FAULT_RESULT_DETAILS%.log}.site_$i.log | tee ${LOG_SUMMARY%.log}.site_$i.log
			    continue
		    fi
            echo "Enabled fault at site: $i" | tee ${LOG_FAULT_RESULT_DETAILS%.log}.site_$i.log
        fi

        if [ "${TARGET_PREFIX}" == "lighttpd" ]; then
            lighdsrcdir=$(dirname ${TARGET_LL_FILE})
            cp ${lighdsrcdir}/../install/sbin/lighttpd ${lighdsrcdir}/lighttpd
        fi
        get_fault_site_info_from_ll 2>&1 | tee -a ${LOG_FAULT_RESULT_DETAILS%.log}.site_$i.log
        rm /tmp/rcvry.log.txt || true

        tail -1 ${LOG_FAULT_RESULT_DETAILS%.log}.site_$i.log | cut -d '!' -f 1 | grep "sigaction_handler" && continue

		pname=`basename TARGET_LL_FILE`
		pname=${pname%.bcl.ll}
		for l in `pgrep $pname`; do echo $l; kill -9 $l; done || true
        execute_fault_at_site $i 2>&1 | tee ${LOG_FAULT_RUN%.log}.site_$i.log
        if [ "${TARGET_PREFIX}" == "lighttpd" ]; then
            if [ -f "/tmp/rcvry.log.txt" ]; then
                cat "/tmp/rcvry.log.txt" | tee -a ${LOG_FAULT_RESULT_DETAILS%.log}.site_$i.log
            fi
        fi

		if [ ${TIME_TO_RECOVER} -eq 1 ]; then
            mv /tmp/rcvry_time__* ${LOG_DIR} || true
			[ -d ${LOG_DIR}/site_$i ] || mkdir -p ${LOG_DIR}/site_$i
			mv /tmp/rcvry_action* ${LOG_DIR}/site_$i || true
        fi

        (get_fault_site_info ${LOG_FAULT_RUN%.log}.site_$i.log) 2>&1 | tee -a ${LOG_FAULT_RESULT_DETAILS%.log}.site_$i.log
        summarize $i 2>&1 | tee ${LOG_SUMMARY%.log}.site_$i.log

        if [ "${INTERACTIVE}" == "1" ]; then
            echo "Press any key to continue..." && read x
        fi
    done
}

if [ $# -ge 1 ]; then
    if [ "$1" == "-h" ]; then
        echo "Script execution options:"
	    echo "SKIP_BBTRACE_GEN, BBTRACE_EXEC_PHASE, START_FAULT_INDEX, LIMIT_NUM_SITES"
    fi
    exit 1
fi

main
