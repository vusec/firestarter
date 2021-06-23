#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/errno.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/file.h>
#include <unistd.h>

#include <edfi/df/df.h>
#include <edfi/df/statfile.h>

#define EDFI_STAT_PRINTER(args...) edfi_printf(args)

unsigned long edfi_onfdp_count;

#ifdef EDFI_ENABLE_INJECTION_ON_START
int edfi_faultinjection_enabled = 1;
#else
int edfi_faultinjection_enabled;
#endif
int edfi_faultinjection_enabled_version;
int edfi_inject_bb = -1;
edfi_context_t edfi_context_buff = {
	.canary_value1 = EDFI_CANARY_VALUE,
	.c = {
		.rand_seed = 123,
	},
	.canary_value2 = EDFI_CANARY_VALUE,
};

edfi_context_t *edfi_context = &edfi_context_buff;

#ifndef EDFI_ONFDP_DEFAULT
#define EDFI_ONFDP_DEFAULT edfi_onfdp_default
#endif

#ifndef EDFI_ONFAULT_DEFAULT
#define EDFI_ONFAULT_DEFAULT edfi_onfault_default
#endif

/*TODO: must be volatile to ensure linkage? */
edfi_onstart_t edfi_onstart_p = edfi_onstart_default;
edfi_onfdp_t edfi_onfdp_p = EDFI_ONFDP_DEFAULT;
edfi_onfault_t edfi_onfault_p = EDFI_ONFAULT_DEFAULT;
edfi_onstop_t edfi_onstop_p = edfi_onstop_default;
char* edfi_module_name = "NONE"; /* Set by the pass */

/*TODO: cmdline swith fault injector: either test switch or callback */

void edfi_onstart_default(char *params){
    edfi_context->fault_fdp_count = edfi_context->c.min_fdp_interval;
    edfi_context->start_time = edfi_getcurrtime_ns();
}

unsigned long edfi_last_fault_time;

int edfi_onfdp_min_fdp_interval()
{
    if(edfi_context->c.min_fdp_interval > 0 && edfi_context->fault_fdp_count < edfi_context->c.min_fdp_interval){
        edfi_context->fault_fdp_count++;
        return 0;
    }
    edfi_onfdp_count += edfi_context->c.min_fdp_interval;
    return 1;
}

int edfi_onfdp_min_fault_time_interval()
{
    edfi_last_fault_time = 0;
    if(edfi_context->c.min_fault_time_interval > 0 && edfi_context->fault_time != 0) {
    	edfi_last_fault_time = edfi_getcurrtime_ns();
        if(edfi_context->fault_time + edfi_context->c.min_fault_time_interval < edfi_last_fault_time){
            return 0;
        }
    }
    edfi_onfdp_count += edfi_context->c.min_fault_time_interval;
    return 1;
}

int edfi_onfdp_fault_prob()
{
    if(edfi_context->c.fault_prob_randmax == 0) return 1;
    if(edfi_rand() >= edfi_context->c.fault_prob_randmax) return 0;
    edfi_onfdp_count++;
    return 1;
}

int edfi_onfdp_dyn_fault_prob(int bb_index)
{
    if (!edfi_faultinjection_enabled || edfi_context->c.fault_prob_randmax == 0) {
    	return 0;
    }
    if (edfi_rand() >= edfi_context->c.fault_prob_randmax){
        return 0;
    }
    return 1;
}

int edfi_onfdp_bb_index(int bb_index)
{
    if (!edfi_faultinjection_enabled) {
        return 0;
    }
    if (bb_index != edfi_context->c.faulty_bb_index && edfi_context->c.faulty_bb_index > 0) {
        return 0;
    }
    /* note: this value will only be checked for zero/non-zero status */
    if (edfi_context->c.faulty_bb_index > 0) edfi_onfdp_count = 1;
    return 1;
}

void edfi_onfdp_min_fdp_interval_update()
{
    edfi_context->fault_fdp_count = 1;
}

void edfi_onfdp_min_fault_time_interval_update()
{
    if(edfi_context->c.min_fault_time_interval > 0){
        edfi_context->fault_time = edfi_last_fault_time ? edfi_last_fault_time : edfi_getcurrtime_ns();
    }
}

int edfi_onfdp_max_time()
{
    if(edfi_context->c.max_time > 0 
    	    && edfi_context->start_time + edfi_context->c.max_time >= edfi_getcurrtime_ns()) {
        edfi_stop();
        return 0;
    }
    edfi_onfdp_count += edfi_context->c.max_time;
    return 1;
}

int edfi_onfdp_max_faults()
{
    if(edfi_context->c.max_faults > 0 && edfi_context->total_faults > edfi_context->c.max_faults){
        return 0;
    }
    edfi_onfdp_count += edfi_context->c.max_faults;
    return 1;
}

int edfi_onfdp_default(int bb_index) {

#ifdef EDFI_COUNT_ALL_BLOCKS
    /* first value is a canary, but bb_index is 1-based so no compensation needed */
    edfi_context->bb_num_executions[bb_index]++;
#endif

    if(!edfi_faultinjection_enabled){
        return 0;
    }
    edfi_onfdp_count = 0;

    if (edfi_onfdp_min_fdp_interval() == 0) {
        return 0;
    }

    if (edfi_onfdp_min_fault_time_interval() == 0) {
        return 0;
    }

    if (edfi_onfdp_fault_prob() == 0) {
        return 0;
    }

    if (edfi_onfdp_bb_index(bb_index) == 0) {
        return 0;
    }

    if (edfi_onfdp_max_time() == 0) {
        return 0;
    }

    if (edfi_onfdp_max_faults() == 0) {
        return 0;
    }

    if (edfi_onfdp_count == 0) {
        return 0;
    }

    edfi_onfdp_min_fdp_interval_update();
    edfi_onfdp_min_fault_time_interval_update();

    return 1;
}

#ifndef EDFI_DUMMY_COUNTERS_HACK
/*void edfi_onfault_default(const char *file, int line, int num_fault_types, ...){*/
void edfi_onfault_default(int bb_index){
#ifndef EDFI_COUNT_ALL_BLOCKS
    /* first value is a canary, but bb_index is 1-based so no compensation needed */
    edfi_context->bb_num_executions[bb_index]++;
#endif

    /* subtract 1 because bb_index is 1-based */
    edfi_context->total_faults += edfi_context->bb_num_faults[bb_index - 1];
}
#else
int bb_num_executions_fix=0;
/*void edfi_onfault_default(const char *file, int line, int num_fault_types, ...){*/
void edfi_onfault_default(int bb_index){
    bb_num_executions_fix++;
}
#endif

void edfi_onstop_default(){
    edfi_print_stats();
}

int edfi_start(edfi_context_t *context, char *params) {
    int ret;

    ret = edfi_update_context(context);
    if (ret < 0) {
        return ret;
    }
    edfi_faultinjection_enabled = 1;
    edfi_faultinjection_enabled_version++;
    (*edfi_onstart_p)(params);
    return 0;
}

void edfi_stop() {
    edfi_faultinjection_enabled = 0;
    edfi_faultinjection_enabled_version++;
    (*edfi_onstop_p)();
}

void edfi_test(){
    edfi_printf("edfi_test done\n");
}

int edfi_stat(edfi_context_t *context){
    edfi_print_stats();
    return edfi_update_context(context);
}

void edfi_print_stats_old(char *fault_name, int fault_count){
    edfi_printf("%20s: %d\n", fault_name, fault_count);
}

static void edfi_print_stats_section(char *title, int section){
    int fault_index;
    EDFI_STAT_PRINTER(EDFI_STATS_HEADER_FMT, title);
    for(fault_index=0; fault_index < edfi_context->num_fault_types; fault_index++){
        exec_count num_injected_executed = 0, num_candidates_executed = 0;
        int bb_index;
        fault_type_stats *fault_type = &edfi_context->fault_type_stats[fault_index];
        for(bb_index=0; bb_index < edfi_context->num_bbs; bb_index++){
            exec_count num_executions = edfi_context->bb_num_executions[bb_index + 1];
            num_injected_executed += fault_type->bb_num_injected[bb_index] * num_executions;
            num_candidates_executed += fault_type->bb_num_candidates[bb_index] * num_executions;
        }
        if(section == EDFI_STATS_SECTION_PROB){
            EDFI_STAT_PRINTER(EDFI_STATS_LD_FMT, fault_type->name, num_candidates_executed ? ((long double)num_injected_executed)/num_candidates_executed : 0);
        }else if(section == EDFI_STATS_SECTION_FAULTS){
            EDFI_STAT_PRINTER(EDFI_STATS_ULL_FMT, fault_type->name, num_injected_executed);
        }else if(section == EDFI_STATS_SECTION_CANDIDATES){
            EDFI_STAT_PRINTER(EDFI_STATS_ULL_FMT, fault_type->name, num_candidates_executed);
        }else{
            edfi_printf("Unknown section number: %d\n", section);
        }
    }
}

#ifdef EDFI_UPDATE_STATS_FILE
static void edfi_stats_per_bb_add_data(
	const void *data, 
	size_t size,
	unsigned long long *result_p,
	int *index_p) {

	int index = *index_p;
	unsigned char *p = (unsigned char *) data;

	while (size-- > 0) {
		((unsigned char *) result_p)[index++] ^= *(p++);
		if (index >= sizeof(*result_p)) {
			*result_p *= 3;
			index = 0;
		}
	}
	*index_p = index;
}

static unsigned long long edfi_stats_per_bb_get_id(void) {
	int fault_index, result_index = 0;
	unsigned long long result = 0;

	/* create a (hopefully unique) identifier for the binary based on the
	 * information stored about the fault candidates per basic block
	 */
	edfi_stats_per_bb_add_data(
		&edfi_context->num_bbs,
		sizeof(edfi_context->num_bbs),
		&result,
		&result_index);
	edfi_stats_per_bb_add_data(
		&edfi_context->num_fault_types,
		sizeof(edfi_context->num_fault_types),
		&result,
		&result_index);
	for (fault_index = 0; fault_index < edfi_context->num_fault_types; fault_index++) {
		fault_type_stats *fault_type = &edfi_context->fault_type_stats[fault_index];
		edfi_stats_per_bb_add_data(
			fault_type->bb_num_candidates,
			edfi_context->num_bbs * sizeof(fault_type->bb_num_candidates[0]),
			&result,
			&result_index);
	}

	return result ^ result_index;
}

static const char *get_exe_path(void) {
	static int initialized;
	static char path[PATH_MAX];

	if (!initialized) {
		readlink("/proc/self/exe", path, sizeof(path));
		initialized = 1;
	}
	return path;
}

static const char *edfi_stats_per_bb_get_path(void) {
	static char path[512];
	static int initialized;

	if (!initialized) {
		unsigned long long id = edfi_stats_per_bb_get_id();
		
		snprintf(path, sizeof(path), "%s/%s.%.*llx",
			edfi_context->output_dir, EDFI_STAT_FILE, sizeof(id) * 2, id);
		initialized = 1;
	}
	return path;
}

void edfi_stats_per_bb_add_with_fd(int fd) {
	void *data;
	struct edfi_stats_header *data_header;
	char *data_fault_names;
	uint64_t *data_bb_num_executions;
	int *data_bb_num_candidates;
	size_t datasize;
	fault_type_stats *fault_type;
	int i;
	ssize_t r;

	/* allocate space for entire file */
	datasize = 
		sizeof(struct edfi_stats_header) + /* header */
		EDFI_STATS_FAULT_NAME_LEN * edfi_context->num_fault_types + /* fault_names */
		sizeof(uint64_t) * edfi_context->num_bbs + /* bb_num_executions */
		sizeof(int) * edfi_context->num_bbs * edfi_context->num_fault_types; /* bb_num_candidates */

	data = malloc(datasize);
	if (!data) {
		perror("ERROR: cannot allocate memory for statistics file");
		return;
	}

	data_header = (struct edfi_stats_header *) data;
	data_fault_names = (char *) (data_header + 1);
	data_bb_num_executions = (uint64_t *) (data_fault_names + EDFI_STATS_FAULT_NAME_LEN * edfi_context->num_fault_types);
	data_bb_num_candidates = (int *) (data_bb_num_executions + edfi_context->num_bbs);

	r = read(fd, data, datasize);
	if (r < 0) {
		perror("ERROR: read of statistics file failed");
		goto cleanup;
	} else if (r == 0) {
		/* build new file */
		memset(data, 0, datasize);
		data_header->magic = EDFI_STATS_MAGIC;
		data_header->num_bbs = edfi_context->num_bbs;
		data_header->num_fault_types = edfi_context->num_fault_types;
		data_header->fault_name_len = EDFI_STATS_FAULT_NAME_LEN;
		for (i = 0; i < edfi_context->num_fault_types; i++) {
			fault_type = &edfi_context->fault_type_stats[i];
			strncpy(data_fault_names + i * EDFI_STATS_FAULT_NAME_LEN,
				fault_type->name,
				EDFI_STATS_FAULT_NAME_LEN);
			memcpy(data_bb_num_candidates + i * edfi_context->num_bbs,
				fault_type->bb_num_candidates,
				sizeof(int) * edfi_context->num_bbs);
		}
	} else if (r == datasize) {
		/* verify old file */
		if (data_header->magic != EDFI_STATS_MAGIC ||
			data_header->num_bbs != edfi_context->num_bbs ||
			data_header->num_fault_types != edfi_context->num_fault_types ||
			data_header->fault_name_len != EDFI_STATS_FAULT_NAME_LEN) {
			fprintf(stderr, "ERROR: old statistics file header inconsistent with executable\n");
			goto cleanup;
		}
		for (i = 0; i < edfi_context->num_fault_types; i++) {
			fault_type = &edfi_context->fault_type_stats[i];
			if (strncmp(data_fault_names + i * EDFI_STATS_FAULT_NAME_LEN,
				fault_type->name,
				EDFI_STATS_FAULT_NAME_LEN) != 0) {
				fprintf(stderr, "ERROR: old statistics file fault names inconsistent with executable\n");
				goto cleanup;
			}
			if (memcmp(data_bb_num_candidates + i * edfi_context->num_bbs,
				fault_type->bb_num_candidates,
				sizeof(int) * edfi_context->num_bbs) != 0) {
				fprintf(stderr, "ERROR: old statistics file fault candidate counts inconsistent with executable\n");
				goto cleanup;
			}
		}
	} else {
		fprintf(stderr, "ERROR: old statistics size inconsistent with executable\n");
		goto cleanup;
	}

	/* add execution counts to those in the file */
	for (i = 0; i < edfi_context->num_bbs; i++) {
		data_bb_num_executions[i] += edfi_context->bb_num_executions[i + 1];
	}

	/* return to the start */
	if (lseek(fd, 0, SEEK_SET) != 0) {
		perror("ERROR: seek of statistics file failed");
		goto cleanup;
	}

	/* write out new file */
	r = write(fd, data, datasize);
	if (r != datasize) {
		perror("ERROR: write of statistics file failed or did not complete entirely");
		goto cleanup;
	}

cleanup:
	free(data);
}

static int edfi_stats_available(void)
{
	int i;

	for (i = 1; i <= edfi_context->num_bbs; i++) {
		if (edfi_context->bb_num_executions[i] > 0) return 1;
	}

	return 0;
}

void edfi_stats_per_bb_add(void) {
	int fd;
	const char *path;

	if (!edfi_stats_available()) {
		fprintf(stderr, "WARNING: no statistics collected\n");
		return;
	}

	path = edfi_stats_per_bb_get_path();
	EDFI_STAT_PRINTER(EDFI_STATS_HEADER_FMT, EDFI_STATS_SECTION_PER_BB_NAME);
	EDFI_STAT_PRINTER(EDFI_STATS_S_FMT, "stats_path", path);
	EDFI_STAT_PRINTER(EDFI_STATS_S_FMT, "exe_path", get_exe_path());

	fd = open(path, O_CREAT | O_RDWR, 0644);
	if (fd < 0) {
		fprintf(stderr, "ERROR: cannot open %s: %s\n", path, strerror(errno));
		return;
	}

	if (flock(fd, LOCK_EX) < 0) {
		fprintf(stderr, "ERROR: cannot lock %s: %s\n", path, strerror(errno));
		close(fd);
		return;
	}

	edfi_stats_per_bb_add_with_fd(fd);

	close(fd);
}
#endif

#ifdef EDFI_PRINT_HYPERMEMLIKE_STATS
void edfi_print_hypermemlike_stats(void);
#endif

void edfi_print_stats(){
    unsigned int statistics_table_size, edfi_context_size, total_static_data_size;
    int i;

    statistics_table_size= edfi_context->num_bbs * sizeof(exec_count);
    for(i=0;i<edfi_context->num_fault_types;i++){
        statistics_table_size += strlen(edfi_context->fault_type_stats[i].name)+1
            + sizeof(fault_type_stats) + 2 * edfi_context->num_bbs * sizeof(int);
    }
    edfi_context_size= sizeof(edfi_context_t);
    total_static_data_size = statistics_table_size + edfi_context_size;

    EDFI_STAT_PRINTER(EDFI_STATS_HEADER_FMT, EDFI_STATS_SECTION_DEBUG_NAME);
    EDFI_STAT_PRINTER(EDFI_STATS_UL_FMT, "n_fault_types", edfi_context->num_fault_types);
    EDFI_STAT_PRINTER(EDFI_STATS_UL_FMT, "n_instrumented_bbs", edfi_context->num_bbs);
    EDFI_STAT_PRINTER(EDFI_STATS_UL_FMT, "statistics_table_size", statistics_table_size);
    EDFI_STAT_PRINTER(EDFI_STATS_UL_FMT, "edfi_context_size", edfi_context_size);
    EDFI_STAT_PRINTER(EDFI_STATS_UL_FMT, "total_static_data_size", total_static_data_size);

    edfi_print_stats_section(EDFI_STATS_SECTION_PROB_NAME, EDFI_STATS_SECTION_PROB);
    edfi_print_stats_section(EDFI_STATS_SECTION_FAULTS_NAME, EDFI_STATS_SECTION_FAULTS);
    edfi_print_stats_section(EDFI_STATS_SECTION_CANDIDATES_NAME, EDFI_STATS_SECTION_CANDIDATES);
#ifdef EDFI_PRINT_HYPERMEMLIKE_STATS
    edfi_print_hypermemlike_stats();
#endif
}
volatile unsigned long edfi_ensure_linkage2 = (unsigned long) &edfi_print_stats_old;

#ifdef EDFI_PRINT_HYPERMEMLIKE_STATS
void edfi_print_hypermemlike_stats(void) {
    exec_count count, countrep;
    int i, repeats;

    EDFI_STAT_PRINTER(EDFI_STATS_HEADER_FMT, EDFI_STATS_SECTION_HYPERMEMLIKE_STATS);

    EDFI_STAT_PRINTER("edfi_dump_stats_module name=%s", edfi_module_name);
    countrep = 0;
    repeats = 0;
    for (i = 1; i <= edfi_context->num_bbs; i++) {
        count = edfi_context->bb_num_executions[i];
	if (countrep == count) {
	    repeats++;
	} else {
	    if (repeats == 1) {
		EDFI_STAT_PRINTER(" %lu", (long) countrep);
	    } else if (repeats != 0) {
		EDFI_STAT_PRINTER(" %lux%u", (long) countrep, repeats);
	    }
	    countrep = count;
	    repeats = 1;
	}
    }
    if (repeats == 1) {
	EDFI_STAT_PRINTER(" %lu", (long) countrep);
    } else if (repeats != 0) { 
	EDFI_STAT_PRINTER(" %lux%u", (long) countrep, repeats);
    }
    EDFI_STAT_PRINTER("\n");
}
#endif

int edfi_update_context(edfi_context_t *new_context)
{
    int ret;

    if (!new_context)
        return 0;

    edfi_srand(new_context->c.rand_seed);
    if (strlen(new_context->c.dflib_path) > 0) {
        ret = edfi_load_dflib(new_context->c.dflib_path);
        if (ret < 0) {
            return ret;
        }
    }

    memcpy(&edfi_context->c, &new_context->c, sizeof(edfi_context_conf_t));
    EDFI_PRINT_CONTEXT(edfi_printf, edfi_context);

    return 0;
}

int edfi_process_cmd(edfi_cmd_data_t *data) {
    int ret = 0;

    switch(data->cmd) {
    case EDFI_CTL_CMD_STOP:
         edfi_stop();
         break;
    case EDFI_CTL_CMD_START:
         ret = edfi_start(data->context, data->params);
         break;
    case EDFI_CTL_CMD_TEST:
         edfi_test();
         break;
    case EDFI_CTL_CMD_STAT:
         edfi_stat(data->context);
         break;
    case EDFI_CTL_CMD_UPDATE:
         ret = edfi_update_context(data->context);
         break;
    default:
    	 ret = -1;
    }

    return ret;
}

#if EDFI_USE_DYN_WRAPPER_FAULT_PROBS
static float edfi_dyn_overflow_prob = 1;
#define Nto2N(X) (((float)edfi_rand()) / (((unsigned)RAND_MAX)+1) < edfi_dyn_overflow_prob ? X + 1 + (edfi_rand()%X) : X)
#else
#define Nto2N(X) ( X + 1 + (edfi_rand()%X) )
#endif

void *edfi_memcpy_wrapper(void *dest, const void *src, size_t n){
    return memcpy(dest, src, Nto2N(n));
}

void *edfi_memcpy64_wrapper(void *dest, const void *src, long long n){
    return memcpy(dest, src, Nto2N(n));
}

void *edfi_memmove_wrapper(void *dest, const void *src, size_t n){
    return memmove(dest, src, Nto2N(n));
}

void *edfi_memmove64_wrapper(void *dest, const void *src, long long n){
    return memmove(dest, src, Nto2N(n));
}

void *edfi_memset_wrapper(void *s, int c, size_t n){
    return memset(s, c, Nto2N(n));
}

void *edfi_memset64_wrapper(void *s, int c, long long n){
    return memset(s, c, Nto2N(n));
}

char *edfi_strncpy_wrapper(char *dest, const char *src, size_t n){
    return strncpy(dest, src, Nto2N(n));
}

char *edfi_strcpy_wrapper(char *dest, const char *src){
    return edfi_strncpy_wrapper(dest, src, strlen(dest)+1);
}

void *edfi_malloc_wrapper(size_t size){
    return malloc(edfi_rand() % size);
}

void edfi_free_wrapper(void *ptr){
    /* skip free */
}

int edfi_munmap_wrapper(void *addr, size_t length){
    /* skip munmap */
    return 0;
}

void edfi_print_sections(void)
{
    edfi_printf("edfi_print_sections: data=[0x%08x;0x%08x], ro=[0x%08x;0x%08x], text=[0x%08x;0x%08x]",
           (unsigned long) EDFI_DATA_SECTION_START, (unsigned long) EDFI_DATA_SECTION_END,
           (unsigned long) EDFI_DATA_RO_SECTION_START, (unsigned long) EDFI_DATA_RO_SECTION_END,
           (unsigned long) EDFI_TEXT_SECTION_START, (unsigned long) EDFI_TEXT_SECTION_END);
}

#ifdef EDFI_BB_TRACING_FOR_RCVRY
uint32_t edfi_bbtrace_hashlist[EDFI_BB_TRACING_HASHLIST_SIZE];  // helps in avoiding duplicates
uint32_t edfi_bbtrace_history[EDFI_BB_TRACING_HISTORY_SIZE];    // last executed trace window
uint32_t edfi_bbtrace_startuplist[EDFI_BB_TRACING_HASHLIST_SIZE];
uint32_t edfi_bbtrace_next_index=0, edfi_bbtrace_num_bbs=0, edfi_bbtrace_is_startup=1, edfi_disable_tracing=0;

__attribute__((constructor))
void edfi_trace_init()
{
    char *envVar;
    assert(edfi_context->num_bbs < EDFI_BB_TRACING_HASHLIST_SIZE);
    memset(&edfi_bbtrace_hashlist[0], 0, edfi_context->num_bbs * sizeof(unsigned));
    edfi_bbtrace_next_index = 0;
    edfi_bbtrace_num_bbs = edfi_context->num_bbs;
    edfi_bbtrace_is_startup = 1;
    for (uint32_t i=0; i < EDFI_BB_TRACING_HASHLIST_SIZE; i++) {
        edfi_bbtrace_startuplist[i] = 0;
    }
    if (NULL != (envVar = getenv("DISABLE_BBTRACE_DUMP"))) {
        edfi_disable_tracing = (strtoul(envVar, NULL, 0) == 1 ? 1 : 0);
    }
#ifdef EDFI_TRACING_DEBUG
    printf("edfi_bbtrace_is_startup: %d, edfi_disable_tracing: %d\n", edfi_bbtrace_is_startup, edfi_disable_tracing);
#endif
    return;
}

void edfi_trace_bb(unsigned bb_id)
{
    if (edfi_disable_tracing) {
        return;
    }
#ifdef EDFI_TRACING_DEBUG
    printf("edfi_trace_bb: bb %d\n", bb_id);
#endif
    assert(bb_id < edfi_context->num_bbs);
	if (0 == edfi_bbtrace_hashlist[bb_id]) {
		edfi_bbtrace_hashlist[bb_id] = bb_id;
		edfi_bbtrace_history[edfi_bbtrace_next_index++] = bb_id;
        assert(edfi_bbtrace_next_index < EDFI_BB_TRACING_HISTORY_SIZE);
	}
    if (edfi_bbtrace_is_startup) {
        edfi_bbtrace_startuplist[bb_id] = bb_id;
    }
}
#endif
