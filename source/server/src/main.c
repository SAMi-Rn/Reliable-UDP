#include "fsm.h"
#include "protocol.h"
#include "server_config.h"
#include "command_line.h"
#include <pthread.h>

#define TIMER_TIME 1

enum application_states
{
    STATE_PARSE_ARGUMENTS = FSM_USER_START,
    STATE_HANDLE_ARGUMENTS,
    STATE_CONVERT_ADDRESS,
    STATE_CREATE_SOCKET,
    STATE_BIND_SOCKET,
    STATE_LISTEN,
    STATE_CREATE_GUI_THREAD,
    STATE_WAIT,
    STATE_COMPARE_CHECKSUM,
    STATE_SEND_SYN_ACK,
    STATE_CHECK_SEQ_NUMBER,
    STATE_CREATE_TIMER_THREAD,
    STATE_WAIT_FOR_ACK,
    STATE_SEND_PACKET,
    STATE_UPDATE_SEQ_NUMBER,
    STATE_CLEANUP,
    STATE_ERROR
};

enum gui_stats
{
    SENT_PACKET,
    RECEIVED_PACKET,
    RESENT_PACKET,
    DROPPED_CLIENT_PACKET,
    DELAYED_CLIENT_PACKET,
    DROPPED_SERVER_PACKET,
    DELAYED_SERVER_PACKET
};

static int parse_arguments_handler(struct fsm_context *context, struct fsm_error *err);
static int handle_arguments_handler(struct fsm_context *context, struct fsm_error *err);
static int convert_address_handler(struct fsm_context *context, struct fsm_error *err);
static int create_socket_handler(struct fsm_context *context, struct fsm_error *err);
static int bind_socket_handler(struct fsm_context *context, struct fsm_error *err);
static int listen_handler(struct fsm_context *context, struct fsm_error *err);
static int create_gui_thread_handler(struct fsm_context *context, struct fsm_error *err);
static int wait_handler(struct fsm_context *context, struct fsm_error *err);
static int compare_checksum_handler(struct fsm_context *context, struct fsm_error *err);
static int send_syn_ack_handler(struct fsm_context *context, struct fsm_error *err);
static int create_timer_handler(struct fsm_context *context, struct fsm_error *err);
static int check_seq_number_handler(struct fsm_context *context, struct fsm_error *err);
static int wait_for_ack_handler(struct fsm_context *context, struct fsm_error *err);
static int send_packet_handler(struct fsm_context *context, struct fsm_error *err);
static int update_seq_num_handler(struct fsm_context *context, struct fsm_error *err);
static int cleanup_handler(struct fsm_context *context, struct fsm_error *err);
static int error_handler(struct fsm_context *context, struct fsm_error *err);

static void                     sigint_handler(int signum);
static int                      setup_signal_handler(struct fsm_error *err);

static volatile sig_atomic_t exit_flag = 0;

void *init_timer_function(void *ptr);
void *init_gui_function(void *ptr);

typedef struct arguments
{
    int                     sockfd, num_of_threads, is_handshake_ack;
    int                     server_gui_fd, connected_gui_fd, is_connected_gui;
    char                    *server_addr, *client_addr, *server_port_str, *client_port_str;
    in_port_t               server_port, client_port;
    struct sockaddr_storage server_addr_struct, client_addr_struct, gui_addr_struct;
    struct packet           temp_packet;
    uint32_t                expected_seq_number;
    pthread_t               accept_gui_thread;
    pthread_t               *thread_pool;
} arguments;

int main(int argc, char **argv)
{
    struct fsm_error err;
    struct arguments args = {
            .expected_seq_number    = 0,
            .is_connected_gui       = 0
    };
    struct fsm_context context = {
            .argc = argc,
            .argv = argv,
            .args = &args
    };

    static struct client_fsm_transition transitions[] = {
            {FSM_INIT,                      STATE_PARSE_ARGUMENTS,      parse_arguments_handler},
            {STATE_PARSE_ARGUMENTS,         STATE_HANDLE_ARGUMENTS,     handle_arguments_handler},
            {STATE_HANDLE_ARGUMENTS,        STATE_CONVERT_ADDRESS,      convert_address_handler},
            {STATE_CONVERT_ADDRESS,         STATE_CREATE_SOCKET,        create_socket_handler},
            {STATE_CREATE_SOCKET,           STATE_BIND_SOCKET,          bind_socket_handler},
            {STATE_BIND_SOCKET,             STATE_LISTEN,               listen_handler},
            {STATE_LISTEN,                  STATE_CREATE_GUI_THREAD,    create_gui_thread_handler},
            {STATE_CREATE_GUI_THREAD,       STATE_WAIT,                 wait_handler},
            {STATE_WAIT,                    STATE_COMPARE_CHECKSUM,     compare_checksum_handler},
            {STATE_COMPARE_CHECKSUM,        STATE_CHECK_SEQ_NUMBER,     check_seq_number_handler},
            {STATE_COMPARE_CHECKSUM,        STATE_WAIT,                 wait_handler},
            {STATE_WAIT,                    STATE_CLEANUP,              cleanup_handler},
            {STATE_CHECK_SEQ_NUMBER,       STATE_SEND_PACKET,          send_packet_handler},
            {STATE_CHECK_SEQ_NUMBER,       STATE_SEND_SYN_ACK,         send_syn_ack_handler},
            {STATE_SEND_SYN_ACK,           STATE_UPDATE_SEQ_NUMBER,    update_seq_num_handler},
            {STATE_CHECK_SEQ_NUMBER,       STATE_WAIT,                 wait_handler },
            {STATE_SEND_PACKET,            STATE_UPDATE_SEQ_NUMBER,    update_seq_num_handler},
            {STATE_SEND_PACKET,            STATE_WAIT,                 wait_handler},
            {STATE_UPDATE_SEQ_NUMBER,      STATE_WAIT,                 wait_handler},
            {STATE_UPDATE_SEQ_NUMBER,      STATE_CREATE_TIMER_THREAD,  create_timer_handler},
            {STATE_CREATE_TIMER_THREAD,    STATE_WAIT_FOR_ACK,         wait_for_ack_handler},
            {STATE_WAIT_FOR_ACK,           STATE_WAIT,                 wait_handler},
            {STATE_WAIT_FOR_ACK,           STATE_CLEANUP,              cleanup_handler},
            {STATE_ERROR,                  STATE_CLEANUP,               cleanup_handler},
            {STATE_PARSE_ARGUMENTS,        STATE_ERROR,                 error_handler},
            {STATE_HANDLE_ARGUMENTS,       STATE_ERROR,                 error_handler},
            {STATE_CONVERT_ADDRESS,        STATE_ERROR,                 error_handler},
            {STATE_CREATE_SOCKET,          STATE_ERROR,                 error_handler},
            {STATE_BIND_SOCKET,            STATE_ERROR,                 error_handler},
            {STATE_LISTEN,                 STATE_ERROR,                 error_handler},
            {STATE_CREATE_GUI_THREAD,      STATE_ERROR,                 error_handler},
            {STATE_WAIT,                   STATE_ERROR,                 error_handler},
            {STATE_CREATE_TIMER_THREAD,    STATE_ERROR,                 error_handler},
            {STATE_WAIT_FOR_ACK,           STATE_ERROR,                 error_handler},
            {STATE_CLEANUP,                FSM_EXIT,                    NULL},
    };

    fsm_run(&context, &err, transitions);

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
    if (convert_address(ctx -> args -> server_addr, &ctx -> args -> server_addr_struct,
                        ctx -> args -> server_port, err) != 0)
    {
        return STATE_ERROR;
    }

    if (convert_address(ctx -> args -> server_addr, &ctx -> args -> gui_addr_struct,
                        61000, err) != 0)
    {
        return STATE_ERROR;
    }

    if (convert_address(ctx -> args -> client_addr, &ctx -> args -> client_addr_struct,
                        ctx -> args -> client_port, err) != 0)
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
    ctx -> args -> sockfd = socket_create(ctx -> args -> server_addr_struct.ss_family,
                                          SOCK_DGRAM, 0, err);
    if (ctx -> args -> sockfd == -1)
    {
        return STATE_ERROR;
    }

    ctx -> args -> server_gui_fd = socket_create(ctx -> args -> server_addr_struct.ss_family,
                                                 SOCK_STREAM, 0, err);
    if (ctx -> args -> server_gui_fd == -1)
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

    if (socket_bind(ctx -> args -> server_gui_fd, &ctx -> args -> gui_addr_struct, err))
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
    if (start_listening(ctx -> args -> server_gui_fd, SOMAXCONN, err))
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

        if (ctx -> args -> is_connected_gui)
        {
            send_stats_gui(ctx -> args -> connected_gui_fd, RECEIVED_PACKET);
        }

        return STATE_COMPARE_CHECKSUM;
    }

    return STATE_CLEANUP;
}

static int compare_checksum_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "", "STATE_COMPARE_CHECKSUM");

    if (compare_checksum(ctx -> args -> temp_packet.hd.checksum, ctx -> args -> temp_packet.data,
                         strlen(ctx -> args -> temp_packet.data)))
    {

        return STATE_CHECK_SEQ_NUMBER;
    }

    if (ctx -> args -> is_connected_gui)
    {
        send_stats_gui(ctx -> args -> connected_gui_fd, DROPPED_CLIENT_PACKET);
    }

    return STATE_WAIT;
}
static int check_seq_number_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "", "STATE_CHECK_SEQ_NUMBER");

    if (check_seq_number(ctx -> args -> temp_packet.hd.seq_number, ctx -> args -> expected_seq_number))
    {
        if (ctx -> args -> temp_packet.hd.flags == SYN)
        {
            return STATE_SEND_SYN_ACK;
        }

        return STATE_SEND_PACKET;
    }

    return STATE_WAIT;
}

static int send_syn_ack_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in connect socket", "STATE_START_HANDSHAKE");
    ctx -> args -> is_handshake_ack++;
    create_syn_ack_packet(ctx -> args -> sockfd, &ctx -> args -> client_addr_struct,
                         &ctx -> args -> temp_packet, err);

    send_packet(ctx -> args -> sockfd, &ctx -> args -> client_addr_struct,
                &ctx -> args -> temp_packet, err);

    if (ctx -> args -> is_connected_gui)
    {
        send_stats_gui(ctx -> args -> connected_gui_fd, SENT_PACKET);
    }

    return STATE_UPDATE_SEQ_NUMBER;
}

static int create_timer_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context  *ctx;
    pthread_t           *temp_thread_pool;

    ctx                 = context;
    temp_thread_pool    = ctx -> args -> thread_pool;
    SET_TRACE(context, "", "STATE_CREATE_TIMER_THREAD");

    ctx -> args -> num_of_threads++;

    temp_thread_pool    = (pthread_t *) realloc(temp_thread_pool,
                                             sizeof(pthread_t) * ctx -> args -> num_of_threads);
    if (temp_thread_pool == NULL)
    {
        return STATE_ERROR;
    }

    ctx -> args -> thread_pool = temp_thread_pool;

    pthread_create(&ctx->args->thread_pool[ctx->args->num_of_threads], NULL, init_timer_function, (void *) ctx);

    return STATE_WAIT_FOR_ACK;
}

static int wait_for_ack_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ssize_t result;

    ctx = context;
    SET_TRACE(context, "", "STATE_WAIT_FOR_ACK");
    while (!exit_flag)
    {
        result = receive_packet(ctx->args->sockfd, &ctx -> args -> temp_packet, err);

        if (result == -1)
        {
            return STATE_ERROR;
        }

        if (ctx -> args -> is_connected_gui)
        {
            send_stats_gui(ctx -> args -> connected_gui_fd, RECEIVED_PACKET);
        }

        if (ctx -> args -> temp_packet.hd.flags == ACK &&
            check_if_equal(ctx -> args -> temp_packet.hd.seq_number, ctx -> args -> expected_seq_number))
        {
            printf("received handshake ack\n");
            ctx -> args -> is_handshake_ack = 0;
            return STATE_WAIT;
        }

        if (check_if_less(ctx -> args -> temp_packet.hd.seq_number, ctx -> args -> expected_seq_number))
        {
            printf("received less seq number\n");
            read_received_packet(ctx -> args -> sockfd, &ctx -> args -> client_addr_struct,
                                 &ctx -> args -> temp_packet, err);
        }
        printf("received some garbage\n");
    }

    return STATE_CLEANUP;
}

static int send_packet_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "", "STATE_SEND_PACKET");

    read_received_packet(ctx -> args -> sockfd, &ctx -> args -> client_addr_struct,
                             &ctx -> args -> temp_packet, err);

    if (ctx -> args -> is_connected_gui)
    {
        send_stats_gui(ctx -> args -> connected_gui_fd, SENT_PACKET);
    }

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

    if (ctx -> args -> temp_packet.hd.flags == SYNACK)
    {
        ctx -> args -> expected_seq_number = update_expected_seq_number(ctx -> args -> temp_packet.hd.ack_number, 0);
        return STATE_CREATE_TIMER_THREAD;
    }

    ctx -> args -> expected_seq_number = update_expected_seq_number(ctx -> args -> temp_packet.hd.seq_number,
                                                                    strlen(ctx -> args -> temp_packet.data));

    return STATE_WAIT;
}
static int cleanup_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context  *ctx;
    ctx                 = context;
    SET_TRACE(context, "in cleanup handler", "STATE_CLEANUP");

    pthread_join(ctx -> args -> accept_gui_thread, NULL);
    if (socket_close(ctx -> args -> sockfd, err))
    {
        printf("close socket error");
    }

    if (socket_close(ctx -> args -> server_gui_fd, err))
    {
        printf("close socket error");
    }

    if (socket_close(ctx -> args -> connected_gui_fd, err))
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

void *init_timer_function(void *ptr)
{
    struct fsm_context  *ctx;
    struct fsm_error    err;

    packet              packet_to_send;
    int                 counter;

    ctx                     = (struct fsm_context*) ptr;
    packet_to_send          = ctx -> args -> temp_packet;
    counter                 = 0;

    while (!exit_flag || ctx -> args -> is_handshake_ack)
    {
        sleep(TIMER_TIME);
        if (ctx -> args -> is_handshake_ack)
        {
            send_packet(ctx->args->sockfd, &ctx->args->server_addr_struct,
                        &packet_to_send, &err);

            if (ctx -> args -> is_connected_gui)
            {
                send_stats_gui(ctx -> args -> connected_gui_fd, RESENT_PACKET);
            }

            counter++;
        }
    }

    pthread_exit(NULL);
}

void *init_gui_function(void *ptr)
{
    struct fsm_context *ctx = (struct fsm_context*) ptr;
    struct fsm_error err;

    while(!exit_flag)
    {
        ctx->args->connected_gui_fd = socket_accept_connection(ctx->args->server_gui_fd, &err);
        ctx->args->is_connected_gui++;
    }

    return NULL;
}
