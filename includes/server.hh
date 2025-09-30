#pragma once
#include "buffer.hh"
#include "hashtable.hh"
#include "protocol.hh"
#include "dlist.hh"

struct Conn { 
    int fd{-1};
    bool want_read{true}, want_write{false}, want_close{false};
    Buffer in, out;
    DList idle_node;
    uint64_t last_active_ms{0};
};

struct g_data {
    HMap db;
    DList idle_list;
};

int run_server(g_data &data, const char* host, uint16_t port);
void handle_destroy(Conn *c, std::vector<Conn *> &fd2conn);