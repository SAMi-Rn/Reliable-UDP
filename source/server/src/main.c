#include "fsm.h"
#include "protocol.h"
#include "server_config.h"
#include "command_line.h"
#include <pthread.h>

enum application_states
{
    STATE_PARSE_ARGUMENTS = FSM_USER_START,
    STATE_HANDLE_ARGUMENTS,
    STATE_CONVERT_ADDRESS,
    STATE_CREATE_SOCKET,
    STATE_BIND_SOCKET,
    STATE_WAIT,
    STATE_CHECK_SEQ_NUMBER,
    STATE_CHECK_FLAGS,
    STATE_SEND_PACKET,
    STATE_UPDATE_SEQ_NUMBER,
    STATE_CLEANUP,
    STATE_ERROR
};

static int parse_arguments_handler(struct fsm_context *context, struct fsm_error *err);
static int handle_arguments_handler(struct fsm_context *context, struct fsm_error *err);
static int convert_address_handler(struct fsm_context *context, struct fsm_error *err);
static int create_socket_handler(struct fsm_context *context, struct fsm_error *err);
static int bind_socket_handler(struct fsm_context *context, struct fsm_error *err);
static int wait_handler(struct fsm_context *context, struct fsm_error *err);
static int check_seq_number_handler(struct fsm_context *context, struct fsm_error *err);
static int send_packet_handler(struct fsm_context *context, struct fsm_error *err);
static int update_seq_num_handler(struct fsm_context *context, struct fsm_error *err);
static int cleanup_handler(struct fsm_context *context, struct fsm_error *err);
static int error_handler(struct fsm_context *context, struct fsm_error *err);

static void                     sigint_handler(int signum);
static int                      setup_signal_handler(struct fsm_error *err);

static volatile sig_atomic_t exit_flag = 0;

typedef struct arguments
{
    int                     sockfd;
    char                    *server_addr, *client_addr, *server_port_str, *client_port_str;
    in_port_t               server_port, client_port;
    struct sockaddr_storage server_addr_struct, client_addr_struct;
    struct packet           temp_packet;
    uint32_t                expected_seq_number;
    pthread_t               recv_thread;
} arguments;



int main(int argc, char **argv)
{
    struct fsm_error err;
    struct arguments args = {
            .expected_seq_number = 0
    };
    struct fsm_context context = {
            .argc = argc,
            .argv = argv,
            .args = &args
    };

    static struct client_fsm_transition transitions[] = {
            {FSM_INIT,                  STATE_PARSE_ARGUMENTS,      parse_arguments_handler},
            {STATE_PARSE_ARGUMENTS,     STATE_HANDLE_ARGUMENTS,     handle_arguments_handler},
            {STATE_HANDLE_ARGUMENTS,    STATE_CONVERT_ADDRESS,      convert_address_handler},
            {STATE_CONVERT_ADDRESS,     STATE_CREATE_SOCKET,        create_socket_handler},
            {STATE_CREATE_SOCKET,       STATE_BIND_SOCKET,          bind_socket_handler},
            {STATE_BIND_SOCKET,         STATE_WAIT,                 wait_handler},
            {STATE_WAIT,                STATE_CHECK_SEQ_NUMBER,     check_seq_number_handler},
            {STATE_CHECK_SEQ_NUMBER,    STATE_SEND_PACKET,          send_packet_handler},
            {STATE_CHECK_SEQ_NUMBER,    STATE_WAIT,                 wait_handler },
            {STATE_CHECK_FLAGS,         STATE_SEND_PACKET,          send_packet_handler},
            {STATE_SEND_PACKET,        STATE_UPDATE_SEQ_NUMBER,     update_seq_num_handler},
            {STATE_SEND_PACKET,        STATE_WAIT,     wait_handler},
            {STATE_UPDATE_SEQ_NUMBER,  STATE_WAIT,                  wait_handler},
            {STATE_ERROR,              STATE_CLEANUP,               cleanup_handler},
            {STATE_PARSE_ARGUMENTS,    STATE_ERROR,                 error_handler},
            {STATE_HANDLE_ARGUMENTS,   STATE_ERROR,                 error_handler},
            {STATE_CONVERT_ADDRESS,    STATE_ERROR,                 error_handler},
            {STATE_CREATE_SOCKET,      STATE_ERROR,                 error_handler},
            {STATE_BIND_SOCKET,        STATE_ERROR,                 error_handler},
            {STATE_WAIT,               STATE_ERROR,                 error_handler},
            {STATE_CHECK_SEQ_NUMBER,   STATE_ERROR,                 error_handler},
            {STATE_CHECK_FLAGS,        STATE_ERROR,                 error_handler},
            {STATE_SEND_PACKET,        STATE_ERROR,                 error_handler},
            {STATE_CLEANUP,            FSM_EXIT,                    NULL},
    };

    fsm_run(&context, &err, 0, 0 , transitions);

    return 0;
}

static int parse_arguments_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in parse arguments handler", "STATE_PARSE_ARGUMENTS");
    if (parse_arguments(ctx -> argc, ctx -> argv,
                        &ctx -> args -> server_addr, &ctx -> args -> client_addr,
                        &ctx -> args -> server_port_str, &ctx -> args -> client_port_str,
                        err) != 0)
    {
        return STATE_ERROR;
    }

    return STATE_HANDLE_ARGUMENTS;
}
static int handle_arguments_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in handle arguments", "STATE_HANDLE_ARGUMENTS");
    if (handle_arguments(ctx -> argv[0], ctx -> args -> server_addr,
                         ctx -> args -> client_addr, ctx -> args -> server_port_str,
                         ctx -> args -> client_port_str, &ctx -> args -> server_port,
                         &ctx -> args -> client_port, err) != 0)
    {
        return STATE_ERROR;
    }

    return STATE_CONVERT_ADDRESS;
}

static int convert_address_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in convert server_addr", "STATE_CONVERT_ADDRESS");
    if (convert_address(ctx -> args -> server_addr, &ctx -> args -> server_addr_struct, ctx -> args -> server_port, err) != 0)
    {
        return STATE_ERROR;
    }

    if (convert_address(ctx -> args -> client_addr, &ctx -> args -> client_addr_struct, ctx -> args -> client_port, err) != 0)
    {
        return STATE_ERROR;
    }

    return STATE_CREATE_SOCKET;
}

static int create_socket_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in create socket", "STATE_CREATE_SOCKET");
    ctx -> args -> sockfd = socket_create(ctx -> args -> server_addr_struct.ss_family, SOCK_DGRAM, 0, err);
    if (ctx -> args -> sockfd == -1)
    {
        return STATE_ERROR;
    }

    return STATE_BIND_SOCKET;
}

static int bind_socket_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in bind socket", "STATE_BIND_SOCKET");
    if (socket_bind(ctx -> args -> sockfd, &ctx -> args -> server_addr_struct, err))
    {
        return STATE_ERROR;
    }

    return STATE_WAIT;
}

static int wait_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ssize_t result;

    ctx = context;
    SET_TRACE(context, "", "STATE_LISTEN_SERVER");
    while (!exit_flag)
    {
        result = receive_packet(ctx->args->sockfd, &ctx -> args -> temp_packet, err);
        if (result == -1)
        {
            return STATE_ERROR;
        }

        return STATE_CHECK_SEQ_NUMBER;
    }

    return FSM_EXIT;
}

static int check_seq_number_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "", "STATE_CHECK_SEQ_NUMBER");

    if (check_seq_number(ctx -> args -> temp_packet.hd.seq_number, ctx -> args -> expected_seq_number))
    {
        return STATE_SEND_PACKET;
    }

    return STATE_WAIT;
}

static int send_packet_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "", "STATE_SEND_PACKET");

    read_received_packet(ctx -> args -> sockfd, &ctx -> args -> client_addr_struct,
                             &ctx -> args -> temp_packet, err);

    if (check_if_less(ctx -> args -> temp_packet.hd.seq_number, ctx -> args -> expected_seq_number))
    {
        return STATE_WAIT;
    }

    printf("%s\n", ctx -> args -> temp_packet.data);
    return STATE_UPDATE_SEQ_NUMBER;
}

static int update_seq_num_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "", "STATE_UPDATE_SEQ_NUMBER");

    if (ctx -> args -> temp_packet.hd.flags == SYN)
    {
        ctx -> args -> expected_seq_number = update_expected_seq_number(ctx -> args -> temp_packet.hd.seq_number, 1);
        return STATE_WAIT;
    }

    ctx -> args -> expected_seq_number = update_expected_seq_number(ctx -> args -> temp_packet.hd.seq_number, strlen(ctx -> args -> temp_packet.data));
//    printf("expected: %u\n", ctx -> args -> expected_seq_number );
    return STATE_WAIT;
}
static int cleanup_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in cleanup handler", "STATE_CLEANUP");
    pthread_join(ctx -> args -> recv_thread, NULL);
    if (socket_close(ctx -> args -> sockfd, err))
    {
        printf("close socket error");
    }

    return FSM_EXIT;
}

static int error_handler(struct fsm_context *context, struct fsm_error *err)
{
    fprintf(stderr, "ERROR %s\nIn file %s in function %s on line %d\n",
            err -> err_msg, err -> file_name, err -> function_name, err -> error_line);

    return STATE_CLEANUP;
}
