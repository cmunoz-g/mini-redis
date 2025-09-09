#pragma once
#include <cstdint>

int set_non_blocking(int fd);
int socket_listen(const char *host, uint16_t port);