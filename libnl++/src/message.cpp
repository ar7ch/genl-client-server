#include "libnl++/wlanapp_common.hpp"
#include <cerrno>
#include <libnl++/message.hpp>
#include <net/if.h>
#include <spdlog/spdlog.h>

namespace nl {

void NlMsgDeleter::operator()(struct nl_msg *nlmsg) const {
  if (nlmsg != nullptr) {
    nlmsg_free(nlmsg);
  }
}

nlmsg_unique_ptr Message::create_nlmsg() {
  struct nl_msg *nlmsg_raw = nlmsg_alloc();
  if (nlmsg_raw == nullptr) {
    throw std::bad_alloc();
  }
  nlmsg_unique_ptr nlmsg{nlmsg_raw};
  return nlmsg;
}

Message &Message::put_header(uint8_t nl_cmd,
                                           int family_id) {
  u8 *hdr_ptr = (u8 *)genlmsg_put(nlmsg.get(),
                                  /* pid= */ 0,
                                  /* seq= */ 0,
                                  /* family= */ family_id,
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

Message &Message::put_vendor_id(u32 vendor_id,
                                              int attr_vendor_id) {
  int ret = nla_put(nlmsg.get(), attr_vendor_id, sizeof(u32), &vendor_id);
  if (ret != 0) {
    throw std::runtime_error("put_vendor_id failed");
  }
  return *this;
}

Message &
Message::put_vendor_subcmd(const u32 cmdid,
                                  const int attr_vendor_subcmd) {
  int res = nla_put(nlmsg.get(), attr_vendor_subcmd, sizeof(u32), &cmdid);
  if (res != 0) {
    throw std::runtime_error("put_vendor_subcmd failed");
  }
  return *this;
}

Message &Message::put_iface_idx(const std::string &iface,
                                              const int attr_ifindex) {
  u32 iface_idx = if_nametoindex(iface.c_str());
  if (iface_idx == 0) {
    std::string err_msg{fmt::format(
        "failed to get ifindex for interface {}: {}", iface, strerror(errno))};
    throw std::runtime_error(err_msg);
  }
  int res = nla_put(nlmsg.get(), attr_ifindex, sizeof(u32), &iface_idx);
  if (res != 0) {
    throw std::runtime_error("put_vendor_subcmd failed");
  }
  return *this;
}

Message &Message::start_vendor_attr_block(const int vendor_attr) {
  nested_attr_start = nla_nest_start(nlmsg.get(), vendor_attr);
  return *this;
}

Message &Message::end_vendor_attr_block() {
  if (nested_attr_start == nullptr) {
    throw std::runtime_error("expected nested_attr_start to be non-nullptr");
  }
  nla_nest_end(nlmsg.get(), nested_attr_start);
  return *this;
}

Message &Message::put_string(int attr, const std::string &data) {
  int res = nla_put(nlmsg.get(), attr, (int)data.length() + 1, data.c_str());
  if (res != 0) {
    spdlog::error("nla_put failed for attr id {} and string {}", attr, data);
    throw std::bad_alloc();
  }
  return *this;
}

}; // namespace nl
