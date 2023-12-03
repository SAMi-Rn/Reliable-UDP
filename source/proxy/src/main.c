#include "fsm.h"
#include "server_config.h"
#include "command_line.h"
#include "proxy_config.h"
#include <pthread.h>

#define PROXY_CLIENT_PORT 8000
#define PROXY_SERVER_PORT 8050
#define GUI_PORT 61060
#define DELAY_TIME 5

enum main_application_states
{
    STATE_PARSE_ARGUMENTS = FSM_USER_START,
    STATE_HANDLE_ARGUMENTS,
    STATE_CONVERT_ADDRESS,
    STATE_CREATE_SOCKET,
    STATE_BIND_SOCKET,
    STATE_LISTEN,
    STATE_CREATE_GUI_THREAD,
    STATE_CREATE_WINDOW,
    STATE_CREATE_SERVER_THREAD,
    STATE_CREATE_KEYBOARD_THREAD,
    STATE_LISTEN_CLIENT,
    STATE_CLIENT_CALCULATE_LOSSINESS,
    STATE_CLIENT_DROP,
    STATE_CLIENT_DELAY_PACKET,
    STATE_SEND_CLIENT_PACKET,
    STATE_CLEANUP,
    STATE_ERROR
};

enum server_thread_states
{
    STATE_LISTEN_SERVER = FSM_USER_START,
    STATE_SERVER_CALCULATE_LOSSINESS,
    STATE_SERVER_DELAY_PACKET,
    STATE_SERVER_DROP,
    STATE_SEND_SERVER_PACKET
};

enum keyboard_thread_states
{
    STATE_READ_FROM_KEYBOARD = FSM_USER_START,
};

enum gui_stats
{
    SENT_PACKET,
    RECEIVED_PACKET,
    RESENT_PACKET,
    DROPPED_CLIENT_PACKET,
    DELAYED_CLIENT_PACKET,
    DROPPED_SERVER_PACKET,
    DELAYED_SERVER_PACKET,
};

static int parse_arguments_handler(struct fsm_context *context, struct fsm_error *err);
static int handle_arguments_handler(struct fsm_context *context, struct fsm_error *err);
static int convert_address_handler(struct fsm_context *context, struct fsm_error *err);
static int create_socket_handler(struct fsm_context *context, struct fsm_error *err);
static int bind_socket_handler(struct fsm_context *context, struct fsm_error *err);
static int listen_handler(struct fsm_context *context, struct fsm_error *err);
static int create_gui_thread_handler(struct fsm_context *context, struct fsm_error *err);
static int create_server_thread_handler(struct fsm_context *context, struct fsm_error *err);
static int create_keyboard_thread_handler(struct fsm_context *context, struct fsm_error *err);
static int listen_client_handler(struct fsm_context *context, struct fsm_error *err);
static int calculate_client_lossiness_handler(struct fsm_context *context, struct fsm_error *err);
static int client_drop_packet_handler(struct fsm_context *context, struct fsm_error *err);
static int client_delay_packet_handler(struct fsm_context *context, struct fsm_error *err);
static int send_client_packet_handler(struct fsm_context *context, struct fsm_error *err);
static int cleanup_handler(struct fsm_context *context, struct fsm_error *err);
static int error_handler(struct fsm_context *context, struct fsm_error *err);

static int listen_server_handler(struct fsm_context *context, struct fsm_error *err);
static int calculate_server_lossiness_handler(struct fsm_context *context, struct fsm_error *err);
static int server_drop_packet_handler(struct fsm_context *context, struct fsm_error *err);
static int server_delay_packet_handler(struct fsm_context *context, struct fsm_error *err);
static int send_server_packet_handler(struct fsm_context *context, struct fsm_error *err);

static int read_from_keyboard_handler(struct fsm_context *context, struct fsm_error *err);

static void                     sigint_handler(int signum);
static int                      setup_signal_handler(struct fsm_error *err);

static volatile sig_atomic_t exit_flag = 0;

void *init_server_thread(void *ptr);
void *init_keyboard_thread(void *ptr);
void *init_client_delay_thread(void *ptr);
void *init_server_delay_thread(void *ptr);
void *init_gui_function(void *ptr);

typedef struct arguments
{
    int                     client_sockfd, server_sockfd, num_of_threads;
    int                     proxy_gui_fd, connected_gui_fd, is_connected_gui;
    uint8_t                 window_size, client_first_empty_packet, server_first_empty_packet;
    char                    *server_addr, *client_addr, *server_port_str, *client_port_str, *proxy_addr;
    in_port_t               server_port, client_port;
    struct sockaddr_storage server_addr_struct, client_addr_struct, proxy_addr_struct, gui_addr_struct;
    pthread_t               server_thread, keyboard_thread, accept_gui_thread;
    pthread_t               *thread_pool;
    struct packet           server_packet, client_packet;
    uint8_t                 client_delay_rate, server_delay_rate, client_drop_rate, server_drop_rate;
} arguments;

int main(int argc, char **argv)
{

    struct fsm_error err;
    struct arguments args = {
            .client_delay_rate  = 101,
            .client_drop_rate   = 101,
            .server_delay_rate  = 101,
            .server_drop_rate   = 101,
            .num_of_threads     = 0,
            .is_connected_gui   = 0
    };

    struct fsm_context context = {
            .argc = argc,
            .argv = argv,
            .args = &args
    };

    static struct fsm_transition transitions[] = {
            {FSM_INIT,                          STATE_PARSE_ARGUMENTS,          parse_arguments_handler},
            {STATE_PARSE_ARGUMENTS,             STATE_HANDLE_ARGUMENTS,         handle_arguments_handler},
            {STATE_HANDLE_ARGUMENTS,            STATE_CONVERT_ADDRESS,          convert_address_handler},
            {STATE_CONVERT_ADDRESS,             STATE_CREATE_SOCKET,            create_socket_handler},
            {STATE_CREATE_SOCKET,               STATE_BIND_SOCKET,              bind_socket_handler},
            {STATE_BIND_SOCKET,                 STATE_LISTEN,                   listen_handler},
            {STATE_LISTEN,                      STATE_CREATE_GUI_THREAD,        create_gui_thread_handler},
            {STATE_CREATE_GUI_THREAD,           STATE_CREATE_SERVER_THREAD,     create_server_thread_handler},
            {STATE_CREATE_SERVER_THREAD,        STATE_CREATE_KEYBOARD_THREAD,   create_keyboard_thread_handler},
            {STATE_CREATE_KEYBOARD_THREAD,      STATE_LISTEN_CLIENT,            listen_client_handler},
            {STATE_LISTEN_CLIENT,               STATE_CLIENT_CALCULATE_LOSSINESS,calculate_client_lossiness_handler},
            {STATE_LISTEN_CLIENT,               STATE_CLEANUP,                  cleanup_handler},
            {STATE_CLIENT_CALCULATE_LOSSINESS,  STATE_CLIENT_DROP,               client_drop_packet_handler},
            {STATE_CLIENT_CALCULATE_LOSSINESS,  STATE_CLIENT_DELAY_PACKET,       client_delay_packet_handler},
            {STATE_CLIENT_CALCULATE_LOSSINESS,  STATE_SEND_CLIENT_PACKET,        send_client_packet_handler},
            {STATE_CLIENT_DROP,                 STATE_LISTEN_CLIENT,             listen_client_handler},
            {STATE_CLIENT_DELAY_PACKET,         STATE_LISTEN_CLIENT,             listen_client_handler},
            {STATE_SEND_CLIENT_PACKET,          STATE_LISTEN_CLIENT,             listen_client_handler},
            {STATE_ERROR,                       STATE_CLEANUP,                   cleanup_handler},
            {STATE_PARSE_ARGUMENTS,             STATE_ERROR,                     error_handler},
            {STATE_HANDLE_ARGUMENTS,            STATE_ERROR,                     error_handler},
            {STATE_CONVERT_ADDRESS,             STATE_ERROR,                     error_handler},
            {STATE_CREATE_SOCKET,               STATE_ERROR,                     error_handler},
            {STATE_BIND_SOCKET,                 STATE_ERROR,                     error_handler},
            {STATE_CREATE_WINDOW,               STATE_ERROR,                     error_handler},
            {STATE_CREATE_SERVER_THREAD,        STATE_ERROR,                     error_handler},
            {STATE_CREATE_KEYBOARD_THREAD,      STATE_ERROR,                     error_handler},
            {STATE_LISTEN_CLIENT,               STATE_ERROR,                     error_handler},
            {STATE_CLIENT_DROP,                 STATE_ERROR,                     error_handler},
            {STATE_SEND_CLIENT_PACKET,          STATE_ERROR,                     error_handler},
            {STATE_CLEANUP,                     FSM_EXIT,                        NULL},
    };
    srand(time(NULL));

    fsm_run(&context, &err, 0, 0 , transitions);

    return 0;
}

static int parse_arguments_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in parse arguments handler", "STATE_PARSE_ARGUMENTS");
    if (parse_arguments(ctx -> argc, ctx -> argv, &ctx -> args -> server_addr,
                        &ctx -> args -> client_addr, &ctx -> args -> proxy_addr,
                        &ctx -> args -> server_port_str, &ctx -> args -> client_port_str,
                        &ctx -> args -> client_delay_rate, &ctx -> args -> client_drop_rate,
                        &ctx -> args -> server_delay_rate, &ctx -> args -> server_drop_rate,
                        err) == -1)
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
                         ctx -> args -> proxy_addr, ctx -> args -> client_port_str,
                         &ctx -> args -> server_port, &ctx -> args -> client_port,
                         ctx -> args -> client_delay_rate, ctx -> args -> client_drop_rate,
                         ctx -> args -> server_delay_rate, ctx -> args -> server_drop_rate, err) != 0)
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
    if (convert_address(ctx -> args -> proxy_addr, &ctx -> args -> proxy_addr_struct, 5, err) != 0)
    {
        return STATE_ERROR;
    }

    if (convert_address(ctx -> args -> server_addr, &ctx -> args -> server_addr_struct, ctx -> args -> server_port, err) != 0)
    {
        return STATE_ERROR;
    }

    if (convert_address(ctx -> args -> client_addr, &ctx -> args -> client_addr_struct, ctx -> args -> client_port, err) != 0)
    {
        return STATE_ERROR;
    }

    if (convert_address(ctx -> args -> proxy_addr, &ctx -> args -> gui_addr_struct, GUI_PORT, err) != 0)
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
    ctx -> args -> client_sockfd = socket_create(ctx -> args -> proxy_addr_struct.ss_family, SOCK_DGRAM, 0, err);
    if (ctx -> args -> client_sockfd == -1)
    {
        return STATE_ERROR;
    }

    ctx -> args -> server_sockfd = socket_create(ctx -> args -> proxy_addr_struct.ss_family, SOCK_DGRAM, 0, err);
    if (ctx -> args -> server_sockfd == -1)
    {
        return STATE_ERROR;
    }

    ctx -> args -> proxy_gui_fd = socket_create(ctx -> args -> proxy_addr_struct.ss_family, SOCK_STREAM, 0, err);
    if (ctx -> args -> proxy_gui_fd == -1)
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
    if (socket_bind(ctx -> args -> client_sockfd, &ctx -> args -> proxy_addr_struct, PROXY_CLIENT_PORT, err))
    {
        return STATE_ERROR;
    }

    if (socket_bind(ctx -> args -> server_sockfd, &ctx -> args -> proxy_addr_struct,PROXY_SERVER_PORT , err))
    {
        return STATE_ERROR;
    }

    if (socket_bind(ctx -> args -> proxy_gui_fd, &ctx -> args -> gui_addr_struct, GUI_PORT, err))
    {
        return STATE_ERROR;
    }

    return STATE_LISTEN;
}

static int listen_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in start listening", "STATE_START_LISTENING");
    if (start_listening(ctx -> args -> proxy_gui_fd, SOMAXCONN, err))
    {
        return STATE_ERROR;
    }

    return STATE_CREATE_GUI_THREAD;
}

static int create_gui_thread_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context      *ctx;
    int                     result;
    ctx = context;
    SET_TRACE(context, "", "STATE_CREATE_GUI_THREAD");
    result = pthread_create(&ctx -> args -> accept_gui_thread, NULL, init_gui_function,
                            (void *) ctx);
    if (result < 0)
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

    return STATE_CREATE_KEYBOARD_THREAD;
}

static int create_keyboard_thread_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    int result;
    ctx = context;
    SET_TRACE(context, "in create keyboard thread", "STATE_CREATE_KEYBOARD_THREAD");
    result = pthread_create(&ctx->args->keyboard_thread, NULL, init_keyboard_thread, (void *) ctx);
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
    SET_TRACE(context, "in connect socket", "STATE_LISTEN_CLIENT");
    while (!exit_flag)
    {
        result = receive_packet(ctx->args->client_sockfd, &ctx->args->client_packet);

        if (result == -1)
        {
            return STATE_ERROR;
        }
        printf("Client packet with seq number: %u ack number: %u flags: %u received\n",
               ctx -> args -> client_packet.hd.seq_number, ctx -> args -> client_packet.hd.ack_number,
               ctx -> args -> client_packet.hd.flags);

        if (ctx -> args -> is_connected_gui)
        {
            send_stats_gui(ctx -> args -> connected_gui_fd, RECEIVED_PACKET);
        }

        return STATE_CLIENT_CALCULATE_LOSSINESS;
    }

    return STATE_CLEANUP;
}

static int calculate_client_lossiness_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context      *ctx;
    int                     result;
    ctx = context;
    SET_TRACE(context, "", "STATE_CLIENT_CALCULATE_LOSSINESS");
    result = calculate_lossiness(ctx -> args -> client_drop_rate, ctx -> args -> client_delay_rate);
    if (result == DROP)
    {
        return STATE_CLIENT_DROP;
    }
    else if (result == DELAY)
    {
        return STATE_CLIENT_DELAY_PACKET;
    }

    return STATE_SEND_CLIENT_PACKET;
}

static int client_drop_packet_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "", "STATE_CLIENT_DROP");

    printf("Client packet with seq number: %u ack number: %u flags: %u dropped\n",
           ctx -> args -> client_packet.hd.seq_number, ctx -> args -> client_packet.hd.ack_number,
           ctx -> args -> client_packet.hd.flags);

    if (ctx -> args -> is_connected_gui)
    {
        send_stats_gui(ctx -> args -> connected_gui_fd, DROPPED_CLIENT_PACKET);
    }

    return STATE_LISTEN_CLIENT;
}

static int client_delay_packet_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    pthread_t *temp_thread_pool;

    ctx = context;
    temp_thread_pool = ctx -> args -> thread_pool;
    SET_TRACE(context, "", "STATE_CLIENT_DELAY_PACKET");
    printf("Client packet with seq number: %u ack number: %u flags: %u delayed\n", ctx -> args -> client_packet.hd.seq_number, ctx -> args -> client_packet.hd.ack_number, ctx -> args -> client_packet.hd.flags);

    ctx -> args -> num_of_threads++;
    temp_thread_pool = (pthread_t *) realloc(temp_thread_pool, sizeof(pthread_t) * ctx -> args -> num_of_threads);
    if (temp_thread_pool == NULL)
    {
        return STATE_ERROR;
    }

    ctx -> args -> thread_pool = temp_thread_pool;

    pthread_create(&ctx->args->thread_pool[ctx->args->num_of_threads], NULL, init_client_delay_thread, (void *) ctx);

    if (ctx -> args -> is_connected_gui)
    {
        send_stats_gui(ctx -> args -> connected_gui_fd, DELAYED_CLIENT_PACKET);
    }

    return STATE_LISTEN_CLIENT;
}

static int send_client_packet_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    int result;

    ctx = context;

    SET_TRACE(context, "", "STATE_SEND_CLIENT_PACKET");
    result = send_packet(ctx -> args -> server_sockfd, &ctx -> args -> client_packet,
                         &ctx -> args -> server_addr_struct);
    if (result < 0)
    {
        return STATE_ERROR;
    }

    printf("Client packet with seq number: %u ack number: %u flags: %u sent\n",
           ctx -> args -> client_packet.hd.seq_number, ctx -> args -> client_packet.hd.ack_number,
           ctx -> args -> client_packet.hd.flags);

    if (ctx -> args -> is_connected_gui)
    {
        send_stats_gui(ctx -> args -> connected_gui_fd, SENT_PACKET);
    }

    return STATE_LISTEN_CLIENT;
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

static int listen_server_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ssize_t result;

    ctx = context;
    result = 0;
    SET_TRACE(context, "", "STATE_LISTEN_SERVER");
    while (!exit_flag)
    {
        result = receive_packet(ctx->args->server_sockfd, &ctx -> args -> server_packet);
        if (result == -1)
        {
            return STATE_ERROR;
        }

        printf("Server packet with seq number: %u ack number: %u flags: %u received\n",
               ctx -> args -> server_packet.hd.seq_number, ctx -> args -> server_packet.hd.ack_number,
               ctx -> args -> server_packet.hd.flags);

        if (ctx -> args -> is_connected_gui)
        {
            send_stats_gui(ctx -> args -> connected_gui_fd, RECEIVED_PACKET);
        }

        return STATE_SERVER_CALCULATE_LOSSINESS;
    }

    return FSM_EXIT;
}

static int calculate_server_lossiness_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context      *ctx;
    int                     result;
    ctx = context;
    SET_TRACE(context, "", "STATE_SERVER_CALCULATE_LOSSINESS");
    result = calculate_lossiness(ctx -> args -> server_drop_rate, ctx -> args -> server_delay_rate);
    if (result == DROP)
    {
        return STATE_SERVER_DROP;
    }
    else if (result == DELAY)
    {
        return STATE_SERVER_DELAY_PACKET;
    }

    return STATE_SEND_SERVER_PACKET;
}

static int server_drop_packet_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;

    ctx = context;
    SET_TRACE(context, "", "STATE_SERVER_DROP");

    if (ctx -> args -> is_connected_gui)
    {
        send_stats_gui(ctx -> args -> connected_gui_fd, DROPPED_SERVER_PACKET);
    }

    printf("Server packet with seq number: %u ack number: %u flags: %u dropped\n",
           ctx -> args -> server_packet.hd.seq_number, ctx -> args -> server_packet.hd.ack_number,
           ctx -> args -> server_packet.hd.flags);

    return STATE_LISTEN_SERVER;
}

static int server_delay_packet_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    pthread_t *temp_thread_pool;

    ctx = context;
    temp_thread_pool = ctx -> args -> thread_pool;
    SET_TRACE(context, "", "STATE_SERVER_DELAY_PACKET");
    ctx -> args -> num_of_threads++;
    temp_thread_pool = (pthread_t *) realloc(temp_thread_pool, sizeof(pthread_t) * ctx -> args -> num_of_threads);
    if (temp_thread_pool == NULL)
    {
        return STATE_ERROR;
    }

    ctx -> args -> thread_pool = temp_thread_pool;

    pthread_create(&ctx->args->thread_pool[ctx->args->num_of_threads], NULL, init_server_delay_thread, (void *) ctx);

    if (ctx -> args -> is_connected_gui)
    {
        send_stats_gui(ctx -> args -> connected_gui_fd, DELAYED_SERVER_PACKET);
    }

    return STATE_LISTEN_SERVER;
}

static int send_server_packet_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    int result;

    ctx = context;
    SET_TRACE(context, "", "STATE_SEND_SERVER_PACKET");

    result = send_packet(ctx -> args -> client_sockfd, &ctx -> args -> server_packet,
                         &ctx -> args -> client_addr_struct);
    if (result < 0)
    {
        return STATE_ERROR;
    }

    if (ctx -> args -> is_connected_gui)
    {
        send_stats_gui(ctx -> args -> connected_gui_fd, SENT_PACKET);
    }

    printf("Server packet with seq number: %u ack number: %u flags: %u sent\n",
           ctx -> args -> server_packet.hd.seq_number, ctx -> args -> server_packet.hd.ack_number,
           ctx -> args -> server_packet.hd.flags);

    return STATE_LISTEN_SERVER;
}

static int error_handler(struct fsm_context *context, struct fsm_error *err)
{
    fprintf(stderr, "ERROR %s\nIn file %s in function %s on line %d\n",
            err -> err_msg, err -> file_name, err -> function_name, err -> error_line);

    return STATE_CLEANUP;
}

static int read_from_keyboard_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "", "STATE_READ_FROM_KEYBOARD");

    while (!exit_flag)
    {
        read_keyboard(&ctx->args->client_drop_rate,&ctx->args->client_delay_rate,
                      &ctx->args->server_drop_rate, &ctx->args->server_delay_rate );
    }

    return FSM_EXIT;
}

void *init_server_thread(void *ptr)
{
    struct fsm_context *ctx = (struct fsm_context*) ptr;
    struct fsm_error err;

    static struct fsm_transition transitions[] = {
            {FSM_INIT,                          STATE_LISTEN_SERVER,                listen_server_handler},
            {STATE_LISTEN_SERVER,               STATE_SERVER_CALCULATE_LOSSINESS,   calculate_server_lossiness_handler},
            {STATE_SERVER_CALCULATE_LOSSINESS,  STATE_SERVER_DROP,                  server_drop_packet_handler},
            {STATE_SERVER_CALCULATE_LOSSINESS,  STATE_SERVER_DELAY_PACKET,          server_delay_packet_handler},
            {STATE_SERVER_CALCULATE_LOSSINESS,  STATE_SEND_SERVER_PACKET,           send_server_packet_handler},
            {STATE_SERVER_DROP,                 STATE_LISTEN_SERVER,                listen_server_handler},
            {STATE_SERVER_DELAY_PACKET,         STATE_LISTEN_SERVER,                listen_server_handler},
            {STATE_SEND_SERVER_PACKET,          STATE_LISTEN_SERVER,                listen_server_handler},
            {STATE_LISTEN_SERVER,               FSM_EXIT,                           NULL},
            {STATE_ERROR,                       FSM_EXIT,                           NULL},
    };

    fsm_run(ctx, &err, 0, 0 , transitions);

    return NULL;
}

void *init_keyboard_thread(void *ptr)
{
    struct fsm_context *ctx = (struct fsm_context*) ptr;
    struct fsm_error err;

    static struct fsm_transition transitions[] = {
            {FSM_INIT,                          STATE_READ_FROM_KEYBOARD,   read_from_keyboard_handler},
            {STATE_READ_FROM_KEYBOARD,          FSM_EXIT,                   NULL},
            {STATE_ERROR,                       FSM_EXIT,                   NULL},
    };

    fsm_run(ctx, &err, 0, 0 , transitions);

    return NULL;
}

void *init_client_delay_thread(void *ptr)
{
    struct fsm_context   *ctx;
    struct packet        *temp_packet;

    ctx                  = (struct fsm_context *) ptr;
    temp_packet          = &ctx -> args -> client_packet;

    printf("Client packet with seq number: %u ack number: %u flags: %u delayed for %u seconds\n",
           temp_packet -> hd.seq_number, temp_packet -> hd.ack_number, temp_packet -> hd.flags, DELAY_TIME);

    delay_packet(DELAY_TIME);
    send_packet(ctx -> args -> server_sockfd, temp_packet, &ctx -> args -> server_addr_struct);

    if (ctx -> args -> is_connected_gui)
    {
        send_stats_gui(ctx -> args -> connected_gui_fd, SENT_PACKET);
    }

    printf("Client packet with seq number: %u ack number: %u flags: %u sent\n",
           temp_packet -> hd.seq_number, temp_packet -> hd.ack_number, temp_packet -> hd.flags);

    return NULL;
}

void *init_server_delay_thread(void *ptr)
{
    struct fsm_context   *ctx;
    struct packet        *temp_packet;

    ctx                  = (struct fsm_context *) ptr;
    temp_packet          = &ctx -> args -> server_packet;

    printf("Server packet with seq number: %u ack number: %u flags: %u delayed for %u seconds\n",
           temp_packet -> hd.seq_number, temp_packet -> hd.ack_number, temp_packet -> hd.flags, DELAY_TIME);

    delay_packet(DELAY_TIME);
    send_packet(ctx -> args -> client_sockfd, temp_packet, &ctx -> args -> client_addr_struct);

    if (ctx -> args -> is_connected_gui)
    {
        send_stats_gui(ctx -> args -> connected_gui_fd, SENT_PACKET);
    }

    printf("Server packet with seq number: %u ack number: %u flags: %u sent\n",
           temp_packet -> hd.seq_number, temp_packet -> hd.ack_number, temp_packet -> hd.flags);

    return NULL;
}

void *init_gui_function(void *ptr)
{
    struct fsm_context *ctx = (struct fsm_context*) ptr;
    struct fsm_error err;

    while(!exit_flag)
    {
        ctx->args->connected_gui_fd = socket_accept_connection(ctx->args->proxy_gui_fd, &err);
        ctx->args->is_connected_gui++;
    }

    return NULL;
}
