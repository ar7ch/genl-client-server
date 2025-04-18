#pragma once

#include "common.hpp"
#include <memory>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/handlers.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <nl80211_copy.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

class NlMsgDeleter {
public:
  void operator()(struct nl_msg *nlmsg) const {
    if (nlmsg != nullptr) {
      nlmsg_free(nlmsg);
    }
  }
};

struct FamilyCmdContext {
  std::string group;
  int id;
};

using nlmsg_unique_ptr = std::unique_ptr<struct nl_msg, NlMsgDeleter>;
using nlmsg_raw_ptr = struct nl_msg *;

/*
 * Class that stores and builds a netlink message (a thick wrapper around struct
 * nl_msg et al)
 */
class NetlinkMessage {
  nlmsg_unique_ptr nlmsg;

  /* we keep a raw ptr to the start of a nested attribute - it is a portion of
   * the nlmsg. we don't need to control the lifetime of that ptr here, since
   * nlmsg already manages that memory
   */
  struct nlattr *nested_attr_start = nullptr;
  static nlmsg_unique_ptr create_nlmsg();
  NetlinkMessage(const NetlinkMessage &other) = delete;
  NetlinkMessage &operator=(const NetlinkMessage &other) = delete;

public:
  // move constructors are allowed - we use an underlying unique_ptr
  NetlinkMessage(NetlinkMessage &&other) = default;
  NetlinkMessage &operator=(NetlinkMessage &&other) = default;

  NetlinkMessage() : nlmsg(create_nlmsg()) {}
  struct nl_msg *get() { return nlmsg.get(); }

  NetlinkMessage &put_header(uint8_t nl_cmd, int nl80211_family_id);
  NetlinkMessage &put_vendor_id(u32 vendor_id);
  NetlinkMessage &put_vendor_subcmd(u32 cmdid);
  NetlinkMessage &put_iface_idx(const std::string &iface);
  NetlinkMessage &start_vendor_attr_block();
  NetlinkMessage &end_vendor_attr_block();

  /**
   * Add a unspecific attribute to netlink message.
   * @arg msg		Netlink message.
   * @arg attrtype	Attribute type.
   * @arg datalen		Length of data to be used as payload.
   * @arg data		Pointer to data to be used as attribute payload.
   *
   * Reserves room for a unspecific attribute and copies the provided data
   * into the message as payload of the attribute. Returns an error if there
   * is insufficient space for the attribute.
   *
   * @see nla_reserve
   * @return 0 on success or a negative error code.
   */
  template <typename T> NetlinkMessage &put_attr(int attr, T data) {
    int res = nla_put(nlmsg.get(), attr, sizeof(T), &data);
    if (res != 0) {
      throw std::runtime_error(
          fmt::format("nla_put failed for attr id {}", attr));
    }
    return *this;
  }

  NetlinkMessage &put_string(int attr, const std::string &data) {
    int res = nla_put(nlmsg.get(), attr, (int)data.length() + 1, data.c_str());
    if (res != 0) {
      spdlog::error("nla_put failed for attr id {} and string {}", attr, data);
      throw std::bad_alloc();
    }
    return *this;
  }
};
