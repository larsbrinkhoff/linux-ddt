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
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "ccmd.h"
#include "jobs.h"
#include "user.h"
#include "term.h"
#include "debugger.h"

void help(char *);
void list_builtins(char *);
static void retry(char *arg);
static void new(char *arg);
static void version_(char *);
static void sstatus_(char *);

#define ALTMODE 033

const char helptext[] =
  "\r\n You are typing at \"DDT\", a top level command interpreter/debugger for Linux.\r\n"
  " DDT commands start with a colon and are usually terminated by a carriage return.\r\n"
  " Type :? <cr> to list them.\r\n"
  " Type :login <your name> to log in.\r\n"
  " To list a file directory, type :listf <directory name><cr>.\r\n"
  " To print a file, type :print <file name><cr>.\r\n"
  " If a command is not recognized, it is tried as the name of a system program to run.\r\n"
  " Type control-Z to return to DDT after running a program\r\n"
  "(Some return to DDT by themselves when done, printing \":kill\").\r\n";

struct builtin {
  const char *name;
  const char *arghelp;
  const char *desc;
  void (*fn) (char *arg);
};

struct builtin builtins[] =
  {
   {"clear", "", "clear screen [^L]", clear},
   {"chuname", "<new uname>", "change user name (log out and in again)", chuname},
   {"continue", "", "continue program, giving job TTY [$p]", contin},
   {"cwd", "<dir>", "change working directory [$$^s]", cwd},
   {"ddtmode", "", "leave MONIT mode", set_ddtmode},
   {"delete", "<file>", "delete file [^o]", delete_file},
   {"forget", "", "hide a job from DDT wihout killing it", forget},
   {"genjob", "", "rename current job to a generated unique name", genjob},
   {"go", "<start addr (opt)>", "start inferior [$g]", go},
   {"gzp", "<start addr (opt)>", "start job without tty [$g^z^p]", gzp},
   {"help", "", "print out basic information", help},
   {"intest", "", "execute init file, etc.", intest},
   {"jcl", "<line>", "set job control string", jcl},
   {"jclprt", "", "print the job control strong", jclprt},
   {"job", "", "create or select job [$j]", select_job},
   {"kill", "", "kill current job [$^x.]", kill_currjob},
   {"lfile", "", "print filename of last file loaded", lfile},
   {"listp", "", "list block struct of the job's symbol table", listp},
   {"listf", "<dir>", "list files [^f]", listf},
   {"listj", "", "list jobs [$$v]", listj},
   {"lists", "", "list job's symbols", lists},
   {"load", "<file>", "load file into core [$l]", load_prog},
   {"login", "<name>", "log in [$u]", login_as},
   {"logout", "", "log off [$$u]", logout},
   {"massacre", "", "kill all your jobs", massacre},
   {"monmode", "", "enter MONIT mode", set_monmode},
   {"new", "<prgm> <opt jcl>", "invoke <prgm>. If already using <pgrm>, make a second copy", new},
   {"nfdir", "<dir1>,<dir2>...", "add file directories to search list", nfdir},
   {"ofdir", "<dir1>,<dir2>...", "remove file directories from search list", ofdir},
   {"outtest", "", "perform actions normally associated with logging out", outtest},
   {"print", "<file>", "print file [^r]", print_file},
   {"proced", "", "same as proceed", proced},
   {"proceed", "", "proceed job, leave tty to DDT [$p]", proced},
   {"retry", "<prgm> <opt jcl>", "invoke <prgm>, clobbering any old copy", retry},
   {"self", "", "select DDT as current job", self},
   {"sl", "<file>", "same as :symlod (load symbols only, don't clobber core)", symlod},
   {"slist", "", "same as :lists", lists},
   {"sstatus", "", "type system status", sstatus_},
   {"start", "<start addr (opt)>", "start inferior [<addr>$g]", go},
   {"symlod", "<file>", "load symbols only (don't clobber core)", symlod},
   {"version", "", "type version number of Linux and DDT", version_},
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

static void crlf(void)
{
  fputs("\r\n", stderr);
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

void ccmd(char *cmdline, int altmodes)
{
  char *cmd = skip_ws(skip_comment(cmdline));
  char *arg = skip_ws(skip_prgm(cmd));
  if (altmodes || !builtin(cmd, arg))
    {
      if (!runame())
	fputs("\r\n(Please Log In)\r\n\r\n:kill\r\n", stderr);
      else
	run_(cmd, arg, genjfl, altmodes);
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

void set_monmode(char *unused)
{
  crlf();
  monmode = 1;
  prompt = ":";
}

void set_ddtmode(char *unused)
{
  crlf();
  monmode = 0;
  prompt = "*";
}

static void retry(char *arg)
{
  char *jcl = skip_ws(skip_prgm(arg));
  run_(arg, jcl, 0, 0);
}

static void new(char *arg)
{
  char *jcl = skip_ws(skip_prgm(arg));
  run_(arg, jcl, 1, 0);
}

static void version_(char *unused)
{
  crlf();
  version(NULL);
}

static void sstatus_(char *unused)
{
  crlf();
  sstatus(NULL);
}
