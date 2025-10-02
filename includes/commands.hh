#pragma once

#include "hashtable.hh"
#include "protocol.hh"
#include "sortedset.hh"
#include <string>
#include <vector>

void do_set(g_data &data, std::vector<std::string> &cmd, Buffer &out);
void do_get(g_data &data, std::vector<std::string> &cmd, Buffer &out);
void do_del(g_data &data, std::vector<std::string> &cmd, Buffer &out);
void do_keys(g_data &data, std::vector<std::string> &cmd, Buffer &out);
void do_zscore(g_data &data, std::vector<std::string> &cmd, Buffer &out);
void do_zrem(g_data &data, std::vector<std::string> &cmd, Buffer &out);
void do_zadd(g_data &data, std::vector<std::string> &cmd, Buffer &out);
void do_zquery(g_data &data, std::vector<std::string> &cmd, Buffer &out);
void do_expire(g_data &data, std::vector<std::string> &cmd, Buffer &out);
void do_quit(g_data &data, std::vector<std::string> &cmd, Buffer &out);