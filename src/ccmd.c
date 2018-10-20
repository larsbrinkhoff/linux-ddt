#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>
#include "ccmd.h"

void version(void);

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
   {"help", "", "print out basic information", help},
   {"version", "", "type version number of Linux and DDT", version},
   {"?", "", "list all : commands", list_builtins},
   {0, 0, 0, 0}
  };

static int builtin(char *name)
{
  for (struct builtin *p = builtins; p->name; p++)
    if (strncmp(p->name, name, 6) == 0)
      {
	p->fn();
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
  if (!builtin(cmd))
    fprintf (stderr, "\r\nSystem command: %s arg: %s\r\n", cmd, arg);
}

void list_builtins(void)
{
  fputs("\r\n<The commands explicitly listed here are part of DDT, not separate programs>\r\n", stderr);
  for (struct builtin *p = builtins; p->name; p++)
    fprintf(stderr, ":%-8s %s\t%s\r\n",
	    p->name, p->arghelp, p->desc);
  fprintf(stderr, ":%-8s %s\t%s\r\n",
	  "<prgm>", "<optional jcl>", "invoke program, passing JCL if present");
}

void help(void)
{
  fputs(helptext, stderr);
}

void version(void)
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
