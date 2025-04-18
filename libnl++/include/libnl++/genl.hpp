#pragma once

#include <string>

namespace genl {
class Family {
public:
  static void register_family(const std::string &family_name);
};

}; // namespace genl
