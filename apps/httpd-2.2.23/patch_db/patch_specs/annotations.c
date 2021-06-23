// Update points
// long-lived loop used for crash recovery, but not useful as an update point { .module_name=main.c (0x81bb6ca), .line_num=442, .function_name=main (0x81bb6d1), .site_name=entry (0x81bb6c4), .site_depth=0 }, { .module_name=main.c (0x81bb6ca), .line_num=695, .function_name=main (0x81bb6d1), .site_name=for.cond (0x81cab3d), .site_depth=1 }, { .module_name=worker.c (0x81c3438), .line_num=1684, .function_name=ap_mpm_run (0x81c346b), .site_name=entry (0x81bb6c4), .site_depth=0 }, { .module_name=worker.c (0x81c3438), .line_num=1586, .function_name=server_main_loop (0x81c34b0), .site_name=entry (0x81bb6c4), .site_depth=0 }, { .module_name=worker.c (0x81c3438), .line_num=1594, .function_name=server_main_loop (0x81c34b0), .site_name=while.cond (0x81cab46), .site_depth=1 }
thread_update_point(); // { .module_name=main.c (0x81bb6ca), .line_num=442, .function_name=main (0x81bb6d1), .site_name=entry (0x81bb6c4), .site_depth=0 }, { .module_name=main.c (0x81bb6ca), .line_num=695, .function_name=main (0x81bb6d1), .site_name=for.cond (0x81cab3d), .site_depth=1 }, { .module_name=mod_cgid.c (0x81c2950), .line_num=836, .function_name=cgid_start (0x81c2aae), .site_name=entry (0x81bb6c4), .site_depth=0 }, { .module_name=mod_cgid.c (0x81c2950), .line_num=594, .function_name=cgid_server (0x81c2aea), .site_name=entry (0x81bb6c4), .site_depth=0 }, { .module_name=mod_cgid.c (0x81c2950), .line_num=664, .function_name=cgid_server (0x81c2aea), .site_name=while.cond (0x81cab46), .site_depth=1 }
thread_update_point(); // { .module_name=main.c (0x81bb6ca), .line_num=442, .function_name=main (0x81bb6d1), .site_name=entry (0x81bb6c4), .site_depth=0 }, { .module_name=main.c (0x81bb6ca), .line_num=695, .function_name=main (0x81bb6d1), .site_name=for.cond (0x81cab3d), .site_depth=1 }, { .module_name=worker.c (0x81c3438), .line_num=1684, .function_name=ap_mpm_run (0x81c346b), .site_name=entry (0x81bb6c4), .site_depth=0 }, { .module_name=worker.c (0x81c3438), .line_num=1586, .function_name=server_main_loop (0x81c34b0), .site_name=entry (0x81bb6c4), .site_depth=0 }, { .module_name=worker.c (0x81c3438), .line_num=1594, .function_name=server_main_loop (0x81c34b0), .site_name=while.cond (0x81cab46), .site_depth=1 }, { .module_name=worker.c (0x81c3438), .line_num=1567, .function_name=perform_idle_server_maintenance (0x81c35e0), .site_name=for.cond124 (0x81cb31d), .site_depth=1 }, { .module_name=worker.c (0x81c3438), .line_num=1132, .function_name=child_main (0x81c3600), .site_name=entry (0x81bb6c4), .site_depth=0 }, { .module_name=worker.c (0x81c3438), .line_num=1259, .function_name=child_main (0x81c3600), .site_name=while.body (0x81cabbc), .site_depth=1 }, { .module_name=pod.c (0x81c38a5), .line_num=44, .function_name=ap_mpm_pod_check (0x81c38c0), .site_name=entry (0x81bb6c4), .site_depth=0 }
thread_update_point(); // { .module_name=threadproc/unix/thread.c (0x81ca100), .line_num=139, .function_name=dummy_worker (0x81ca1f2), .site_name=entry (0x81bb6c4), .site_depth=0 }, { .module_name=worker.c (0x81c3438), .line_num=820, .function_name=worker_thread (0x81c36d5), .site_name=entry (0x81bb6c4), .site_depth=0 }, { .module_name=worker.c (0x81c3438), .line_num=843, .function_name=worker_thread (0x81c36d5), .site_name=while.cond (0x81cab46), .site_depth=1 }, { .module_name=worker.c (0x81c3438), .line_num=859, .function_name=worker_thread (0x81c36d5), .site_name=worker_pop (0x81cb329), .site_depth=2 }, { .module_name=fdqueue.c (0x81c3792), .line_num=335, .function_name=ap_queue_pop (0x81c386f), .site_name=entry (0x81bb6c4), .site_depth=0 }, { .module_name=locks/unix/thread_cond.c (0x81c82b0), .line_num=63, .function_name=apr_thread_cond_wait (0x81c8310), .site_name=entry (0x81bb6c4), .site_depth=0 } (seen by 50 threads)
thread_update_point(); // { .module_name=threadproc/unix/thread.c (0x81ca100), .line_num=139, .function_name=dummy_worker (0x81ca1f2), .site_name=entry (0x81bb6c4), .site_depth=0 }, { .module_name=worker.c (0x81c3438), .line_num=594, .function_name=listener_thread (0x81c3707), .site_name=entry (0x81bb6c4), .site_depth=0 }, { .module_name=worker.c (0x81c3438), .line_num=634, .function_name=listener_thread (0x81c3707), .site_name=while.body (0x81cabbc), .site_depth=1 }

// Unions with inner pointers
typedef  YYSTYPE pxfer_YYSTYPE;
typedef union cmd_func pxfer_cmd_func;
typedef ap_filter_func pxfer_ap_filter_func;
typedef apr_descriptor pxfer_apr_descriptor;
typedef apr_pollcb_pset pxfer_apr_pollcb_pset;
typedef apr_anylock_u_t pxfer_apr_anylock_u_t;
typedef apr_bucket_structs noxfer_apr_bucket_structs;
typedef union semun cixfer_semun;
union pxfer_definea_arg_u { /*...*/ };
union pxfer_allowdeyn_u { /*....*/ };

union bind_arg_u { /*...*/ };

char* st_typename_keys[] = { "bind_arg_u", "ap_mgmt_value", NULL };
#define st_idx_bind_arg_u 0
#define st_idx_ap_mgmt_value 0

PRIVATE int st_transfer_ap_mgmt_value(_magic_selement_t *selement, _magic_sel_analyzed_t *sel_analyzed, _magic_sel_stats_t *sel_stats, struct st_cb_info *cb_info) {
    ap_mgmt_item_t *sel_parent_data;
    _magic_selement_t parent_selement_buff, *parent_selement;
    int ret;

    parent_selement = magic_selement_get_parent(selement, &parent_selement_buff);
    assert(parent_selement);

    sel_parent_data = (ap_mgmt_item_t*) parent_selement->address;
    if(!sel_parent_data->type) {
        /* Skip when unused. */
        return MAGIC_SENTRY_ANALYZE_SKIP_PATH;
    }
    switch(sel_parent_data->type) {
        case 2:
            /* Identity transfer when no ptr is involved. */
            ret = st_cb_transfer_identity(selement, sel_analyzed, sel_stats, cb_info);
            break;
        case 1:
        case 3:
            /* Pointer transfer otherwise. */
            ret = st_cb_transfer_ptr(selement, sel_analyzed, sel_stats, cb_info);
            break;
        default:
            /* Unknown? Report error. */
            ST_CB_PRINT(ST_CB_ERR, "Bad type!", selement, sel_analyzed, sel_stats, cb_info);
            ret = EFAULT;
            break;
    }
    return ret;
}
PRIVATE int st_transfer_bind_arg_u(_magic_selement_t *selement, _magic_sel_analyzed_t *sel_analyzed, _magic_sel_stats_t *sel_stats, struct st_cb_info *cb_info) {
    bind_arg *sel_parent_data;
    _magic_selement_t parent_selement_buff, *parent_selement;
    int ret;

    parent_selement = magic_selement_get_parent(selement, &parent_selement_buff);
    assert(parent_selement);

    sel_parent_data = (struct bind_arg*) parent_selement->address;
    if(!sel_parent_data->type) {
        /* Skip when unused. */
        return MAGIC_SENTRY_ANALYZE_SKIP_PATH;
    }
    switch(sel_parent_data->type) {
        case 3:
        case 4:
        case 5:
            /* Identity transfer when no ptr is involved. */
            ret = st_cb_transfer_identity(selement, sel_analyzed, sel_stats, cb_info);
            break;
        case 1:
        case 2:
        case 5:
            /* Pointer transfer otherwise. */
            ret = st_cb_transfer_ptr(selement, sel_analyzed, sel_stats, cb_info);
            break;
        default:
            /* Unknown? Report error. */
            ST_CB_PRINT(ST_CB_ERR, "Bad type!", selement, sel_analyzed, sel_stats, cb_info);
            ret = EFAULT;
            break;
    }
    return ret;
}
int st_sf_transfer_typename(_magic_selement_t *selement, _magic_sel_analyzed_t *sel_analyzed, _magic_sel_stats_t *sel_stats, struct st_cb_info *cb_info) {
    const char *typename_key = ST_TYPE_NAME_KEY(selement->type);
    if(ST_TYPE_NAME_MATCH(st_typename_keys[st_idx_bind_arg_u],typename_key)) {
        return st_transfer_bind_arg_u(selement, sel_analyzed, sel_stats, cb_info);
    }
    else if(ST_TYPE_NAME_MATCH(st_typename_keys[st_idx_ap_mgmt_value],typename_key)) {
        return st_transfer_ap_mgmt_value(selement, sel_analyzed, sel_stats, cb_info);
    }
    return ST_CB_NOT_PROCESSED;
}

st_register_typename_keys(st_typename_keys);
st_setcb_selement_transfer(st_sf_transfer_typename, ST_CB_TYPE_TYPENAME);
