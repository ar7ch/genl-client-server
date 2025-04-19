#include <libnl++/genl.hpp>
#include <libnl++/socket.hpp>

#define FAMILY_NAME "ar7ch"

void client(int server_port) {
  nl::genl::Family::register_family(FAMILY_NAME);
  nl::NetlinkSocket sock(FAMILY_NAME);
}

/*void server(int local_port, std::string op, std::string a, std::string b) {}*/

int main() {
  // genl-app server 5000
  // genl-app client add 1 2
}
