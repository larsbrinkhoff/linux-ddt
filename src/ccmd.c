#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <termios.h>
#include "ccmd.h"
#include "files.h"
#include "jobs.h"
#include "user.h"

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
   {"continue", "", "continue program, giving job TTY [$p]", contin},
   {"delete", "<file>", "delete file [^o]", delete_file},
   {"go", "<start addr (opt)>", "start inferior [$g]", go},
   {"gzp", "<start addr (opt)>", "start job without tty [$g^z^p]", gzp},
   {"help", "", "print out basic information", help},
   {"jcl", "<line>", "set job control string", jcl},
   {"jclprt", "", "print the job control strong", jclprt},
   {"job", "", "create or select job [$j]", select_job},
   {"kill", "", "kill current job [$^x.]", kill_currjob},
   {"listj", "", "list jobs [$$v]", listj},
   {"load", "<file>", "load file into core [$l]", load_prog},
   {"login", "<name>", "log in [$u]", login_as},
   {"logout", "", "log off [$$u]", logout},
   {"massacre", "", "kill all your jobs", massacre},
   {"proced", "", "same as proceed", proced},
   {"proceed", "", "proceed job, leave tty to DDT [$p]", proced},
   {"start", "<start addr (opt)>", "start inferior [<addr>$g]", go},
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
    {
      if (!runame())
	fputs("\r\n(Please Log In)\r\n\r\n:kill\r\n", stderr);
      else if (syscommand(cmd, arg) == -1)
	fprintf (stderr, "\r\n%s - Unknown command\r\n", cmd);
    }
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
