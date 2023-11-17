#include "fsm.h"
#include "server_config.h"
#include "command_line.h"
#include "proxy_config.h"
#include <pthread.h>

#define MAX_THREADS 25

enum application_states
{
    STATE_PARSE_ARGUMENTS = FSM_USER_START,
    STATE_HANDLE_ARGUMENTS,
    STATE_CONVERT_ADDRESS,
    STATE_CREATE_SOCKET,
    STATE_BIND_SOCKET,
    STATE_CREATE_WINDOW,
    STATE_CREATE_SERVER_THREAD,
    STATE_LISTEN_CLIENT,
    STATE_LISTEN_SERVER,
    STATE_CLIENT_CALCULATE_LOSSINESS,
    STATE_SERVER_CALCULATE_LOSSINESS,
    STATE_SEND_MESSAGE,
    STATE_CLIENT_DELAY_PACKET,
    STATE_SERVER_DELAY_PACKET,
    STATE_CLEANUP,
    STATE_ERROR
};

static int parse_arguments_handler(struct fsm_context *context, struct fsm_error *err);
static int handle_arguments_handler(struct fsm_context *context, struct fsm_error *err);
static int convert_address_handler(struct fsm_context *context, struct fsm_error *err);
static int create_socket_handler(struct fsm_context *context, struct fsm_error *err);
static int bind_socket_handler(struct fsm_context *context, struct fsm_error *err);
static int create_window_handler(struct fsm_context *context, struct fsm_error *err);
static int create_server_thread_handler(struct fsm_context *context, struct fsm_error *err);
static int listen_client_handler(struct fsm_context *context, struct fsm_error *err);
static int listen_server_handler(struct fsm_context *context, struct fsm_error *err);
static int send_message_handler(struct fsm_context *context, struct fsm_error *err);
static int calculate_client_lossiness_handler(struct fsm_context *context, struct fsm_error *err);
static int calculate_server_lossiness_handler(struct fsm_context *context, struct fsm_error *err);
static int client_delay_packet_handler(struct fsm_context *context, struct fsm_error *err);
static int server_delay_packet_handler(struct fsm_context *context, struct fsm_error *err);
static int cleanup_handler(struct fsm_context *context, struct fsm_error *err);
static int error_handler(struct fsm_context *context, struct fsm_error *err);

static void                     sigint_handler(int signum);
static int                      setup_signal_handler(struct fsm_error *err);

static volatile sig_atomic_t exit_flag = 0;

void *init_server_thread(void *ptr);
void *init_delay_thread(void *ptr);

typedef struct arguments
{
    int                     client_sockfd, server_sockfd, num_of_threads, client_delay_index;
    uint8_t                 window_size, client_first_empty_packet, server_first_empty_packet;
    char                    *server_addr, *client_addr, *server_port_str, *client_port_str;
    in_port_t               server_port, client_port;
    struct sockaddr_storage server_addr_struct, client_addr_struct;
    pthread_t               server_thread;
    pthread_t               *thread_pool;
    struct sent_packet      *server_window;
    struct sent_packet      *client_window;
    uint8_t                 client_delay_rate, server_delay_rate, client_drop_rate, server_drop_rate;
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
            {FSM_INIT,               STATE_PARSE_ARGUMENTS,     parse_arguments_handler},
            {STATE_PARSE_ARGUMENTS,  STATE_HANDLE_ARGUMENTS,    handle_arguments_handler},
            {STATE_HANDLE_ARGUMENTS, STATE_CONVERT_ADDRESS,     convert_address_handler},
            {STATE_CONVERT_ADDRESS,  STATE_CREATE_SOCKET,       create_socket_handler},
            {STATE_CREATE_SOCKET,    STATE_BIND_SOCKET,         bind_socket_handler},
            {STATE_LISTEN_CLIENT,    STATE_CLEANUP,      cleanup_handler},
            {STATE_ERROR,            STATE_CLEANUP,             cleanup_handler},
            {STATE_PARSE_ARGUMENTS,  STATE_ERROR,               error_handler},
            {STATE_HANDLE_ARGUMENTS, STATE_ERROR,               error_handler},
            {STATE_CONVERT_ADDRESS,  STATE_ERROR,               error_handler},
            {STATE_CREATE_SOCKET,    STATE_ERROR,              error_handler},
            {STATE_BIND_SOCKET,      STATE_ERROR,              error_handler},
            {STATE_CREATE_SERVER_THREAD,    STATE_ERROR,               error_handler},
            {STATE_LISTEN_CLIENT,    STATE_ERROR,               error_handler},
            {STATE_CLEANUP,          FSM_EXIT,                  NULL},
    };
    fsm_run(&context, &err, 0, 0 , transitions);

    return 0;
}

static int parse_arguments_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in parse arguments handler", "STATE_PARSE_ARGUMENTS");
    if (parse_arguments(ctx -> argc, ctx -> argv, &ctx -> args -> server_addr,
                        &ctx -> args -> client_addr, &ctx -> args -> server_port_str,
                        &ctx -> args -> client_port_str, err) != 0)
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
    ctx -> args -> client_sockfd = socket_create(ctx -> args -> client_addr_struct.ss_family, SOCK_DGRAM, 0, err);
    if (ctx -> args -> client_sockfd == -1)
    {
        return STATE_ERROR;
    }

    ctx -> args -> server_sockfd = socket_create(ctx -> args -> server_addr_struct.ss_family, SOCK_DGRAM, 0, err);
    if (ctx -> args -> server_sockfd == -1)
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
    if (socket_bind(ctx -> args -> client_sockfd, &ctx -> args -> client_addr_struct, ctx -> args -> client_port, err))
    {
        return STATE_ERROR;
    }

    if (socket_bind(ctx -> args -> server_sockfd, &ctx -> args -> server_addr_struct, ctx -> args -> server_port, err))
    {
        return STATE_ERROR;
    }

    return STATE_CREATE_SERVER_THREAD;
}

static int create_window_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in create window", "STATE_CREATE_WINDOW");
    if (create_window(&ctx -> args -> server_window, ctx -> args -> window_size, &ctx -> args -> server_first_empty_packet) != 0)
    {
        return STATE_ERROR;
    }

    if (create_window(&ctx -> args -> client_window, ctx -> args -> window_size, &ctx -> args -> client_first_empty_packet) != 0)
    {
        return STATE_ERROR;
    }

    return STATE_CREATE_SERVER_THREAD;
}
static int create_server_thread_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context      *ctx;
    int                     result;
    ctx = context;
    SET_TRACE(context, "in create receive thread", "STATE_CREATE_RECV_THREAD");
    result = pthread_create(&ctx->args->server_thread, NULL, init_server_thread, (void *) ctx);
    if (result < 0)
    {
        return STATE_ERROR;
    }

    return STATE_LISTEN_CLIENT;
}

static int listen_client_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ssize_t result;

    ctx = context;
    result = 0;
    SET_TRACE(context, "in connect socket", "STATE_CONNECT_SOCKET");
    while (!exit_flag)
    {
        result = receive_packet(ctx->args->client_sockfd, &ctx->args->client_window[ctx -> args -> client_first_empty_packet].pt);

        if (result == -1)
        {
            return STATE_ERROR;
        }

        return STATE_CLIENT_CALCULATE_LOSSINESS;
    }

    return STATE_CLEANUP;
}

static int listen_server_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ssize_t result;

    ctx = context;
    result = 0;
    SET_TRACE(context, "in connect socket", "STATE_CONNECT_SOCKET");
    while (!exit_flag)
    {
        result = receive_packet(ctx->args->server_sockfd, &ctx->args->server_pt);

        if (result == -1)
        {
            return STATE_ERROR;
        }

        return STATE_SERVER_CALCULATE_LOSSINESS;
    }

    return STATE_CLEANUP;
}

static int send_message_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in connect socket", "STATE_CONNECT_SOCKET");

    return STATE_CLEANUP;
}

static int calculate_client_lossiness_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context      *ctx;
    int                     result;
    ctx = context;
    SET_TRACE(context, "in create receive thread", "STATE_CREATE_RECV_THREAD");
    result = calculate_lossiness(client_drop_rate, client_delay_rate);
    if (result == DROP)
    {
        ctx -> args -> client_delay_index = ;
        return STATE_LISTEN_CLIENT;
    }
    else if (result == DELAY)
    {
        return STATE_CLIENT_DELAY_PACKET;
        delay_packet(&ctx -> args -> server_pt, client_delay_rate);
    }

    return STATE_LISTEN_CLIENT;
}

static int calculate_server_lossiness_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context      *ctx;
    int                     result;
    ctx = context;
    SET_TRACE(context, "in create receive thread", "STATE_CREATE_RECV_THREAD");
    result = calculate_lossiness(server_drop_rate, server_delay_rate);
    if (result == DROP)
    {
        return STATE_LISTEN_CLIENT;
    }
    else if (result == DELAY)
    {
        return STATE_SERVER_DELAY_PACKET;
        delay_packet(&ctx -> args -> server_pt, server_delay_rate);
    }

    return STATE_LISTEN_CLIENT;
}

static int client_delay_packet_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    pthread_t *temp_thread_pool;

    ctx = context;
    temp_thread_pool = ctx -> args -> thread_pool;
    SET_TRACE(context, "in cleanup handler", "STATE_CLEANUP");
    temp_thread_pool = (pthread_t *) realloc(temp_thread_pool, sizeof(pthread_t) * ctx -> args -> num_of_threads++);
    if (temp_thread_pool == NULL)
    {
        return STATE_ERROR;
    }

    ctx -> args -> thread_pool = temp_thread_pool;

    pthread_create(&ctx -> args -> thread_pool[ctx -> args -> num_of_threads], NULL, init_delay_thread, (void *) ctx);

    return STATE_LISTEN_CLIENT;
}

static int server_delay_packet_handler(struct fsm_context *context, struct fsm_error *err)
{

    struct fsm_context *ctx;
    pthread_t *temp_thread_pool;

    ctx = context;
    temp_thread_pool = ctx -> args -> thread_pool;
    SET_TRACE(context, "in cleanup handler", "STATE_CLEANUP");
    temp_thread_pool = (pthread_t *) realloc(temp_thread_pool, sizeof(pthread_t) * ctx -> args -> num_of_threads++);
    if (temp_thread_pool == NULL)
    {
        return STATE_ERROR;
    }

    ctx -> args -> thread_pool = temp_thread_pool;

    pthread_create(&ctx -> args -> thread_pool[ctx -> args -> num_of_threads], NULL, init_delay_thread, (void *) ctx);

    return STATE_LISTEN_SERVER;
}

static int cleanup_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in cleanup handler", "STATE_CLEANUP");
    pthread_join(ctx -> args -> server_thread, NULL);
//    if (socket_close(ctx -> args -> sockfd, err))
//    {
//        printf("close socket error");
//    }
//    globfree(&ctx -> args -> glob_result);
//    for (int i = 0; i < ctx -> args -> window_size; i++)
//    {
//        free(ctx -> args -> window + i * sizeof(sent_packet));
//    }

    return FSM_EXIT;
}

static int error_handler(struct fsm_context *context, struct fsm_error *err)
{
    fprintf(stderr, "ERROR %s\nIn file %s in function %s on line %d\n",
            err -> err_msg, err -> file_name, err -> function_name, err -> error_line);

    return STATE_CLEANUP;
}

void *init_server_thread(void *ptr)
{
    struct fsm_context *ctx = (struct fsm_context*) ptr;

    while (!exit_flag)
    {
        printf("thread that reads packets\n");
//        receive_packet(ctx -> args -> sockfd, &ctx -> args -> server_addr_struct, ctx -> args -> window);
    }

    return NULL;
}

void *init_delay_thread(void *ptr)
{
   struct fsm_context *ctx = (struct fsm_context *) ptr;

    delay_packet()
}
