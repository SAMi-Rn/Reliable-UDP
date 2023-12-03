#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glob.h>
#include <stdbool.h>
#include <errno.h>
#include <inttypes.h>
#include "command_line.h"

int                 parse_arguments(int argc, char *argv[], char **server_addr,
                                    char **client_addr, char **proxy_addr, char **server_port_str,
                                    char **client_port_str, uint8_t *client_delay_rate,
                                    uint8_t *client_drop_rate, uint8_t *server_delay_rate,
                                    uint8_t *server_drop_rate, struct fsm_error *err)
{
    int opt;
    bool C_flag, S_flag, s_flag, c_flag, D_flag, d_flag, P_flag, L_flag, l_flag, w_flag;

    opterr = 0;
    C_flag = 0;
    c_flag = 0;
    S_flag = 0;
    s_flag = 0;
    D_flag = 0;
    d_flag = 0;
    P_flag = 0;
    L_flag = 0;
    l_flag = 0;

    while ((opt = getopt(argc, argv, "C:c:S:s:P:D:d:L:l:h")) != -1)
    {
        switch (opt)
        {
            case 'C':
            {
                if (C_flag)
                {
                    char message[40];

                    snprintf(message, sizeof(message), "option '-C' can only be passed in once.");
                    usage(argv[0]);
                    SET_ERROR(err, message);

                    return -1;
                }

                C_flag++;
                *client_addr = optarg;
                break;
            }
            case 'c':
            {
                if (c_flag)
                {
                    char message[40];

                    snprintf(message, sizeof(message), "option '-c' can only be passed in once.");
                    usage(argv[0]);
                    SET_ERROR(err, message);

                    return -1;
                }

                c_flag++;
                *client_port_str = optarg;
                break;
            }
            case 'S':
            {
                if (S_flag)
                {
                    char message[40];

                    snprintf(message, sizeof(message), "option '-S' can only be passed in once.");
                    usage(argv[0]);
                    SET_ERROR(err, message);

                    return -1;
                }

                S_flag++;
                *server_addr = optarg;
                break;
            }
            case 's':
            {
                if (s_flag)
                {
                    char message[40];

                    snprintf(message, sizeof(message), "option '-s' can only be passed in once.");
                    usage(argv[0]);
                    SET_ERROR(err, message);

                    return -1;
                }

                s_flag++;
                *server_port_str = optarg;
                break;
            }
            case 'P':
            {
                if (P_flag)
                {
                    char message[40];

                    snprintf(message, sizeof(message), "option '-P' can only be passed in once.");
                    usage(argv[0]);
                    SET_ERROR(err, message);

                    return -1;
                }

                P_flag++;
                *proxy_addr = optarg;
                break;
            }
            case 'D':
            {
                if (D_flag)
                {
                    char message[40];

                    snprintf(message, sizeof(message), "option '-D' can only be passed in once.");
                    usage(argv[0]);
                    SET_ERROR(err, message);

                    return -1;
                }

                D_flag++;

                char *temp;
                temp = optarg;

                if (convert_to_int(argv[0], temp, client_drop_rate, err) == -1)
                {
                    return -1;
                }
                break;
            }
            case 'd':
            {
                if (d_flag)
                {
                    char message[40];

                    snprintf(message, sizeof(message), "option '-d' can only be passed in once.");
                    usage(argv[0]);
                    SET_ERROR(err, message);

                    return -1;
                }

                d_flag++;

                char *temp;
                temp = optarg;

                if (convert_to_int(argv[0], temp, server_drop_rate, err) == -1)
                {
                    return -1;
                }
                break;
            }

            case 'L':
            {
                if (L_flag)
                {
                    char message[40];

                    snprintf(message, sizeof(message), "option '-L' can only be passed in once.");
                    usage(argv[0]);
                    SET_ERROR(err, message);

                    return -1;
                }

                L_flag++;

                char *temp;
                temp = optarg;

                if (convert_to_int(argv[0], temp, client_delay_rate, err) == -1)
                {
                    return -1;
                }
                break;
            }
            case 'l':
            {
                if (l_flag)
                {
                    char message[40];

                    snprintf(message, sizeof(message), "option '-l' can only be passed in once.");
                    usage(argv[0]);
                    SET_ERROR(err, message);

                    return -1;
                }

                l_flag++;

                char *temp;
                temp = optarg;

                if (convert_to_int(argv[0], temp, server_delay_rate, err) == -1)
                {
                    return -1;
                }
                break;
            }
            case 'h':
            {
                usage(argv[0]);
                SET_ERROR(err, "user called for help");

                return -1;
            }
            case '?':
            {
                char message[24];

                snprintf(message, sizeof(message), "Unknown option '-%c'.", optopt);
                usage(argv[0]);
                SET_ERROR(err, message);

                return -1;
            }
            default:
            {
                usage(argv[0]);
            }
        }
    }

    return 0;
}

void usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s [-C] <value> [-c] <value> [-S] <value> [-s] <value> [-P] <value>\n", program_name);
    fprintf(stderr, "[-w] <value> [-D] <value>[-d] <value> [-L] <value> [-l] <value> [-h]\n");
    fputs("Options:\n", stderr);
    fputs("  -h                     Display this help message\n", stderr);
    fputs("  -C <value>             Option 'C' (required) with value, Sets the IP client_addr\n", stderr);
    fputs("  -c <value>             Option 'c' (required) with value, Sets the client port\n", stderr);
    fputs("  -S <value>             Option 'S' (required) with value, Sets the IP server_addr\n", stderr);
    fputs("  -s <value>             Option 's' (required) with value, Sets the server port\n", stderr);
    fputs("  -P <value>             Option 'P' (required) with value, Sets the IP proxy_addr\n", stderr);
    fputs("  -D <value>             Option 'D' (required) with value, Sets the client drop rate\n", stderr);
    fputs("  -d <value>             Option 'd' (required) with value, Sets the server drop rate\n", stderr);
    fputs("  -L <value>             Option 'L' (required) with value, Sets the client delay rate\n", stderr);
    fputs("  -l <value>             Option 'l' (required) with value, Sets the server delay rate\n", stderr);
}

int handle_arguments(const char *binary_name, const char *server_addr,
                     const char *client_addr, const char *server_port_str,
                     const char *proxy_addr,  const char *client_port_str,
                     in_port_t *server_port, in_port_t *client_port,
                     uint8_t client_delay_rate, uint8_t client_drop_rate,
                     uint8_t server_delay_rate, uint8_t server_drop_rate,
                     struct fsm_error *err)
{
    if(client_addr == NULL)
    {
        SET_ERROR(err, "The client_addr is required.");
        usage(binary_name);

        return -1;
    }

    if(server_addr == NULL)
    {
        SET_ERROR(err, "The server_addr is required.");
        usage(binary_name);

        return -1;
    }

    if(server_port_str == NULL)
    {
        SET_ERROR(err, "The server port is required.");
        usage(binary_name);

        return -1;
    }

    if(client_port_str == NULL)
    {
        SET_ERROR(err, "The client port is required.");
        usage(binary_name);

        return -1;
    }

    if(proxy_addr == NULL)
    {
        SET_ERROR(err, "The proxy_addr is required.");
        usage(binary_name);

        return -1;
    }

    if(client_drop_rate > 100)
    {
        SET_ERROR(err, "The client drop rate is required.");
        usage(binary_name);

        return -1;
    }

    if(client_delay_rate > 100)
    {
        SET_ERROR(err, "The client delay rate is required.");
        usage(binary_name);

        return -1;
    }

    if(server_drop_rate > 100)
    {
        SET_ERROR(err, "The server drop rate is required.");
        usage(binary_name);

        return -1;
    }

    if(server_delay_rate > 100)
    {
        SET_ERROR(err, "The server delay rate is required.");
        usage(binary_name);

        return -1;
    }

    if (parse_in_port_t(binary_name, server_port_str, server_port, err) == -1)
    {
        return -1;
    }

    if (parse_in_port_t(binary_name, client_port_str, client_port, err) == -1)
    {
        return -1;
    }

    return 0;
}

int parse_in_port_t(const char *binary_name, const char *str, in_port_t *port, struct fsm_error *err)
{
    char            *endptr;
    uintmax_t       parsed_value;

    errno           = 0;
    parsed_value    = strtoumax(str, &endptr, 10);

    if(errno != 0)
    {
        SET_ERROR(err, strerror(errno));

        return -1;
    }

    // Check if there are any non-numeric characters in the input string
    if(*endptr != '\0')
    {
        SET_ERROR(err, "Invalid characters in input.");
        usage(binary_name);

        return -1;
    }

    // Check if the parsed value is within the valid range for in_port_t
    if(parsed_value > UINT16_MAX)
    {
        SET_ERROR(err, "in_port_t value out of range.");
        usage(binary_name);

        return -1;
    }

    *port = (in_port_t)parsed_value;
    return 0;
}

int convert_to_int(const char *binary_name, char *string, uint8_t *value, struct fsm_error *err)
{
    char            *endptr;
    uintmax_t       parsed_value;

    errno = 0;
    parsed_value = strtoumax(string, &endptr, 10);

    if (errno != 0)
    {
        SET_ERROR(err, strerror(errno));

        return -1;
    }

    if(*endptr != '\0')
    {
        SET_ERROR(err, "Invalid characters in input.");
        usage(binary_name);

        return -1;
    }

    if (parsed_value > 100)
    {
        char error_message[25];
        snprintf(error_message, sizeof(error_message), "%s value out of range.", string);
        SET_ERROR(err, error_message);
        usage(binary_name);

        return -1;
    }

    *value = (uint8_t) parsed_value;

    return 0;
}

