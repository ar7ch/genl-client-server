#include <libnl++/socket.hpp>
#include <netlink/errno.h>
#include <netlink/socket.h>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace nl {

void NlSockDeleter::operator()(struct nl_sock *nlsock) const {
  if (nlsock != nullptr) {
    nl_socket_free(nlsock);
  }
}

nlsock_unique_ptr Socket::_create_nl_socket(int protocol, u32 port) {
  struct nl_sock *sock_raw = nl_socket_alloc();
  if (sock_raw == NULL) {
    throw std::runtime_error("Failed to create NL socket");
  }
  nl_socket_set_local_port(sock_raw, port);
  if (nl_connect(sock_raw, protocol) != 0) {
    nl_socket_free(sock_raw);
    throw std::runtime_error("nl_connect failed");
  }
  nlsock_unique_ptr nlsock{sock_raw};
  return nlsock;
}

void Socket::_send_msg_auto(Message &nlmsg) {
  int ret = nl_send_auto_complete(nlsock.get(), nlmsg.get());
  if (ret < 0) {
    throw std::runtime_error(fmt::format(
        "Sending netlink message failed, ret={} ({})", ret, nl_geterror(ret)));
  }
}

void Socket::_add_membership(int multicast_group_id) {
  int ret = nl_socket_add_membership(nlsock.get(), multicast_group_id);
  if (ret < 0) {
    throw std::runtime_error(fmt::format(
        "Failed to add multicast membership {}: {}", ret, strerror(-ret)));
  }
}
void Socket::_set_local_port(const u32 port) {
  nl_socket_set_local_port(nlsock.get(), port);
}

void Socket::_set_peer_port(u32 port) {
  nl_socket_set_peer_port(nlsock.get(), port);
}

void Socket::_set_default_callbacks() {
  spdlog::debug("Start registering default callbacks");
  nlcbs.register_cb(NL_CB_SEQ_CHECK, RxCallbacks::default_seq_disable, NULL);
  nlcbs.register_cb(NL_CB_FINISH, RxCallbacks::default_finish_handler, this);
  nlcbs.register_cb(NL_CB_ACK, RxCallbacks::default_ack_handler, this);
  nlcbs.register_err_cb(RxCallbacks::default_error_handler, this);
  nlcbs.register_cb(NL_CB_VALID, RxCallbacks::response_handler_wrapper, this);
  nl_socket_set_cb(nlsock.get(), nlcbs.get());
  spdlog::debug("Register default callbacks ok");
}

int Socket::RxCallbacks::default_ack_handler(struct nl_msg *msg, void *arg) {
  spdlog::debug("Ack callback triggered");
  if (arg == nullptr) {
    throw std::invalid_argument("passed null argument to ack handler");
  }
  Socket *nlsock = (Socket *)arg;
  nlsock->recv_ctx.nl_recv_status = RecvStatus::FINISH;
  return NL_STOP;
}

int Socket::RxCallbacks::default_finish_handler(struct nl_msg *msg, void *arg) {
  spdlog::debug("Finish callback triggered");
  if (arg == nullptr) {
    throw std::invalid_argument("passed null argument to finish handler");
  }
  Socket *nlsock = (Socket *)arg;
  nlsock->recv_ctx.nl_recv_status = RecvStatus::FINISH;
  return NL_SKIP;
}

int Socket::RxCallbacks::default_error_handler(struct sockaddr_nl *nla,
                                               struct nlmsgerr *err,
                                               void *arg) {
  spdlog::debug("Error callback triggered");
  if (arg == nullptr) {
    throw std::invalid_argument("passed null argument to error handler");
  }
  Socket *nlsock = (Socket *)arg;
  nlsock->recv_ctx.nl_recv_status = RecvStatus::ERROR;
  spdlog::error("Error handler received response with error code {}",
                err->error);
  return NL_SKIP;
}

int Socket::RxCallbacks::default_seq_disable(struct nl_msg *msg, void *arg) {
  return NL_OK;
}

int Socket::RxCallbacks::response_handler_wrapper(struct nl_msg *msg,
                                                  void *arg) {
  if (arg == nullptr) {
    throw std::invalid_argument("passed null argument to finish handler");
  }
  spdlog::debug("Incoming valid response from driver to wrapper function");
  Socket *const nlsock = static_cast<Socket *>(arg);

  NetlinkValidCallback valid_cb = nlsock->recv_ctx.valid_cb_ctx_pair.first;
  void *valid_cb_ctx = nlsock->recv_ctx.valid_cb_ctx_pair.second;

  spdlog::debug("Starting handler callback...");
  nl_cb_action parse_res = valid_cb(msg, valid_cb_ctx);
  spdlog::debug("Callback done");
  if (parse_res == NL_STOP) {
    spdlog::debug("Callback returns NL_STOP, clearing rx buffer and stopping "
                  "recv loop...");
    nlsock->recv_ctx.nl_recv_status = RecvStatus::FINISH;
  } else {
    spdlog::debug("Callback returns NL_SKIP or NL_OK, proceeding...");
    nlsock->recv_ctx.nl_recv_status = RecvStatus::CONTINUE;
  }
  return parse_res;
}

} // namespace nl
