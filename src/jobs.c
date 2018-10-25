#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "jobs.h"
#include "user.h"

#define MAXJOBS 8
#define MAX_ARGS 256

struct job jobs[MAXJOBS] = { 0 };
struct job *jobsend = &jobs[MAXJOBS];
struct job *currjob = 0;

static struct job *getjob(char *jname)
{
  for (struct job *j = jobs; j < jobsend; j++)
    if (j->state && strcmp(j->jname, jname) == 0)
      return j;
  return 0;
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
  if (currjob->jname)
    free(currjob->jname);
  currjob->jname = strdup(jname);
  fputs("\r\n", stderr);
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
    fprintf(stderr, "\r\nWould set self name\r\n");
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
  fputs("\r\n", stderr);
  for (struct job *j = jobs; j < jobsend; j++)
    if (j->state != 0)
      fprintf(stderr, "%c %s %c %d\r\n",
	      (j != currjob)?' ':'*',
	      j->jname, j->state, j->slot);
}

void job(char *jname)
{
  struct job *j;
  char slot;
  if ((j = getjob(jname)))
    {
      currjob = j;
      fputs("\r\n", stderr);
      return;
    }

  if ((slot = getopenslot()) != -1)
    {
      j = &jobs[slot];
      j->jname = strdup(jname);
      j->jcl = NULL;
      j->state = '-';
      j->slot = slot;
      j->proc.prog = NULL;
      j->proc.argv = NULL;
      j->proc.env = NULL;
      j->proc.pid = 0;
      j->proc.status = 0;
      j->proc.dirfd = -1;
      currjob = j;
      fputs("\r\n!\r\n", stderr);
    }
  else
    fprintf(stderr, " %d jobs already? ", MAXJOBS);
}

static void free_job(struct job *j)
{
  if (j->jname) free(j->jname);
  if (j->jcl) free(j->jcl);
  if (j->proc.prog) free(j->proc.prog);
  if (j->proc.argv) free(j->proc.argv);
  if (j->proc.env) free(j->proc.env);
  j->state = 0;
}

void kill_job(struct job *j)
{
  if (j->state != '-')
    {
      fprintf(stderr, "\r\nCan't do that yet.\r\n");
      return;
    }
  free_job(j);
}

void kill_currjob(char *arg)
{
  if (currjob)
    {
      kill_job(currjob);
      currjob = 0;
      char slot;
      if ((slot = nextslot()) != -1)
	{
	  currjob = &jobs[slot];
	  fprintf(stderr, "\r\n %s$j", currjob->jname);
	}
      fputs("\r\n", stderr);
    }
  else
    {
      fprintf(stderr, "\r\nPrompt login? here.\r\n");
    }
}

void massacre(char *arg)
{
  for (struct job *j = jobs; j < jobsend; j++)
    if (j->state)
      kill_job(j);
  currjob = 0;
  fputs("\r\n", stderr);
}

void jclprt(char *notused)
{
  if (currjob)
    {
      char **argv = currjob->proc.argv;
      fputs("\r\n", stderr);
      argv++;
      while (*argv)
	{
	  fprintf(stderr, "%s ", *argv);
	  argv++;
	}
      fputs("\r\n", stderr);
    }
}

void jcl(char *argstr)
{
  if (!currjob)
    {
      fprintf(stderr, "\r\nTried to set self jcl\r\n");
      return;
    }

  if (currjob->jcl) free(currjob->jcl);
  if (currjob->proc.argv) {
    char **v = currjob->proc.argv;
    while (*v)
      {
	free(*v);
	v++;
      }
    free(currjob->proc.argv);
  }

  char *buf;
  if ((buf = (char *)malloc(strlen(argstr))) == NULL)
    {
      fprintf(stderr, "\r\nmalloc fail\r\n");
      return;
    }
  strcpy(buf, argstr);
  if ((currjob->proc.argv = malloc(MAX_ARGS * sizeof(char **))) == NULL)
    {
      fprintf(stderr, "\r\nmalloc fail\r\n");
      return;
    }

  int argc = 1;
  currjob->proc.argv[argc++] = strtok(buf, " \t");
  while (argc < MAX_ARGS
	 && ((currjob->proc.argv[argc] = strtok(NULL, " \t")) != NULL))
    currjob->proc.argv[++argc] == NULL;
  fputs("\r\n", stderr);
}

void child_load(void)
{
  fprintf(stderr, "\r\nchild loading %s\r\n", currjob->proc.prog);
  _exit(1);
}

void load(char *name)
{
  if (!runame())
    {
      fprintf(stderr, "\r\n(Please Log In)\r\n\r\n:kill\r\n");
      return;
    }

  if (!currjob)
    {
      fprintf(stderr, "\r\nno current job\r\n");
      return;
    }

  if (currjob->state != '-')
    {
      fprintf(stderr, "\r\nJob already loaded\r\n");
      return;
    }

  errno = 0;
  pid_t child = fork();

  if (child == -1)
    {
      fprintf(stderr, "\r\nfork failed\r\n");
      return;
    }

  if (!child)
    child_load();

  int status = 0;
  waitpid(child, &status, 0);
  if (WIFEXITED(status) || WIFSIGNALED(status))
    {
      fprintf(stderr, "\r\nchild exec failed: %d\r\n", status);
      return;
    }

  currjob->proc.pid = child;
  setpgid(child, currjob->proc.pid);
  currjob->state = '~';

  fputs("\r\n", stderr);
}
