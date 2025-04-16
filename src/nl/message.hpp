#pragma once

#include "nl_common.hpp"
#include "spdlog/spdlog.h"
#include <cfg80211_external.h> // QCA_NL80211_VENDOR_SUBCMD_*
#include <cfg80211_ven_cmd.h>  // for IEEE80211_PARAM_SPLITMAC etc
#include <ieee802_11_defs_copy.h>
#include <memory>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/handlers.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <nl80211_copy.h>    // for NL80211_CMD_*
#include <qca-vendor_copy.h> // for QCA_WLAN_VENDOR_ATTR_*
#include <stdexcept>
#include <wlanapp_commands.hpp>

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

  static NetlinkMessage CreateGetSplitmacMsg(const std::string &iface,
                                             int nl_family_id) {
    // look how easy and beautiful it is to build a nl message <3
    NetlinkMessage nlmsg;
    nlmsg.put_header(NL80211_CMD_VENDOR, nl_family_id)
        .put_vendor_id(QCA_OUI)
        .put_vendor_subcmd(GET_PARAM)
        .put_iface_idx(iface)
        .start_vendor_attr_block()
        .put_attr<u32>(QCA_WLAN_VENDOR_ATTR_CONFIG_GENERIC_VALUE,
                       IEEE80211_PARAM_SPLITMAC)
        .put_attr<u32>(QCA_WLAN_VENDOR_ATTR_CONFIG_GENERIC_COMMAND,
                       QCA_NL80211_VENDOR_SUBCMD_WIFI_PARAMS)
        .end_vendor_attr_block();
    return nlmsg;
  }

  static NetlinkMessage CreateSetSplitmacMsg(const std::string &iface,
                                             u32 new_value, int nl_family_id) {
    NetlinkMessage nlmsg;
    nlmsg.put_header(NL80211_CMD_VENDOR, nl_family_id)
        .put_vendor_id(QCA_OUI)
        .put_vendor_subcmd(SET_PARAM)
        .put_iface_idx(iface)
        .start_vendor_attr_block()
        .put_attr<u32>(QCA_WLAN_VENDOR_ATTR_CONFIG_GENERIC_VALUE,
                       IEEE80211_PARAM_SPLITMAC)
        .put_attr<u32>(QCA_WLAN_VENDOR_ATTR_CONFIG_GENERIC_COMMAND,
                       QCA_NL80211_VENDOR_SUBCMD_WIFI_PARAMS)
        .put_attr<u32>(QCA_WLAN_VENDOR_ATTR_CONFIG_GENERIC_DATA, new_value)
        .end_vendor_attr_block();
    return nlmsg;
  }

  static void parse_nl_attribute(struct nlattr *tb_vendor) {
    spdlog::info("value: {}", *(uint32_t *)nla_data(tb_vendor));
  }

  static int parse_vendor_data_attr(u8 *vendor_data, int vendor_data_len,
                                    int max_attr) {
    struct nlattr *tb_vendor[MAX_ATTR];
    int ret = 0;
    ret = nla_parse(tb_vendor, max_attr, (struct nlattr *)vendor_data,
                    vendor_data_len, NULL);
    if (ret != 0) {
      throw std::runtime_error(fmt::format(
          "nla_parse() for incoming response failed with code {}", ret));
    }
    for (int i = 0; i <= max_attr; ++i) {
      if (tb_vendor[i] != nullptr) {
        spdlog::debug("Match on vendor attr with idx {}", i);
        parse_nl_attribute(tb_vendor[i]);
      }
    }
    return 0;
  }

  static nl_cb_action parse_vendor_data_response(nlmsg_raw_ptr msg, void *ctx) {
    int max_resp_attr = *static_cast<int *>(ctx);
    nl_cb_action status = NL_OK;
    struct nlattr *nl_attrs[NL80211_ATTR_MAX_INTERNAL + 1] = {0};
    struct genlmsghdr *nl_header =
        (struct genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));

    int ret = nla_parse(nl_attrs, NL80211_ATTR_MAX_INTERNAL,
                        genlmsg_attrdata(nl_header, 0),
                        genlmsg_attrlen(nl_header, 0), NULL);
    if (ret != 0) {
      throw std::runtime_error(
          fmt::format("nla_parse() failed with retcode {}", ret));
    }

    // for (int i = 0; i < NL80211_ATTR_MAX; i++) {
    //   if (nl_attrs[i] != nullptr) {
    //     spdlog::debug("Response message has non-empty attrbute idx {}", i);
    //   }
    // }

    struct nlattr *vendor_data_attr = nl_attrs[NL80211_ATTR_VENDOR_DATA];
    if (vendor_data_attr == nullptr) {
      spdlog::debug("Vendor data not found in response, skipping");
      status = NL_SKIP;
    } else {
      spdlog::debug(
          "Response has non-empty attribute NL80211_ATTR_VENDOR_DATA ({})",
          (int)NL80211_ATTR_VENDOR_DATA);
      u8 *vendor_data = (u8 *)nla_data(vendor_data_attr);
      if (vendor_data == nullptr) {
        throw std::runtime_error("Failed to get vendor data");
      }
      int vendor_data_len = nla_len(vendor_data_attr);
      spdlog::debug("Vendor data parse success, len={}", vendor_data_len);

      int res =
          parse_vendor_data_attr(vendor_data, vendor_data_len, max_resp_attr);
    }
    return NL_STOP;
  }

  static nl_cb_action parse_mcast_group_response(struct nl_msg *msg,
                                                 void *arg) {
    FamilyCmdContext *res = static_cast<FamilyCmdContext *>(arg);
    struct nlattr *nl_attrs[CTRL_ATTR_MAX + 1];
    struct genlmsghdr *header = (struct genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *mcast_group;

    nla_parse(nl_attrs, CTRL_ATTR_MAX, genlmsg_attrdata(header, 0),
              genlmsg_attrlen(header, 0), NULL);
    if (nl_attrs[CTRL_ATTR_MCAST_GROUPS] == nullptr) {
      spdlog::error("no multicast groups in the message, skipping");
      return NL_SKIP;
    }

    int none;
    nla_for_each_nested(mcast_group, nl_attrs[CTRL_ATTR_MCAST_GROUPS], none) {
      struct nlattr *mcast_group_attrs[CTRL_ATTR_MCAST_GRP_MAX + 1];
      nla_parse(mcast_group_attrs, CTRL_ATTR_MCAST_GRP_MAX,
                (struct nlattr *)nla_data(mcast_group), nla_len(mcast_group),
                NULL);
      if (mcast_group_attrs[CTRL_ATTR_MCAST_GRP_NAME] != nullptr and
          mcast_group_attrs[CTRL_ATTR_MCAST_GRP_ID] != nullptr) {
        std::string group_name{
            nla_get_string(mcast_group_attrs[CTRL_ATTR_MCAST_GRP_NAME])};
        if (group_name == res->group) {
          res->id = (int)nla_get_u32(mcast_group_attrs[CTRL_ATTR_MCAST_GRP_ID]);
          spdlog::debug("found group name {} with id {}", group_name, res->id);
          return NL_STOP;
        }
      }
    };

    return NL_SKIP;
  }

  static nl_cb_action parse_vendor_event(struct nl_msg *msg, void *arg) {
    spdlog::debug("Start parsing netlink event");
    struct genlmsghdr *header = (genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *nl_attrs[NL80211_ATTR_MAX + 1];
    struct nlattr *tb_array[QCA_WLAN_VENDOR_ATTR_CONFIG_MAX + 1];
    struct nlattr *vendor_generic_data_attr;

    nla_parse(nl_attrs, NL80211_ATTR_MAX, genlmsg_attrdata(header, 0),
              genlmsg_attrlen(header, 0), NULL);

    try {
      if (header->cmd != NL80211_CMD_VENDOR) {
        throw std::invalid_argument(
            fmt::format("event cmd != NL_CMD_VENDOR, skipping"));
      }

      // this attr is guaranteed to be present
      u32 vendor_id = nla_get_u32(nl_attrs[NL80211_ATTR_VENDOR_ID]);
      if (vendor_id != OUI_QCA) {
        throw std::invalid_argument(fmt::format(
            "VENDOR_ID ({:x}) != {:x}, skipping", vendor_id, OUI_QCA));
      }

      // this attr is guaranteed to be present
      u32 vendor_subcmd = nla_get_u32(nl_attrs[NL80211_ATTR_VENDOR_SUBCMD]);
      if (vendor_subcmd != QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_CONFIGURATION) {
        throw std::invalid_argument(
            fmt::format("VENDOR_SUBCMD ({}) != {}, skipping", vendor_subcmd,
                        (int)QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_CONFIGURATION));
      }

      if (nl_attrs[NL80211_ATTR_IFINDEX] == nullptr) {
        throw std::invalid_argument(fmt::format("IFINDEX not found"));
      }
      u32 ifidx = nla_get_u32(nl_attrs[NL80211_ATTR_IFINDEX]);

      if (nl_attrs[NL80211_ATTR_VENDOR_DATA] == nullptr) {
        throw std::invalid_argument(fmt::format("VENDOR_DATA not found"));
      }
      u8 *vendor_data = (u8 *)nla_data(nl_attrs[NL80211_ATTR_VENDOR_DATA]);
      int vendor_data_len = nla_len(nl_attrs[NL80211_ATTR_VENDOR_DATA]);

      int ret = nla_parse(tb_array, QCA_WLAN_VENDOR_ATTR_CONFIG_MAX,
                          (struct nlattr *)vendor_data, vendor_data_len, NULL);
      if (ret < 0) {
        throw std::invalid_argument(
            fmt::format("failed to parse VENDOR_DATA not found"));
      }

      u32 vendor_generic_command =
          nla_get_u32(tb_array[QCA_WLAN_VENDOR_ATTR_CONFIG_GENERIC_COMMAND]);

      if (vendor_generic_command != QCA_NL80211_VENDOR_SUBCMD_FWD_MGMT_FRAME) {
        throw std::invalid_argument(
            fmt::format("Expected vendor generic command {}, got {}",
                        (int)QCA_NL80211_VENDOR_SUBCMD_FWD_MGMT_FRAME,
                        vendor_generic_command));
      }

      vendor_generic_data_attr =
          tb_array[QCA_WLAN_VENDOR_ATTR_CONFIG_GENERIC_DATA];

      if (vendor_generic_data_attr == nullptr) {
        throw std::invalid_argument(
            fmt::format("Vendor generic data not found"));
      }
      void *frame = nla_data(vendor_generic_data_attr);
      int frame_len = nla_len(vendor_generic_data_attr);
      NetlinkMessage::parse_wifi_frame(frame, frame_len);
    } catch (std::invalid_argument &exc) {
      spdlog::error("Event payload is invalid: {}, skipping this message",
                    exc.what());
    }
    spdlog::debug("Done parsing event");
    return NL_SKIP;
  }

  static std::string mac_to_str(const u8 mac[6]) {
    return fmt::format("{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}", mac[0],
                       mac[1], mac[2], mac[3], mac[4], mac[5]);
  }

  static void parse_wifi_frame(void *buf, int buflen) {
    if (buflen < IEEE80211_HDRLEN) {
      spdlog::error("Frame size too small: {}, expected at least {}, skipping",
                    buflen, IEEE80211_HDRLEN);
      return;
    }

    const struct ieee80211_mgmt *mgmt =
        static_cast<const struct ieee80211_mgmt *>(buf);

    u16 fc = (mgmt->frame_control);

    if (WLAN_FC_GET_TYPE(fc) != WLAN_FC_TYPE_MGMT) {
      spdlog::error("frame type is not mgmt, skipping");
      return;
    }

    u16 stype = WLAN_FC_GET_STYPE(fc);

    switch (stype) {
    case WLAN_FC_STYPE_BEACON: {
      spdlog::debug("**********************");
      spdlog::debug("Type: beacon frame");
      spdlog::debug("{} -> {}", mac_to_str(mgmt->sa), mac_to_str(mgmt->da));
      spdlog::debug("**********************");
      break;
    }
    case WLAN_FC_STYPE_PROBE_REQ: {
      spdlog::debug("**********************");
      spdlog::debug("Type: probe req");
      spdlog::debug("{} -> {}", mac_to_str(mgmt->sa), mac_to_str(mgmt->da));
      spdlog::debug("**********************");
      break;
    }
    case WLAN_FC_STYPE_PROBE_RESP:
      spdlog::debug("**********************");
      spdlog::debug("Type: probe resp");
      spdlog::debug("{} -> {}", mac_to_str(mgmt->sa), mac_to_str(mgmt->da));
      spdlog::debug("**********************");
      break;
    case WLAN_FC_STYPE_AUTH: {
      spdlog::info("**********************");
      spdlog::info("Type: auth frame");
      spdlog::info("{} -> {}", mac_to_str(mgmt->sa), mac_to_str(mgmt->da));
      spdlog::info("**********************");
      break;
    }
    case WLAN_FC_STYPE_DEAUTH: {
      spdlog::info("**********************");
      spdlog::info("Type: deauth frame");
      spdlog::info("{} -> {}", mac_to_str(mgmt->sa), mac_to_str(mgmt->da));
      spdlog::info("**********************");
      break;
    }
    case WLAN_FC_STYPE_DISASSOC: {
      spdlog::info("**********************");
      spdlog::info("Type: disassoc frame");
      spdlog::info("{} -> {}", mac_to_str(mgmt->sa), mac_to_str(mgmt->da));
      spdlog::info("**********************");
      break;
    }
    case WLAN_FC_STYPE_ACTION: {
      spdlog::info("**********************");
      spdlog::info("Type: action frame");
      spdlog::info("{} -> {}", mac_to_str(mgmt->sa), mac_to_str(mgmt->da));
      spdlog::info("**********************");
      break;
    }
    case WLAN_FC_STYPE_ASSOC_REQ: {
      spdlog::info("**********************");
      spdlog::info("Type: assoc req");
      spdlog::info("{} -> {}", mac_to_str(mgmt->sa), mac_to_str(mgmt->da));
      spdlog::info("**********************");
      break;
    }
    case WLAN_FC_STYPE_ASSOC_RESP:
      spdlog::info("**********************");
      spdlog::info("{} -> {}", mac_to_str(mgmt->sa), mac_to_str(mgmt->da));
      spdlog::info("**********************");
      spdlog::info("Type: assoc resp");
      break;
    case WLAN_FC_STYPE_REASSOC_REQ:
      spdlog::info("**********************");
      spdlog::info("Type: reassoc req");
      spdlog::info("{} -> {}", mac_to_str(mgmt->sa), mac_to_str(mgmt->da));
      spdlog::info("**********************");
      break;
    case WLAN_FC_STYPE_REASSOC_RESP:
      spdlog::info("**********************");
      spdlog::info("Type: reassoc resp");
      spdlog::info("{} -> {}", mac_to_str(mgmt->sa), mac_to_str(mgmt->da));
      spdlog::info("**********************");
      break;
    default:
      spdlog::info("**********************");
      spdlog::info("Frame subtype not covered: {}", (int)stype);
      spdlog::info("{} -> {}", mac_to_str(mgmt->sa), mac_to_str(mgmt->da));
      spdlog::info("**********************");
      break;
    }
  }
};
