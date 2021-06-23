#include <edfi/ctl/server.h>
#include <minix/com.h>
#include <minix/ipc.h>
#include <minix/syslib.h>
#include <minix/sysutil.h>
#include <minix/timers.h>
#include <stdarg.h>
#include <stdio.h>

/* Warning: all the Minix C library functions called from here should be whitelisted in the EDFI pass. */

static unsigned long edfi_rand_next = 1;

typedef unsigned int reg_t;
extern reg_t get_bp(void);

int edfi_ctl_process_request(void *ctl_request)
{
    message *m_ptr = (message*) ctl_request;
    char *buff;
    size_t buff_size;
    cp_grant_id_t gid;
    int ret;
    edfi_cmd_data_t *data;

    gid = m_ptr->m_lsys_fi_ctl.gid;
    buff_size = m_ptr->m_lsys_fi_ctl.size;
    if (buff_size != sizeof(edfi_cmd_data_t) + sizeof(data->context_size) +
        sizeof(data->params_size)) {
        return E2BIG;
    }
    buff = (char*) alloca(buff_size);
    assert(buff);
    ret = sys_safecopyfrom(m_ptr->m_source, gid, 0,
        (vir_bytes) buff, sizeof(buff_size));
    assert(ret == 0);
    data = (edfi_cmd_data_t*) buff;
    buff += sizeof(edfi_cmd_data_t);
    data->context = (edfi_context_t*) buff;
    buff += sizeof(data->context_size);
    data->params = buff;
    ret = edfi_process_cmd(data);

    m_ptr->m_lsys_fi_reply.status = ret;
    _ipc_send_intr(m_ptr->m_source, m_ptr);

    return ret;
}

void edfi_print_stacktrace()
{
    reg_t bp, pc, hbp;

    bp= get_bp();
    while(bp)
    {
        pc= ((reg_t *)bp)[1];
        hbp= ((reg_t *)bp)[0];
        edfi_printf("0x%lx ", (unsigned long) pc);
        if (hbp != 0 && hbp <= bp) {
            edfi_printf("0x%lx ", (unsigned long) -1);
            break;
        }
        bp= hbp;
    }
    edfi_printf("\n");
}

unsigned long edfi_get_dynamic_fault_id()
{
    reg_t bp, pc, hbp;
    unsigned long df_id =0;
    char buff[16];
    buff[15] = '\0';

    bp= get_bp();
    while(bp)
    {
        pc= ((reg_t *)bp)[1];
        hbp= ((reg_t *)bp)[0];
        memcpy(buff, &pc, sizeof(unsigned long));
        df_id = edfi_hash(df_id, buff);
        bp= hbp;
    }
    edfi_printf("\n");

    return df_id;
}

void edfi_printf(char* fmt, ...)
{
    va_list args;
    va_start(args,fmt);
    vprintf(fmt,args);
    va_end(args);
}

void edfi_srand(unsigned int seed)
{
    edfi_rand_next = (unsigned long) seed;
}

int edfi_rand()
{
    edfi_rand_next = edfi_rand_next * 1103515245 + 12345;
    return (int)(edfi_rand_next % ((unsigned long)RAND_MAX + 1));
}

__attribute__((always_inline)) unsigned long long edfi_getcurrtime_ns(void) {
    u64_t now;
    read_tsc_64(&now);
    unsigned long long ns = tsc_64_to_micros(now);
    ns *= 1000;
    return ns;
}

int edfi_load_dflib(const char* path)
{
    return ENOSYS;
}

void *edfi_context_realloc(void)
{
    return &edfi_context_buff;
}

static void edfi_ctl_context_init() {
    extern int edfi_faultinjection_enabled;
    edfi_context->c.rand_seed = 17;
#ifdef EDFI_ENABLE_INJECTION_ON_START
    edfi_faultinjection_enabled = 1;
#else
    edfi_faultinjection_enabled = 0;
#endif
}

void edfi_ctl_init(){
    int i;

    edfi_context = (edfi_context_t*) edfi_context_realloc();
    assert(edfi_context);
    edfi_ctl_context_init();
    edfi_srand(edfi_context->c.rand_seed);

    if (edfi_context->bb_num_executions) {
	edfi_context->bb_num_executions[0] = EDFI_CANARY_VALUE;
	for(i=1; i <= edfi_context->num_bbs; i++){
	    edfi_context->bb_num_executions[i] = 0;
	}
	edfi_context->bb_num_executions[edfi_context->num_bbs + 1] = EDFI_CANARY_VALUE;
    }
    EDFI_CHECK_CONTEXT(edfi_context);
}

