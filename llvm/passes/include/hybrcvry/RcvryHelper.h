#ifndef RCVRY_HELPER_H
#define RCVRY_HELPER_H

#include <common/pass_common.h>
#include "ltckpt/ltCkptPassTsx.h"               // We need tsx hook names
#include <common/tx_common.h>
#include <sstream>
#include <string>

extern "C"
{
   #include <static/ltckpt/ltckpt_types.h>
   #include <static/rcvry/rcvry_dispatch.h>
   #include <sys/types.h>
   #include <sys/socket.h>
   #include <netdb.h>
   #include <zlib.h>
}

#define RCVRY_GET_HOOK(M, LM, LH) M.getFunction(LH "_" + LM);

#define RCVRY_INIT_RINFO(TX_TYPE, RCVRY_TYPE, ARGC)             ( {                     \
        SmallVector< Constant*, 4 > rcvryInfoValues;                                    \
        SmallVector< Constant*, RCVRY_ACTIONS_MAX_ARGS > argvsNullArray;                \
        ArrayType *arrType = ArrayType::get(VOID_PTR_TY(*(this->M)),                    \
                                                RCVRY_ACTIONS_MAX_ARGS);                \
        for (int i=0; i < RCVRY_ACTIONS_MAX_ARGS; i++) {                                \
            argvsNullArray.push_back(ConstantPointerNull::get(VOID_PTR_TY(*(this->M))));\
        }                                                                               \
        Constant *constArray = ConstantArray::get(arrType, argvsNullArray);             \
                                                                                        \
        /* Setup the values accordingly */                                              \
        rcvryInfoValues.push_back(CONSTANT_INT(*(this->M), txTypeMap[TX_TYPE]));        \
        rcvryInfoValues.push_back(CONSTANT_INT(*(this->M), RCVRY_TYPE));                \
        rcvryInfoValues.push_back(CONSTANT_INT(*(this->M), ARGC));                      \
        rcvryInfoValues.push_back(constArray);                                          \
        rcvryInfoValues;                                                                \
        })

#define RCVRY_SET_RINFO(I, SITE_ID, TX_TYPE, RCVRY_TYPE, ARGC, ARGV0, ARGV1, ARGV2) ({  \
        std::vector<Value*> indices;                                                    \
        indices.push_back(ZERO_CONSTANT_INT(*(this->M)));                               \
        indices.push_back(CONSTANT_INT(*(this->M), SITE_ID));                           \
        DEBUG(errs() << "Accessing the struct rinfo for site:" << SITE_ID << "\n");     \
        /* Access the site specific rinfo struct */                                     \
        Instruction *GEPRinfo = PassUtil::createGetElementPtrInstruction(               \
                                        rcvryInfoGV, indices, "", I);                   \
        LoadInst *LIRinfo __attribute__((unused)) 					\
                          = new LoadInst(GEPRinfo, "", false, I);                       \
                                                                                        \
        DEBUG(errs() << "Accessing the type and argc\n");                               \
        /* Access the type and argc */                                                  \
        indices.push_back(CONSTANT_INT(*(this->M), 0));                                 \
        Instruction *GEPrinfo_txType = PassUtil::createGetElementPtrInstruction(        \
                                        rcvryInfoGV, indices, "", I);                   \
        DEBUG(errs() << "GEPrinfo_type successful.\n");                                 \
        StoreInst *SIRinfoTxType __attribute__((unused))				\
                                 = new StoreInst(CONSTANT_INT(*(this->M),               \
                                        txTypeMap[TX_TYPE]),                            \
                                        GEPrinfo_txType,                                \
                                        "store TYPE rcvry_info[site_id].ltckpt_type",   \
                                        I);                                             \
        indices.pop_back(); indices.push_back(CONSTANT_INT(*(this->M), 1));             \
        Instruction *GEPrinfo_type = PassUtil::createGetElementPtrInstruction(          \
                                        rcvryInfoGV, indices, "", I);                   \
        StoreInst *SIRinfoType __attribute__((unused))                                  \
                               = new StoreInst(CONSTANT_INT(*(this->M), RCVRY_TYPE),    \
                                        GEPrinfo_type,                                  \
                                        "store TYPE rcvry_info[site_id].rcvry_type",    \
                                        I);                                             \
        DEBUG(errs() << "SIRinfoType successful.\n");                                   \
        indices.pop_back(); indices.push_back(CONSTANT_INT(*(this->M), 2));             \
        Instruction *GEPrinfo_argc = PassUtil::createGetElementPtrInstruction(          \
                                        rcvryInfoGV, indices, "", I);                   \
        DEBUG(errs() << "GEPrinfo_argc successful.\n");                                 \
        StoreInst *SIRinfoArgc __attribute__((unused))                                  \
                               = new StoreInst(CONSTANT_INT(*(this->M), ARGC),          \
                                        GEPrinfo_argc,                                  \
                                        "store ARGC rcvry_info[site_id].argc",          \
                                        I);                                             \
        DEBUG(errs() << "SIRinfoArgc successful.\n");                                   \
                                                                                        \
        DEBUG(errs() << "Accessing the argv[] array\n");                                \
        /* Access the argv[] array */                                                   \
        indices.pop_back(); indices.push_back(CONSTANT_INT(*(this->M), 3));             \
        Instruction *GEPrinfo_argv = PassUtil::createGetElementPtrInstruction(          \
                                        rcvryInfoGV, indices, "", I);                   \
        LoadInst *LIRinfoArgv __attribute__((unused))                                   \
                              = new LoadInst(GEPrinfo_argv, "LI rcvry_info.argv",       \
                                        false, I);                                      \
                                                                                        \
        DEBUG(errs() << "Accessing the argv[] values\n");                               \
        /* Access and set argv[] values */                                              \
        indices.push_back(CONSTANT_INT(*(this->M), 0));                                 \
        Instruction *GEPrinfo_argv_0 = PassUtil::createGetElementPtrInstruction(        \
                                       rcvryInfoGV, indices, "", I);                    \
        DEBUG(errs() << "Storing value to ARGV0\n");                                    \
        BitCastInst *voidPtrArgv0 = new BitCastInst(GEPrinfo_argv_0,                    \
                                                VOID_PTR_PTR_TY(*(this->M)),            \
                                                "", I);                                 \
        StoreInst *SIRinfoArgv0 __attribute__((unused))					\
                                = new StoreInst(ARGV0, voidPtrArgv0,                    \
                                        "store ARGC rcvry_info[site_id].argv[0]",       \
                                        I);                                             \
        indices.pop_back(); indices.push_back(CONSTANT_INT(*(this->M), 1));             \
        Instruction *GEPrinfo_argv_1 = PassUtil::createGetElementPtrInstruction(        \
                                       rcvryInfoGV, indices, "", I);                    \
        DEBUG(errs() << "Storing value to ARGV1\n");                                    \
        StoreInst *SIRinfoArgv1 __attribute__((unused))					\
                                = new StoreInst(ARGV1, GEPrinfo_argv_1,                 \
                                        "store ARGC rcvry_info[site_id].argv[1]",       \
                                        I);                                             \
        indices.pop_back(); indices.push_back(CONSTANT_INT(*(this->M), 2));             \
        Instruction *GEPrinfo_argv_2 = PassUtil::createGetElementPtrInstruction(        \
                                       rcvryInfoGV, indices, "", I);                    \
        StoreInst *SIRinfoArgv2 __attribute__((unused))                                 \
                                = new StoreInst(ARGV2, GEPrinfo_argv_2,                 \
                                        "store ARGC rcvry_info[site_id].argv[2]",       \
                                        I);                                             \
        })

#define RCVRY_TO_VOID_PTR(V, I)         ( {                                             \
        Value *val;                                                                     \
        if (V->getType()->isIntegerTy()) {                                              \
                val = new IntToPtrInst(V, VOID_PTR_TY(*(this->M)), "int2ptr", I);       \
        } else {                                                                        \
                val = new BitCastInst(V, VOID_PTR_TY(*(this->M)), "bitcast", I);        \
        }                                                                               \
        val;                                                                            \
        } )

#define RCVRY_MARKER_SYM                "RCVRY"
#define RCVRY_SITEID_NAMESPACE_NAME     "RCVRY"
#define RCVRY_GV_NAME_CURR_SITEID       "rcvry_current_site_id"
#define RCVRY_GV_NAME_MAX_SITEID        "rcvry_libcall_max_site_id"
#define RCVRY_GV_NAME_RETURN_VALUE      "rcvry_return_value"
#define RCVRY_GV_NAME_GATES             "rcvry_libcall_gates"
#define RCVRY_GV_NAME_SITES_INFO        "rcvry_info"
#define RCVRY_LTCKPT_TOL_UNDOLOG        "ltckpt_top_of_the_loop_undolog"
#define RCVRY_LTCKPT_MEMCPY_HOOK        "ltckpt_memcpy_hook_undolog"

using namespace llvm;

namespace llvm
{

class RcvryHelper
{
public:
    RcvryHelper(Module *M, GlobalVariable *rcvryInfoGV, Function *endOfWindowHook,
                std::vector<std::string> STMLibCalls)
                : endOfWindowHook(endOfWindowHook), M(M), rcvryInfoGV(rcvryInfoGV),
                  STMLibCalls(STMLibCalls) { init(); };
    void setRecoveryAction(uint32_t site_id, tx_type_t txType, CallInst *libCallInst,
                           AllocaInst *retVal, Instruction *insertBefore, bool profile=false);
    void prepareTrueBlock(BasicBlock *tb, CallInst *libCallInst, AllocaInst *localRetValCtrl);
    void prepareFalseBlock(BasicBlock *fb, CallInst *libCallInst, AllocaInst *localRetValCtrl,
                           Value *newReturnValueVariable);
    void addLibCall(CallInst *libCallInst);
    Function *countRcvryBranchHitsHook, *countRcvryHitsProtectedWindowsHook,
             *countRcvryHitsSkippedNonFaultablesHook, *countRcvryHitsSkippedNoUsersHook,
             *fiHook, *ltckptTOLFuncHook, *ltckptMemCpyFuncHook, *endOfWindowHook;

private:
    Module *M;
    int txTypeMap[__NUM_TX_TYPES];
    std::map<std::string, rcvry_type_t> libCallRecoveryMap;
    std::map<Function *, rcvry_type_t> libCallRecoveryMapCache;
    GlobalVariable *rcvryInfoGV;
    std::vector<std::string> STMLibCalls;
    void init();
    bool isCkptSentinel(CallInst *CI);
    void removeCheckpointing(CallInst *libCallInst);
};

void RcvryHelper::init()
{
    // Initialize the libcall recovery map
    libCallRecoveryMap.insert(std::make_pair("__fxstat", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("__fxstat64", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("__xstat", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("accept", RCVRY_TYPE_CLOSE_SOCKET));
    libCallRecoveryMap.insert(std::make_pair("accept4", RCVRY_TYPE_CLOSE_SOCKET));
    libCallRecoveryMap.insert(std::make_pair("bind", RCVRY_TYPE_CLOSE_SOCKET));
    libCallRecoveryMap.insert(std::make_pair("calloc", RCVRY_TYPE_UNDO_MALLOC));
    libCallRecoveryMap.insert(std::make_pair("chdir", RCVRY_TYPE_SET_RET_ERRNO_EACCES));
    libCallRecoveryMap.insert(std::make_pair("chmod", RCVRY_TYPE_SET_RET_ERRNO_EACCES));
    libCallRecoveryMap.insert(std::make_pair("chown", RCVRY_TYPE_SET_RET_ERRNO_EACCES));
    libCallRecoveryMap.insert(std::make_pair("closedir", RCVRY_TYPE_SET_RET_ERRNO_EBADF));
    libCallRecoveryMap.insert(std::make_pair("clock_gettime", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("close", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("connect", RCVRY_TYPE_CLOSE_SOCKET));
    libCallRecoveryMap.insert(std::make_pair("deflate", RCVRY_TYPE_SET_RET_ERRNO_ZSTREAM_ERROR));
    libCallRecoveryMap.insert(std::make_pair("deflateEnd", RCVRY_TYPE_SET_RET_ERRNO_ZSTREAM_ERROR));
    libCallRecoveryMap.insert(std::make_pair("dlopen", RCVRY_TYPE_DLCLOSE));
    libCallRecoveryMap.insert(std::make_pair("dlsym", RCVRY_TYPE_FAIL));
    libCallRecoveryMap.insert(std::make_pair("dup2", RCVRY_TYPE_CLOSE));
    libCallRecoveryMap.insert(std::make_pair("epoll_create", RCVRY_TYPE_CLOSE));
    libCallRecoveryMap.insert(std::make_pair("epoll_ctl", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("epoll_wait", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("event_add", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("event_base_set", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("event_del", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("event_set", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("fcntl", RCVRY_TYPE_FAIL));        // TODO: Can impl fcntl compensators
    libCallRecoveryMap.insert(std::make_pair("fstat64", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("ftruncate64", RCVRY_TYPE_SET_RET_ERRNO_EACCES));
    libCallRecoveryMap.insert(std::make_pair("fdopen", RCVRY_TYPE_CLOSE_RET_NULL));
    libCallRecoveryMap.insert(std::make_pair("fopen64", RCVRY_TYPE_SET_RET_NULL_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("fork", RCVRY_TYPE_UNDO_FORK));
    libCallRecoveryMap.insert(std::make_pair("fsync", RCVRY_TYPE_SET_RET_ERRNO_ENOSPC));
    libCallRecoveryMap.insert(std::make_pair("getaddrinfo", RCVRY_TYPE_UNDO_ADDRINFO));
    libCallRecoveryMap.insert(std::make_pair("getenv", RCVRY_TYPE_SET_RET_NULL_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("getgrnam", RCVRY_TYPE_SET_RET_NULL_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("gethostname", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("getloadavg", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("getnameinfo", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("getpeername", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("getpwnam", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("getrlimit64", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("getsockname", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("getsockopt", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));   
    libCallRecoveryMap.insert(std::make_pair("gettimeofday", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("initgroups", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));  
    libCallRecoveryMap.insert(std::make_pair("listen", RCVRY_TYPE_CLOSE_SOCKET));
    libCallRecoveryMap.insert(std::make_pair("localtime", RCVRY_TYPE_SET_RET_NULL_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("lseek", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("lseek64", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("lstat64", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM)); 
    libCallRecoveryMap.insert(std::make_pair("malloc", RCVRY_TYPE_UNDO_MALLOC));
    libCallRecoveryMap.insert(std::make_pair("mkdir", RCVRY_TYPE_UNDO_MKDIR));
    libCallRecoveryMap.insert(std::make_pair("mktime", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM)); 
    libCallRecoveryMap.insert(std::make_pair("mmap64", RCVRY_TYPE_UNDO_MMAP));
    libCallRecoveryMap.insert(std::make_pair("mmap", RCVRY_TYPE_UNDO_MMAP));
    libCallRecoveryMap.insert(std::make_pair("msync", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("munmap", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("open", RCVRY_TYPE_CLOSE));
    libCallRecoveryMap.insert(std::make_pair("open64", RCVRY_TYPE_CLOSE));
    libCallRecoveryMap.insert(std::make_pair("opendir", RCVRY_TYPE_SET_RET_NULL_ERRNO_EBADF));
    libCallRecoveryMap.insert(std::make_pair("pipe", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("poll", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("posix_memalign", RCVRY_TYPE_UNDO_MEMALIGN));
    libCallRecoveryMap.insert(std::make_pair("pread64", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("pthread_mutex_init", RCVRY_TYPE_UNDO_PTHRDMUTX_INIT));
    libCallRecoveryMap.insert(std::make_pair("pthread_mutex_lock", RCVRY_TYPE_UNDO_PTHRDMUTX_LOCK));
    libCallRecoveryMap.insert(std::make_pair("pthread_mutex_trylock", RCVRY_TYPE_UNDO_PTHRDMUTX_LOCK));
    libCallRecoveryMap.insert(std::make_pair("realloc", RCVRY_TYPE_UNDO_REALLOC));
    libCallRecoveryMap.insert(std::make_pair("readv", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("readdir64", RCVRY_TYPE_SET_RET_NULL_ERRNO_EBADF));
    libCallRecoveryMap.insert(std::make_pair("recv", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("recvmsg", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("rename", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("sched_setaffinity", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("select", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("sendmsg", RCVRY_TYPE_SET_RET_ERRNO_EFAULT));
    libCallRecoveryMap.insert(std::make_pair("setgid", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("setitimer", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("setlocale", RCVRY_TYPE_UNDO_SETLOCALE));
    libCallRecoveryMap.insert(std::make_pair("setpriority", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("setrlimit64", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("setsid", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("setsockopt", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("setuid", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("shutdown", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));
    libCallRecoveryMap.insert(std::make_pair("socket", RCVRY_TYPE_CLOSE));
    libCallRecoveryMap.insert(std::make_pair("socketpair", RCVRY_TYPE_SET_RET_ERRNO_EMFILE));
    libCallRecoveryMap.insert(std::make_pair("stat64", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("statfs64", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("sysconf", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("time", RCVRY_TYPE_SET_RET_ERRNO_EFAULT));
    libCallRecoveryMap.insert(std::make_pair("uname", RCVRY_TYPE_SET_RET_ERRNO_EFAULT));
    libCallRecoveryMap.insert(std::make_pair("usleep", RCVRY_TYPE_SET_RET_ERRNO_ENOMEM));
    libCallRecoveryMap.insert(std::make_pair("waitpid", RCVRY_TYPE_SET_RET_ERRNO_EINVAL));

    // Irrecoverables
    libCallRecoveryMap.insert(std::make_pair("pread64", RCVRY_TYPE_FAIL));
    libCallRecoveryMap.insert(std::make_pair("pthread_cond_signal", RCVRY_TYPE_FAIL));
    libCallRecoveryMap.insert(std::make_pair("pthread_cond_wait", RCVRY_TYPE_FAIL));
    libCallRecoveryMap.insert(std::make_pair("read", RCVRY_TYPE_FAIL));
    libCallRecoveryMap.insert(std::make_pair("semctl", RCVRY_TYPE_FAIL));
    libCallRecoveryMap.insert(std::make_pair("shmctl", RCVRY_TYPE_FAIL));
    libCallRecoveryMap.insert(std::make_pair("shmdt", RCVRY_TYPE_FAIL));
    libCallRecoveryMap.insert(std::make_pair("strstr", RCVRY_TYPE_FAIL));
    libCallRecoveryMap.insert(std::make_pair("sigaction", RCVRY_TYPE_FAIL));
    libCallRecoveryMap.insert(std::make_pair("sigemptyset", RCVRY_TYPE_FAIL));
    libCallRecoveryMap.insert(std::make_pair("utimes", RCVRY_TYPE_SET_RET_ERRNO_EACCES));

    txTypeMap[TX_TYPE_INVALID] = LTCKPT_TYPE_INVALID;
    txTypeMap[TX_TYPE_LTCKPT] = LTCKPT_TYPE_UNDOLOG;
    txTypeMap[TX_TYPE_TSX] =  LTCKPT_TYPE_TSX;

    countRcvryBranchHitsHook = NULL;
    countRcvryHitsProtectedWindowsHook = NULL;
    countRcvryHitsSkippedNonFaultablesHook = NULL;
    countRcvryHitsSkippedNoUsersHook = NULL;
    Constant *countRcvryBranchHitsFunc = M->getFunction("rcvry_prof_count_rcvry_branch_hits");
    Constant *countRcvryHitsProtectedWindowsFunc = M->getFunction("rcvry_prof_count_libcalls__protected");
    Constant *countRcvryHitsSkippedNonFaultablesFunc = M->getFunction("rcvry_prof_count_libcalls__skipped_nonfaultable");
    Constant *countRcvryHitsSkippedNoUsersFunc = M->getFunction("rcvry_prof_count_libcalls__skipped_nousers");
    if (NULL != countRcvryBranchHitsFunc
        && NULL != countRcvryHitsProtectedWindowsFunc
        && NULL != countRcvryHitsSkippedNonFaultablesFunc
        && NULL != countRcvryHitsSkippedNoUsersFunc) {
        countRcvryBranchHitsHook = cast<Function>(countRcvryBranchHitsFunc);
        countRcvryHitsProtectedWindowsHook = cast<Function>(countRcvryHitsProtectedWindowsFunc);
        countRcvryHitsSkippedNonFaultablesHook = cast<Function>(countRcvryHitsSkippedNonFaultablesFunc);
        countRcvryHitsSkippedNoUsersHook = cast<Function>(countRcvryHitsSkippedNoUsersFunc);
    }

    // ckpt hooks
    Constant *tolFunc = M->getFunction(RCVRY_LTCKPT_TOL_UNDOLOG);
    assert(NULL != tolFunc);
    Constant *ltckptMemCpyFunc = M->getFunction(RCVRY_LTCKPT_MEMCPY_HOOK);
    assert(NULL != ltckptMemCpyFunc);
    ltckptTOLFuncHook = cast<Function>(tolFunc);
    assert(NULL != ltckptTOLFuncHook);
    ltckptMemCpyFuncHook = cast<Function>(ltckptMemCpyFunc);
    assert(NULL != ltckptMemCpyFuncHook);

    Constant *faultFatalFunc = M->getFunction("rcvry_fi_fatal_fault");
    fiHook = NULL;
    if (NULL != faultFatalFunc) {
        fiHook = cast<Function>(faultFatalFunc);
    }
}

void RcvryHelper::addLibCall(CallInst *libCallInst)
{
    Function *libFunc = libCallInst->getCalledFunction();
    assert(NULL != libFunc);
    if (0 == libCallRecoveryMapCache.count(libFunc)) {
        std::map<std::string, rcvry_type_t>::iterator rcvryMapIt
                                          = libCallRecoveryMap.find(libFunc->getName());
        if (rcvryMapIt != libCallRecoveryMap.end()) {
            libCallRecoveryMapCache.insert(std::make_pair(libFunc, rcvryMapIt->second));
        }
    }
}

/*
 * Prepare the basic block, which will be used when the gate is OPEN (1)
 * The libcall will be executed and its actual return value will be set to
 * the localRetValCtrl variable that controls the execution that follows.
 * This must be called on both the tx variant basic blocks.
 */
void RcvryHelper::prepareTrueBlock(BasicBlock *tb, CallInst *libCallInst,
                                   AllocaInst *localRetValCtrl)
{
    assert(NULL != tb);
    assert(NULL != libCallInst && NULL != localRetValCtrl);
    assert(tb == libCallInst->getParent());

    // trueblock before: 
    //         libcallInst;
    //         TOL;
    //
    // trueblock after: 
    //         libcallInst;
    //         rcvry_return_value = (void *) return-value;
    //         TOL;

    DEBUG(errs() << "\t[trueblock] StoreInst to store the return value to the global retValController\n");
    StoreInst *storeToRetValController __attribute__((unused));
    storeToRetValController = new StoreInst(libCallInst, localRetValCtrl,
                                                       "store callInst->RetValController",
                                                        tb->getTerminator());
    return;
}

void RcvryHelper::prepareFalseBlock(BasicBlock *fb, CallInst *libCallInst,
                                    AllocaInst *localRetValCtrl,
                                    Value *newReturnValueVariable)
{
    assert(NULL != fb);
    assert(NULL != libCallInst && NULL != localRetValCtrl);

    LoadInst *loadRetVal = new LoadInst(newReturnValueVariable,
                                        "load rcvry_return_value", fb->getTerminator());

    // falseblock: load voidptr; ptrtoint; store ptrtoint to alloca
    if (libCallInst->getType()->isPointerTy()) {
        DEBUG(errs() << "\t[falseblock: ret is ptrTy] bitcast the global faked return value\n");
        BitCastInst *voidPtrBitcastGlobalRetVal = new BitCastInst(
                                                 loadRetVal,
                                                 libCallInst->getType(),
                                                 "bitcast rcvry_return_value",
                                                 fb->getTerminator());
        DEBUG(errs() << "\t[falseblock: ret is ptrTy] load the bitcasted faked returned value\n");
        StoreInst *storeRetToAlloca __attribute__((unused));
        storeRetToAlloca = new StoreInst(voidPtrBitcastGlobalRetVal,
                                         localRetValCtrl,
                                         fb->getTerminator());
    } else if (libCallInst->getType()->isIntegerTy()) {
        PtrToIntInst *voidPtr2IntGlobalRetVal = new PtrToIntInst(loadRetVal,
                                                  libCallInst->getType(),
                                                  "ptr2int rcvry_return_value",
                                                  fb->getTerminator());
        DEBUG(errs() << "\t[falseblock: ret is intTy] Store the faked return value into the localRetValCtrl\n");
        StoreInst *storeRetToAlloca __attribute__((unused));
        storeRetToAlloca = new StoreInst(voidPtr2IntGlobalRetVal,
                                         localRetValCtrl,
                                         fb->getTerminator());
    } else {
        assert(0 && "Return type, not supported.");
    }
    return;
}

bool RcvryHelper::isCkptSentinel(CallInst *CI)
{
    if (NULL == this->endOfWindowHook) {
        return PassUtil::isLibCallInst(CI);
    }
    return (CI->getCalledFunction() == this->endOfWindowHook);
}

void RcvryHelper::removeCheckpointing(CallInst *libCallInst)
{
    std::vector<BasicBlock*> BBqueue;
    std::set<BasicBlock*> BBsuccessors;
    std::vector<Instruction*> instsToRemove;
    BasicBlock::iterator BI(libCallInst);
    BasicBlock *BB = NULL;
    BBqueue.push_back(libCallInst->getParent());
    do {
        BB = BBqueue[0];
        if (BB != libCallInst->getParent()) {
            BI = BB->begin();
        } else {
            BI++;
        }
        while(BI != BB->end()) {
            Instruction *I = &(*BI);
            CallInst *CI = dyn_cast<CallInst>(I);
            if (NULL != CI) {
                if (isCkptSentinel(CI)) {
                    break;
                }

                if (CI->getCalledFunction()->getName().contains("ltckpt_")) {
                    DEBUG(errs() << "Removed callinst to function: " << CI->getCalledFunction()->getName() << "\n");
                    instsToRemove.push_back(I);
                }
            } else if (I->isTerminator()) {
                BranchInst *brInst = dyn_cast<BranchInst>(I);
                if (NULL != brInst) {
                    for (unsigned i=0; i < brInst->getNumSuccessors(); i++) {
                        BasicBlock *successor = brInst->getSuccessor(i);
                        if (BBsuccessors.end() == BBsuccessors.find(successor)) {
                            BBqueue.push_back(brInst->getSuccessor(i));
                            BBsuccessors.insert(successor);
                        }
                    }
                }
            }
            BI++;
        }
        BBqueue.erase(BBqueue.begin());
    } while(0 != BBqueue.size());

    for (unsigned i=0; i < instsToRemove.size(); i++) {
        instsToRemove[i]->eraseFromParent();
    }
    return;
}

void RcvryHelper::setRecoveryAction(uint32_t site_id, tx_type_t txType, CallInst *libCallInst,
                                  AllocaInst *retVal, Instruction *insertBefore, bool profile)
{
    int set_errno = EINVAL;

    assert(NULL != libCallInst);
    Function *libFunc = libCallInst->getCalledFunction();
    assert(NULL != libFunc);
    assert(NULL != insertBefore);

    if (profile) {
        assert (NULL != countRcvryBranchHitsHook);
        std::vector<Value *> args(1);
        args[0] = CONSTANT_INT(*(this->M), site_id);
        PassUtil::createCallInstruction(countRcvryBranchHitsHook, args, "", insertBefore);
    }
    rcvry_type_t rcvryType;
    std::map<Function *, rcvry_type_t>::iterator rIt;
    SmallVector< Constant*, 4 > rcvryInfoValues;

    DEBUG(errs() << "\tSetting recovery action for [call " << libFunc->getName() << "], site_id: " << site_id << "\n");
    rIt = libCallRecoveryMapCache.find(libFunc);
    if (rIt == libCallRecoveryMapCache.end()) {
        errs() << "No recovery action set of libfunc: " << libFunc->getName() << "\n";
        rcvryType = RCVRY_TYPE_FAIL;
    } else {
        rcvryType = rIt->second;
    }
    DEBUG(errs() << "\trcvryType selected: " << rcvryType << "\n");

#ifdef RCVRY_NO_CKPT_IN_FAILS
    // If rcvryType is RCVRY_TYPE_FAIL, then
    // remove all ltckpt instrumentation calls in that window.
    if (rcvryType == RCVRY_TYPE_FAIL) {
        // No need to have checkpointing in this window.
        errs() << "removing ckpt for libcall inst: " << libCallInst->getCalledFunction()->getName() << "\n";
        removeCheckpointing(libCallInst);
        return;
    }
#endif

    // Set statically, the ltckpt type to undolog for libcalls specified via cmdline args.
    for (std::string fName : STMLibCalls) {
        if (libFunc->getName() == fName) {
            txType = TX_TYPE_LTCKPT;
            break;
        }
    }

    Constant *constPtrNull = Constant::getNullValue(VOID_PTR_TY(*(this->M)));
    CallSite callsite = CallSite::CallSite(libCallInst);
    Instruction *L = insertBefore;
    DEBUG(errs() << "going to call RCVRY_SET_RINFO()\n");

    std::vector<Value*> args;
    Constant *rcvrySiteID = M->getGlobalVariable("rcvry_current_site_id");
    LoadInst *loadSiteID = new LoadInst(rcvrySiteID, "", L);

    // Choose the rcvry_action args according to rcvry_type
    switch(rcvryType) {
        case RCVRY_TYPE_UNDO_MALLOC:
        case RCVRY_TYPE_DLCLOSE:
        case RCVRY_TYPE_UNDO_FORK:
	case RCVRY_TYPE_UNDO_SETLOCALE:
                RCVRY_SET_RINFO(L, site_id, txType, rcvryType, 1,
                                RCVRY_TO_VOID_PTR(retVal, L), constPtrNull, constPtrNull); // ret
                break;

        case RCVRY_TYPE_UNDO_REALLOC:
                /* Let's set the errno to ENOMEM - possibly a safe option so that some error handling would
                 * exist for this error in most sane applications
                 */
                RCVRY_SET_RINFO(L, site_id, txType, rcvryType, 2,
                                RCVRY_TO_VOID_PTR(retVal, L),
                                RCVRY_TO_VOID_PTR(callsite.getArgument(0), L),          // ptr
                                constPtrNull);
                break;

        case RCVRY_TYPE_CLOSE:
                /* This does close(ret), which is different from CLOSE_SOCKET *
                 * Let's set the errno to ENOMEM - possibly a safe option so that some error handling would
                 * exist for this error in most sane applications
                 */
                set_errno = ENOMEM;

                if ((0 == libFunc->getName().compare("dup2"))) {
                    set_errno = EBADF;
                }
                RCVRY_SET_RINFO(L, site_id, txType, rcvryType, 3,
                                RCVRY_TO_VOID_PTR(retVal, L),                           // ret (fd)
                                RCVRY_TO_VOID_PTR(CONSTANT_INT(*(this->M), -1), L),      // ret value
                                RCVRY_TO_VOID_PTR(CONSTANT_INT(*(this->M), set_errno), L)); // errno
                break;

        case RCVRY_TYPE_CLOSE_RET_NULL:
                /* This does close(ret), which is different from CLOSE_SOCKET *
                 * Let's set the errno to ENOMEM - possibly a safe option so that some error handling would
                 * exist for this error in most sane applications
                 */
                RCVRY_SET_RINFO(L, site_id, txType, rcvryType, 3,
                                RCVRY_TO_VOID_PTR(retVal, L),                           // ret (fd)
                                RCVRY_TO_VOID_PTR(CONSTANT_INT(*(this->M), 0), L),      // ret value, set to NULL
                                RCVRY_TO_VOID_PTR(CONSTANT_INT(*(this->M), ENOMEM), L)); // errno
                break;

        case RCVRY_TYPE_CLOSE_SOCKET:
                set_errno = ENOMEM;
                if ((0 == libFunc->getName().compare("accept"))) {
                    set_errno = EINVAL;
                } else if ((0 == libFunc->getName().compare("connect"))) {
                    set_errno = EBADF;
                }
                RCVRY_SET_RINFO(L, site_id, txType, rcvryType, 3,
                                RCVRY_TO_VOID_PTR(callsite.getArgument(0), L),          // fd
                                RCVRY_TO_VOID_PTR(CONSTANT_INT(*(this->M), -1), L),      // ret value
                                RCVRY_TO_VOID_PTR(CONSTANT_INT(*(this->M), set_errno), L)); // errno
                break;

        case RCVRY_TYPE_UNDO_MEMALIGN:
                RCVRY_SET_RINFO(L, site_id, txType, rcvryType, 1,
                                RCVRY_TO_VOID_PTR(callsite.getArgument(0), L),
                                constPtrNull,
                                constPtrNull);
                break;

        case RCVRY_TYPE_UNDO_ADDRINFO:
                RCVRY_SET_RINFO(L, site_id, txType, rcvryType, 2,
                                RCVRY_TO_VOID_PTR(callsite.getArgument(3), L),  // res
                                RCVRY_TO_VOID_PTR(CONSTANT_INT(*(this->M), EAI_MEMORY), L), // errno
                                constPtrNull);
                break;

        case RCVRY_TYPE_UNDO_MKDIR:
                RCVRY_SET_RINFO(L, site_id, txType, rcvryType, 2,
                                RCVRY_TO_VOID_PTR(callsite.getArgument(0), L),          // path
                                RCVRY_TO_VOID_PTR(CONSTANT_INT(*(this->M),ENOMEM), L),  // errno
                                constPtrNull);
                break;

        case RCVRY_TYPE_UNDO_MMAP:
                RCVRY_SET_RINFO(L, site_id, txType, rcvryType, 3,
                                RCVRY_TO_VOID_PTR(callsite.getArgument(0), L),             // addr
                                RCVRY_TO_VOID_PTR(callsite.getArgument(1), L),             // len
                                RCVRY_TO_VOID_PTR(CONSTANT_INT(*(this->M), ENOMEM), L));   // errno
                break;
        case RCVRY_TYPE_UNDO_PTHRDMUTX_INIT:
                RCVRY_SET_RINFO(L, site_id, txType, rcvryType, 2,
                                RCVRY_TO_VOID_PTR(callsite.getArgument(0), L),         // mutex
                                RCVRY_TO_VOID_PTR(CONSTANT_INT(*(this->M),ENOMEM), L), // errno
                                constPtrNull);
                break;

        case RCVRY_TYPE_UNDO_PTHRDMUTX_LOCK:
                RCVRY_SET_RINFO(L, site_id, txType, rcvryType, 2,
                                RCVRY_TO_VOID_PTR(callsite.getArgument(0), L),          // mutex
                                RCVRY_TO_VOID_PTR(CONSTANT_INT(*(this->M), EINVAL), L), // errno
                                constPtrNull);
                break;

#define RCVRY_SET_RET_ERRNO(RET, ERRNO)     ({                                                  \
        RCVRY_SET_RINFO(L, site_id, txType, rcvryType, 2,                                       \
                        RCVRY_TO_VOID_PTR(CONSTANT_INT(*(this->M), RET), L),   /* ret */        \
                        RCVRY_TO_VOID_PTR(CONSTANT_INT(*(this->M), ERRNO), L),    /* errno */   \
                        constPtrNull); })

        case RCVRY_TYPE_SET_RET_ERRNO_ENOMEM:
                RCVRY_SET_RET_ERRNO(-1, ENOMEM);
                break;
        
	case RCVRY_TYPE_SET_RET_ERRNO_ENOSPC:
                RCVRY_SET_RET_ERRNO(-1, ENOSPC);
                break;

        case RCVRY_TYPE_SET_RET_ERRNO_EINVAL:
                RCVRY_SET_RET_ERRNO(-1, EINVAL);
                break;
        
        case RCVRY_TYPE_SET_RET_ERRNO_EACCES:
                RCVRY_SET_RET_ERRNO(-1, EACCES);
                break;
        
        case RCVRY_TYPE_SET_RET_ERRNO_EBADF:
                RCVRY_SET_RET_ERRNO(-1, EBADF);
                break;

        case RCVRY_TYPE_SET_RET_ERRNO_EMFILE:
                RCVRY_SET_RET_ERRNO(-1, EMFILE);
                break;

        case RCVRY_TYPE_SET_RET_ERRNO_EFAULT:
                RCVRY_SET_RET_ERRNO(-1, EBADF);
                break;

        case RCVRY_TYPE_SET_RET_ERRNO_ZSTREAM_ERROR:
                RCVRY_SET_RET_ERRNO(Z_STREAM_ERROR, EINVAL); //Z_STREAM_ERROR would be -2
                break;
        
        case RCVRY_TYPE_SET_RET_NULL_ERRNO_ENOMEM:
                RCVRY_SET_RET_ERRNO(0, ENOMEM);
                break;
        
         case RCVRY_TYPE_SET_RET_NULL_ERRNO_EBADF:
                 RCVRY_SET_RET_ERRNO(0, EBADF);
                 break;

        case RCVRY_TYPE_CKPT_BUFF_PREAD:
                // 1) Insert TOL undolog hook, 2) Insert MEMCPY hook to ckpt the buffer
                assert(NULL != loadSiteID);
                args.clear();
                args.push_back(loadSiteID);
                errs() << "creating callinst to ltckptTOLFuncHook. args size: " << args.size() << "\n";
                PassUtil::createCallInstruction(ltckptTOLFuncHook, args, "", L);
                args.clear();
                args.push_back(callsite.getArgument(1)); // buff
                args.push_back(callsite.getArgument(2)); // count
                PassUtil::createCallInstruction(ltckptMemCpyFuncHook, args, "", L);
                break;

        case RCVRY_TYPE_FAIL:
        case RCVRY_TYPE_DEFAULT:
        default:
                DEBUG(errs() << "rcvry_type: fail\n");
                RCVRY_SET_RINFO(L, site_id, txType, rcvryType, 0,
                                constPtrNull, constPtrNull, constPtrNull);
                break;
    }

    return;
}
}
#endif
