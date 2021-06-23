// Update points
// long-lived loop used for crash recovery, but not useful as an update point { .module_name=src/core/nginx.c (0x810b8f0), .line_num=199, .function_name=main (0x810b901), .site_name=entry (0x810b906), .site_depth=0 }, { .module_name=src/os/unix/ngx_process_cycle.c (0x810e640), .line_num=83, .function_name=ngx_master_process_cycle (0x810e660), .site_name=entry (0x810b906), .site_depth=0 }, { .module_name=src/os/unix/ngx_process_cycle.c (0x810e640), .line_num=146, .function_name=ngx_master_process_cycle (0x810e660), .site_name=for.cond31 (0x8114ba3), .site_depth=1 }
thread_update_point(); // { .module_name=src/core/nginx.c (0x810b8f0), .line_num=199, .function_name=main (0x810b901), .site_name=entry (0x810b906), .site_depth=0 }, { .module_name=src/os/unix/ngx_process_cycle.c (0x810e640), .line_num=83, .function_name=ngx_master_process_cycle (0x810e660), .site_name=entry (0x810b906), .site_depth=0 }, { .module_name=src/os/unix/ngx_process_cycle.c (0x810e640), .line_num=342, .function_name=ngx_start_worker_processes (0x810e680), .site_name=entry (0x810b906), .site_depth=0 }, { .module_name=src/os/unix/ngx_process_cycle.c (0x810e640), .line_num=351, .function_name=ngx_start_worker_processes (0x810e680), .site_name=for.cond (0x8114aca), .site_depth=1 }, { .module_name=src/os/unix/ngx_process.c (0x810e430), .line_num=85, .function_name=ngx_spawn_process (0x810e470), .site_name=entry (0x810b906), .site_depth=0 }, { .module_name=src/os/unix/ngx_process_cycle.c (0x810e640), .line_num=712, .function_name=ngx_worker_process_cycle (0x810e870), .site_name=entry (0x810b906), .site_depth=0 }, { .module_name=src/os/unix/ngx_process_cycle.c (0x810e640), .line_num=771, .function_name=ngx_worker_process_cycle (0x810e870), .site_name=for.cond (0x8114aca), .site_depth=1 }

// Unions with inner pointers
typedef epoll_data_t noxfer_epoll_data_t;
union pxfer_ngx_resolver_node_t_u;

// Memory mgm instrumentation
char* st_typename_keys[] = { "ngx_hash_elt_t", NULL };
#define st_idx_ngx_hash_elt_t 0

static int st_transfer_ngx_hash_elt_t(_magic_selement_t *selement, _magic_sel_analyzed_t *sel_analyzed, _magic_sel_stats_t *sel_stats, struct st_cb_info *cb_info) {
      ngx_hash_elt_t *elt = (ngx_hash_elt_t*) selement->address;
      ngx_hash_elt_t *local_elt = (ngx_hash_elt_t*) cb_info->local_selement->address;
      int ret;
      int saved_bits;

      saved_bits = elt->value & 3;
      ret = st_cb_transfer_selement_generic(selement, sel_analyzed, sel_stats, cb_info);
      assert(MAGIC_SENTRY_ANALYZE_IS_VALID_RET(ret));
      local_elt->value |= saved_bits;

      return ret;
}
int st_sf_transfer_typename(_magic_selement_t *selement, _magic_sel_analyzed_t *sel_analyzed, _magic_sel_stats_t *sel_stats, struct st_cb_info *cb_info) {
      const char *typename_key = ST_TYPE_NAME_KEY(selement->type);
      if(ST_TYPE_NAME_MATCH(st_typename_keys[st_idx_ngx_hash_elt_t],typename_key)) {
          return st_transfer_ngx_hash_elt_t(selement, sel_analyzed, sel_stats, cb_info);
      }
      return ST_CB_NOT_PROCESSED;
}

st_register_typename_keys(st_typename_keys);
st_setcb_selement_transfer(st_sf_transfer_typename, ST_CB_TYPE_TYPENAME);
