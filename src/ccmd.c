#include <stdio.h>
#include <string.h>
#include "ccmd.h"

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
   {"?", "", "list all : commands", list_builtins},
   {0, 0, 0, 0}
  };

int builtin(char *name)
{
  for (struct builtin *p = builtins; p->name; p++)
    {
      if (strncmp(p->name, name, 6) == 0)
	{
	  p->fn();
	  return 1;
	}
    }
  return 0;
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
