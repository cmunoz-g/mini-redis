#pragma once
#include <stdint.h>

int set_non_blocking(int fd);
int socket_listen(const char *host, uint16_t port);