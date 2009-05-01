#include "taskstat.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>

#include <sys/socket.h>
#include <linux/genetlink.h>
#include <linux/taskstats.h>

int print_time_mem(struct taskstats *t)
{
  time_t btime = t->ac_btime;
  printf("\npid: %u (%s ppid: %u) started: %s\trss: %15llu kb vm: %15llu kb \n"
      "\telapsed: %8.3f s user: %8.3f s sys: %8.3f s\n",
      t->ac_pid, t->ac_comm, t->ac_ppid, ctime(&btime),
      (unsigned long long) t->hiwater_rss,
      (unsigned long long) t->hiwater_vm,
      t->ac_etime / 1000000.0,
      t->ac_utime / 1000000.0,
      t->ac_stime / 1000000.0

      );
  return 0;
}

#define CHECK_ERR(a) if (a<0) { assert(0); exit(23); }

void child_exit(int i)
{
}

void install_chld_handler()
{
  struct sigaction sa = { 0 };
  sa.sa_handler = child_exit;
  sa.sa_flags = SA_NOCLDSTOP;
  int r = sigaction(SIGCHLD, &sa,  0); CHECK_ERR(r);
}

void wait_for_child(pid_t pid)
{
  int status;
  pid_t r = waitpid(pid, &status, 0); CHECK_ERR(r);
  if (WIFEXITED(status))
    fprintf(stderr, "Exit status: %d\t", WEXITSTATUS(status));
  if (WIFSIGNALED(status))
    fprintf(stderr, "Signal: %d\t", WTERMSIG(status));
  // _BSD_SOURCE
  //if (WCOREDUMP(status))
  //  fprintf(stderr, "coredumped");
  fprintf(stderr, "\n");
}

int main(int argc, char **argv)
{
  dbg = 1;
  //rcvbufsz = 0;

  int r = 0;
  struct ts_t t;
  r = ts_init(&t); CHECK_ERR(r);

  //install_chld_handler();

 
  char cpumask[100];
  int cpus = sysconf(_SC_NPROCESSORS_CONF);
  snprintf(cpumask, 100, "0-%d", cpus);
  fprintf(stderr, "#CPUS: %d\n", cpus);
  r = ts_set_cpus(&t, cpumask); CHECK_ERR(r);


  pid_t pid = fork(); CHECK_ERR(pid);
  if (!pid) {
    malloc(10 * 1024 * 1024);
    sleep(2);
    return 0;
  }
  //pause();
  //r = ts_set_pid(&t, pid); CHECK_ERR(r);

  r = ts_wait(&t, pid, print_time_mem); CHECK_ERR(r);
  //r = ts_wait(&t, 0, print_time_mem); CHECK_ERR(r);

  wait_for_child(pid);


  ts_finish(&t);
  return 0;
}

