#pragma once
#include <cstdint>

uint64_t get_monotonic_msec();
int32_t next_timer_ms(DList &idle, DList &read, DList &write);
void process_timers(DList &idle, DList &read, DList &write, std::vector<Conn *> &fd2conn);