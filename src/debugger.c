#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <termios.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include "files.h"
#include "jobs.h"

void typeout_pc(struct job *j)
{
  long long pc = ptrace(PTRACE_PEEKUSER, j->proc.pid, RIP * 8, NULL);
  long long data = ptrace(PTRACE_PEEKDATA, j->proc.pid, pc, NULL);

  fprintf(stderr, "%llx)   %llx   ", pc, data);
}

void step_job(struct job *j)
{
  int status;
  if (ptrace(PTRACE_SINGLESTEP, j->proc.pid, NULL, NULL) == -1)
    errout("ptrace");
  check_jobs();
}
