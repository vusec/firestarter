#ifndef LTCKPT_LTCKPT_RECOVER_H
#define LTCKPT_LTCKPT_RECOVER_H 1

#define UINT64_T unsigned long long

#define LTCKPT_SHUTTER_BOARD_IPC_END	1024
#define LTCKPT_MAX_WINDOW_SHUTTERS		2560
#define LTCKPT_MODULE_NAME_SIZE			10

// Bitmasks indicating recovery policies

#define LTCKPT_RECOVERY_MASK_IDEMPOTENT			0x1
#define LTCKPT_RECOVERY_MASK_REQUEST_SPECIFIC	0x2
#define LTCKPT_RECOVERY_MASK_PROCESS_SPECIFIC	0x4
#define LTCKPT_RECOVERY_MASK_FAIL_STOP			0x8
#define LTCKPT_RECOVERY_REPROCESSING_REQ  		0x10
#define LTCKPT_RECOVERY_MASK_TEST_MODE			0x20

#define LTCKPT_RECOVERY_NAIVE_MODE_DEFAULT		0
#define LTCKPT_RECOVERY_NAIVE_MODE_NEVER_REPLY		5
#define LTCKPT_RECOVERY_NAIVE_MODE_CONTIDIONAL_REPLY	6
#define LTCKPT_RECOVERY_NAIVE_MODE_ALWAYS_REPLY		7

#define LTCKPT_RECOVERY_FAILURE					-1
#define LTCKPT_RECOVERY_SUCCESS					 0

#define OK 0
#define NOT_OK -1

// sef_llvm MINIX library function;
#define SEF_LLVM_IS_USR_ENDPOINT 	sef_llvm_is_user_endpoint
#define SEF_GET_CURRENT_MESSAGE		sef_get_current_message

#define CHECK_SHUTTER_BOARD_SIZE(INDEX, RETVAL) 	\
		if ( ((INDEX >= g_num_window_shutters) && (INDEX <= LTCKPT_SHUTTER_BOARD_IPC_END)) \
				|| ( (INDEX > LTCKPT_SHUTTER_BOARD_IPC_END) && (INDEX > (LTCKPT_SHUTTER_BOARD_IPC_END + g_num_kernelcall_shutters)) ) \
			  ) \
		{ \
			ltckpt_printf_error("ERROR: site_id specified (%s:%llu) is outside the shutter_board. ipc_sites_max_range: %llu kernelcall_max_range: %llu.\n", g_module_name, INDEX, g_num_window_shutters, LTCKPT_SHUTTER_BOARD_IPC_END + g_num_kernelcall_shutters); \
			return RETVAL; \
		}

#define RECOVERY_BITMASK_CALLSITE_COUNT 64

#include "ltckpt_aop.h"

extern unsigned int g_recovery_bitmask;
extern unsigned int g_recovery_naive_mode;
extern const char *g_recovery_bitmask_reason;
extern uint64_t g_recovery_bitmask_callsites[RECOVERY_BITMASK_CALLSITE_COUNT];
extern int g_recovery_bitmask_callsite_count;
extern uint64_t g_recovery_bitmask_total_count;
extern int suicide_on_window_close;
extern int sa_window__is_open;
extern wstat_t wprof_currwindow_bb_in;
extern int wprof_enable_stats;
extern wprof_t wprof;
static int prior_have_handled_message = 0;
static int wshutter_prof_enabled = 0;

typedef unsigned long long wdata_t;

typedef struct wshutter_prof
{
	wdata_t num_hits;
	
	wdata_t g_first_bb_in;	// global rwindow bb_in when this site was first ever reached.
	wdata_t g_last_bb_in;	// global rwindow bb_in when this site was reached last.
	wdata_t min_bb_in;	// min in-window bbs seen while crossing this site
	wdata_t max_bb_in;	// max in-window bbs seen while crossing this site
	wdata_t num_in_window;  // num of times this site was within window before it was crossed.
	wdata_t num_end;	// num of times this site was the cause for closing the window
} wshutter_prof_t;

typedef struct window_shutter
{
	int do_suicide;
	wshutter_prof_t prof;
} window_shutter;

char *hypermem_cat(char *p, char *pend, const char *s);
char *hypermem_cat_num(char *p, char *pend, long long n);
void hypermem_log(const char *msg);

void ltckpt_do_suicide();
int ltckpt_do_suicide_per_site(UINT64_T site_id);
void ltckpt_open_window();
void ltckpt_dump_shutter_board();
void ltckpt_set_idempotent_recovery(UINT64_T site_id);
void ltckpt_set_request_specific_recovery(UINT64_T site_id);
void ltckpt_set_process_local_recovery(UINT64_T site_id);
void ltckpt_set_fail_stop_recovery(UINT64_T site_id);
int ltckpt_recovery_dispatcher(void *info);
int ltckpt_recovery_idempotent(void *info);
int ltckpt_recovery_request_specific(void *info);
int ltckpt_recovery_process_specific(void *info);
int ltckpt_recovery_fail_stop(void *info);
int ltckpt_is_req_rehandling();
void ltckpt_activate_req_rehandling();
void ltckpt_deactivate_req_rehandling();
int ltckpt_is_message_replyable(int *rs_fi_flag, int *type);
int sef_fi_custom();
void inc_counter_inwindow();
void inc_counter_outsidewindow();
#endif
