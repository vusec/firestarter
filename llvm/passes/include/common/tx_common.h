#ifndef TX_COMMON_H
#define TX_COMMON_H

#define TX_NAMESPACE_TX_TYPE "TX_TYPE"

enum tx_type_e {
	TX_TYPE_INVALID = 0,
	TX_TYPE_LTCKPT = 1,
	TX_TYPE_TSX,
	TX_TYPE_HYBRID,
	__NUM_TX_TYPES
};

typedef enum tx_type_e tx_type_t;

// metadata name instructing to start HW Tx (value is: >0 max size, 0 unknown size, -1 useless tx w/o memory accesses)
#define TXWINDOW_SIZE_NAMESPACE_NAME "TXWINDOWSIZE"
// metadata name instructing to end HW Tx
#define TXWINDOW_END_NAMESPACE_NAME "TXWINDOWEND"
// metadata name instructing to switch to HW Tx (value is: see above)
#define TXWINDOW_SWITCH_SIZE_NAMESPACE_NAME "TXWINDOWSWITCHSIZE"
// metadata name instructing to end HW Tx
#define TXWINDOW_SWITCH_END_NAMESPACE_NAME "TXWINDOWSWITCHEND"

#include <pass.h>

using namespace llvm;

namespace llvm {

class TxUtil
{
public:
    static bool isNonFaultableLibCall(CallInst *libCallInst);
};

inline bool TxUtil::isNonFaultableLibCall(CallInst *libCallInst)
{
    static std::set<std::string> NonFaultableLibCalls = {
    "__ctype_b_loc",
    "__errno_location",
    "__h_errno_location",
    "__libc_start_main",
    "__mulsc3",
    "__isnan",
    "__isinf",
    "atoi",
    "abs",
    "bsearch",
    "exit",
    "freeaddrinfo",
    "getegid",
    "geteuid",
    "getpagesize",
    "getpid",
    "getppid",
    "getopt",
    "gettimeofday",
    "getuid",
    "gmtime",
    "htons",
    "htonl",
    "iswxdigit",
    "localtime",
    "localtime_r",
    "log",
    "memchr",
    "memcmp",
    "memcpy",
    "memmove",
    "memset",
    "modf",
    "nl_langinfo",
    "ntohs",
    "ntohl",
    "pow",
    "printf",
    "random",
    "realloc",
    "setbuf",
    "sprintf",
    "srandom",
    "strcasecmp",
    "strcat",
    "strchr",
    "strcmp",
    "strcpy",
    "strcspn",
    "strdup",
    "strerror",
    "strftime",
    "strlen",
    "strncat",
    "strncasecmp",
    "strncmp",
    "strpbrk",
    "strrchr",
    "strspn",
    "strstr",
    "strtol",
    "strtold",
    "strtof",
    "strtoul",
    "_ZNSi5tellgEv",	// tellg
    "tolower",
    "toupper",
    "truncate",
    "time",
    "ungetc",
    "vsnprintf",

    "acos",
    "acosh",
    "asin",
    "asinh",
    "atan",
    "atan2",
    "atanh",
    "atof",
    "cbrt",
    "clearerr",
    "copysign",
    "cos",
    "cosh",
    "crc32",
    "ctime",
    "difftime",
    "erand48",
    "erfc",
    "exp",
    "fabs",
    "fabsf",
    "fileno",
    "floor",
    "fmod",
    "frexp",
    "fscanf",
    "fgets",
    "gcvt",
    "int_vasprintf",
    "isalnum",
    "isalpha",
    "isatty",
    "islower",
    "isspace",
    "isxdigit",
    "ldexp",
    "log10",
    "logb",
    "modf",
    "pcre_exec",
    "qsort",
    "rand",
    "rint",
    "_ZdaPv", // C++ delete()
    "_ZdlPv", // C++ delete[]
    "_Znam",  // C++ new[]
    "_Znwm",  // C++ new()
    "sin",
    "sinh",
    "sqrt",
    "sqrtf",
    "sscanf",
    "strtok",
    "tan",
    "tanh",
    "vsprintf",
    "_ZSt9terminatev",
    "__cxa_begin_catch",
    "ceil",

    "gmtime_r",

    "_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc",
    "_ZNKSt9basic_iosIcSt11char_traitsIcEEcvPvEv",
    "_ZNSt6vectorIjSaIjEE6resizeEmj",
    "backtrace_symbols",
    "strtod",

    "lua_tonumber",
    };

    assert(NULL != libCallInst);
    std::string funcName = libCallInst->getCalledFunction()->getName();
    if (NonFaultableLibCalls.end() == NonFaultableLibCalls.find(funcName)) {
        return false;
    }
    return true;
}

}
#endif
