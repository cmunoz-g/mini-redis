#pragma once
#include "dlist.hh"
#include <cstdint>
#include <vector>
#include "server.hh"

uint64_t get_monotonic_msec();
int32_t next_timer_ms(g_data &data);
void process_timers(g_data &data, std::vector<Conn *> &fd2conn);