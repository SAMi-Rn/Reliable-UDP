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
    STATE_CREATE_WINDOW,
    STATE_WAIT,
    STATE_CHECK_FLAGS,
    STATE_REMOVE_FROM_WINDOW,
    STATE_SEND_PACKET,
    STATE_CLEANUP,
    STATE_ERROR
};

static int parse_arguments_handler(struct fsm_context *context, struct fsm_error *err);
static int handle_arguments_handler(struct fsm_context *context, struct fsm_error *err);
static int convert_address_handler(struct fsm_context *context, struct fsm_error *err);
static int create_socket_handler(struct fsm_context *context, struct fsm_error *err);
static int bind_socket_handler(struct fsm_context *context, struct fsm_error *err);
static int create_window_handler(struct fsm_context *context, struct fsm_error *err);
static int wait_handler(struct fsm_context *context, struct fsm_error *err);
static int check_flags_handler(struct fsm_context *context, struct fsm_error *err);
static int remove_from_window_handler(struct fsm_context *context, struct fsm_error *err);
static int send_packet_handler(struct fsm_context *context, struct fsm_error *err);
static int cleanup_handler(struct fsm_context *context, struct fsm_error *err);
static int error_handler(struct fsm_context *context, struct fsm_error *err);

static void                     sigint_handler(int signum);
static int                      setup_signal_handler(struct fsm_error *err);

static volatile sig_atomic_t exit_flag = 0;

void *init_recv_fucntion(void *ptr);

typedef struct arguments
{
    int                     sockfd;
    uint8_t                 window_size;
    char                    *server_addr, *client_addr, *server_port_str, *client_port_str;
    in_port_t               server_port, client_port;
    struct sockaddr_storage server_addr_struct, client_addr_struct;
    struct sent_packet      *window;
    pthread_t               recv_thread;
} arguments;



int main(int argc, char **argv)
{
    struct fsm_error err;
    struct arguments args;
    struct fsm_context context = {
            .argc = argc,
            .argv = argv,
            .args = &args
    };

    static struct client_fsm_transition transitions[] = {
            {FSM_INIT,                  STATE_PARSE_ARGUMENTS,     parse_arguments_handler},
            {STATE_PARSE_ARGUMENTS,     STATE_HANDLE_ARGUMENTS,    handle_arguments_handler},
            {STATE_HANDLE_ARGUMENTS,    STATE_CONVERT_ADDRESS,     convert_address_handler},
            {STATE_CONVERT_ADDRESS,     STATE_CREATE_SOCKET,       create_socket_handler},
            {STATE_CREATE_SOCKET,       STATE_BIND_SOCKET,         bind_socket_handler},
            {STATE_BIND_SOCKET,         STATE_CREATE_WINDOW,       create_window_handler},
            {STATE_CREATE_WINDOW,       STATE_WAIT,                wait_handler},
            {STATE_WAIT,                STATE_CHECK_FLAGS,         check_flags_handler},
            {STATE_CHECK_FLAGS,         STATE_SEND_PACKET,         send_packet_handler},
            {STATE_CHECK_FLAGS,         STATE_REMOVE_FROM_WINDOW,  remove_from_window_handler},
            {STATE_REMOVE_FROM_WINDOW, STATE_WAIT,                 wait_handler},
            {STATE_SEND_PACKET,         STATE_WAIT,                wait_handler},
            {STATE_ERROR,               STATE_CLEANUP,             cleanup_handler},
            {STATE_PARSE_ARGUMENTS,     STATE_ERROR,               error_handler},
            {STATE_HANDLE_ARGUMENTS,    STATE_ERROR,               error_handler},
            {STATE_CONVERT_ADDRESS,     STATE_ERROR,               error_handler},
            {STATE_CREATE_SOCKET,       STATE_ERROR,               error_handler},
            {STATE_BIND_SOCKET,         STATE_ERROR,               error_handler},
            {STATE_CREATE_WINDOW,       STATE_ERROR,               error_handler},
            {STATE_WAIT,                STATE_ERROR,               error_handler},
            {STATE_CHECK_FLAGS,         STATE_ERROR,               error_handler},
            {STATE_REMOVE_FROM_WINDOW,  STATE_ERROR,               error_handler},
            {STATE_SEND_PACKET,         STATE_ERROR,               error_handler},
            {STATE_CLEANUP,             FSM_EXIT,                  NULL},
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
                        &ctx -> args -> window_size, err) != 0)

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
                         &ctx -> args -> server_port, err) != 0)
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
    if (convert_address(ctx -> args -> server_addr, &ctx -> args -> server_addr_struct, err) != 0)
    {
        return STATE_ERROR;
    }

    if (convert_address(ctx -> args -> client_addr, &ctx -> args -> client_addr_struct, err) != 0)
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
    ctx -> args -> sockfd = socket_create(ctx -> args -> client_addr_struct.ss_family, SOCK_DGRAM, 0, err);
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
    if (socket_bind(ctx -> args -> sockfd, &ctx -> args -> client_addr_struct, ctx -> args -> server_port, err))
    {
        return STATE_ERROR;
    }

    return STATE_CREATE_WINDOW;
}

static int create_window_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in create window", "STATE_CREATE_WINDOW");
    if (create_window(&ctx -> args -> window, ctx -> args -> window_size) != 0)
    {
        return STATE_ERROR;
    }

    for (int i = 0; i < 5; i++)
    {
        printf("in handler: %d: %d\n", i, ctx -> args -> window[i].is_packet_full);
    }

    return STATE_WAIT;
}

static int wait_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ssize_t result;

    ctx = context;
    result = 0;
    SET_TRACE(context, "", "STATE_LISTEN_SERVER");
    while (!exit_flag)
    {
        result = receive_packet(ctx->args->sockfd,
                                &ctx->args->window);
        if (result == -1)
        {
            return STATE_ERROR;
        }

        return STATE_CHECK_FLAGS;
    }

    return FSM_EXIT;
}

static int check_flags_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in connect socket", "STATE_CONNECT_SOCKET");
    while (!exit_flag)
    {
        char *buffer = NULL;

        read_keyboard(&buffer, 500);
        send_data_packet(ctx -> args -> sockfd, &ctx -> args -> server_addr_struct,
                         ctx -> args -> window, buffer);
    }
//    if (protocol_connect(ctx -> args -> sockfd, &ctx -> args -> server_addr_struct, ctx -> args -> server_port, ctx -> args -> window))
//    {
//        return STATE_ERROR;
//    }

    return STATE_SEND_PACKET;
}

static int remove_from_window_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in connect socket", "STATE_CONNECT_SOCKET");
    while (!exit_flag)
    {
        char *buffer = NULL;

        read_keyboard(&buffer, 500);
        send_data_packet(ctx -> args -> sockfd, &ctx -> args -> server_addr_struct,
                         ctx -> args -> window, buffer);
    }
//    if (protocol_connect(ctx -> args -> sockfd, &ctx -> args -> server_addr_struct, ctx -> args -> server_port, ctx -> args -> window))
//    {
//        return STATE_ERROR;
//    }

    return STATE_WAIT;
}
static int send_packet_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in connect socket", "STATE_CONNECT_SOCKET");
    while (!exit_flag)
    {
        char *buffer = NULL;

        read_keyboard(&buffer, 500);
        send_data_packet(ctx -> args -> sockfd, &ctx -> args -> server_addr_struct,
                         ctx -> args -> window, buffer);
    }
//    if (protocol_connect(ctx -> args -> sockfd, &ctx -> args -> server_addr_struct, ctx -> args -> server_port, ctx -> args -> window))
//    {
//        return STATE_ERROR;
//    }

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
//    globfree(&ctx -> args -> glob_result);
//    for (int i = 0; i < ctx -> args -> window_size; i++)
//    {
//        free(ctx -> args -> window + i * sizeof(sent_packet));
//    }

//    free(ctx -> args -> window);
    return FSM_EXIT;
}

static int error_handler(struct fsm_context *context, struct fsm_error *err)
{
    fprintf(stderr, "ERROR %s\nIn file %s in function %s on line %d\n",
            err -> err_msg, err -> file_name, err -> function_name, err -> error_line);

    return STATE_CLEANUP;
}

void *init_recv_fucntion(void *ptr)
{
    struct fsm_context *ctx = (struct fsm_context*) ptr;

    while (!exit_flag)
    {
        printf("thread that reads packets\n");
//        receive_packet(ctx -> args -> sockfd, &ctx -> args -> server_addr_struct, ctx -> args -> window);
    }

    return NULL;
}

