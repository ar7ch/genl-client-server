#pragma once

#include <memory>
#include <netlink/genl/genl.h>
#include <netlink/handlers.h>
#include <stdexcept>

namespace nl {

class NlCbDeleter {
public:
  void operator()(struct nl_cb *nlcb) const {
    if (nlcb) {
      nl_cb_put(nlcb);
    }
  }
};
using nlcb_unique_ptr = std::unique_ptr<struct nl_cb, NlCbDeleter>;
using netlink_callback_t = int (*)(struct nl_msg *nlmsg, void *ctx);
using netlink_err_callback_t = int (*)(struct sockaddr_nl *nla,
                                       struct nlmsgerr *err, void *arg);
class NetlinkCallbackSet {
  nlcb_unique_ptr nlcbs;

  static nlcb_unique_ptr create_nlcb() {
    struct nl_cb *nlcb_raw = nl_cb_alloc(NL_CB_DEFAULT);
    if (nlcb_raw == nullptr) {
      throw std::runtime_error("nl_cb_alloc failed");
    }
    nlcb_unique_ptr nlcb{nlcb_raw};
    return nlcb;
  }

public:
  NetlinkCallbackSet() : nlcbs(create_nlcb()) {}
  struct nl_cb *get() { return nlcbs.get(); }

  /*
   * Register a custom callback that is invoked on certain event.
   *
   * @param type - type of event when a callback is invoked (FINISH, INVALID,
   * SEQ_CHECK, ...)
   * @param handler_cb - callback function pointer, returns int and accepts
   * struct nl_msg* with message and void* with context
   * @param arg - context that will be passed to `func`
   */
  void register_cb(enum nl_cb_type type, nl_recvmsg_msg_cb_t handler_cb,
                   void *arg) {
    enum nl_cb_kind kind = NL_CB_CUSTOM;
    int ret = nl_cb_set(nlcbs.get(), NL_CB_SEQ_CHECK, kind, handler_cb, arg);
    if (ret != 0) {
      throw std::runtime_error("nl_cb_set failed with code " +
                               std::to_string(ret));
    }
  }

  /*
   * Register a custom callback that is invoked on certain event.
   * @param error_cb - error handling callbacj
   * @param - context pointer for error handler
   */
  void register_err_cb(nl_recvmsg_err_cb_t error_cb, void *arg) {
    int ret = nl_cb_err(nlcbs.get(), NL_CB_CUSTOM, error_cb, arg);
    if (ret != 0) {
      throw std::runtime_error("nl_cb_err failed with code " +
                               std::to_string(ret));
    }
  }
};
} // namespace nl
