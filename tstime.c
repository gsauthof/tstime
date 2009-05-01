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

static char message[1024];

int print_time_mem(struct taskstats *t)
{
  pp_taskstats(message, 1024, t);
  return 0;
}

#define CHECK_ERR(a) \
  if (a<0) { \
    fprintf(stderr, "%s:%d ", __FILE__, __LINE__); \
    perror(0); \
    exit(23); \
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
  fprintf(stderr, "\n");
  int code = 42;
  int status;
  pid_t r = waitpid(pid, &status, 0); CHECK_ERR(r);
  if (WIFEXITED(status)) {
    fprintf(stderr, "Exit status: %d\t", WEXITSTATUS(status));
    code = WEXITSTATUS(status);
  }
  if (WIFSIGNALED(status))
    fprintf(stderr, "Signal: %d\t", WTERMSIG(status));
  // _BSD_SOURCE
  //if (WCOREDUMP(status))
  //  fprintf(stderr, "coredumped");
  fprintf(stderr, "\n");
  return code;
}

void help(char *s)
{
  printf("%s COMMAND OPTIONS*\n\n"
      "\texecutes COMMAND and prints its runtime and highwater mem usage\n\n"
      "(uses the taskstat delay accounting API of the Linux Kernel 2.6)\n",
      s);
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
  int start_arg = 1;
  dbg = 0;

  int r = 0;
  struct ts_t t;
  r = ts_init(&t); CHECK_ERR(r);

  //install_chld_handler();

 
  char cpumask[100];
  gen_cpumask(cpumask, 100);
  r = ts_set_cpus(&t, cpumask); CHECK_ERR(r);


  pid_t pid = fork(); CHECK_ERR(pid);
  if (!pid) {
    execvp(argv[start_arg], argv+start_arg);
    perror("execvp failed");
    return 23;
  }
  //pause();
  //r = ts_set_pid(&t, pid); CHECK_ERR(r);

  r = ts_wait(&t, pid, print_time_mem); CHECK_ERR(r);
  //r = ts_wait(&t, 0, print_time_mem); CHECK_ERR(r);

  r = wait_for_child(pid);
  if (r != 23) {
    fprintf(stderr, "%s", message);
  }

  ts_finish(&t);
  return r;
}

