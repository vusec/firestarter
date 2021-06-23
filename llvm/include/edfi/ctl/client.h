#ifndef _EDFI_CTL_CLIENT_H
#define _EDFI_CTL_CLIENT_H

#include <edfi/common.h>

/* CTL client API. */
int edfi_ctl_client_init(int server_id);
int edfi_ctl_client_send(edfi_cmd_data_t *data);
int edfi_ctl_client_close(void);

#endif

