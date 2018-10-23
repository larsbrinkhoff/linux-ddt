#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include "ccmd.h"
#include "jobs.h"

enum sysdir { SYS, SYS1, SYS2, SYS3 };
#define QTY_SYSFDS 4
int sysfds[QTY_SYSFDS];

void help(char *);
void version(char *);
void list_builtins(char *);

#define VERSION "0"

#define ALTMODE 033

char helptext[] = 
  "\r\n You are typing at \"DDT\", a top level command interpreter/debugger for Linux.\r\n"
  " DDT commands start with a colon and are usually terminated by a carriage return.\r\n"
  " Type :? <cr> to list them.\r\n"
  " If a command is not recognized, it is tried as the name of a system program to run.\r\n"
  " Type control-Z to return to DDT after running a program\r\n"
  "(Some return to DDT by themselves when done, printing \":kill\").\r\n";

struct builtin builtins[] =
  {
   {"clear", "", "clear screen [^L]", clear},
   {"help", "", "print out basic information", help},
   {"jcl", "<line>", "set job control string", jcl},
   {"jclprt", "", "print the job control strong", jclprt},
   {"job", "", "create or select job [$j]", job},
   {"kill", "", "kill current job [$^x.]", kill_currjob},
   {"listj", "", "list jobs [$$v]", listj},
   {"massacre", "", "kill all your jobs", massacre},
   {"version", "", "type version number of Linux and DDT", version},
   {"?", "", "list all : commands", list_builtins},
   {0, 0, 0, 0}
  };

static int builtin(char *name, char *arg)
{
  for (struct builtin *p = builtins; p->name; p++)
    if (strncmp(p->name, name, 6) == 0)
      {
	p->fn(arg);
	return 1;
      }
  return 0;
}

void init_ccmd(void)
{
  sysfds[SYS] = openat(AT_FDCWD,
		       "/bin", O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  sysfds[SYS1] = openat(AT_FDCWD,
			"/sbin", O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  sysfds[SYS2] = openat(AT_FDCWD,
			"/usr/bin", O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  sysfds[SYS3] = openat(AT_FDCWD,
			"/usr/sbin", O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
}

static int syscommand(char *name, char *arg)
{
  int fd;
  for (int i = 0; i < QTY_SYSFDS; i++)
    {
      if (sysfds[i] == -1)
	continue;
      if ((fd = faccessat(sysfds[i], name, X_OK, 0)) != -1)
	{
	  fprintf(stderr, "\r\nSystem command: %s arg: %s\r\n", name, arg);
	  return fd;
	}
    }
  return -1;
}

static char *skip_comment(char *buf)
{
  if (*buf == ALTMODE)
    {
      do
	buf++;
      while (*buf && *buf != ALTMODE);
      if (*buf)
	buf++;
    }
  return buf;
}

static char *skip_ws(char *buf)
{
  while (*buf == ' ')
    buf++;
  return buf;
}

static char *skip_prgm(char *buf)
{
  for (; *buf; buf++)
    if (*buf == ' ')
      {
	*buf = '\0';		/* null terminate prgm */
	buf++;
	break;
      }
  return buf;
}

void ccmd(char *cmdline)
{
  char *cmd = skip_ws(skip_comment(cmdline));
  char *arg = skip_ws(skip_prgm(cmd));
  if (!builtin(cmd, arg))
    if (syscommand(cmd, arg) == -1)
      fprintf (stderr, "\r\n%s - Unknown command\r\n", cmd);
}

void list_builtins(char *arg)
{
  fputs("\r\n<The commands explicitly listed here are part of DDT, not separate programs>\r\n", stderr);
  for (struct builtin *p = builtins; p->name; p++)
    fprintf(stderr, ":%-8s %s\t%s\r\n",
	    p->name, p->arghelp, p->desc);
  fprintf(stderr, ":%-8s %s\t%s\r\n",
	  "<prgm>", "<optional jcl>", "invoke program, passing JCL if present");
}

void help(char *arg)
{
  fputs(helptext, stderr);
}

void version(char *arg)
{
  struct utsname luname = { 0 };
  char ttyname[32];

  if (uname(&luname))
    return;			/* fix me - note syscall failure */

  fprintf(stderr, "\r\n%s %s.%s. DDT.%s.\r\n",
	  luname.nodename,
	  luname.sysname,
	  luname.release,
	  VERSION);
  if (!ttyname_r(0, ttyname, 32))
    fprintf(stderr, "%s\r\n", ttyname);
}

void clear(char *arg)
{
  fprintf(stderr, "\033[2J\033[H");
}
