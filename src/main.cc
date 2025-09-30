#include "server.hh"

int main(void) {
    g_data data{};
    dlist_init(&data.idle_list);

    return run_server(data, "0.0.0.0", 1234);
}