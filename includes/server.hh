#pragma once
#include "buffer.hh"
#include "hashtable.hh"
#include "protocol.hh"

int run_server(HMap &db, const char* host, uint16_t port);