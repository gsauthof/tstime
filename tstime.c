/*
 * Copyright Georg Sauthoff 2009, GPLv2+
 */

#include "taskstat.h"
#include "tools.h"
#include "version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <inttypes.h>
#include <sys/socket.h>
#include <linux/genetlink.h>
#include <linux/taskstats.h>

#include <sys/prctl.h>

static char message[1024];

static int tab_out = 0;

int print_tab_time_mem(struct taskstats *t)
{
  snprintf(message, 1024,
	"%llu" ";%llu" ";%llu" ";%llu" ";%llu" ";\n",
      t->ac_etime, t->ac_utime, t->ac_stime, t->hiwater_rss, t->hiwater_vm);
  return 0;
}

int print_time_mem(struct taskstats *t)
{
  pp_taskstats(message, 1024, t);
  return 0;
}

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

int wait_for_child(pid_t pid)
{
  LOG(stderr, "\n");
  int code = 42;
  int status;
  pid_t r = waitpid(pid, &status, 0); CHECK_ERR(r);
  if (WIFEXITED(status)) {
    LOG(stderr, "Exit status: %d\t", WEXITSTATUS(status));
    code = WEXITSTATUS(status);
  }
  if (WIFSIGNALED(status))
    LOG(stderr, "Signal: %d\t", WTERMSIG(status));
  // _BSD_SOURCE
  //if (WCOREDUMP(status))
  //  fprintf(stderr, "coredumped");
  LOG(stderr, "\n");
  return code;
}

void help(const char *s)
{
  printf("options* COMMAND OPTIONS*\n\n"
      "\texecutes COMMAND and prints its runtime and highwater mem usage\n\n"
      "\t-t\t;-delim output\n\n"
      "(uses the taskstat delay accounting API of the Linux Kernel 2.6)\n\n"); 
}

int main(int argc, char **argv)
{
  message[0] = 0;
  if (argc == 1) {
    help(argv[0]);
    return 1;
  }
  if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
    help(argv[0]);
    return 0;
  }
  if (!strcmp(argv[1], "-t")) {
    tab_out = 1;
    ++argv;
  }
  int start_arg = 1;
  dbg = 0;

  int r = 0;
  struct ts_t t;
  r = ts_init(&t); CHECK_ERR_SIMPLE(r);

  //install_chld_handler();

 
  char cpumask[100];
  gen_cpumask(cpumask, 100);
  r = ts_set_cpus(&t, cpumask); CHECK_ERR(r);


  pid_t pid = fork(); CHECK_ERR(pid);
  if (!pid) {
    int r = prctl(PR_SET_PDEATHSIG, SIGTERM, 0, 0, 0);
    CHECK_ERR(r);
    r = execvp(argv[start_arg], argv+start_arg);
    CHECK_ERR(r);
  }
  //pause();
  //r = ts_set_pid(&t, pid); CHECK_ERR(r);

  if (tab_out) {
    r = ts_wait(&t, pid, print_tab_time_mem); CHECK_ERR(r);
  } else {
    r = ts_wait(&t, pid, print_time_mem); CHECK_ERR(r);
  }
  //r = ts_wait(&t, 0, print_time_mem); CHECK_ERR(r);

  r = wait_for_child(pid);
  if (r != 23) {
    fprintf(stderr, "%s", message);
  }

  ts_finish(&t);
  return r;
}

