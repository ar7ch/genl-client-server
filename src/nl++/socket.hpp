#pragma once
#include "callback.hpp"
#include "common.hpp"
#include "message.hpp"
#include "spdlog/spdlog.h"
#include <memory>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <stdexcept>
#include <wlanapp_common.hpp>

class NlSockDeleter {
public:
  void operator()(struct nl_sock *nlsock) const {
    if (nlsock != nullptr) {
      nl_socket_free(nlsock);
    }
  }
};

using nlsock_unique_ptr = std::unique_ptr<struct nl_sock, NlSockDeleter>;

class NetlinkSocket {
  nlsock_unique_ptr nlsock;
  std::string genl_family_name;
  int nl_family_id;
  NetlinkCallbackSet nlcbs;

  static nlsock_unique_ptr _create_nl_socket(int protocol) {
    struct nl_sock *sock_raw = nl_socket_alloc();
    if (sock_raw == NULL) {
      throw std::runtime_error("Failed to create NL socket");
    }
    if (nl_connect(sock_raw, protocol) != 0) {
      nl_socket_free(sock_raw);
      throw std::runtime_error("nl_connect failed");
    }
    nlsock_unique_ptr nlsock{sock_raw};
    return nlsock;
  }

  int _resolve_genl_family_id(const std::string &name) {
    nl_family_id = genl_ctrl_resolve(nlsock.get(), name.c_str());
    if (nl_family_id < 0) {
      throw std::runtime_error("Could not resolve family id " + name);
    }
    return nl_family_id;
  }

  void _send_msg_auto(NetlinkMessage &nlmsg) {
    int ret = nl_send_auto_complete(nlsock.get(), nlmsg.get());
    if (ret < 0) {
      throw std::runtime_error("Sending netlink message failed");
    }
  }

  void _add_membership(int multicast_group_id) {
    int ret = nl_socket_add_membership(nlsock.get(), multicast_group_id);
    if (ret < 0) {
      throw std::runtime_error(fmt::format(
          "Failed to add multicast membership {}: {}", ret, strerror(-ret)));
    }
  }

  void _set_default_callbacks() {
    spdlog::debug("Start registering default callbacks");
    nlcbs.register_cb(NL_CB_SEQ_CHECK, RxCallbacks::default_seq_disable, NULL);
    nlcbs.register_cb(NL_CB_FINISH, RxCallbacks::default_finish_handler, this);
    nlcbs.register_cb(NL_CB_ACK, RxCallbacks::default_ack_handler, this);
    nlcbs.register_err_cb(RxCallbacks::default_error_handler, this);
    nlcbs.register_cb(NL_CB_VALID, RxCallbacks::response_handler_wrapper, this);
    nl_socket_set_cb(nlsock.get(), nlcbs.get());
    spdlog::debug("Register default callbacks ok");
  }

public:
  RecvContext recv_ctx;
  NetlinkSocket(int nl_protocol = NETLINK_GENERIC,
                const std::string &genl_family_name = "nl80211")
      : nlsock(_create_nl_socket(nl_protocol)),
        genl_family_name{genl_family_name},
        nl_family_id{_resolve_genl_family_id(genl_family_name)} {
    _set_default_callbacks();
  }

  int get_nl_family_id() const { return nl_family_id; }

  /*
   * Send netlink message.
   * @param nlmsg Netlink message
   */
  void send_msg(NetlinkMessage &nlmsg) { _send_msg_auto(nlmsg); }

  /*
   * Receive netlink message.
   * @param cb_ctx_pair a pair of callback and callback argument if received for
   * a valid response
   */
  void recv_msg(const std::pair<NetlinkValidCallback, void *> &cb_ctx_pair) {
    recv_ctx.valid_cb_ctx_pair = cb_ctx_pair;
    recv_ctx.nl_recv_status = NetlinkRecvStatus::CONTINUE;
    while (recv_ctx.nl_recv_status == NetlinkRecvStatus::CONTINUE) {
      spdlog::debug("starting recv()");
      int res = nl_recvmsgs(nlsock.get(), nlcbs.get());
      if (res != 0) {
        spdlog::error("nl_recvmsgs() failed with code {}");
        // nl_recv_status is updated in the callback itself
      }
    }
  }

  // /*
  //  * Create an event socket into an event socket.
  //  * @param multicast_group_name multicast group you want to join
  //  */
  // static NetlinkSocket
  // CreateEventSocket(const std::string &multicast_group_name = "vendor") {
  //   const std::string GENL_FAMILY_NLCTRL{"nlctrl"};
  //   NetlinkSocket event_socket;
  //   NetlinkSocket cmd_socket{NETLINK_GENERIC, GENL_FAMILY_NLCTRL};
  //
  //   NetlinkMessage family_req_msg;
  //   family_req_msg.put_header(CTRL_CMD_GETFAMILY,
  //   cmd_socket.get_nl_family_id())
  //       .put_string(CTRL_ATTR_FAMILY_NAME, "nl80211");
  //
  //   FamilyCmdContext cmd_ctx = {.group = multicast_group_name, .id =
  //   -ENOENT}; spdlog::debug("Sending get multicast groups request");
  //   cmd_socket.send_msg(family_req_msg);
  //   cmd_socket.recv_msg({NetlinkMessage::parse_mcast_group_response,
  //                        static_cast<void *>(&cmd_ctx)});
  //
  //   if (cmd_ctx.id == -ENOENT) {
  //     throw std::runtime_error("recv family id failed");
  //   }
  //   int vendor_mcast_group_id = cmd_ctx.id;
  //   event_socket._add_membership(vendor_mcast_group_id);
  //   return event_socket;
  // }

  struct RxCallbacks {
    static int default_ack_handler(struct nl_msg *msg, void *arg);
    static int default_finish_handler(struct nl_msg *msg, void *arg);
    static int default_error_handler(struct sockaddr_nl *nla,
                                     struct nlmsgerr *err, void *arg);
    static int default_seq_disable(struct nl_msg *msg, void *arg);
    static int response_handler_wrapper(struct nl_msg *msg, void *arg);
  };
};
