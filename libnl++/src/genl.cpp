#include <libnl++/genl.hpp>
#include <netlink/genl/mngt.h>
#include <spdlog/spdlog.h>

namespace genl {

void Family::register_family(const std::string &family_name) {
  struct genl_ops ops = {.o_name = (char *)family_name.c_str()};
  int ret = genl_register_family(&ops);
  if (ret != 0) {
    throw std::runtime_error(
        fmt::format("genl_register_family failed with return code {}", ret));
  }
};

}; // namespace genl
