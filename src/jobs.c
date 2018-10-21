#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include "jobs.h"

#define MAXJOBS 8

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

void kill_currjob(char *arg)
{
  if (currjob)
    {
      if (currjob->state != '-')
	{
	  fprintf(stderr, "\r\nCan't do that yet.\r\n");
	  return;
	}
      free_job(currjob);
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
