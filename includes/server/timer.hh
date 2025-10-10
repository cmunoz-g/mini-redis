#pragma once
#include "dlist.hh"
#include "server.hh"
#include <stdint.h>
#include <vector>

/* API */
uint64_t get_monotonic_msec();
int32_t next_timer_ms(g_data &data);
void process_timers(g_data &data, std::vector<Conn *> &fd2conn);