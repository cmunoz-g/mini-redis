#include "timer.hh"
#include "dlist.hh"
#include "server.hh"
#include "utils.hh"
#include <poll.h>
#include <time.h>

// Before : Exercise: Add IO timeouts for read() and write() (with a different timeout value).
// Make sure all the ttl, timer stuff is completely clear

static constexpr uint64_t k_idle_timeout_ms = 5000;

uint64_t get_monotonic_msec() {
    struct timespec tv = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return static_cast<uint64_t>(tv.tv_sec) * 1000
        + static_cast<uint64_t>(tv.tv_nsec / 1000000);
}

int32_t next_timer_ms(DList &node) {
    if (dlist_empty(&node)) return -1;

    uint64_t now = get_monotonic_msec();
    Conn *c = container_of(node.next, Conn, idle_node);
    uint64_t next_ms = c->last_active_ms + k_idle_timeout_ms;
    
    if (next_ms <= now) return 0;

    return static_cast<int32_t>(next_ms - now);
}

void process_timers(DList &node, std::vector<Conn *> &fd2conn) {
    uint64_t now = get_monotonic_msec();
    while (!dlist_empty(&node)) {
        Conn *c = container_of(node.next, Conn, idle_node);
        uint64_t next_ms = c->last_active_ms + k_idle_timeout_ms;
        if (next_ms >= now) break;
        fprintf(stderr, "removing idle connection: %d\n", c->fd);
        handle_destroy(c, fd2conn);
    }
}