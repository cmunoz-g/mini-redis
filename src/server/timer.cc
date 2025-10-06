#include "timer.hh"
#include "dlist.hh"
#include "utils.hh"
#include "entry.hh"
#include <algorithm>
#include <poll.h>
#include <time.h>

static constexpr uint64_t k_idle_timeout_ms = 120000;
static constexpr uint64_t k_read_timeout_ms = 60000;
static constexpr uint64_t k_write_timeout_ms = 60000;
static constexpr size_t k_max_key_works = 2000;

uint64_t get_monotonic_msec() {
    struct timespec tv = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return static_cast<uint64_t>(tv.tv_sec) * 1000
        + static_cast<uint64_t>(tv.tv_nsec / 1000000);
}

int32_t next_timer_ms(g_data &data) {
    if (dlist_empty(&data.idle_list) && dlist_empty(&data.read_list)
        && dlist_empty(&data.write_list)) return -1;

    uint64_t now = get_monotonic_msec();

    Conn *c_idle = container_of(data.idle_list.next, Conn, idle_node);
    Conn *c_read = container_of(data.read_list.next, Conn, read_node);
    Conn *c_write = container_of(data.write_list.next, Conn, write_node);

    uint64_t next_ms = std::min({c_write->last_active_ms + k_write_timeout_ms,  // min, ::min or std::min ? whats cleaner
        c_read->last_read_ms + k_read_timeout_ms,
        c_idle->last_write_ms + k_idle_timeout_ms});
    
    if (!data.heap.empty() && data.heap[0].val < next_ms)
        next_ms = data.heap[0].val;

    if (next_ms == HEAP_INVALID) return -1;
    if (next_ms <= now) return 0;

    return static_cast<int32_t>(next_ms - now);
}

static void process_idle_timers(DList &list, std::vector<Conn *> &fd2conn, uint64_t now) {
    while (!dlist_empty(&list)) {
        Conn *c = container_of(list.next, Conn, idle_node);
        uint64_t expire = c->last_active_ms + k_idle_timeout_ms;
        if (expire > now) break;
        fprintf(stderr, "removing idle connection: %d\n", c->fd);
        handle_destroy(c, fd2conn);
    }
}

static void process_read_timers(DList &list, std::vector<Conn *> &fd2conn, uint64_t now) {
    while (!dlist_empty(&list)) {
        Conn *c = container_of(list.next, Conn, read_node);
        uint64_t expire = c->last_read_ms + k_read_timeout_ms;
        if (expire > now) break;
        fprintf(stderr, "removing connection (read timeout): %d\n", c->fd);
        handle_destroy(c, fd2conn);
    }
}

static void process_write_timers(DList &list, std::vector<Conn *> &fd2conn, uint64_t now) {
    while (!dlist_empty(&list)) {
        Conn *c = container_of(list.next, Conn, write_node);
        uint64_t expire = c->last_write_ms + k_write_timeout_ms;
        if (expire > now) break;
        fprintf(stderr, "removing connection (write timeout): %d\n", c->fd);
        handle_destroy(c, fd2conn);
    }
}

static void process_key_timers(g_data &data, uint64_t now) {
    size_t nworks = 0;
    while (!data.heap.empty() && data.heap[0].val < now && nworks++ < k_max_key_works) {
        Entry *ent = container_of(data.heap[0].ref, Entry, heap_idx);
        hm_delete(&data.db, &ent->node, &hnode_same);
        entry_del(data, ent);
    }
}

void process_timers(g_data &data, std::vector<Conn *> &fd2conn) {
    uint64_t now = get_monotonic_msec();

    process_idle_timers(data.idle_list, fd2conn, now);
    process_read_timers(data.read_list, fd2conn, now);
    process_write_timers(data.write_list, fd2conn, now);
    process_key_timers(data, now);
}