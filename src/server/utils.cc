#include "utils.hh"
#include <stdio.h>

int err_msg(const char *msg, int fatal) {
    fprintf(stderr, "Error: %s", msg);
    if (fatal) fprintf(stderr, "Closing server\n");
    return -1;
}
