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
#include <stdint.h>
#include <termios.h>
#include "files.h"

struct process {
  // struct process *next;
  struct file ufname;
  char **argv;
  char **env;
  char *syms;
  size_t symlen;
  pid_t pid;
  int status;
};

typedef void (typeoutfunc)(uint64_t);

struct job {
  char *jname;
  char *xjname;
  char *jcl;
  char state;
  char slot;
  struct termios tmode;
  struct process proc;
  typeoutfunc *tdquote;
  typeoutfunc *tnmsgn;
  typeoutfunc *tdollar;
  typeoutfunc *tperce;
  typeoutfunc *tamper;
  typeoutfunc *tprime;
};

void jobs_init(void);
int fgwait(void);
void check_jobs(void);
void list_currjob(void);
void next_job(void);
void stop_currjob(void);

void select_job(char *jname);
void listj(char *);
void show_currjob(char *arg);
void kill_currjob(char *);

void set_currjname(char *jname);
void jcl(char *argstr);
void jclprt(char *);
void massacre(char *);
void load_prog(char *name);
void go(char *addr);
void gzp(char *addr);
void contin(char *);
void proced(char *);
void lfile(char *);
void forget(char *);
void self(char *);
void genjob(char *);

void run_(char *jname, char *arg, int genj, int loadsyms);

void errout(char *arg);

extern struct termios def_termios;
extern struct job *currjob;
extern int clobrf;
extern int genjfl;
