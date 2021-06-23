#include "ltckpt_local.h"
#include <signal.h>

#include "ltckpt_debug.h"
#include "ltckpt_recover.h"

// #define uthash_malloc(sz) lt_malloc(sz)
// #define uthash_free(ptr,sz) lt_free(ptr)

#include <common/ut/uthash.h>

/* sanity check of hash indexing of the stats entries.
 * makes it very slow. a check against internal bugs and
 * outside interference (e.g. proxfs writing on our indexing data).
 */
#define HASH_SANITY 0

/*
 * Hook used for aopify-style instrumentation. Sample instrumentation (instruments 50% function entries deterministically):
 * LLVMGOLD_OPTFLAGS_EXTRA="-aopify-prob=0.5 -aopify-rand-seed=1 -aopify-start-hook-map=(^\$)|(^[^l].*\$)|(^l[^t].*\$)/^.*$/ltckpt_aop_hook -tol=main -inline -dse" ./build.llvm basicaa ltckpt ltckptbasic ltckpt_inline aopify
 */

__attribute__((always_inline))
int ltckpt_aop_hook(int hook_type, int block_offset, int argc, void** argv)
{
    CONF(top_of_the_loop_hook());
    return 0;
}

/*
 * Hook used for aopify-style instrumentation to count the number of functions executed:
 * LLVMGOLD_OPTFLAGS_EXTRA="-aopify-start-hook-map=(^\$)|(^[^l].*\$)|(^l[^t].*\$)/^my_tol_function$/ltckpt_aop_hook_tol,(^\$)|(^[^l].*\$)|(^l[^t].*\$)/^.*$/ltckpt_aop_hook_func -tol=main" ./build.llvm basicaa ltckpt ltckptbasic aopify
 */
__attribute__((always_inline))
int ltckpt_aop_hook_func(int hook_type, int block_offset, int argc, void** argv)
{
    CTX_INC(num_aop_funcs);
    return 0;
}

__attribute__((always_inline))
int ltckpt_aop_hook_tol(int hook_type, int block_offset, int argc, void** argv)
{
    CTX_INC(num_aop_tols);
    return 0;
}

#ifdef __MINIX

#include <minix/sef.h>
#include "ltckpt_aop.h"

wprof_t wprof;
wstat_t wprof_currwindow_bb_in;
int last_have_handled_message = 0;
int pes_have_handled_message = 0;
int dfa_have_handled_message = 0;
int wprof_enable_stats = 0;
int wprof_enable = 1;
static int wprof_init = 0;
extern int inside_trusted_compute_base;

/* contexts that first closed the currently closed window, if any */
wprof_pol_t *pes_closer = NULL, *dfa_closer = NULL, *opt_closer = NULL;

static const char *lt_n2d(unsigned long long n)
{
#define LEN 20
	static char s[LEN];
	char *p = s + LEN - 2;
	p[1] = '\0';

	do {
		int r = n % 10;
		p[0] = '0' + r;
		n -= r;
		n /= 10;
		p--;
	} while(n > 0 && p >= s);

	return p + 1;
}

static const char *lt_n2h(unsigned long long n)
{
#define LEN 20
	static char s[LEN];
	char *p = s + LEN - 2;
	p[1] = '\0';

	do {
		int r = n % 16;
		if(r < 10)
			p[0] = '0' + r;
		else
			p[0] = 'a' + r - 10;
		n -= r;
		n /= 16;
		p--;
	} while(n > 0 && p >= s);

	return p + 1;
}

__attribute__((always_inline))
static void resummarize(const char *name, wprof_set_t *where)
{
	/* sprintf() recurses, we do our own formatting */
	memset(where->summary, 0, sizeof(where->summary));
	strcat(where->summary, name);
	strcat(where->summary, ","); strcat(where->summary, lt_n2d(where->pes.bb_in));
	strcat(where->summary, ","); strcat(where->summary, lt_n2d(where->pes.bb_out));
	strcat(where->summary, ","); strcat(where->summary, lt_n2d(where->dfa.bb_in));
	strcat(where->summary, ","); strcat(where->summary, lt_n2d(where->dfa.bb_out));
	strcat(where->summary, ","); strcat(where->summary, lt_n2d(where->opt.bb_in));
	strcat(where->summary, ","); strcat(where->summary, lt_n2d(where->opt.bb_out));
	strcat(where->summary, "\n");
}

#define RECURSIVE_OFF 0xc0ffee
#define RECURSIVE_ON 0xbadda7e
static int recursive = RECURSIVE_OFF;

wprof_set_t *lt_wprof_func_stats(void *caller2, void *caller1, void *caller0, const char *func_name_final, wprof_t *where)
{
	int i;
	struct wprof_scope *wprof_lookup = NULL, *new_node;
	char func_name[FUNCNAME];

	/* recursion check */
        lt_assert(recursive == RECURSIVE_ON || recursive == RECURSIVE_OFF);
        lt_assert(recursive == RECURSIVE_OFF);
	recursive = RECURSIVE_ON;

	/* parameters check */
	lt_assert(where);
	lt_assert(func_name_final);

	/* construct name + caller */
	memset(func_name, 0, sizeof(func_name));
	strcat(func_name, "0x");
	strcat(func_name, lt_n2h((unsigned long long) caller2));
	strcat(func_name, ",0x");
	strcat(func_name, lt_n2h((unsigned long long) caller1));
	strcat(func_name, ",0x");
	strcat(func_name, lt_n2h((unsigned long long) caller0));
	strcat(func_name, ",");
	strcat(func_name, func_name_final);
	lt_assert(strlen(func_name) < sizeof(func_name));

	HASH_FIND_STR(where->index, func_name, wprof_lookup);
	if(wprof_lookup) { recursive = RECURSIVE_OFF; return &wprof_lookup->stats; }

#if HASH_SANITY
	/* quick (?) sanity check of our data structure */
	for(i = 0; i < where->wprof_next_free; i++) {
		/* all these must be in use. */
		lt_assert(where->funcs[i].funcname[0] != '\0');

		/* sanity check - this item must be found as this particular object */
		HASH_FIND_STR(where->index, where->funcs[i].funcname, wprof_lookup);
		lt_assert(wprof_lookup == &where->funcs[i]);
	}
#endif

	/* check for more space */

	i = where->wprof_next_free++;
	assert(i < MAXFUNCS);	/* insta-fail if out of slots */

	/* add entry at i */

	lt_assert(i >= 0 && i < MAXFUNCS);
	new_node = &where->funcs[i];
	lt_assert(new_node->funcname[0] == '\0');

	/* create one for our func. */
	strlcpy(new_node->funcname, func_name, sizeof(new_node->funcname));

#if HASH_SANITY
	lt_assert(new_node->funcname[0] != '\0');

	/* this item must be missing before we add it */
	HASH_FIND_STR(where->index, func_name, wprof_lookup);
	lt_assert(wprof_lookup == NULL);
#endif

	HASH_ADD_STR(where->index, funcname, new_node);

#if HASH_SANITY
	/* this item must be found as this particular object after we add it */
	HASH_FIND_STR(where->index, func_name, wprof_lookup);
	lt_assert(wprof_lookup == &where->funcs[i]);
	lt_assert(new_node->funcname[0] != '\0');
#endif

	recursive = RECURSIVE_OFF;
	return &new_node->stats;
}

/*
 * Hook used for aopify-style instrumentation to profile the checkpoint window:
 * LLVM_PASS_ARGS="-aopify-hook-args=%NUM_INSTS%,%FUNCTION_NAME% '-aopify-start-hook-map=(^\$)|(^[^l].*\$)|(^l[^t].*\$)/^.*$/ltckpt_aop_hook_wprof'" ./build.llvm aopify aopify-block
 */
__attribute__((always_inline))
int ltckpt_aop_hook_wprof(int hook_type, int block_offset, int argc, void** argv)
{
    extern int have_handled_message;
    int num_insts;
    lt_assert(argc == 2);
    const char *func_name = (const char*) argv[1];
    wprof_set_t *funcprof = NULL;

    if(!wprof_init) {
	memset(&wprof, 0, sizeof(wprof));
	wprof_init = 1;
    }

    /* Do stuff only when profiling is enabled. */
    if (!wprof_enable) {
	/* just keep last in sync, for when we reactivate the profiling. */
	last_have_handled_message = have_handled_message;
	return 0;
    }

    lt_assert(func_name);

    if(wprof_enable_stats) {
    	void *c0 = NULL, *c1 = NULL, *c2 = NULL;
	if(__builtin_frame_address(1)) c0 = __builtin_return_address(1);
	if(__builtin_frame_address(2)) c1 = __builtin_return_address(2);
	if(__builtin_frame_address(3)) c2 = __builtin_return_address(3);
	if(!(funcprof = lt_wprof_func_stats(c2, c1, c0, func_name, &wprof))) {
		return 0;
	}
    }

    if (hook_type == 0) {
	/* First basic block, so this is also the start of the function. */
	const char *names[] = { "_ipc_send", "_ipc_notify", "_do_kernel_call", NULL };
	int i;

	for (i = 0; pes_have_handled_message && names[i]; i++) {
	    /* If we match any of the names[] prefix, we are leaving the window
	     * for the pessimistic approach. */
        if (strncmp(func_name, names[i], strlen(names[i])) == 0) {
            pes_have_handled_message = 0;
			if(funcprof) pes_closer = &funcprof->pes;
			if (wprof_enable_stats) {
			    wprof.glo.pes.end++;
			    if (i == 0) wprof.glo.pes.end_s++;
			    else if (i == 1) wprof.glo.pes.end_n++;
			    else if (i == 2) wprof.glo.pes.end_k++;
			    funcprof->opt.end++;
			}
			if (suicide_on_window_close) {
				suicide_on_window_close = 0;
				// Lets crash here.
				ltckpt_do_suicide();
			}
    	}
	}
    }

    if (have_handled_message && !last_have_handled_message) {
	/* We are entering the window, so make sure all the policy see the
	 * change. */
	pes_have_handled_message = dfa_have_handled_message = 1;

	/* window open, so no closers */
	pes_closer = opt_closer = dfa_closer = NULL;
    } else if (!have_handled_message && last_have_handled_message) {
	/* We are leaving the window for the optimistic policy, so make sure all
	 * the policy see the change.  Most other policies should have already
	 * closed it, but just to be sure, we enforce it here. */
	pes_have_handled_message = dfa_have_handled_message = sa_window__is_open = 0;

	if (wprof_enable_stats) {
	    wprof.glo.opt.end++;
	    wprof.glo.opt.end_k++;
	    funcprof->opt.end++;
	    opt_closer = &funcprof->opt;
	}
	if (suicide_on_window_close) {
		suicide_on_window_close = 0;
		// Lets crash here.
		ltckpt_do_suicide();
	}

    }

    if (wprof_enable_stats) {
	/* Basic Block accounting */
	num_insts = (int) argv[0];

	if (have_handled_message) {
	    wprof.glo.opt.bb_in += num_insts;
	    funcprof->opt.bb_in += num_insts;
	} else {
	    wprof.glo.opt.bb_out += num_insts;
	    funcprof->opt.bb_out += num_insts;
	}

	if (pes_have_handled_message) {
	    wprof.glo.pes.bb_in += num_insts;
	    funcprof->pes.bb_in += num_insts;
	} else {
	    wprof.glo.pes.bb_out += num_insts;
	    funcprof->pes.bb_out += num_insts;
	}

	if (dfa_have_handled_message) {
	    wprof.glo.dfa.bb_in += num_insts;
	    funcprof->dfa.bb_in += num_insts;
	} else {
	    wprof.glo.dfa.bb_out += num_insts;
	    funcprof->dfa.bb_out += num_insts;
	}
	if (sa_window__is_open)
	{
		//wprof_currwindow_bb_in += num_insts;
		wprof.glo.rwindow.bb_in += num_insts;
	}
	else
	{
		wprof.glo.rwindow.bb_out += num_insts;
	}
	if(hook_type == 0) {
	        resummarize(func_name, funcprof);
		resummarize("GLOBAL$", &wprof.glo);
	}

	/* if we're outside the window, keep blaming the cause */
	if(opt_closer) opt_closer->ended_cumulative += num_insts;
	if(pes_closer) pes_closer->ended_cumulative += num_insts;
    }

    last_have_handled_message = have_handled_message;

    return 0;
}

/*
 * Hook used for aopify-style instrumentation to profile the "RECOVERY WINDOW"
 * LLVM_PASS_ARGS="-aopify-hook-args=%NUM_INSTS%'-aopify-start-hook-map=(^\$)|(^[^l].*\$)|(^l[^t].*\$)/^.*$/ltckpt_aop_hook_rwindow_prof'" ./build.llvm aopify aopify-block
 */
int ltckpt_aop_hook_rwindowprof(int hook_type, int block_offset, int argc, void** argv)
{
  int num_insts;
  extern int sa_window__is_open;

  if (!wprof_enable)
  {
    return 0;
  }

  if(!wprof_init)
  {
    memset(&wprof, 0, sizeof(wprof));
    wprof_init = 1;
  }

  if (wprof_enable_stats)
  {
    // we only need the num_insts
    assert(argc == 2);
    num_insts = (int) argv[0];

    if (inside_trusted_compute_base)
    {
	wprof.glo.rwindow.bb_tcb += num_insts;
    }
    else if (sa_window__is_open)
  	{
     	  /* recovery window is open */
  	  wprof.glo.rwindow.bb_in += num_insts;	
	  wprof_currwindow_bb_in += num_insts;
  	}
  	else
  	{
	  /* recovery window is closed */
	  wprof.glo.rwindow.bb_out += num_insts;
      	  if (suicide_on_window_close)
	  {
    	    suicide_on_window_close = 0;
    	    // Lets crash here.
    	    ltckpt_do_suicide();
    	  }
  	}
  }
  return 0;
}

void ltckpt_wprof_pol_print(const char* name, wprof_pol_t *pol)
{
    printf("                     %s: bb_in=%llu, bb_out=%llu, end(T:k+s+n)=(%llu:%llu+%llu+%llu)\n",
        name, pol->bb_in, pol->bb_out, pol->end, pol->end_k, pol->end_s, pol->end_n);
}

void sef_signal_handle_default_hook(int signum)
{
    extern endpoint_t sef_self_endpoint;
    if (signum == SIGUSR2) {
        memset(&wprof, 0, sizeof(wprof));
    }
    if (signum == SIGTERM) {
    	suicide_on_window_close = 1;
    	ltckpt_printf("Enabled crash-on-window-close.\n");
    	ltckpt_dump_shutter_board();
    }
    if (signum != SIGUSR1)
        return;
    wprof_enable_stats = wprof_enable_stats ? 0 : 1;
    if (!wprof_enable_stats) {
    printf("sef_signal_default_hook (endpoint=%d):\n", sef_self_endpoint);
    ltckpt_wprof_pol_print("PES", &wprof.glo.pes);
    ltckpt_wprof_pol_print("DFA", &wprof.glo.dfa);
    ltckpt_wprof_pol_print("OPT", &wprof.glo.opt);
    ltckpt_wprof_pol_print("RWINDOW", &wprof.glo.rwindow);
    }
}
#endif
