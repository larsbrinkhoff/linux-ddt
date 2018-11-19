#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <termios.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include "jobs.h"

uint64_t qreg = 0;

void typeout_pc(struct job *j)
{
  long pc = ptrace(PTRACE_PEEKUSER, j->proc.pid, RIP * 8, NULL);
  long data = ptrace(PTRACE_PEEKDATA, j->proc.pid, pc, NULL);

  fprintf(stderr, "%lx)   %lx   ", pc, data);
}

void step_job(struct job *j)
{
  int status;
  if (ptrace(PTRACE_SINGLESTEP, j->proc.pid, NULL, NULL) == -1)
    errout("ptrace");
  check_jobs();
}
