#pragma once

#include "hashtable.hh"
#include "protocol.hh"
#include <string>
#include <vector>

struct Entry {
    struct HNode node;
    std::string key;
    std::string value;
};

void do_set(HMap &db, std::vector<std::string> &cmd, Buffer &out);
void do_get(HMap &db, std::vector<std::string> &cmd, Buffer &out);
void do_del(HMap &db, std::vector<std::string> &cmd, Buffer &out);