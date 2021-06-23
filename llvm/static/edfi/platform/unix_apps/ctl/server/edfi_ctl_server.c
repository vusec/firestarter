#define _GNU_SOURCE

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <dlfcn.h>
#include <sched.h>
#include <sys/mman.h>
#include <execinfo.h>
#include <signal.h>

#include <edfi/ctl/server.h>

#define EDFI_CONTEXT_RELOCATE_DEFAULT 1
#ifndef EDFI_CONTEXT_RELOCATE
#define EDFI_CONTEXT_RELOCATE EDFI_CONTEXT_RELOCATE_DEFAULT
#endif

static FILE *edfi_fp = NULL;
static void *edfi_dflib = NULL;
static int edfi_server_fd = -1;
static volatile int edfi_ctl_num_completed_requests;
static volatile int edfi_ctl_max_num_requests;
static volatile int edfi_svr_thread_done;

static void edfi_ctl_time_init(void);

static void edfi_ctl_init();
static void edfi_ctl_close();
static void __edfi_ctl_init() __attribute__ ((constructor));
static void __edfi_ctl_close() __attribute__ ((destructor));
static void *__edfi_ctl_srv(void *);

#ifdef MAGIC_WITH_EDFI
#undef EDFI_CONTEXT_RELOCATE
#define EDFI_CONTEXT_RELOCATE 0
#include <magic_mem.h>
#define EDFI_WRAPPER_BEGIN MAGIC_MEM_WRAPPER_BEGIN
#define EDFI_WRAPPER_END MAGIC_MEM_WRAPPER_END
#else
#define EDFI_WRAPPER_BEGIN()
#define EDFI_WRAPPER_END()
#endif

static void __edfi_ctl_init()
{
    EDFI_WRAPPER_BEGIN();
    edfi_ctl_init();
    EDFI_WRAPPER_END();
}
static void __edfi_ctl_close()
{
    EDFI_WRAPPER_BEGIN();
    edfi_ctl_close();
    EDFI_WRAPPER_END();
}

#ifndef EDFI_DISABLE_CTL_SRV
static void *edfi_ctl_srv(void *args) {
    void *ret;
    EDFI_WRAPPER_BEGIN();
    ret = __edfi_ctl_srv(args);
    EDFI_WRAPPER_END();
    return ret;
}

static void *__edfi_ctl_srv(void *args){

    int s, len;
    struct sockaddr_un local, remote;
    sigset_t set;

    if ((edfi_server_fd = socket(AF_UNIX, SOCK_STREAM|SOCK_CLOEXEC, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    local.sun_family = AF_UNIX;
    snprintf(local.sun_path, UNIX_PATH_MAX, "#%s.%d", EDFI_CTL_PATH, getpid());
    len = SUN_LEN(&local);
    local.sun_path[0] = '\0';

    if (bind(edfi_server_fd, (struct sockaddr *)&local, len) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(edfi_server_fd, 5) == -1) {
        perror("listen");
        exit(1);
    }

    assert(sigfillset(&set) == 0);
    assert(pthread_sigmask(SIG_SETMASK, &set, NULL) == 0);

    do {
        char *data_buff;
        size_t data_buff_size;
        unsigned int t = sizeof(remote);
        edfi_cmd_data_t data;

        data_buff = NULL;
        memset(&data, 0, sizeof(edfi_cmd_data_t));
        if ((s = accept(edfi_server_fd, (struct sockaddr *)&remote, &t)) == -1) {
            perror("accept");
            exit(1);
        }

        if (recv(s, &data, sizeof(edfi_cmd_data_t), 0) < 0) {
            perror("recv");
            exit(1);
        }

        data_buff_size = data.context_size + data.params_size;
        if (data_buff_size > 0) {
            if (data.context_size != sizeof(edfi_context_t)) {
                edfi_printf("Wrong context size (%d != %d)\n", data.context_size, sizeof(edfi_context_t));
                exit(1);
            }
            data_buff = calloc(1, data_buff_size);
            if (recv(s, data_buff, data_buff_size, 0) < 0) {
                perror("recv_data");
                exit(1);
            }
            data.context = (edfi_context_t*) data_buff;
            data.params = data_buff + sizeof(edfi_context_t);
        }

        data.status = edfi_ctl_process_request(&data);

        /* send back same message to confirm that we're done */
        if (send(s, &data, sizeof(data), 0) == -1) {
            perror("send");
            exit(1);
        }

        close(s);
        free(data_buff);
        edfi_ctl_num_completed_requests++;
    } while(edfi_ctl_max_num_requests == 0 || edfi_ctl_num_completed_requests < edfi_ctl_max_num_requests);

    close(edfi_server_fd);
    edfi_svr_thread_done = 1;

    return 0;
}
#endif /* !defined(EDFI_DISABLE_CTL_SRV) */

#ifdef EDFI_PRINT_HYPERMEMLIKE_STATS
void edfi_print_hypermemlike_stats(void);
#endif    
#ifdef EDFI_UPDATE_STATS_FILE
void edfi_stats_per_bb_add(void);
#endif

static void edfi_set_proc_fp() {
    static char buff[512];

    sprintf(buff, "%s/%s.%d", edfi_context->output_dir, EDFI_OUTPUT_FILE, getpid());
    edfi_fp = fopen(buff, "w");
    if (!edfi_fp) {
        fprintf(stderr, "edfi_set_proc_fp: warning: unable to open output file %s, resorting to stdout...", buff);
        return;
    }
    setbuf(edfi_fp , NULL);

#ifdef EDFI_PRINT_HYPERMEMLIKE_STATS
    atexit(edfi_print_hypermemlike_stats);
#endif    
#ifdef EDFI_UPDATE_STATS_FILE
    atexit(edfi_stats_per_bb_add);
#endif
}

static void edfi_fork_noop() {}

static void edfi_fork_child()
{
    EDFI_WRAPPER_BEGIN();
    edfi_ctl_close();
    edfi_ctl_init();
    EDFI_WRAPPER_END();
}

static unsigned long long edfi_cycles_per_ns = 0;

static struct timespec timespec_diff(struct timespec *t, struct timespec *s)
{
  struct timespec diff_ts;
  diff_ts.tv_sec = t->tv_sec - s->tv_sec;
  diff_ts.tv_nsec = t->tv_nsec - s->tv_nsec;
  if (diff_ts.tv_nsec < 0) {
    diff_ts.tv_sec--;
    diff_ts.tv_nsec += 1000 * 1000 * 1000;
  }

  return diff_ts;
}

static void edfi_ctl_time_init(void) {
    cpu_set_t cpu_mask;
    struct timespec start_ts, end_ts, diff_ts;
    unsigned long long i, start = 0, end = 0, diff;

    CPU_ZERO(&cpu_mask);
    CPU_SET(0, &cpu_mask);
    sched_setaffinity(0, sizeof(cpu_mask), &cpu_mask);

    edfi_cycles_per_ns = 1;
    clock_gettime(CLOCK_MONOTONIC, &start_ts);
    start = edfi_getcurrtime_ns();
    for (i = 0; i < 1000000; i++);
    end = edfi_getcurrtime_ns();
    clock_gettime(CLOCK_MONOTONIC, &end_ts);

    diff_ts = timespec_diff(&end_ts, &start_ts);
    diff = diff_ts.tv_sec * (1000 * 1000 * 1000) + diff_ts.tv_nsec;
    edfi_cycles_per_ns = (end - start)/(diff ? diff : 1);
    if (edfi_cycles_per_ns == 0) {
        edfi_cycles_per_ns = 1;
    }
}

static int parse_int_env_var(const char *env_name, int default_value)
{
    char *env_value_str = getenv(env_name);
    char *tail_ptr = NULL;
    int env_value;

    if (!env_value_str) {
        return default_value;
    }
    env_value = strtol(env_value_str, &tail_ptr, 0);
    if (env_value == 0 && env_value_str == tail_ptr) {
        return default_value;
    }

    return env_value;
}

static float parse_float_env_var(const char *env_name, float default_value)
{
    char *env_value_str = getenv(env_name);
    char *tail_ptr = NULL;
    float env_value;

    if (!env_value_str) {
        return default_value;
    }
    env_value = strtof(env_value_str, &tail_ptr);
    if (env_value == 0.0 && env_value_str == tail_ptr) {
        return default_value;
    }

    return env_value;
}

static char *parse_str_env_var(const char *env_name, char *default_value)
{
    char *env_value_str = getenv(env_name);

    if (!env_value_str)
        return default_value;

    return strdup(env_value_str);
}

static void edfi_ctl_context_init() {
    extern int edfi_faultinjection_enabled;
#ifdef EDFI_DISABLE_CTL_SRV
    edfi_context->no_svr_thread = 1;
#else
    edfi_context->no_svr_thread = parse_int_env_var("EDFI_NO_SVR_THREAD", 0);
#endif
    edfi_context->num_requests_on_start = parse_int_env_var("EDFI_NUM_REQUESTS_ON_START", 0);
    edfi_context->c.fault_prob = parse_float_env_var("EDFI_FAULT_PROB", 0.0);
    edfi_context->c.fault_prob_randmax = (EDFI_RAND_MAX + 1) * edfi_context->c.fault_prob;
    if (edfi_context->c.fault_prob > 0 && edfi_context->c.fault_prob_randmax == 0) {
	edfi_context->c.fault_prob_randmax = 1;
    }
    edfi_context->c.rand_seed = parse_int_env_var("EDFI_RAND_SEED", 0);
    edfi_context->verbosity = parse_int_env_var("EDFI_VERBOSITY", 0);
#ifdef EDFI_ENABLE_INJECTION_ON_START
    edfi_faultinjection_enabled = 1;
#else
    edfi_faultinjection_enabled = parse_int_env_var("EDFI_FI_ENABLED", 0);
#endif
    edfi_context->output_dir = parse_str_env_var("LOGDIR", EDFI_OUTPUT_DIR);

    if (edfi_context->verbosity > 0) {
        fprintf(stderr, "edfi_ctl_context_init: EDFI_NO_SVR_THREAD=%d EDFI_NUM_REQUESTS_ON_START=%d EDFI_FAULT_PROB=%f EDFI_RAND_SEED=%d EDFI_VERBOSITY=%d EDFI_FI_ENABLED=%d LOGDIR=%s\n",
            edfi_context->no_svr_thread, edfi_context->num_requests_on_start, edfi_context->c.fault_prob, edfi_context->c.rand_seed, edfi_context->verbosity, edfi_faultinjection_enabled, edfi_context->output_dir);
    }
}

static void edfi_ctl_init(){
    pthread_t thread;
    int i;
    static int first_run=1;

    if (first_run) {
        edfi_context = (edfi_context_t*) edfi_context_realloc();
        assert(edfi_context);
        edfi_ctl_time_init();
        edfi_ctl_context_init();
    }
    edfi_srand(edfi_context->c.rand_seed);

#if EDFI_USE_PROC_OUTPUT_FILES
    edfi_set_proc_fp();
#else
    if(edfi_fp == NULL){
        int fd = dup(STDOUT_FILENO);
        if(fd < 0){
            printf("ERROR: stdout duplication result: %d\n", fd);
            exit(1);
        }
        edfi_fp = fdopen(fd, "w");
        setbuf(edfi_fp , NULL);
    }
#endif

    if (edfi_context->bb_num_executions) {
	edfi_context->bb_num_executions[0] = EDFI_CANARY_VALUE;
	for(i=1; i <= edfi_context->num_bbs; i++){
	    edfi_context->bb_num_executions[i] = 0;
	}
	edfi_context->bb_num_executions[edfi_context->num_bbs + 1] = EDFI_CANARY_VALUE;
    }

#ifndef EDFI_DISABLE_CTL_SRV
    if(!edfi_context->no_svr_thread || edfi_context->num_requests_on_start > 0) {
        edfi_ctl_num_completed_requests = 0;
        edfi_ctl_max_num_requests = 0;
        edfi_svr_thread_done = 0;
        if (edfi_context->no_svr_thread) {
            edfi_ctl_max_num_requests = edfi_context->num_requests_on_start;
        }
        pthread_create(&thread, NULL, edfi_ctl_srv, NULL);
        if (edfi_context->num_requests_on_start > 0) {
            while (edfi_ctl_num_completed_requests < edfi_context->num_requests_on_start) {
                usleep(100000);
            }
        }
        if (edfi_context->no_svr_thread) {
            while (!edfi_svr_thread_done) {
                usleep(100000);
            }
        }
    }
#endif

    if (first_run)
        assert(pthread_atfork(edfi_fork_noop, edfi_fork_noop, edfi_fork_child) == 0);
    first_run = 0;
    edfi_context->num_requests_on_start = 0;
    EDFI_CHECK_CONTEXT(edfi_context);
}

static void edfi_ctl_close(){
    close(edfi_server_fd);
    fclose(edfi_fp);
}

int edfi_ctl_process_request(void *ctl_request)
{
    return edfi_process_cmd((edfi_cmd_data_t*) ctl_request);
}

void edfi_print_stacktrace()
{
    void *array[100];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace(array, 100);
    strings = backtrace_symbols(array, size);

    for (i = 0; i < size; i++)
       edfi_printf("%s\n", strings[i]);

    free (strings);
}

unsigned long edfi_get_dynamic_fault_id()
{
    void *array[100];
    size_t size;
    char **strings;
    size_t i;
    unsigned long df_id = 0;

    size = backtrace(array, 100);
    strings = backtrace_symbols(array, size);

    for (i = 0; i < size; i++)
       df_id = edfi_hash(df_id, strings[i]);

    free (strings);

    return df_id;
}

void edfi_printf(char* fmt, ...){
    va_list args;
    va_start(args,fmt);
    if(edfi_fp){
        vfprintf(edfi_fp, fmt,args);
    }else{
        vprintf(fmt,args);
    }
    va_end(args);
}

static unsigned long int edfi_rand_next = 1;

void edfi_srand(unsigned int seed)
{
    edfi_rand_next = seed;
}

int edfi_rand(void)
{
    edfi_rand_next = edfi_rand_next * 1103515245 + 12345;
    return (unsigned int)(edfi_rand_next/65536) % 32768;
}

__attribute__((always_inline)) inline unsigned long long edfi_getcurrtime_ns(void) {
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x/edfi_cycles_per_ns;
}

int edfi_load_dflib(const char* path)
{
    edfi_onstart_t onstart_p;
    edfi_onfdp_t onfdp_p;
    edfi_onfault_t onfault_p;
    edfi_onstop_t onstop_p;
    void *dflib;

    dflib = dlopen(path, RTLD_LAZY);
    if (!dflib) {
        return -1;
    }

    *(void **) (&onstart_p) = dlsym(dflib, "edfi_onstart");
    if (!onstart_p) {
        return -2;
    }
    *(void **) (&onfdp_p) = dlsym(dflib, "edfi_onfdp");
    if (!onfdp_p) {
        return -3;
    }
    *(void **) (&onfault_p) = dlsym(dflib, "edfi_onfault");
    if (!onfault_p) {
        return -4;
    }
    *(void **) (&onstop_p) = dlsym(dflib, "edfi_onstop");
    if (!onstop_p) {
        return -5;
    }

    edfi_onstart_p = onstart_p;
    edfi_onfdp_p = onfdp_p;
    edfi_onfault_p = onfault_p;
    edfi_onstop_p = onstop_p;

    if (edfi_dflib) {
        dlclose(edfi_dflib);
    }
    edfi_dflib = dflib;

    return 0;
}

void *edfi_context_realloc(void)
{
#if EDFI_CONTEXT_RELOCATE
    int r;
    long page_size = sysconf(_SC_PAGESIZE);
    char *addr = (void*) EDFI_CONTEXT_ADDRESS;
    char *base_addr = addr - page_size;

    assert(sizeof(edfi_context_t) <= page_size);
    r = msync(base_addr, page_size*3, 0);
    assert(r < 0 && errno == ENOMEM);

    base_addr = mmap(base_addr, page_size*3, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, 0, 0);
    assert(base_addr != MAP_FAILED);
    addr = mmap(addr, page_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, 0, 0);
    assert(addr != MAP_FAILED);

    memcpy(addr, &edfi_context_buff, sizeof(edfi_context_t));

    return addr;
#else
    return &edfi_context_buff;
#endif
}

