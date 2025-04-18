#pragma once

#include <libnl++/wlanapp_common.hpp>
#include <netlink/netlink.h>
#include <utility>

class NetlinkManager;
class NetlinkSocket;
class NetlinkMessage;

using NetlinkValidCallback = nl_cb_action (*)(struct nl_msg *, void *);

struct NetlinkValidCbCtxPair {
  NetlinkValidCallback cb = nullptr;
  void *ctx = nullptr;
};

enum class NetlinkRecvStatus : u8 { CONTINUE, FINISH, ERROR };

struct RecvContext {
  NetlinkRecvStatus nl_recv_status = NetlinkRecvStatus::CONTINUE;
  int max_resp_attr = 0; // how many response attrs to expect
  std::pair<NetlinkValidCallback, void *> valid_cb_ctx_pair{nullptr, nullptr};

  void reset_all() {
    nl_recv_status = NetlinkRecvStatus::CONTINUE;
    max_resp_attr = 0;
    valid_cb_ctx_pair =
        std::pair<NetlinkValidCallback, void *>{nullptr, nullptr};
  }
};

static const unsigned int WLANAPP_NL_CMD_SOCK_PORT = 888;
static const constexpr unsigned int WLANAPP_NL_EVENT_SOCK_PORT = 889;
static const unsigned NL80211_ATTR_MAX_INTERNAL = 256;
#define MAX_ATTR 100

#define SOCKET_BUFFER_SIZE (32768U)
#define RECV_BUF_SIZE (4096)
#define DEFAULT_EVENT_CB_SIZE (64)
#define DEFAULT_CMD_SIZE (64)
