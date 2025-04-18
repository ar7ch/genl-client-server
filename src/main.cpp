#include <netlink/genl/mngt.h>

int main() {
  struct genl_ops ops = {.o_name = (char *)"ar7ch"};
  genl_register_family(&ops);
}
