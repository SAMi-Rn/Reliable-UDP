#ifndef CLIENT_COMMAND_LINE_H
#define CLIENT_COMMAND_LINE_H

#include <glob.h>
#include <netinet/in.h>
#include "fsm.h"

int                 parse_arguments(int argc, char *argv[], glob_t *glob_result, char **server_addr, char **client_addr, char **port_str, uint8_t *window_size, struct fsm_error *err);
void                usage(const char *program_name);
//void                get_files(const char *pattern, glob_t *glob_result, struct fsm_error *err);
int                 handle_arguments(const char *binary_name, const char *server_addr, const char *client_addr, const char *port_str, in_port_t *port, struct fsm_error *err);
int                 parse_in_port_t(const char *binary_name, const char *str, in_port_t *port, struct fsm_error *err);

#endif //CLIENT_COMMAND_LINE_H
