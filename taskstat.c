/*
 * taskstat.c
 *
 * GPL 2+
 *
 * Georg Sauthoff 2009
 *
 * copy pasted needed stuff to do the taskstats delay accounting communication
 * from the getdelays.c, which is an example distributed with the Linux Kernel
 *
 * added the ts_* wrapper to make a change to a libnl 'backend' easy
 *
 * so far failed to use libnl in a proper way (see nl.c for a try)
 *
 */

/* getdelays.c
 *
 * Utility to get per-pid and per-tgid delay accounting statistics
 * Also illustrates usage of the taskstats interface
 *
 * Copyright (C) Shailabh Nagar, IBM Corp. 2005
 * Copyright (C) Balbir Singh, IBM Corp. 2006
 * Copyright (c) Jay Lan, SGI. 2006
 *
 * Compile with
 *	gcc -I/usr/src/linux/include getdelays.c -o getdelays
 */

#include "taskstat.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

#include <linux/genetlink.h>
#include <linux/taskstats.h>
#include <linux/cgroupstats.h>



int dbg;

/*
 * Generic macros for dealing with netlink sockets. Might be duplicated
 * elsewhere. It is recommended that commercial grade applications use
 * libnl or libnetlink and use the interfaces provided by the library
 */
#define GENLMSG_DATA(glh)	((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define GENLMSG_PAYLOAD(glh)	(NLMSG_PAYLOAD(glh, 0) - GENL_HDRLEN)
#define NLA_DATA(na)		((void *)((char*)(na) + NLA_HDRLEN))
#define NLA_PAYLOAD(len)	(len - NLA_HDRLEN)

#define err(code, fmt, arg...)			\
	do {					\
		fprintf(stderr, fmt, ##arg);	\
		exit(code);			\
	} while (0)


/* Maximum size of response requested or message sent */
#define MAX_MSG_SIZE	1024
/* Maximum number of cpus expected to be specified in a cpumask */
#define MAX_CPUS	32

struct msgtemplate {
	struct nlmsghdr n;
	struct genlmsghdr g;
	char buf[MAX_MSG_SIZE];
};

/*
 * Create a raw netlink socket and bind
 */
static int create_nl_socket(int protocol)
{
	int fd;
	struct sockaddr_nl local;

	fd = socket(AF_NETLINK, SOCK_RAW, protocol);
	if (fd < 0)
		return -1;


	memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;

	if (bind(fd, (struct sockaddr *) &local, sizeof(local)) < 0)
		goto error;

	return fd;
error:
	close(fd);
	return -1;
}


static int send_cmd(int sd, __u16 nlmsg_type, __u32 nlmsg_pid,
	     __u8 genl_cmd, __u16 nla_type,
	     void *nla_data, int nla_len)
{
	struct nlattr *na;
	struct sockaddr_nl nladdr;
	int r, buflen;
	char *buf;

	struct msgtemplate msg;

	msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
	msg.n.nlmsg_type = nlmsg_type;
	msg.n.nlmsg_flags = NLM_F_REQUEST;
	msg.n.nlmsg_seq = 0;
	msg.n.nlmsg_pid = nlmsg_pid;
	msg.g.cmd = genl_cmd;
	msg.g.version = 0x1;
	na = (struct nlattr *) GENLMSG_DATA(&msg);
	na->nla_type = nla_type;
	na->nla_len = nla_len + 1 + NLA_HDRLEN;
	memcpy(NLA_DATA(na), nla_data, nla_len);
	msg.n.nlmsg_len += NLMSG_ALIGN(na->nla_len);

	buf = (char *) &msg;
	buflen = msg.n.nlmsg_len ;
	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;
	while ((r = sendto(sd, buf, buflen, 0, (struct sockaddr *) &nladdr,
			   sizeof(nladdr))) < buflen) {
		if (r > 0) {
			buf += r;
			buflen -= r;
		} else if (errno != EAGAIN)
			return -1;
	}
	return 0;
}


/*
 * Probe the controller in genetlink to find the family id
 * for the TASKSTATS family
 */
static int get_family_id(int sd)
{
	struct {
		struct nlmsghdr n;
		struct genlmsghdr g;
		char buf[256];
	} ans;

	int id = 0, rc;
	struct nlattr *na;
	int rep_len;

        char name[100];
	strcpy(name, TASKSTATS_GENL_NAME);
	rc = send_cmd(sd, GENL_ID_CTRL, getpid(), CTRL_CMD_GETFAMILY,
			CTRL_ATTR_FAMILY_NAME, (void *)name,
			strlen(TASKSTATS_GENL_NAME)+1);

	rep_len = recv(sd, &ans, sizeof(ans), 0);
	if (ans.n.nlmsg_type == NLMSG_ERROR ||
	    (rep_len < 0) || !NLMSG_OK((&ans.n), rep_len))
		return 0;

	na = (struct nlattr *) GENLMSG_DATA(&ans);
	na = (struct nlattr *) ((char *) na + NLA_ALIGN(na->nla_len));
	if (na->nla_type == CTRL_ATTR_FAMILY_ID) {
		id = *(__u16 *) NLA_DATA(na);
	}
	return id;
}



int ts_init(ts_t *t)
{
  ts_t empty = { 0 };
  *t = empty;
  int nl_sd = -1;
  if ((nl_sd = create_nl_socket(NETLINK_GENERIC)) < 0)
    err(1, "error creating Netlink socket\n");

  __u16 id = get_family_id(nl_sd);
  if (!id) {
    fprintf(stderr, "Error getting family id, errno %d\n", errno);
    return -1;
  }
  t->nl_sd = nl_sd;
  t->id = id;
  return 0;
}

int ts_set_cpus(ts_t *t, char *cpumask)
{
  int nl_sd = t->nl_sd;
  __u16 id = t->id;
  __u32 mypid = getpid();
  int rc = send_cmd(nl_sd, id, mypid, TASKSTATS_CMD_GET,
      TASKSTATS_CMD_ATTR_REGISTER_CPUMASK,
      cpumask, strlen(cpumask) + 1);
  PRINTF("Sent register cpumask %s, retval %d\n", cpumask, rc);
  if (rc < 0) {
    fprintf(stderr, "error sending register cpumask\n");
    return -1;
  }
  t->cpumask = cpumask;
  t->mypid = mypid;
  return 0;
}

int ts_set_pid(ts_t *t, pid_t tid)
{
  int nl_sd = t->nl_sd;
  __u16 id = t->id;
  __u32 mypid = t->mypid;
  int cmd_type = TASKSTATS_CMD_ATTR_PID;
  int rc = send_cmd(nl_sd, id, mypid, TASKSTATS_CMD_GET,
      cmd_type, &tid, sizeof(__u32));
  PRINTF("Sent pid/tgid, retval %d\n", rc);
  if (rc < 0) {
    fprintf(stderr, "error sending tid/tgid cmd\n");
    return -1;
  }
  return 0;
}

int ts_wait(ts_t *t, pid_t pid, int (*callback)(struct taskstats *) )
{
  int nl_sd = t->nl_sd;
  __u16 id = t->id;
  int maskset = 0;
  char *cpumask = 0;
  if (t->cpumask) {
    maskset = 1;
    cpumask = t->cpumask;
  }
  int count = 0;
  int loop = 1;
  do {
    struct msgtemplate msg;
    int rep_len = recv(nl_sd, &msg, sizeof(msg), 0);
    PRINTF("received %d bytes\n", rep_len);

    if (rep_len < 0) {
      fprintf(stderr, "nonfatal reply error: errno %d\n",
          errno);
      continue;
    }
    if (msg.n.nlmsg_type == NLMSG_ERROR ||
        !NLMSG_OK((&msg.n), rep_len)) {
      struct nlmsgerr *err = NLMSG_DATA(&msg);
      fprintf(stderr, "fatal reply error,  errno %d\n",
          err->error);
      goto done;
    }

    PRINTF("nlmsghdr size=%zu, nlmsg_len=%d, rep_len=%d\n",
        sizeof(struct nlmsghdr), msg.n.nlmsg_len, rep_len);


    rep_len = GENLMSG_PAYLOAD(&msg.n);

    struct nlattr *na = (struct nlattr *) GENLMSG_DATA(&msg);
    int len = 0;
    while (len < rep_len) {
      len += NLA_ALIGN(na->nla_len);
      switch (na->nla_type) {
        case TASKSTATS_TYPE_AGGR_TGID:
          /* Fall through */
        case TASKSTATS_TYPE_AGGR_PID:
          {
          int aggr_len = NLA_PAYLOAD(na->nla_len);
          int len2 = 0;
          /* For nested attributes, na follows */
          na = (struct nlattr *) NLA_DATA(na);
          while (len2 < aggr_len) {
            switch (na->nla_type) {
              case TASKSTATS_TYPE_PID:
                {
                //pid_t rtid = *(int *) NLA_DATA(na);
                //if (print_delays)
                //  ; //printf("PID\t%d\n", rtid);
                }
                break;
              case TASKSTATS_TYPE_TGID:
                {
                //rtid = *(int *) NLA_DATA(na);
                //if (print_delays)
                //  printf("TGID\t%d\n", rtid);
                }
                break;
              case TASKSTATS_TYPE_STATS:
                count++;
                struct taskstats * ts = (struct taskstats *) NLA_DATA(na);
                if (!pid || ts->ac_pid == pid)
                  loop = callback(ts);
                if (!loop)
                  goto done;
                break;
              default:
                fprintf(stderr, "Unknown nested"
                    " nla_type %d\n",
                    na->nla_type);
                break;
            }
            len2 += NLA_ALIGN(na->nla_len);
            na = (struct nlattr *) ((char *) na + len2);
          }
          }
          break;
        case CGROUPSTATS_TYPE_CGROUP_STATS:
          //print_cgroupstats(NLA_DATA(na));
          break;
        default:
          fprintf(stderr, "Unknown nla_type %d\n",
              na->nla_type);
          break;
      }
      na = (struct nlattr *) (GENLMSG_DATA(&msg) + len);
    }
  } while (loop);
done:
  if (maskset) {
    __u32 mypid = t->mypid;
    int rc = send_cmd(nl_sd, id, mypid, TASKSTATS_CMD_GET,
        TASKSTATS_CMD_ATTR_DEREGISTER_CPUMASK,
        cpumask, strlen(cpumask) + 1);
    PRINTF("Sent deregister mask, retval %d\n", rc);
    if (rc < 0)
      err(rc, "error sending deregister cpumask\n");
  }
  return 0;
}

void ts_finish(ts_t *t)
{
  close(t->nl_sd);
}


