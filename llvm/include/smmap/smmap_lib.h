/* Shared Library SMMAP header
 *
 * In some occasions we want SMMAP to be available as a shared library. In
 * this manner the functionality can be injected inside a process and used
 * directly from inside the application. This header is realted to the shared
 * library verions of the SMMAP user API.
 */

#ifndef _SMMAP_LIB_H_
#define _SMMAP_LIB_H_

#include <smmap/smmap_common.h>

int lsmmap(void *addr, void *shadow_addr, size_t size);
int lsmunmap(void *addr);
int smc_checkpoint(void);
int smc_rollback_default(void);
int smc_clear_stats(void);
int smc_show_stats(void);

#endif /* _SMMAP_LIB_H_ */
