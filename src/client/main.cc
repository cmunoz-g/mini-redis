// implement redis_client ! test with it !
#include "client.hh"
#include <cstdint>
#include <string>
#include <stdexcept>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./mini-redis-client <port>\n");
        return -1;
    }
    try {
        int temp = std::stoi(argv[1]);
        if (temp < 0 || temp > 65535) throw std::out_of_range("Port must be between 0 and 65525");
        uint16_t port = static_cast<uint16_t>(temp);
        return run_client(port);
    } catch (const std::exception &e) {
        fprintf(stderr, "Usage: %s\n", e.what());
        return 1;
    }
}