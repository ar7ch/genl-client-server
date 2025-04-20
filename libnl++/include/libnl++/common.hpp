#pragma once

#include <libnl++/wlanapp_common.hpp>
#include <netlink/netlink.h>
#include <utility>

namespace nl {

class NetlinkManager;
class Socket;
class Message;

using callback_result_t = nl_cb_action;

using NetlinkValidCallback = nl_cb_action (*)(struct nl_msg *, void *);

struct NetlinkValidCbCtxPair {
  NetlinkValidCallback cb = nullptr;
  void *ctx = nullptr;
};

enum class RecvStatus : u8 { CONTINUE, FINISH, ERROR };

struct RecvContext {
  RecvStatus nl_recv_status = RecvStatus::CONTINUE;
  int max_resp_attr = 0; // how many response attrs to expect
  std::pair<NetlinkValidCallback, void *> valid_cb_ctx_pair{nullptr, nullptr};

  void reset_all() {
    nl_recv_status = RecvStatus::CONTINUE;
    max_resp_attr = 0;
    valid_cb_ctx_pair =
        std::pair<NetlinkValidCallback, void *>{nullptr, nullptr};
  }
};
} // namespace nl
