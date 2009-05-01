#include "tools.h"

#include <sys/socket.h>
#include <linux/genetlink.h>
#include <linux/taskstats.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

void pp_taskstats(char *message, size_t len, struct taskstats *t)
{
  time_t btime = t->ac_btime;
  snprintf(message, len, "\npid: %u (%s) started: %s"
      "\treal %7.3f s, user %7.3f s, sys %7.3fs\n"
      "\trss %8llu kb, vm %8llu kb\n\n",
      t->ac_pid, t->ac_comm, ctime(&btime),
      t->ac_etime / 1000000.0,
      t->ac_utime / 1000000.0,
      t->ac_stime / 1000000.0,
      (unsigned long long) t->hiwater_rss,
      (unsigned long long) t->hiwater_vm

      );
}

void gen_cpumask(char *cpumask, size_t len)
{
  int cpus = sysconf(_SC_NPROCESSORS_CONF) - 1;
  snprintf(cpumask, len, "0-%d", cpus);
}
