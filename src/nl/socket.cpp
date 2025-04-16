#include "socket.hpp"
#include "spdlog/spdlog.h"

int NetlinkSocket::RxCallbacks::default_ack_handler(struct nl_msg *msg,
                                                    void *arg) {
  spdlog::debug("Ack callback triggered");
  if (arg == nullptr) {
    throw std::invalid_argument("passed null argument to ack handler");
  }
  NetlinkSocket *nlsock = (NetlinkSocket *)arg;
  nlsock->recv_ctx.nl_recv_status = NetlinkRecvStatus::FINISH;
  return NL_STOP;
}

int NetlinkSocket::RxCallbacks::default_finish_handler(struct nl_msg *msg,
                                                       void *arg) {
  spdlog::debug("Finish callback triggered");
  if (arg == nullptr) {
    throw std::invalid_argument("passed null argument to finish handler");
  }
  NetlinkSocket *nlsock = (NetlinkSocket *)arg;
  nlsock->recv_ctx.nl_recv_status = NetlinkRecvStatus::FINISH;
  return NL_SKIP;
}

int NetlinkSocket::RxCallbacks::default_error_handler(struct sockaddr_nl *nla,
                                                      struct nlmsgerr *err,
                                                      void *arg) {
  spdlog::debug("Error callback triggered");
  if (arg == nullptr) {
    throw std::invalid_argument("passed null argument to error handler");
  }
  NetlinkSocket *nlsock = (NetlinkSocket *)arg;
  nlsock->recv_ctx.nl_recv_status = NetlinkRecvStatus::ERROR;
  spdlog::error("Error handler received response with error code {}",
                err->error);
  return NL_SKIP;
}

int NetlinkSocket::RxCallbacks::default_seq_disable(struct nl_msg *msg,
                                                    void *arg) {
  return NL_OK;
}

int NetlinkSocket::RxCallbacks::response_handler_wrapper(struct nl_msg *msg,
                                                         void *arg) {
  if (arg == nullptr) {
    throw std::invalid_argument("passed null argument to finish handler");
  }
  spdlog::debug("Incoming valid response from driver to wrapper function");
  NetlinkSocket *const nlsock = static_cast<NetlinkSocket *>(arg);

  NetlinkValidCallback valid_cb = nlsock->recv_ctx.valid_cb_ctx_pair.first;
  void *valid_cb_ctx = nlsock->recv_ctx.valid_cb_ctx_pair.second;

  spdlog::debug("Starting handler callback...");
  nl_cb_action parse_res = valid_cb(msg, valid_cb_ctx);
  spdlog::debug("Callback done");
  if (parse_res == NL_STOP) {
    spdlog::debug("Callback returns NL_STOP, clearing rx buffer and stopping "
                  "recv loop...");
    nlsock->recv_ctx.nl_recv_status = NetlinkRecvStatus::FINISH;
  } else {
    spdlog::debug("Callback returns NL_SKIP or NL_OK, proceeding...");
    nlsock->recv_ctx.nl_recv_status = NetlinkRecvStatus::CONTINUE;
  }
  return parse_res;
}
