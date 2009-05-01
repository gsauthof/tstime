
/*

http://people.ee.ethz.ch/~arkeller/linux/kernel_user_space_howto.html#ss3.3
file:///usr/share/doc/libnl-doc/html/group__nl.html#g01a2aad82350f867e704d5c696438b60
http://www.linuxjournal.com/article/7356
http://www.linuxfoundation.org/en/Net:Generic_Netlink_HOWTO#Userspace_Communication


   */



//FIXME
//#define NL_ERROR_ASSERT

 #include <netlink/netlink.h>
 #include <netlink/genl/genl.h>
 #include <netlink/genl/ctrl.h>
// #include <netlink/msg.h>

#include <linux/taskstats.h>

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>


#define CHECK_ERR(a, b) if (a<0) { fprintf(stderr, "ERROR: " b "\n"); nl_perror(0); exit(1); }

static int parse_cb(struct nl_msg *msg, void *arg)
{
  // NL_SKIP NL_STOP NL_OK
  // nl_error
  struct nlmsghdr *nlh = nlmsg_hdr(msg);
  if (nlh->nlmsg_type == NLMSG_ERROR) {
    fprintf(stderr, "NLMSG_ERROR\n");
    nl_msg_dump(msg, stderr);
    return NL_SKIP;
  } else {
    fprintf(stderr, "type %d\n", nlh->nlmsg_type);
  }
  return 0;
}

int main(int argc, char **argv)
{
  // FIXME
  //nl_debug = 3;

  struct nl_handle *sock = nl_handle_alloc();
  int r = genl_connect(sock);
  CHECK_ERR(r, "Couldn't connect");

  int family = genl_ctrl_resolve(sock, TASKSTATS_GENL_NAME);
  CHECK_ERR(family, "family error");
  fprintf(stderr, "family %d\n", family);

  pid_t pid = getpid();
  fprintf(stderr, "pid %d\n", pid);
  //NL_AUTO_PID
  struct nl_msg *msg = nlmsg_alloc();
  genlmsg_put(msg, pid, NL_AUTO_SEQ, family, 0,
      NLM_F_REQUEST, TASKSTATS_CMD_GET, 0x1);
  r = nla_put_string(msg, TASKSTATS_CMD_ATTR_REGISTER_CPUMASK, argv[1]);
  CHECK_ERR(r, "nla_put_string");
  r = nl_send_auto_complete(sock, msg);
  CHECK_ERR(r, "nl_send_auto_complete");
  nlmsg_free(msg);

  // ACK request is added by nl_send_auto_complete
  //r = nl_wait_for_ack(sock);
  //CHECK_ERR(r, "wait for ack");

  //r = nl_socket_modify_cb(sock, NL_CB_VALID, NL_CB_CUSTOM, parse_cb, NULL);
  r = nl_socket_modify_cb(sock, NL_CB_ACK, NL_CB_CUSTOM, parse_cb, NULL);
  //r = nl_socket_modify_cb(sock, NL_CB_VALID, NL_CB_DEBUG, parse_cb, NULL);
  CHECK_ERR(r, "modify_cb");

  fprintf(stderr, "receive start\n");
  r = nl_recvmsgs_default(sock);
  CHECK_ERR(r, "recvmsgs");
  fprintf(stderr, "receive end %d\n", r);

  struct nl_msg *dereg_msg = nlmsg_alloc();
  genlmsg_put(dereg_msg, pid, NL_AUTO_SEQ, family, 0,
      NLM_F_REQUEST, TASKSTATS_CMD_GET, 0x1);
  r = nla_put_string(dereg_msg, TASKSTATS_CMD_ATTR_DEREGISTER_CPUMASK, argv[1]);
  CHECK_ERR(r, "nla_put_string");
  r = nl_send_auto_complete(sock, dereg_msg);
  CHECK_ERR(r, "nl_send_auto_complete dereg");
  nlmsg_free(dereg_msg);

  //nl_close(sock);
  nl_handle_destroy(sock);
  return 0;
}
