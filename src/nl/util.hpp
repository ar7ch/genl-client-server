#pragma once
#include "nl_common.hpp"
void hexdump(const void *data, std::size_t length,
             std::size_t bytes_per_line = 16);
