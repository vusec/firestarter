#include <stdlib.h>
#include <string.h>
#include "rcvry_common.h"
#include "rcvry_fi.h"

#ifdef RCVRY_FI
void __attribute__((constructor)) rcvry_fi_init()
{
	// Check env variable and initialize the fault_probability values
	// for all rcvry_info[] entries
    char *strFISites = getenv("RCVRY_FI_SITES");
    char *strNonFISites = getenv("RCVRY_NON_FI_SITES");
    char *strSiteID = NULL;
    int skip[30] = { 46, 47, 396, 394, 568, 507, 215, 276, 278, 397, 351, 281, 110, 219, 221,
                     177, 179, 432, 186, 194, 203, 355, 454, 476, 489, 492, 495, 497, 499, 0};

    if ((NULL == strFISites) || (0 == strncmp("", strFISites, strlen(strFISites)))) {
        for (uint32_t i=0; i < RCVRY_MAX_LIBCALL_SITES; i++) {
            rcvry_info[i].rcvry_fi.fault_probability = 100;
        }
        for (int i=0; i < 30; i++) {
            rcvry_info[skip[i]].rcvry_fi.fault_probability = 0;
        }

    } else {
        uint32_t site_id;
        while(NULL != (strSiteID = strsep(&strFISites, ","))) {
            site_id = strtoul(strSiteID, NULL, 0);
            rcvry_info[site_id].rcvry_fi.fault_probability = 100;
        }
    }
    uint32_t site_id;
    rcvry_print_debug("strNonFISites: %s\n", strNonFISites);
    while (NULL != (strSiteID = strsep(&strNonFISites, ","))) {
        site_id = strtoul(strSiteID, NULL, 0);
        rcvry_info[site_id].rcvry_fi.fault_probability = 0;
    }
    return;
}
#endif

int rcvry_fi_enabled(unsigned site_id)
{
	if (rcvry_info[site_id].rcvry_fi.fault_probability == 100) {
		return 1;
	}
	// TODO: Add probability based trigger.

	return 0;
}

void rcvry_fi_fatal_fault(unsigned site_id)
{
	if (rcvry_fi_enabled(site_id)) {
        rcvry_print_info("fi_fatal_fault: site_id: %d\n", site_id);
        rcvry_info[site_id].rcvry_fi.fault_probability = 0; // disable it for next
		volatile int *ptr = NULL;
		*ptr = 0;
	}
}
