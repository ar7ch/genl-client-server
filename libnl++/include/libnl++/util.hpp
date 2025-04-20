#pragma once

#include <cstddef>
namespace nl {

void hexdump(const void *data, std::size_t length,
             std::size_t bytes_per_line = 16);

}
