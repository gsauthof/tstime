/*
 * Copyright Georg Sauthoff 2009, GPLv2+
 * Copyright Jason Pepas 2009, GPLv2+
 */

#include "taskstat.h"
#include "tools.h"
#include "version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <syslog.h>

#include <time.h>
#include <sys/socket.h>
#include <linux/genetlink.h>
#include <linux/taskstats.h>

int syslog_output(struct taskstats *t)
{
  char time_buf[26] = {0};
  time_t btime = t->ac_btime;
  size_t len = 0;

  ctime_r(&btime, time_buf);

  /* knock off the trailing newline supplied by ctime_r() */
  len = strlen(time_buf);
  if (len > 0)
    time_buf[len-1] = '\0';

  syslog(LOG_INFO, "uid: %u, %s (pid: %u), started: %s, real %7.3fs\n",
      t->ac_uid, t->ac_comm, t->ac_pid, time_buf,
      t->ac_etime / 1000000.0
      );
  return 1;
}

void help(const char *s)
{
  printf("%s\n\n"
      "\tprints taskstats (time, mem) of exiting processes/threads"
      " on the system\n\n"
      "(uses the taskstat delay accounting API of the Linux Kernel 2.6)\n\n", s);
}

int main(int argc, char **argv)
{
  if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
    help(argv[0]);
    return 0;
  }
  if (argc > 1) {
    help(argv[0]);
    return 1;
  }
  dbg = 0;

  int r = 0;
  struct ts_t t;
  r = ts_init(&t); CHECK_ERR(r);

  char cpumask[100];
  gen_cpumask(cpumask, 100);
  r = ts_set_cpus(&t, cpumask); CHECK_ERR(r);

  openlog("tslog", LOG_NDELAY, LOG_LOCAL2);
  syslog(LOG_NOTICE, "starting");

  r = ts_wait(&t, 0, syslog_output); CHECK_ERR(r);

  syslog(LOG_NOTICE, "exiting");
  closelog();
  ts_finish(&t);

  return 0;
}

