#pragma once
#include <libnl++/callback.hpp>
#include <libnl++/common.hpp>
#include <libnl++/message.hpp>
#include <libnl++/wlanapp_common.hpp>
#include <memory>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/netlink.h>
#include <spdlog/spdlog.h>

/*struct nl_sock;*/

namespace nl {

class NlSockDeleter {
public:
  void operator()(struct nl_sock *nlsock) const;
};

using nlsock_unique_ptr = std::unique_ptr<struct nl_sock, NlSockDeleter>;

class Socket {
protected:
  nlsock_unique_ptr nlsock;
  NetlinkCallbackSet nlcbs;

  /*
   * Libnl wrapper: creates netlink socket and connects to a given protocol.
   */
  static nlsock_unique_ptr _create_nl_socket(int protocol, u32 port = 0);

  /*
   * Libnl wrapper: send netlink message
   */
  void _send_msg_auto(Message &nlmsg);

  /*
   * Libnl wrapper: add group membership (for multicast groups)
   */
  void _add_membership(int multicast_group_id);

  /*
   * Libnl wrapper: set local port
   */
  void _set_local_port(u32 port);

  /*
   * Libnl wrapper: set remote port
   */
  void _set_peer_port(u32 port);

  /*
   * Libnl wrapper: set default callbacks
   */
  void _set_default_callbacks();

public:
  RecvContext recv_ctx;

  /*
   * Socket ctor.
   * @arg nl_protocol - netlink protocol to use
   * @arg port - port to bind socket on, if equal to zero libnl chooses port by
   */
  Socket(int nl_protocol, u32 port = 0)
      : nlsock(_create_nl_socket(nl_protocol, port)) {
    _set_default_callbacks();
  }

  void set_local_port(u32 port) { _set_local_port(port); }

  void set_peer_port(u32 port) { _set_peer_port(port); }

  /*
   * Send netlink message.
   * @param nlmsg Netlink message
   */
  void send_msg(Message &nlmsg) { _send_msg_auto(nlmsg); }

  /*
   * Receive netlink message.
   * @param cb_ctx_pair a pair of callback and callback argument if received for
   * a valid response
   */
  void recv_msg(const std::pair<NetlinkValidCallback, void *> &cb_ctx_pair) {
    recv_ctx.valid_cb_ctx_pair = cb_ctx_pair;
    recv_ctx.nl_recv_status = RecvStatus::CONTINUE;
    while (recv_ctx.nl_recv_status == RecvStatus::CONTINUE) {
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

} // namespace nl
