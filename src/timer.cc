#include "timer.hh"
#include "dlist.hh"
#include "server.hh"
#include "utils.hh"
#include <poll.h>
#include <time.h>

// Before : Exercise: Add IO timeouts for read() and write() (with a different timeout value).
// Make sure all the ttl, timer stuff is completely clear

static constexpr uint64_t k_idle_timeout_ms = 5000;
static constexpr uint64_t k_read_timeout_ms = 2000;
static constexpr uint64_t k_write_timeout_ms = 3000;

uint64_t get_monotonic_msec() {
    struct timespec tv = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return static_cast<uint64_t>(tv.tv_sec) * 1000
        + static_cast<uint64_t>(tv.tv_nsec / 1000000);
}

int32_t next_timer_ms(DList &idle, DList &read, DList &write) {
    if (dlist_empty(&idle) && dlist_empty(&read) && dlist_empty(&write))
        return -1;

    uint64_t now = get_monotonic_msec();

    Conn *c_idle = container_of(idle.next, Conn, idle_node);
    Conn *c_read = container_of(read.next, Conn, read_node);
    Conn *c_write = container_of(write.next, Conn, write_node);

    uint64_t next_ms = std::min(c_write->last_active_ms + k_write_timeout_ms, 
        c_read->last_read_ms + k_read_timeout_ms,
        c_idle->last_write_ms + k_idle_timeout_ms);
    
    if (next_ms <= now) return 0;

    return static_cast<int32_t>(next_ms - now);
}

static void process_idle_timers(DList &list, std::vector<Conn *> &fd2conn) {
    uint64_t now = get_monotonic_msec();

    while (!dlist_empty(&list)) {
        Conn *c = container_of(list.next, Conn, idle_node);
        uint64_t expire = c->last_active_ms + k_idle_timeout_ms;
        if (expire > now) break;
        fprintf(stderr, "removing idle connection: %d\n", c->fd);
        handle_destroy(c, fd2conn);
    }
}

static void process_read_timers(DList &list, std::vector<Conn *> &fd2conn) {
    uint64_t now = get_monotonic_msec();

    while (!dlist_empty(&list)) {
        Conn *c = container_of(list.next, Conn, read_node);
        uint64_t expire = c->last_read_ms + k_read_timeout_ms;
        if (expire > now) break;
        fprintf(stderr, "removing connection (read timeout): %d\n", c->fd);
        handle_destroy(c, fd2conn);
    }
}

static void process_write_timers(DList &list, std::vector<Conn *> &fd2conn) {
    uint64_t now = get_monotonic_msec();

    while (!dlist_empty(&list)) {
        Conn *c = container_of(list.next, Conn, write_node);
        uint64_t expire = c->last_write_ms + k_write_timeout_ms;
        if (expire > now) break;
        fprintf(stderr, "removing connection (write timeout): %d\n", c->fd);
        handle_destroy(c, fd2conn);
    }
}

void process_timers(DList &idle, DList &read, DList &write, std::vector<Conn *> &fd2conn) {
    process_idle_timers(idle, fd2conn);
    process_read_timers(read, fd2conn);
    process_write_timers(write, fd2conn);
}