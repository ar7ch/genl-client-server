/*#include "common.hpp"*/
/*#include "message.hpp"*/
/*#include "socket.hpp"*/
/*#include "spdlog/spdlog.h"*/
/*#include "util.hpp"*/
/*#include <iostream>*/
/*#include <netlink/genl/ctrl.h>*/
/*#include <netlink/genl/genl.h>*/
/*#include <netlink/handlers.h>*/
/*#include <netlink/netlink.h>*/
/*#include <netlink/socket.h>*/
/*#include <wlanapp_commands.hpp>*/

/*
 * Class that is used as an entrypoint for NL interaction. It forms a message
 * using NetlinkMessage and sends it using a NetlinkSocket.
 */
/*class NetlinkManager {*/
/*  NetlinkSocket cmd_sock;*/
/**/
/*public:*/
/*  void process_command(WlanCommands::CmdEntry &cmd) {*/
/*    if (cmd.type == WlanCommands::CmdType::GET_SPLITMAC or*/
/*        cmd.type == WlanCommands::CmdType::SET_SPLITMAC) {*/
/*      NetlinkMessage msg;*/
/*      if (cmd.type == WlanCommands::CmdType::GET_SPLITMAC) {*/
/*        spdlog::debug("Creating get_splitmac message");*/
/*        msg = NetlinkMessage::CreateGetSplitmacMsg(cmd.iface,*/
/*                                                   cmd_sock.get_nl_family_id());*/
/*      } else {*/
/*        spdlog::debug("Creating set_splitmac message");*/
/*        msg = NetlinkMessage::CreateSetSplitmacMsg(cmd.iface, cmd.new_value,*/
/*                                                   cmd_sock.get_nl_family_id());*/
/*      }*/
/*      spdlog::debug("Netlink message created successfully");*/
/*      cmd_sock.send_msg(msg);*/
/*      cmd_sock.recv_msg(*/
/*          {NetlinkMessage::parse_vendor_data_response,
 * &cmd.max_response_idx});*/
/*    } else if (cmd.type == WlanCommands::CmdType::RECV_FRAMES) {*/
/*      NetlinkSocket event_socket = NetlinkSocket::CreateEventSocket();*/
/*      spdlog::info("Event socket ready, listening for incoming events...");*/
/*      event_socket.recv_msg({NetlinkMessage::parse_vendor_event, nullptr});*/
/*    } else {*/
/*      throw std::logic_error("Unexpected WlanCommand type");*/
/*    }*/
/*    spdlog::info("Success");*/
/*  }*/
/*};*/
