#include <libnl++/genl.hpp>
#include <netlink/errno.h>
#include <netlink/genl/mngt.h>
#include <spdlog/spdlog.h>

namespace nl {
namespace genl {

void Family::register_family(const std::string &family_name,
                             bool allow_exists) {
  struct genl_ops ops = {.o_name = (char *)family_name.c_str()};
  int ret = genl_register_family(&ops);
  if (ret != 0) {
    if (ret == -NLE_EXIST && allow_exists) {
      spdlog::debug("Family '{}' already exists, skip registration",
                    family_name);
    } else {
      throw std::runtime_error(
          fmt::format("genl_register_family failed with return code {}", ret));
    }
  }
};

} // namespace genl
} // namespace nl
