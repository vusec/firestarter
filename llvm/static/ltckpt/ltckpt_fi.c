#include "ltckpt_common.h"
#include "ltckpt_fi.h"

uint32_t  ltckpt_suicide_enabled = 0;
uint32_t  ltckpt_suicide_board[LTCKPT_MAX_SUICIDE_SITES] = { 1 };
uint32_t  ltckpt_suicide_last_site_id; // Set by instrumentation

void ltckpt_fi_suicide()
{
    ltckpt_printf("!!SUICIDE!!\n");
    volatile char *p = 0;
    (*p)++;
}

int ltckpt_fi_suicide_per_site(uint32_t site_id, int only_once)
{
    if (!ltckpt_suicide_enabled)
	return 0;

    assert(0 != site_id && "Invalid site_id");
    assert(site_id <= ltckpt_suicide_last_site_id && "site_id out of bounds.");

    if (0 != ltckpt_suicide_board[site_id]) {
	ltckpt_printf("ltckpt_fi: suicide @ site: %ld\n", site_id);
	if (only_once) {
		ltckpt_suicide_board[site_id] = 0;
	}
	ltckpt_fi_suicide();
        return -1; // Error
    }
    return 0;	// No suicide since it's not enabled at the specified site
}
