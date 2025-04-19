#pragma once

#include <string>

namespace nl {

namespace genl {
class Family {
public:
  static void register_family(const std::string &family_name);
};

}; // namespace genl
   //
}; // namespace nl
