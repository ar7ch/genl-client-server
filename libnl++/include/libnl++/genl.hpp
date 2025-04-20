#pragma once

#include <string>

namespace nl {

namespace genl {
class Family {
public:
  static void register_family(const std::string &family_name,
                              bool allow_exists = true);
};

}; // namespace genl
   //
}; // namespace nl
