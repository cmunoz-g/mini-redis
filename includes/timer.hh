#pragma once
#include <cstdint>

uint64_t get_monotonic_msec();
int32_t next_timer_ms(DList &node);
void process_timers(DList &node, std::vector<Conn *> &fd2conn);