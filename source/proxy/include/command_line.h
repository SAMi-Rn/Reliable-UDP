#ifndef CLIENT_COMMAND_LINE_H
#define CLIENT_COMMAND_LINE_H

#include <glob.h>
#include <netinet/in.h>
#include "fsm.h"

int                 parse_arguments(int argc, char *argv[], char **server_addr,
                                    char **client_addr, char **server_port_str,
                                    char **client_port_str, struct fsm_error *err);
int                 handle_arguments(const char *binary_name, const char *server_addr,
                                     const char *client_addr, const char *server_port_str,
                                     const char *client_port_str, in_port_t *server_port,
                                     in_port_t *client_port, struct fsm_error *err);
void                usage(const char *program_name);
int                 parse_in_port_t(const char *binary_name, const char *str, in_port_t *port, struct fsm_error *err);

#endif //CLIENT_COMMAND_LINE_H
