/*
SPDX-License-Identifier: GPL-3.0-or-later

This file is part of Linux-ddt.

Linux-ddt is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

Linux-ddt is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Linux-ddt. If not, see <https://www.gnu.org/licenses/>.
*/
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <ctype.h>
#include "jobs.h"
#include "user.h"
#include "term.h"
#include "debugger.h"

#define MAXJOBS 8
#define MAXARGS 256

#define EXPECT_STOP 1

struct job jobs[MAXJOBS] = { 0 };
struct job *jobsend = &jobs[MAXJOBS];
struct job *currjob = 0;
struct job *fg = 0;

int clobrf = 1;
int genjfl = 1;

static char errstr[64];
static int pfd1[2], pfd2[2];

static void crlf(void)
{
  fputs("\r\n", stderr);
}

void jobs_init(void)
{
  if (pipe(pfd1) < 0 || pipe(pfd2) < 0) {
    fputs("failed creating pipes\r\n", stderr);
    exit(1);
  }
}

void errout(char *arg)
{
  int errno_ = errno;
  errstr[0] = 0;
  char *e = strerror_r(errno_, errstr, 64);
  if (arg)
    fprintf(stderr, " %s:", arg);
  fprintf(stderr, " %s\r\n", e);
}

static struct job *getjob(char *jname)
{
  for (struct job *j = jobs; j < jobsend; j++)
    if (j->state && strcmp(j->jname, jname) == 0)
      return j;
  return 0;
}

static char *nextuniq(char *jname)
{
  int l = strlen(jname);
  char *nstr;
  unsigned int found = -1;
  if ((nstr = malloc(l+2)) == NULL)
    return NULL;
  for (struct job *j = jobs; j < jobsend; j++)
    {
      if (!j->state)
	continue;
      if ((strlen(j->jname)-1) == l
	  && isdigit(j->jname[l]))
	found ^= 1 << (j->jname[l] - '0');
    }
  strcpy(nstr, jname);
  nstr[l++] = '0' + __builtin_ctz(found);
  nstr[l] = 0;

  return nstr;
}

static char nextslot(void)
{
  for (char i = 0; i < MAXJOBS; i++)
    if (jobs[i].state != 0)
      return i;
  return -1;
}

static char getopenslot(void)
{
  for (char i = 0; i < MAXJOBS; i++)
    if (jobs[i].state == 0)
      return i;
  return -1;
}

void set_currjname (char *jname)
{
  if (currjob->jname) free(currjob->jname);
  currjob->jname = strdup(jname);
}

void show_currjob (char *arg)
{
  if (currjob)
    {
      if (*arg)
	set_currjname(arg);
      else
	list_currjob();
    }
  else if (*arg)
    fputs("Would set self name\r\n", stderr);
}

void list_currjob(void)
{
  if (currjob)
    fprintf(stderr, "\r\n%c %s %c %d\r\n",
	    '*',
	    currjob->jname, currjob->state, currjob->slot);
}

void next_job(void)
{
  for (struct job *j = jobs; j < jobsend; j++)
    if (j->state != 0 && j != currjob)
      {
	currjob = j;
	fprintf(stderr, " %s$j\r\n", currjob->jname);
	break;
      }
}

void listj(char *arg)
{
  crlf();
  for (struct job *j = jobs; j < jobsend; j++)
    if (j->state != 0)
      fprintf(stderr, "%c %s %c %d\r\n",
	      (j != currjob)?' ':'*',
	      j->jname, j->state, j->slot);
}

static struct job *initslot(char slot, char *jname)
{
  struct job *j = &jobs[slot];

  j->jname = strdup(jname);
  j->xjname = strdup(jname);
  j->jcl = NULL;
  j->state = '-';
  j->slot = slot;
  j->tmode = def_termios;
  j->proc.ufname.name = NULL;
  j->proc.ufname.devfd = -1;
  j->proc.ufname.dirfd = -1;
  j->proc.ufname.fd = -1;
  j->proc.argv = malloc(sizeof(char *) * 2);
  j->proc.argv[0] = NULL;
  j->proc.argv[1] = NULL;
  j->proc.env = malloc(sizeof(char *) * 2);
  j->proc.env[0] = NULL;
  j->proc.env[1] = NULL;
  j->proc.syms = 0;
  j->proc.symlen = 0;
  j->proc.pid = 0;
  j->proc.status = 0;
  j->tperce = mperce;
  j->tamper = mamper;
  j->tdollar = mdolla;
  j->tdquote = mdquot;
  j->tprime = mprime;
  j->tnmsgn = mnmsgn;

  return j;
}

void select_job(char *jname)
{
  struct job *j;
  char slot;
  if ((j = getjob(jname)))
    {
      currjob = j;
      crlf();
      return;
    }

  if ((slot = getopenslot()) != -1)
    {
      currjob = initslot(slot, jname);
      fputs("\r\n!\r\n", stderr);
    }
  else
    fprintf(stderr, " %d jobs already? ", MAXJOBS);
}

static void free_job(struct job *j)
{
  if (j->jname) free(j->jname);
  if (j->xjname) free(j->xjname);
  if (j->jcl) free(j->jcl);
  if (j->proc.ufname.name) free(j->proc.ufname.name);
  if (j->proc.argv) free(j->proc.argv);
  // if (j->proc.env) free(j->proc.env);
  if (j->proc.ufname.fd != -1)
    close(j->proc.ufname.fd);
  if (j->proc.syms)
    unload_symbols(j);

  j->jname = 0;
  j->xjname = 0;
  j->jcl = 0;
  j->state = 0;
  j->proc.ufname.name = 0;
  j->proc.syms = 0;
  j->proc.symlen = 0;
  j->proc.argv = 0;
}

static void jobwait(struct job *j, int expect, int sig)
{
  int status = 0;

  waitpid(j->proc.pid, &status, WUNTRACED|WCONTINUED);
  if (WIFEXITED(status))
    {
      if (WEXITSTATUS(status))
	fprintf(stderr, ":exit %d\r\n", WEXITSTATUS(status));
      free_job(j);
    }
  else if (WIFSIGNALED(status))
    {
      fprintf(stderr, ":kill %d\r\n", WTERMSIG(status));
      free_job(j);
    }
  else if (WIFSTOPPED(status))
    {
      if (!(expect & EXPECT_STOP && sig == WSTOPSIG(status)))
	fprintf(stderr, ":stop signal=%d\r\n", WSTOPSIG(status));
      j->state = 'p';
    }
  else
    fprintf(stderr, " wait status=%d\r\n", status);
}

static int kill_job(struct job *j)
{
  int status;
  switch (j->state)
    {
    case '-':
      return 1;
      break;
    case '~':
    case 'p':
      if (!ptrace_detach(j->proc.pid))
	errout("ptrace detach");
    case 'r':
      errno = 0;
      if (kill(j->proc.pid, SIGTERM) == -1)
	errout("kill");
      else
	{
	  return 1;
	}
      break;
    default:
      fputs("\r\nCan't do that yet.\r\n", stderr);
    }
  return 0;
}

void kill_currjob(char *arg)
{
  if (currjob)
    {
      crlf();
      if (kill_job(currjob))
	{
	  jobwait(currjob, 0, 0);
	  currjob = 0;
	  char slot;
	  if ((slot = nextslot()) != -1)
	    {
	      currjob = &jobs[slot];
	      fprintf(stderr, " %s$j\r\n", currjob->jname);
	    }
	}
    }
  else
    fputs("\r\nPrompt login? here.\r\n", stderr);
}

void massacre(char *arg)
{
  for (struct job *j = jobs; j < jobsend; j++)
    if (j->state)
      kill_job(j);
  check_jobs();
  currjob = 0;
}

void jclprt(char *notused)
{
  if (currjob)
    {
      char **argv = currjob->proc.argv;
      crlf();
      argv++;
      while (*argv)
	{
	  fprintf(stderr, "%s ", *argv);
	  argv++;
	}
      crlf();
    }
}

void jcl(char *argstr)
{
  if (!currjob)
    {
      fputs("\r\nTried to set self jcl\r\n", stderr);
      return;
    }

  if (currjob->jcl) free(currjob->jcl);
  if (currjob->proc.argv) {
    for (char **p = currjob->proc.argv; *p; p++)
      free(*p);
    free(currjob->proc.argv);
  }

  char *buf;
  if ((buf = (char *)malloc(strlen(argstr))) == NULL)
    {
      fputs("\r\nmalloc fail\r\n", stderr);
      return;
    }
  strcpy(buf, argstr);
  if ((currjob->proc.argv = malloc(MAXARGS * sizeof(char **))) == NULL)
    {
      fputs("\r\nmalloc fail\r\n", stderr);
      return;
    }

  int argc = 1;
  currjob->proc.argv[argc++] = strtok(buf, " \t");
  while (argc < MAXARGS
	 && ((currjob->proc.argv[argc] = strtok(NULL, " \t")) != NULL))
    currjob->proc.argv[++argc] == NULL;
  crlf();
}

static inline int tell_parent(void)
{
  return (write(pfd2[1], "c", 1) != 1);
}

static inline int wait_parent(void)
{
  char	c;

  return ((read(pfd1[0], &c, 1) == 1) || (c == 'p'));
}

static inline int tell_child(void)
{
  return (write(pfd1[1], "p", 1) != 1);
}

static inline int wait_child(void)
{
  char	c;

  return ((read(pfd2[0], &c, 1) == 1) && (c == 'c'));
}

void child_load(void)
{
  tell_parent();
  wait_parent();

  signal(SIGINT, SIG_DFL);
  signal(SIGQUIT, SIG_DFL);
  signal(SIGTSTP, SIG_DFL);
  signal(SIGTTIN, SIG_DFL);
  signal(SIGTTOU, SIG_DFL);
  signal(SIGCHLD, SIG_DFL);

  __asm__("int3");

  fexecve(currjob->proc.ufname.fd, currjob->proc.argv, currjob->proc.env);
  errout("fexecve");
  _exit(-1);
}

static void load_(void)
{
  errno = 0;
  pid_t childpid = fork();

  if (childpid == -1)
    {
      fputs("\r\nfork failed\r\n", stderr);
      return;
    }

  if (!childpid)
    child_load();

  wait_child();
  if (!ptrace_seize(childpid))
    errout("ptrace seize");
  tell_child();

  int status = 0;
  waitpid(childpid, &status, 0);
  if (WIFEXITED(status))
    fprintf(stderr, "child exec failed. status=%d", WEXITSTATUS(status));
  else if (WIFSIGNALED(status))
    fprintf(stderr, "\r\nchild killed. signal=%d", WTERMSIG(status));
  else if (WIFSTOPPED(status))
    {
      currjob->proc.pid = childpid;
      setpgid(childpid, currjob->proc.pid);
      currjob->state = '~';
    }

  if (!ptrace_setopts(childpid, PTRACE_O_TRACEEXEC))
    errout("ptrace setoptions");
  else if (!ptrace_cont(childpid))
    errout("ptrace cont");
}

void load_prog(char *name)
{
  if (!runame())
    {
      fputs("\r\n(Please Log In)\r\n\r\n:kill\r\n", stderr);
      return;
    }

  if (!currjob)
    {
      fputs("\r\nno current job\r\n", stderr);
      return;
    }

  if (currjob->state != '-')
    {
      fputs("\r\n already loaded? ", stderr);
      return;
    }

  if (currjob->proc.ufname.name) free(currjob->proc.ufname.name);
  currjob->proc.ufname.name = strdup(name);
  if (currjob->proc.argv[0]) free(currjob->proc.argv[0]);
  currjob->proc.argv[0] = strdup(currjob->xjname);

  int fd;

  if ((fd = open_(hsname.fd, currjob->proc.ufname.name, O_RDONLY)) == -1)
    {
      errout("child openat");
      return;
    }

  currjob->proc.ufname.devfd = devices[DEVDSK].fd;
  currjob->proc.ufname.dirfd = msname.fd;
  currjob->proc.ufname.fd = fd;

  load_();

  crlf();
}

static void setfg(struct job *j)
{
  if (j)
    {
      tcsetpgrp(0, j->proc.pid);
      tcsetattr(0, TCSADRAIN, &(j->tmode));
    }
  else
    {
      tcsetpgrp(0, getpid());
      term_raw();
    }
  fg = j;
}

int fgwait(void)
{
  if (!fg)
    return 0;

  jobwait(fg, 0, 0);

  tcgetattr(0, &(fg->tmode));
  setfg(0);

  return 1;
}

void check_jobs(void)
{
  pid_t child;
  int status;

  errno = 0;
  while ((child = waitpid(-1, &status, WNOHANG|WUNTRACED|WCONTINUED)) > 0)
    {
      for (struct job *j = jobs; j < jobsend; j++)
	{
	  if (j->proc.pid == child)
	    {
	      if (WIFEXITED(status))
		{
		  fprintf(stderr, ":exit %d %s$j\r\n", WEXITSTATUS(status), j->jname);
		  free_job(j);
		}
	      else if (WIFSIGNALED(status))
		{
		  fprintf(stderr, ":kill %d %s$j\r\n", WTERMSIG(status), j->jname);
		  free_job(j);
		}
	      else if (WIFSTOPPED(status))
		{
		  fprintf(stderr, ":stop signal=%d %s$j\r\n",
			  WSTOPSIG(status), j->jname);
		  j->state = 'p';
		}
	      else
		fprintf(stderr, "check_jobs status=%d\r\n", status);

	      break;
	    }
	}
    }
  if (child == -1 && errno != ECHILD)
    {
      errout("checkjobs waitpid");
    }
}

static void wrongstate(struct job *j)
{
  switch(j->state)
    {
    case '-':
    case '~':
      fputs(" job never started? ", stderr);
      break;
    case 'r':
      fputs(" already running? ", stderr);
      break;
    case '\0':
      fputs(" empty job? ", stderr);
      break;
    default:
      fprintf(stderr, " unknown state %d? ", j->state);
    }
}

void contin(char *unused)
{
  if (!currjob)
    fputs(" job? ", stderr);
  else
    switch (currjob->state)
      {
      case 'p':
	if (ptrace_cont(currjob->proc.pid))
	  currjob->state = 'r';
	else
	  errout(currjob->proc.ufname.name);
      case 'r':
	crlf();
	setfg(currjob);
	break;
      default:
	wrongstate(currjob);
      }
}

void proced(char *unused)
{
  if (!currjob)
    fputs(" job? ", stderr);
  else
    switch (currjob->state)
      {
      case 'p':
	if (ptrace_cont(currjob->proc.pid))
	  currjob->state = 'r';
	else
	  errout(currjob->proc.ufname.name);
      case 'r':
	crlf();
 	break;
      default:
	wrongstate(currjob);
      }
}

void go(char *addr)
{
  if (addr && *addr)
    fprintf(stderr, "\r\nAddress Prefix for go: %s\r\n", addr);
  else if (!currjob)
    fputs(" job? ", stderr);
  else
    switch (currjob->state)
      {
      case '~':
      case 'p':
	crlf();
	if (ptrace_cont(currjob->proc.pid))
	  {
	    currjob->state = 'r';
	    setfg(currjob);
	  }
	else
	  errout(currjob->proc.ufname.name);
	break;
      default:
	wrongstate(currjob);
      }
}

void gzp(char *addr)
{
  crlf();
  if (addr && *addr)
    fprintf(stderr, "Address Prefix for gzp: %s\r\n", addr);
  else if (!currjob)
    fputs(" job? ", stderr);
  else
    switch (currjob->state)
      {
      case '~':
      case 'p':
	if (ptrace_cont(currjob->proc.pid))
	  currjob->state = 'r';
	else
	  errout(currjob->proc.ufname.name);
	break;
      default:
	wrongstate(currjob);
      }
}

void stop_currjob(void)
{
  if (!currjob)
    {
      fputs(" job? ", stderr);
      return;
    }

  if (ptrace_interrupt(currjob->proc.pid))
    {
      jobwait(currjob, 0, 0);
      currjob->state = 'p';
      typeout_pc(currjob);
    }
  else
    errout("ptrace intr");
}

void lfile(char *unused)
{
  if (!currjob)
    {
      fputs(" job? ", stderr);
      return;
    }
  crlf();
  if (currjob->state == '-')
    {
      fputs(" not loaded? \r\n", stderr);
      return;
    }
  typeout_fname(&(currjob->proc.ufname));
  crlf();
}

void forget(char *unused)
{
  if (currjob)
    {
      crlf();
      free_job(currjob);
      currjob = 0;
    }
  else
    fputs(" job? ", stderr);
}

void self(char *unused)
{
  currjob = 0;
  crlf();
}

void run_(char *jname, char *arg, int genj, int loadsyms)
{
  struct job *j;
  char *defjcl = "";
  char slot;
  char *njname = NULL;

  if ((j = getjob(jname)))
    {
      if (!genj)
	{
	  crlf();
	  if (clobrf && !uquery("Clobber Existing Job"))
	    {
	      crlf();
	      return;
	    }
	  if (!kill_job(j))
	    return;
	  jobwait(j, 0, 0);
	}
      else
	{
	  if ((njname = nextuniq(currjob->jname)) == NULL)
	    {
	      fputs(" uniqerr? ", stderr);
	      return;
	    }
	}
    }
  if ((slot = getopenslot()) != -1)
    {
      currjob = initslot(slot, jname);
      if (genj && njname != NULL) {
	if (currjob->jname) free(currjob->jname);
	currjob->jname = njname;
      }
    }
  else
    {
      fprintf(stderr, " %d jobs already? ", MAXJOBS);
      return;
    }

  jcl (arg ? arg : defjcl);

  struct file *dir;
  if (!(dir = findprog(jname)))
    {
      fprintf(stderr, "%s - file not found\r\n", jname);
      return;
    }

  int fd;
  if ((fd = open_(dir->fd, jname, O_RDONLY)) == -1)
    {
      errout("child openat");
      return;
    }

  currjob->proc.ufname.name = strdup(jname);
  currjob->proc.ufname.devfd = dir->devfd;
  currjob->proc.ufname.dirfd = dir->fd;
  currjob->proc.ufname.fd = fd;
  currjob->proc.argv[0] = strdup(jname);

  if (loadsyms)
    load_symbols(currjob);

  load_();
  jobwait(currjob, EXPECT_STOP, 5);

  if (ptrace_cont(currjob->proc.pid))
    {
      currjob->state = 'r';
      setfg(currjob);
    }
  else
    errout(currjob->proc.ufname.name);
}

void genjob(char *unused)
{
  char *njname;

  if (!currjob)
    {
      fputs(" job? ", stderr);
      return;
    }

  if ((njname = nextuniq(currjob->jname)) == NULL)
    {
      fputs(" err? ", stderr);
      return;
    }
  if (currjob->jname) free(currjob->jname);
  currjob->jname = njname;
  crlf();
}
