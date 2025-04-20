#include <CLI/CLI.hpp>
#include <libnl++/genl.hpp>
#include <libnl++/socket.hpp>
#include <netlink/attr.h>
#include <spdlog/spdlog.h>

using nl::u32;
namespace GenlApp {

#define ATTR_MAX 2
constexpr int CMD_SERVER_REQUEST = 0;
constexpr int CMD_SERVER_RESPONSE = 1;
constexpr int ATTR_PAYLOAD = 0;

nl::callback_result_t parse_request(struct nl_msg *msg, void *ctx) {
  spdlog::debug("Received netlink message");
  struct genlmsghdr *genl_header = (genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
  struct nlattr *nl_attrs[ATTR_MAX];
  nla_parse(nl_attrs, ATTR_MAX, genlmsg_attrdata(genl_header, 0),
            genlmsg_attrlen(genl_header, 0), NULL);
  try {
    if (genl_header->cmd != GenlApp::CMD_SERVER_REQUEST) {
      throw std::invalid_argument(
          fmt::format("event cmd ({}) != CMD_SERVER_REQUEST ({}), skipping",
                      genl_header->cmd, CMD_SERVER_REQUEST));
    }
    struct nlattr *payload_attr = nl_attrs[ATTR_PAYLOAD];
    if (payload_attr != nullptr) {
      spdlog::info("Got non-empty payload, length {}", nla_len(payload_attr));
      spdlog::info("Payload string: {}", (char *)nla_data(payload_attr));
    }
  } catch (std::invalid_argument &exc) {
    spdlog::debug("Event payload is invalid: {}, skipping this message",
                  exc.what());
  }
  return NL_SKIP;
}

void server(u32 server_port) {
  // 1. register family
  /*nl::genl::Family::register_family(GenlApp::FAMILY_NAME, true);*/
  /*spdlog::debug("Registered family {}", GenlApp::FAMILY_NAME);*/
  // 2. create listening socket
  // TODO: create class nl::genl::Socket
  nl::Socket sock{NETLINK_USERSOCK, server_port};
  // 3. set listening port
  sock.set_local_port(server_port);
  spdlog::debug("Opened netlink socket with port {}", server_port);
  spdlog::debug("Waiting for recv...");
  // 4. wait for recv
  sock.recv_msg({parse_request, nullptr});
  spdlog::debug("Message received, shutting down...");
  // 5. check source pid
  // TODO
  // 6. assemble response
  // TODO
  // 7. send response
  // TODO
}

void client(u32 server_port, std::string &payload) {
  // 1. create socket with family name
  // TODO: create class nl::genl::Socket
  nl::Socket sock{NETLINK_USERSOCK};
  // 2. set socket peer port
  sock.set_peer_port(server_port);
  spdlog::debug("Opened netlink socket with peer port {}", server_port);
  // 3. assemble a request message
  nl::Message msg;
  msg.put_header(GenlApp::CMD_SERVER_REQUEST, NETLINK_GENERIC, server_port)
      .put_string(GenlApp::ATTR_PAYLOAD, payload);
  spdlog::debug("Assembled request message, sending...");
  // 4. send it
  sock.send_msg(msg);
  spdlog::debug("Message sent, shutting down...");
  // 5. wait for response
  // TODO
}

}; // namespace GenlApp

int main(int argc, char **argv) {
  spdlog::set_pattern("[%H:%M:%S.%e][%^%l%$] %v");
  spdlog::set_level(spdlog::level::debug);

  CLI::App app{"Generic Netlink client-server app"};

  u32 server_port;
  auto *server_subcmd = app.add_subcommand("server", "Run as server");
  server_subcmd->add_option("port", server_port, "Port number to listen on")
      ->required()
      ->check(CLI::PositiveNumber);

  std::string message;
  auto *client_subcmd = app.add_subcommand("client", "Run as client");
  client_subcmd
      ->add_option("port", server_port, "Server port number to connect to")
      ->required()
      ->check(CLI::PositiveNumber);
  client_subcmd->add_option("message", message, "Message to send to server")
      ->required();

  CLI11_PARSE(app, argc, argv);

  try {
    if (*server_subcmd) {
      spdlog::info("Starting server on port {}...", server_port);
      GenlApp::server(server_port);
    } else if (*client_subcmd) {
      GenlApp::client(server_port, message);
    } else {
      spdlog::error("Either 'server' or 'client' subcommand must be provided");
      std::cout << app.help() << '\n';
      return 1;
    }
  } catch (std::runtime_error &e) {
    spdlog::error("Error occured: {}", e.what());
    return 2;
  }
  return 0;
}
