#include "server.hh"

int main(void) {
    g_data data{};
    dlist_init(&data.idle_list);
    thread_pool_init(&data.thread_pool, 4);

    return run_server(data, "0.0.0.0", 1234);
}