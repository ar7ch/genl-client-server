#pragma once

#include <libnl++/socket.hpp>
#include <string>

namespace nl {

namespace genl {
class Family {
public:
  static void register_family(const std::string &family_name,
                              bool allow_exists = true);
};

class GenlSocket : public nl::Socket {
  std::string genl_family_name;
  int nl_family_id;

  /*
   * Libnl wrapper: resolves family id by string name
   */
  int _resolve_genl_family_id(const std::string &name);

public:
  GenlSocket(const std::string &genl_family_name, u32 port = 0)
      : nl::Socket(NETLINK_GENERIC, port), genl_family_name{genl_family_name},
        nl_family_id{_resolve_genl_family_id(genl_family_name)} {}

  int get_nl_family_id() const { return nl_family_id; }
};

}; // namespace genl
   //
}; // namespace nl
