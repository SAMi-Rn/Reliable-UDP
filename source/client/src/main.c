#include "fsm.h"
#include "protocol.h"
#include "server_config.h"
#include "command_line.h"
#include <pthread.h>

#define TIMER_TIME 1

enum main_application_states
{
    STATE_PARSE_ARGUMENTS = FSM_USER_START,
    STATE_HANDLE_ARGUMENTS,
    STATE_CONVERT_ADDRESS,
    STATE_CREATE_SOCKET,
    STATE_BIND_SOCKET,
    STATE_CREATE_WINDOW,
    STATE_CREATE_RECV_THREAD,
    STATE_CONNECT_SOCKET,
    STATE_READ_FROM_KEYBOARD,
    STATE_CHECK_WINDOW,
    STATE_ADD_PACKET_TO_BUFFER,
    STATE_ADD_PACKET_TO_WINDOW,
    STATE_CHECK_WINDOW_THREAD,
    STATE_SEND_MESSAGE,
    STATE_CREATE_TIMER_THREAD,
    STATE_CLEANUP,
    STATE_ERROR
};

enum receiving_thread_application_states
{
    STATE_LISTEN = FSM_USER_START,
    STATE_CHECK_ACK_NUMBER,
    STATE_REMOVE_FROM_WINDOW,
    STATE_SEND_PACKET,
    STATE_TERMINATION
};

static int parse_arguments_handler(struct fsm_context *context, struct fsm_error *err);
static int handle_arguments_handler(struct fsm_context *context, struct fsm_error *err);
static int convert_address_handler(struct fsm_context *context, struct fsm_error *err);
static int create_socket_handler(struct fsm_context *context, struct fsm_error *err);
static int bind_socket_handler(struct fsm_context *context, struct fsm_error *err);
static int create_window_handler(struct fsm_context *context, struct fsm_error *err);
static int create_recv_thread_handler(struct fsm_context *context, struct fsm_error *err);
static int connect_socket_handler(struct fsm_context *context, struct fsm_error *err);
static int read_from_keyboard_handler(struct fsm_context *context, struct fsm_error *err);
static int check_window_handler(struct fsm_context *context, struct fsm_error *err);
static int add_packet_to_buffer_handler(struct fsm_context *context, struct fsm_error *err);
static int add_packet_to_window_handler(struct fsm_context *context, struct fsm_error *err);
static int check_window_thread_handler(struct fsm_context *context, struct fsm_error *err);
static int send_message_handler(struct fsm_context *context, struct fsm_error *err);
static int create_timer_thread_handler(struct fsm_context *context, struct fsm_error *err);
static int cleanup_handler(struct fsm_context *context, struct fsm_error *err);
static int error_handler(struct fsm_context *context, struct fsm_error *err);

static int listen_handler(struct fsm_context *context, struct fsm_error *err);
static int check_ack_number_handler(struct fsm_context *context, struct fsm_error *err);
static int remove_packet_from_window_handler(struct fsm_context *context, struct fsm_error *err);
static int send_packet_handler(struct fsm_context *context, struct fsm_error *err);
static int termination_handler(struct fsm_context *context, struct fsm_error *err);

static void                     sigint_handler(int signum);
static int                      setup_signal_handler(struct fsm_error *err);

static volatile sig_atomic_t exit_flag = 0;

void *init_recv_function(void *ptr);
void *init_timer_function(void *ptr);

typedef struct arguments
{
    int                     sockfd, num_of_threads;
    uint8_t                 window_size;
    char                    *server_addr, *client_addr, *server_port_str, *client_port_str;
    in_port_t               server_port, client_port;
    struct sockaddr_storage server_addr_struct, client_addr_struct;
    struct sent_packet      *window;
    pthread_t               recv_thread, *thread_pool;
    struct packet           temp_packet, temp_message;
    char                    *temp_buffer, *buffer;
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

    static struct fsm_transition transitions[] = {
            {FSM_INIT,                      STATE_PARSE_ARGUMENTS,     parse_arguments_handler},
            {STATE_PARSE_ARGUMENTS,         STATE_HANDLE_ARGUMENTS,    handle_arguments_handler},
            {STATE_HANDLE_ARGUMENTS,        STATE_CONVERT_ADDRESS,     convert_address_handler},
            {STATE_CONVERT_ADDRESS,         STATE_CREATE_SOCKET,       create_socket_handler},
            {STATE_CREATE_SOCKET,           STATE_BIND_SOCKET,         bind_socket_handler},
            {STATE_BIND_SOCKET,             STATE_CREATE_WINDOW,       create_window_handler},
            {STATE_CREATE_WINDOW,           STATE_CREATE_RECV_THREAD,   create_recv_thread_handler},
            {STATE_CREATE_RECV_THREAD,      STATE_CONNECT_SOCKET,       connect_socket_handler},
            {STATE_CONNECT_SOCKET,          STATE_READ_FROM_KEYBOARD,   read_from_keyboard_handler},
            {STATE_READ_FROM_KEYBOARD,      STATE_CHECK_WINDOW, check_window_handler},
            {STATE_CHECK_WINDOW,      STATE_ADD_PACKET_TO_WINDOW, add_packet_to_window_handler},
            {STATE_CHECK_WINDOW,      STATE_ADD_PACKET_TO_BUFFER, add_packet_to_buffer_handler},
            {STATE_ADD_PACKET_TO_WINDOW,    STATE_SEND_MESSAGE,          send_message_handler},
//            {STATE_ADD_PACKET_TO_WINDOW,    STATE_CHECK_WINDOW_THREAD,  check_window_thread_handler},
            {STATE_CHECK_WINDOW_THREAD,     STATE_READ_FROM_KEYBOARD,   read_from_keyboard_handler},
            {STATE_SEND_MESSAGE,             STATE_CREATE_TIMER_THREAD,   create_timer_thread_handler},
            {STATE_CREATE_TIMER_THREAD,             STATE_READ_FROM_KEYBOARD,   read_from_keyboard_handler},
            {STATE_READ_FROM_KEYBOARD,      STATE_CLEANUP,              cleanup_handler},
            {STATE_ERROR,                   STATE_CLEANUP,              cleanup_handler},
            {STATE_PARSE_ARGUMENTS,         STATE_ERROR,                error_handler},
            {STATE_HANDLE_ARGUMENTS,        STATE_ERROR,                error_handler},
            {STATE_CONVERT_ADDRESS,         STATE_ERROR,                error_handler},
            {STATE_CREATE_SOCKET,           STATE_ERROR,                error_handler},
            {STATE_BIND_SOCKET,             STATE_ERROR,                error_handler},
            {STATE_CREATE_WINDOW,           STATE_ERROR,                error_handler},
            {STATE_CREATE_RECV_THREAD,      STATE_ERROR,                error_handler},
            {STATE_CONNECT_SOCKET,          STATE_ERROR,                error_handler},
            {STATE_SEND_MESSAGE,             STATE_ERROR,                error_handler},
            {STATE_CLEANUP,                 FSM_EXIT,                   NULL},
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
                        &ctx -> args -> client_port_str, &ctx -> args -> window_size,
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
    if (socket_bind(ctx -> args -> sockfd, &ctx -> args -> client_addr_struct, ctx -> args -> client_port, err))
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

    return STATE_CREATE_RECV_THREAD;
}

static int create_recv_thread_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context      *ctx;
    int                     result;
    ctx = context;
    SET_TRACE(context, "in create receive thread", "STATE_CREATE_RECV_THREAD");
    result = pthread_create(&ctx->args->recv_thread, NULL, init_recv_function, (void *) ctx);
    if (result < 0)
    {
        return STATE_ERROR;
    }

    return STATE_CONNECT_SOCKET;
}

static int connect_socket_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in connect socket", "STATE_CONNECT_SOCKET");
    if (protocol_connect(ctx -> args -> sockfd, &ctx -> args -> server_addr_struct, ctx -> args -> server_port, ctx -> args -> window))
    {
        return STATE_ERROR;
    }

    return STATE_READ_FROM_KEYBOARD;
}

static int read_from_keyboard_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "", "STATE_READ_FROM_KEYBOARD");
    while (!exit_flag)
    {

        read_keyboard(&ctx -> args -> temp_buffer, 500);
        return STATE_CHECK_WINDOW;
//        send_data_packet(ctx -> args -> sockfd, &ctx -> args -> server_addr_struct,
//                         ctx -> args -> window, ctx -> args -> temp_buffer);
    }

    return STATE_CLEANUP;
}

static int check_window_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "", "STATE_CHECK_WINDOW");

    if (is_window_available)
    {
        return STATE_ADD_PACKET_TO_WINDOW;
    }

    return STATE_ADD_PACKET_TO_BUFFER;
}

static int add_packet_to_buffer_handler(struct fsm_context *context, struct fsm_error *err)
{

}

static int add_packet_to_window_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "", "STATE_ADD_PACKET_TO_WINDOW");

    create_data_packet(&ctx -> args -> temp_message, ctx -> args -> window, ctx -> args -> temp_buffer);
//    add_packet_to_window(ctx -> args -> window, &ctx -> args -> temp_message);

    return STATE_SEND_MESSAGE;
}

static int check_window_thread_handler(struct fsm_context *context, struct fsm_error *err)
{

}

static int send_message_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "", "STATE_SEND_PACKET");

    if (send_packet(ctx -> args -> sockfd, &ctx -> args -> server_addr_struct,
                    ctx -> args -> window, &ctx -> args -> temp_message) == -1)
    {
        return STATE_ERROR;
    }

    return STATE_CREATE_TIMER_THREAD;
}

static int create_timer_thread_handler(struct fsm_context *context, struct fsm_error *err)
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

    pthread_create(&ctx->args->thread_pool[ctx->args->num_of_threads], NULL, init_timer_function, (void *) ctx);


    return STATE_READ_FROM_KEYBOARD;
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

    free(ctx -> args -> window);
    return FSM_EXIT;
}

static int error_handler(struct fsm_context *context, struct fsm_error *err)
{
    fprintf(stderr, "ERROR %s\nIn file %s in function %s on line %d\n",
            err -> err_msg, err -> file_name, err -> function_name, err -> error_line);

    return STATE_CLEANUP;
}

static int listen_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ssize_t result;

    ctx = context;
    result = 0;
    SET_TRACE(context, "", "STATE_LISTEN_SERVER");
    while (!exit_flag)
    {
        result = receive_packet(ctx->args->sockfd, &ctx -> args -> temp_packet);
        if (result == -1)
        {
            return STATE_ERROR;
        }
        printf("Server packet with seq number: %u received\n", ctx -> args -> temp_packet.hd.seq_number);

        return STATE_CHECK_ACK_NUMBER;
    }

    return FSM_EXIT;
}

static int check_ack_number_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    int result;

    ctx = context;
    SET_TRACE(context, "", "STATE_CHECK_ACK_NUMBER");

    result = read_flags(ctx -> args -> temp_packet.hd.flags);

    if (result == RECV_ACK)
    {
        if (check_ack_number(ctx -> args -> window[first_unacked_packet].expected_ack_number,
                             ctx -> args -> temp_packet.hd.ack_number))
        {
            return STATE_REMOVE_FROM_WINDOW;
        }
    }
    else if (result == END_CONNECTION)
    {
        return STATE_TERMINATION;
    }

    return STATE_SEND_PACKET;
}

static int remove_packet_from_window_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "", "STATE_CHECK_ACK_NUMBER");

    remove_packet_from_window(ctx -> args -> window, &ctx -> args -> temp_packet);

    return STATE_LISTEN;
}

static int send_packet_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "", "STATE_SEND_PACKET");

    read_received_packet(ctx -> args -> sockfd, &ctx -> args -> server_addr_struct,
                         ctx -> args -> window, &ctx -> args -> temp_packet );

//    add_packet_to_window(ctx -> args -> window, &ctx -> args -> temp_packet);

    return STATE_LISTEN;
}

static int termination_handler(struct fsm_context *context, struct fsm_error *err)
{

}

void *init_recv_function(void *ptr)
{
    struct fsm_context *ctx = (struct fsm_context*) ptr;
    struct fsm_error err;

    static struct fsm_transition transitions[] = {
            {FSM_INIT,               STATE_LISTEN,             listen_handler},
            {STATE_LISTEN,           STATE_CHECK_ACK_NUMBER,   check_ack_number_handler},
            {STATE_CHECK_ACK_NUMBER, STATE_REMOVE_FROM_WINDOW, remove_packet_from_window_handler},
            {STATE_CHECK_ACK_NUMBER, STATE_SEND_PACKET,        send_packet_handler},
            {STATE_CHECK_ACK_NUMBER, STATE_TERMINATION,        send_packet_handler},
            {STATE_REMOVE_FROM_WINDOW, STATE_LISTEN,        listen_handler},
            {STATE_SEND_PACKET,      STATE_LISTEN,             listen_handler},
            {STATE_LISTEN,           STATE_ERROR,   error_handler},
            {STATE_LISTEN,           FSM_EXIT,                   NULL},
            {STATE_ERROR,            FSM_EXIT,                   NULL},
    };

    fsm_run(ctx, &err, 0, 0 , transitions);

    return NULL;
}

void *init_timer_function(void *ptr)
{
    struct fsm_context *ctx = (struct fsm_context*) ptr;
    int index;
    int counter;

    index = previous_index(ctx -> args -> window);
    counter = 0;

    while (ctx -> args -> window[index].is_packet_full)
    {
        sleep(TIMER_TIME);
        if (ctx -> args -> window[index].is_packet_full)
        {
            send_packet(ctx->args->sockfd, &ctx->args->server_addr_struct, ctx->args->window,
                        &ctx->args->window[index].pt);
            counter++;
        }
    }

    return NULL;
}
