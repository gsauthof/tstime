#ifndef TOOLS_H
#define TOOLS_H

#include <sys/types.h>

struct taskstats;

void pp_taskstats(char *message, size_t len, struct taskstats *t);
void gen_cpumask(char *cpumask, size_t len);

#endif
