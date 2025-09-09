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

void do_set(HMap&, std::vector<std::string> &cmd, Response &);
void do_get(HMap&, std::vector<std::string> &cmd, Response &);
void do_del(HMap&, std::vector<std::string> &cmd, Response &resp);