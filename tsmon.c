#include "taskstat.h"
#include "tools.h"

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
  char message[200];
  pp_taskstats(message, 200, t);
  fprintf(stderr, "%s", message);
  return 1;
}

#define CHECK_ERR(a) \
  if (a<0) { \
    fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
    perror(0); \
    exit(23); \
  }


void help(char *s)
{
  printf("%s\n\n"
      "\tprints taskstats (time, mem) of exiting processes/threads"
      " on the system\n\n"
      "(uses the taskstat delay accounting API of the Linux Kernel 2.6)\n",
      s);
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

  r = ts_wait(&t, 0, print_time_mem); CHECK_ERR(r);

  ts_finish(&t);

  return 0;
}

