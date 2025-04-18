#include <cerrno>
#include <message.hpp>
#include <net/if.h>
#include <spdlog/spdlog.h>

nlmsg_unique_ptr NetlinkMessage::create_nlmsg() {
  struct nl_msg *nlmsg_raw = nlmsg_alloc();
  if (nlmsg_raw == nullptr) {
    throw std::bad_alloc();
  }
  nlmsg_unique_ptr nlmsg{nlmsg_raw};
  return nlmsg;
}

NetlinkMessage &NetlinkMessage::put_header(uint8_t nl_cmd,
                                           int nl80211_family_id) {
  u8 *hdr_ptr = (u8 *)genlmsg_put(nlmsg.get(),
                                  /* pid= */ 0,
                                  /* seq= */ 0,
                                  /* family= */ nl80211_family_id,
                                  /* hdrlen= */ 0,
                                  /* flags= */ 0,
                                  /* cmd = */ nl_cmd,
                                  /* version= */ 0);

  if (nullptr == hdr_ptr) {
    spdlog::error("genlmsg_put() failed");
    throw std::bad_alloc();
  }
  return *this;
}

NetlinkMessage &NetlinkMessage::put_vendor_id(u32 vendor_id) {
  int ret =
      nla_put(nlmsg.get(), NL80211_ATTR_VENDOR_ID, sizeof(u32), &vendor_id);
  if (ret != 0) {
    throw std::runtime_error("put_vendor_id failed");
  }
  return *this;
}

NetlinkMessage &NetlinkMessage::put_vendor_subcmd(u32 cmdid) {
  int res =
      nla_put(nlmsg.get(), NL80211_ATTR_VENDOR_SUBCMD, sizeof(u32), &cmdid);
  if (res != 0) {
    throw std::runtime_error("put_vendor_subcmd failed");
  }
  return *this;
}

NetlinkMessage &NetlinkMessage::put_iface_idx(const std::string &iface) {
  u32 iface_idx = if_nametoindex(iface.c_str());
  if (iface_idx == 0) {
    std::string err_msg{fmt::format(
        "failed to get ifindex for interface {}: {}", iface, strerror(errno))};
    throw std::runtime_error(err_msg);
  }
  int res = nla_put(nlmsg.get(), NL80211_ATTR_IFINDEX, sizeof(u32), &iface_idx);
  if (res != 0) {
    throw std::runtime_error("put_vendor_subcmd failed");
  }
  return *this;
}

NetlinkMessage &NetlinkMessage::start_vendor_attr_block() {
  nested_attr_start = nla_nest_start(nlmsg.get(), NL80211_ATTR_VENDOR_DATA);
  return *this;
}

NetlinkMessage &NetlinkMessage::end_vendor_attr_block() {
  if (nested_attr_start == nullptr) {
    throw std::runtime_error("expected nested_attr_start to be non-nullptr");
  }
  nla_nest_end(nlmsg.get(), nested_attr_start);
  return *this;
}
