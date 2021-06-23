#include <edfi/ctl/client.h>

#include <minix/com.h>
#include <minix/ipc.h>
#include <minix/safecopies.h>

#define EDFI_CTL_CLIENT_ERR (-1)

static endpoint_t endpoint;

int edfi_ctl_client_init(int server_id)
{
    endpoint = (endpoint_t) server_id;

    return 0;
}

int edfi_ctl_client_send(edfi_cmd_data_t *data)
{
    message m;
    char *buff, *ptr;
    size_t buff_size;
    cp_grant_id_t gid;
    int r;

    if (data->params_size > 0 && data->context_size <= 0) {
        errno = EINVAL;
        return EDFI_CTL_CLIENT_ERR;
    }

    /* Allocate single data buffer. */
    buff_size = sizeof(edfi_cmd_data_t) + sizeof(data->context_size) +
        sizeof(data->params_size);
    buff = (char*) malloc(buff_size);
    assert(buff);
    ptr = buff;
    memcpy(ptr, data, sizeof(edfi_cmd_data_t));
    ptr += sizeof(edfi_cmd_data_t);
    memcpy(ptr, data->context, sizeof(data->context_size));
    ptr += sizeof(data->context_size);
    memcpy(ptr, data->params, sizeof(data->params_size));

    /* Grant read access to it. */
    gid = cpf_grant_direct(endpoint, (vir_bytes) buff,
        sizeof(buff_size), CPF_READ);
    assert(gid != GRANT_INVALID);

    /* Send message. */
    m.m_type = COMMON_REQ_FI_CTL;
    m.m_lsys_fi_ctl.gid = gid;
    m.m_lsys_fi_ctl.size = buff_size;
    r = ipc_sendrec(endpoint, &m);
    assert(r == 0);
    free(buff);
    if (m.m_lsys_fi_reply.status != 0) {
        return EDFI_CTL_CLIENT_ERR;
    }

    return 0;
}

int edfi_ctl_client_close(void)
{
    return 0;
}
