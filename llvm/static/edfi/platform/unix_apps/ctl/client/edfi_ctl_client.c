#include <edfi/ctl/client.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#define EDFI_CTL_CLIENT_ERR (-1)

static struct sockaddr_un remote;
static int fd;

int edfi_ctl_client_init(int server_id)
{
    int len;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        return EDFI_CTL_CLIENT_ERR;
    }

    remote.sun_family = AF_UNIX;
    snprintf(remote.sun_path, UNIX_PATH_MAX, "#%s.%d", EDFI_CTL_PATH, server_id);
    len = SUN_LEN(&remote);
    remote.sun_path[0] = '\0';
    if (connect(fd, (struct sockaddr *)&remote, len) < 0) {
        return EDFI_CTL_CLIENT_ERR;
    }

    return 0;
}

int edfi_ctl_client_send(edfi_cmd_data_t *data)
{
    if (data->params_size > 0 && data->context_size <= 0) {
    	errno = EINVAL;
    	return EDFI_CTL_CLIENT_ERR;
    }
    if (send(fd, data, sizeof(edfi_cmd_data_t), 0) < 0) {
        return EDFI_CTL_CLIENT_ERR;
    }
    if (data->context_size > 0 && send(fd, data->context, data->context_size, 0) < 0) {
        return EDFI_CTL_CLIENT_ERR;
    }
    if (data->params_size > 0 && send(fd, data->params, data->params_size, 0) < 0) {
        return EDFI_CTL_CLIENT_ERR;
    }

    /* receive same message, to confirm that command has been executed. */
    if (recv(fd, data, sizeof(edfi_cmd_data_t), 0) < 0) {
        return EDFI_CTL_CLIENT_ERR;
    }
    if (data->status < 0) {
    	errno = -data->status;
    	return EDFI_CTL_CLIENT_ERR;
    }

    return 0;
}

int edfi_ctl_client_close(void)
{
    return close(fd);
}

