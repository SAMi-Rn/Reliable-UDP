#include <stdio.h>
#include "fsm.h"
#include "packet_config.h"
#include "server_config.h"
#include "command_line.h"

enum application_states
{
    STATE_PARSE_ARGUMENTS = FSM_USER_START,
    STATE_HANDLE_ARGUMENTS,
    STATE_CONVERT_ADDRESS,
    STATE_CREATE_SOCKET,
    STATE_CREATE_WINDOW,
    STATE_CLEANUP,
    STATE_ERROR
};

static int parse_arguments_handler(struct fsm_context *context, struct fsm_error *err);
static int handle_arguments_handler(struct fsm_context *context, struct fsm_error *err);
static int convert_address_handler(struct fsm_context *context, struct fsm_error *err);
static int create_socket_handler(struct fsm_context *context, struct fsm_error *err);
static int create_window_handler(struct fsm_context *context, struct fsm_error *err);
static int cleanup_handler(struct fsm_context *context, struct fsm_error *err);
static int error_handler(struct fsm_context *context, struct fsm_error *err);

typedef struct arguments
{
    int                     sockfd;
    uint8_t                 window_size;
    char                    *address, *port_str;
    in_port_t               port;
    struct sockaddr_storage addr;
    glob_t                  glob_result;
    struct sent_packet      *window;
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
            {STATE_CREATE_SOCKET,    STATE_CREATE_WINDOW,       create_window_handler},
            {STATE_CREATE_WINDOW,    STATE_CLEANUP,             cleanup_handler},
            {STATE_ERROR,            STATE_CLEANUP,             cleanup_handler},
            {STATE_PARSE_ARGUMENTS,  STATE_ERROR,               error_handler},
            {STATE_HANDLE_ARGUMENTS, STATE_ERROR,               error_handler},
            {STATE_CONVERT_ADDRESS,  STATE_ERROR,               error_handler},
            {STATE_CREATE_SOCKET,    STATE_ERROR,              create_window_handler},
            {STATE_CREATE_SOCKET,    STATE_ERROR,               error_handler},
            {STATE_CLEANUP,          FSM_EXIT,                  NULL},
    };
    fsm_run(&context, &err, 0, 0 , transitions);

//    struct sockaddr_in client_addr;
//    struct sockaddr_in server_addr;
//    server_addr.sin_family = AF_INET;
//    server_addr.sin_port = htons(60000);
//    server_addr.sin_addr.s_addr = inet_addr("192.168.1.83");
//
//    int sd = socket_create(AF_INET, SOCK_DGRAM, 0, &err);
//
//
//    struct packet pt;
//
//    char temp[510] = "fjfksfjwe";
//    strcpy(pt.data, temp);
//    pt.hd.acknowledgment_number = 471264781;
//    pt.hd.sequence_number = 28141084;
//    pt.hd.window_size = 10;
//    pt.hd.flags = 2;
//    gettimeofday(&pt.hd.tv, NULL);
//
//    struct sent_packet pp;
//    struct sent_packet *ddp;
//    if (create_window(&ddp, 3) != 0)
//    {
//        printf("error");
//        exit(EXIT_FAILURE);
//    }
//    window_empty(ddp, 3);
//    printf("is_empty: %d", can_send_packet);
//    ddp[0].pt.hd.acknowledgment_number = 2312313;
//    ddp[1].pt.hd.acknowledgment_number = 34141;
//    ddp[2].pt.hd.acknowledgment_number = 442342;
//    window_empty(&ddp, 3);
//    printf("is_empty: %d", can_send_packet);

//    printf("1 packet ack: %u", ddp[0].pt.hd.acknowledgment_number);
//    printf("2 packet ack: %u", ddp[1].pt.hd.acknowledgment_number);
//    printf("3 packet ack: %u\n", ddp[2].pt.hd.acknowledgment_number);


//    sendto(sd, &pt, sizeof(pt), 0, (struct sockaddr*) &server_addr, sizeof(server_addr));
//    while (1)
//    {
//        recvfrom(sd, &pt, sizeof(pt), 0, (struct sockaddr*)  &client_addr, (socklen_t *) sizeof(client_addr));
//    }

//    free(ddp);
    return 0;
}

static int parse_arguments_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in parse arguments handler", "STATE_PARSE_ARGUMENTS");
    if (parse_arguments(ctx -> argc, ctx -> argv, &ctx -> args -> glob_result, &ctx -> args -> address, &ctx -> args -> port_str, &ctx -> args -> window_size, err) != 0)
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
    if (handle_arguments(ctx -> argv[0], ctx -> args -> address, ctx -> args -> port_str, &ctx -> args -> port, err) != 0)
    {
        return STATE_ERROR;
    }

    return STATE_CONVERT_ADDRESS;
}

static int convert_address_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in convert address", "STATE_CONVERT_ADDRESS");
    if (convert_address(ctx -> args -> address, &ctx -> args -> addr, err) != 0)
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
    ctx -> args -> sockfd = socket_create(ctx -> args -> addr.ss_family, SOCK_DGRAM, 0, err);
    if (ctx -> args -> sockfd == -1)
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

    printf("first: %u\nunacked: %u\n", first_empty_packet, first_unacked_packet);
    return STATE_CLEANUP;
}
static int cleanup_handler(struct fsm_context *context, struct fsm_error *err)
{
    struct fsm_context *ctx;
    ctx = context;
    SET_TRACE(context, "in cleanup handler", "STATE_CLEANUP");
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
