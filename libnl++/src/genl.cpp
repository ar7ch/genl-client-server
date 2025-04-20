#include <libnl++/genl.hpp>
#include <netlink/errno.h>
#include <netlink/genl/mngt.h>
#include <spdlog/spdlog.h>

namespace nl {
namespace genl {

int genl::GenlSocket::_resolve_genl_family_id(const std::string &name) {
  nl_family_id = genl_ctrl_resolve(nlsock.get(), name.c_str());
  if (nl_family_id < 0) {
    throw std::runtime_error(
        fmt::format("Could not resolve family id {}, ret={} ({})", name,
                    nl_family_id, nl_geterror(nl_family_id)));
  }
  return nl_family_id;
}

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
  spdlog::debug("Family {} register ok", family_name);
};

} // namespace genl
} // namespace nl
