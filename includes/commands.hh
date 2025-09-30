#pragma once

#include "hashtable.hh"
#include "protocol.hh"
#include "sortedset.hh"
#include <string>
#include <vector>

void do_set(HMap &db, std::vector<std::string> &cmd, Buffer &out);
void do_get(HMap &db, std::vector<std::string> &cmd, Buffer &out);
void do_del(HMap &db, std::vector<std::string> &cmd, Buffer &out);
void do_keys(HMap &db, std::vector<std::string> &cmd, Buffer &out);
void do_zscore(HMap &db, std::vector<std::string> &cmd, Buffer &out);
void do_zrem(HMap &db, std::vector<std::string> &cmd, Buffer &out);
void do_zadd(HMap &db, std::vector<std::string> &cmd, Buffer &out);
void do_zquery(HMap &db, std::vector<std::string> &cmd, Buffer &out);