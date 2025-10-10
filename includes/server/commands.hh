#pragma once
#include "hashtable.hh"
#include "protocol.hh"
#include "sortedset.hh"
#include <unordered_map>
#include <string>
#include <vector>

struct Request {
    g_data &data;
    std::vector<std::string> &cmd;
    Buffer &out;
};

/* API */
void do_set(Request &req);
void do_get(Request &req);
void do_del(Request &req);
void do_keys(Request &req);
void do_zscore(Request &req);
void do_zrem(Request &req);
void do_zadd(Request &req);
void do_zquery(Request &req);
void do_expire(Request &req);
void do_quit(Request &req);

/* Function handlers */

using Handler = void(*)(Request&);

struct Command {
    size_t arity;
    Handler f;
};

const std::unordered_map<std::string, Command> command_list {
    {"get", {2, do_get}},
    {"set", {3, do_set}},
    {"del", {2, do_del}},
    {"keys", {1, do_keys}},
    {"zadd", {4, do_zadd}},
    {"zrem", {3, do_zrem}},
    {"zscore", {3, do_zscore}},
    {"zquery", {6, do_zquery}},
    {"pexpire", {3, do_expire}},
    {"quit", {1, do_quit}},
    {"exit", {1, do_quit}}
};