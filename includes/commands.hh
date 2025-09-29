#pragma once

#include "hashtable.hh"
#include "protocol.hh"
#include "sortedset.hh"
#include <string>
#include <vector>

void do_set(HMap &db, std::vector<std::string> &cmd, Buffer &out);
void do_get(HMap &db, std::vector<std::string> &cmd, Buffer &out);
void do_del(HMap &db, std::vector<std::string> &cmd, Buffer &out);
bool do_keys(HMap &db, Buffer &out);