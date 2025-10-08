#include "server.hh"
#include <stdexcept>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./mini-redis <port>\n");
        return 1;
    }
    try {
        int port = std::stoi(argv[1]);
        if (port < 0 || port > 65535) throw std::out_of_range("Port must be between 0 and 65535");

        g_data data{};
        dlist_init(&data.idle_list);
        dlist_init(&data.read_list);
        dlist_init(&data.write_list);
        thread_pool_init(&data.thread_pool, 4);

        return run_server(data, "0.0.0.0", port);

    } catch (const std::invalid_argument &e) {
        fprintf(stderr, "Usage: Port must be an integer number\n");
    } catch (const std::out_of_range &e) {
        fprintf(stderr, "Usage: %s\n", e.what());
    }
    return 1;
}
