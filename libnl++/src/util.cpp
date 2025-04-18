#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <libnl++/util.hpp>

void hexdump(const void *data, size_t length, size_t bytes_per_line) {
  const uint8_t *ptr = static_cast<const uint8_t *>(data);

  for (size_t i = 0; i < length; i += bytes_per_line) {
    // print offset
    std::cout << std::setw(8) << std::setfill('0') << std::hex << i << "  ";

    // print hex values
    for (size_t j = 0; j < bytes_per_line; ++j) {
      if (i + j < length) {
        std::cout << std::setw(2) << static_cast<int>(ptr[i + j]) << " ";
      } else {
        std::cout << "   ";
      }
      if (j == 7)
        std::cout << " "; // extra space in middle
    }

    std::cout << " ";

    // print ascii representation
    for (size_t j = 0; j < bytes_per_line; ++j) {
      if (i + j < length) {
        char cur_char = static_cast<char>(ptr[i + j]);
        std::cout << (std::isprint(static_cast<unsigned char>(cur_char)) != 0
                          ? cur_char
                          : '.');
      } else {
        std::cout << ' ';
      }
    }

    std::cout << "\n";
  }

  std::cout << std::dec; // reset to decimal output
}
