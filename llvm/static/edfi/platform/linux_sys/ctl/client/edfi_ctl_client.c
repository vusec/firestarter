#include <edfi/ctl/client.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#define EDFI_CTL_CLIENT_ERR (-1)
#define EDFI_PROC_DIR       "/proc"
#define EDFI_PROC_ENTRY     "edfi_ctl"
#define EDFI_BUFSIZE        16384

static int fd;

int edfi_ctl_client_init(int server_id)
{
    char path[UNIX_PATH_MAX];

    memset(path, 0, UNIX_PATH_MAX);
    snprintf(path, UNIX_PATH_MAX, "%s/%s", EDFI_PROC_DIR, EDFI_PROC_ENTRY);

    if (open(path, O_RDONLY | O_WRONLY) < 0) {
        perror("Failed during init open");
        return EDFI_CTL_CLIENT_ERR;
    }
    return 0;
}

int edfi_ctl_client_send(edfi_cmd_data_t *data)
{
    char send_buf[EDFI_BUFSIZE];
    char *next;
    int ret;

    if (data->params_size > 0 && data->context_size <= 0) {
        errno = EINVAL;
        return EDFI_CTL_CLIENT_ERR;
    }
    memset(send_buf, 0, EDFI_BUFSIZE);

    next = (char*)memcpy(send_buf, data, sizeof(edfi_cmd_data_t)) + sizeof(edfi_cmd_data_t);
    if (data->context_size > 0) 
        next = (char*)memcpy(next, data->context, data->context_size) + data->context_size;
    if (data->params_size > 0)
        next = (char*)memcpy(next, data->params, data->params_size) + data->params_size;

    if (write(fd, send_buf, (int)(next - send_buf)) < 0) {
        perror("Failed to send command");
        return EDFI_CTL_CLIENT_ERR;
    }

    /* receive same message, to confirm that command has been executed. */
    if (read(fd, send_buf, EDFI_BUFSIZE) < 0) {
        perror("Failed to get command confirmation");
        return EDFI_CTL_CLIENT_ERR;
    }
    if ((ret = atoi(send_buf)) < 0) {
    	errno = -ret;
    	return EDFI_CTL_CLIENT_ERR;
    }

    return 0;
}

int edfi_ctl_client_close(void)
{
    return close(fd);
}

