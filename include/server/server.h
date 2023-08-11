#pragma once

#include <stdint.h>
#include <stdbool.h>

bool server_init(const char *addr_string, uint16_t port);
void server_quit();
bool server_start_loop();
