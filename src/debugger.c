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
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <errno.h>
#include <elf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include "jobs.h"
#include "debugger.h"

uint64_t qreg = 0;

struct location {
  pid_t pid;
  uint32_t _pad;
  uint64_t addr;
};

#define DOTRING_SIZE 8
struct location *openloc = NULL;
struct location dotring[DOTRING_SIZE] = { 0 };
unsigned char dr_start = 0;
unsigned char dr_end = 0;

static void crlf(void)
{
  fputs("\r\n", stderr);
}

void step_job(struct job *j)
{
  int status;
  if (ptrace(PTRACE_SINGLESTEP, j->proc.pid, NULL, NULL) == -1)
    errout("ptrace");
  check_jobs();
}

void pushdot(pid_t pid, uint64_t value)
{
  dr_end = ++dr_end % DOTRING_SIZE;

  dotring[dr_end].pid = pid;
  dotring[dr_end].addr = value;

  if (dr_end == dr_start)
    dr_start = ++dr_start % DOTRING_SIZE;
}

sigjmp_buf point;

static void segvhandler(int sig, siginfo_t *ign1, void *ign2)
{
  longjmp(point, 1);
}

int openlocation(pid_t pid, uint64_t addr)
{
  int ret = 1;

  pushdot(pid, addr);
  openloc = &dotring[dr_start];
  if (pid)
    {
      errno = 0;
      long data = ptrace(PTRACE_PEEKDATA, pid, addr, NULL);
      if (errno)
	{
	  errout("mem err?");
	  ret = 0;
	}
      else
	qreg = data;
    }
  else
    {
      struct sigaction sa;
      memset(&sa, 0, sizeof(sigaction));
      sigemptyset(&sa.sa_mask);
      sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
      sa.sa_sigaction = segvhandler;
      sigaction(SIGSEGV, &sa, NULL);
      if (setjmp(point) == 0)
	qreg = *(char *)addr;
      else
	{
	  fprintf(stderr, "mem err? ");
	  ret = 0;
	}
    }

  return ret;
}

void closelocation(void)
{
  openloc = NULL;
}

int ptrace_seize(pid_t pid)
{
  errno = 0;

  return (ptrace(PTRACE_SEIZE, pid, NULL, NULL) != -1);
}

int ptrace_detach(pid_t pid)
{
  errno = 0;

  return (ptrace(PTRACE_DETACH, pid, NULL, NULL) != -1);
}

int ptrace_interrupt(pid_t pid)
{
  errno = 0;

  return (ptrace(PTRACE_INTERRUPT, pid, NULL, NULL) != -1);
}

int ptrace_setopts(pid_t pid, int opts)
{
  errno = 0;

  return (ptrace(PTRACE_SETOPTIONS, pid, NULL, opts) != -1);
}

int ptrace_cont(pid_t pid)
{
  errno = 0;

  return (ptrace(PTRACE_CONT, pid, NULL, NULL) != -1);
}

void unload_symbols(struct job *j)
{
  munmap(j->proc.syms, j->proc.symlen);
}

void load_symbols(struct job *j)
{
  Elf64_Ehdr *ehdr;
  struct stat status;

  errno = 0;
  fstatat(j->proc.ufname.fd, "", &status, AT_EMPTY_PATH);
  if (!errno)
    {
      if ((ehdr = (Elf64_Ehdr *)mmap(0, status.st_size,
				     PROT_READ, MAP_PRIVATE,
				     j->proc.ufname.fd, 0)) != (Elf64_Ehdr *)(MAP_FAILED))
	{
	  j->proc.syms = (char *)ehdr;
	  j->proc.symlen = status.st_size;
	}
      else
	errout("mmap");
    }
  else
    errout("fstatat");
}

void listp(char *unused)
{
  if (!currjob)
    {
      fprintf(stderr, " job? ");
      return;
    }
  if (!currjob->proc.syms)
    {
      fprintf(stderr, " not loaded? ");
      return;
    }

  Elf64_Ehdr *ehdr = (Elf64_Ehdr *)currjob->proc.syms;
  Elf64_Shdr *shdr = (Elf64_Shdr *)(currjob->proc.syms + ehdr->e_shoff);
  Elf64_Shdr *sh_strtab = &shdr[ehdr->e_shstrndx];
  const char *const sh_strtab_p = currjob->proc.syms + sh_strtab->sh_offset;

  for (int i = 0; i < ehdr->e_shnum; i++)
    {
      const char *s = sh_strtab_p + shdr[i].sh_name;
      if (*s)
	fprintf(stderr, "%-16s ", s);
      if ((i % 4) == 0)
	crlf();
    }
  crlf();
}

void lists(char *unused)
{
  if (!currjob)
    {
      fprintf(stderr, " job? ");
      return;
    }
  if (!currjob->proc.syms)
    {
      fprintf(stderr, " not loaded? ");
      return;
    }

  Elf64_Ehdr *ehdr = (Elf64_Ehdr *)currjob->proc.syms;
  Elf64_Shdr *shdr = (Elf64_Shdr *)(currjob->proc.syms + ehdr->e_shoff);
  Elf64_Shdr *sh_strtab = &shdr[ehdr->e_shstrndx];
  const char *const sh_strtab_p = currjob->proc.syms + sh_strtab->sh_offset;
  Elf64_Shdr *strtab = NULL;
  Elf64_Shdr *symtab = NULL;

  for (int i = 0; i < ehdr->e_shnum; i++)
    {
      switch (shdr[i].sh_type)
	{
	case SHT_SYMTAB:
	  if (strcmp(sh_strtab_p + shdr[i].sh_name, ".symtab") == 0)
	    symtab = &shdr[i];
	  break;
	case SHT_STRTAB:
	  if (strcmp(sh_strtab_p + shdr[i].sh_name, ".strtab") == 0)
	    strtab = &shdr[i];
	  break;
	default:
	  continue;
	}
    }

  crlf();

  if (strtab == NULL)
    {
      fprintf(stderr, " no string table?\r\n");
      return;
    }
  if (symtab == NULL)
    {
      fprintf(stderr, " no symbol table?\r\n");
      return;
    }


  Elf64_Sym *symtab_p = (Elf64_Sym *)(currjob->proc.syms + symtab->sh_offset);
  const char *const strtab_p = currjob->proc.syms + strtab->sh_offset;
  int qsyms = symtab->sh_size / symtab->sh_entsize;

  for (int i = 0; i < qsyms; i++)
    if (symtab_p[i].st_name)
      fprintf(stderr, "%s\r\n", strtab_p + symtab_p[i].st_name);
}

void symlod(char *arg)
{
  if (!currjob)
    {
      fprintf(stderr, " job? ");
      return;
    }

  crlf();

  if (arg && *arg)
    {
      fprintf(stderr, "Would load symbols from %s\r\n", arg);
      return;
    }

  if (currjob->proc.syms)
    unload_symbols(currjob);

  load_symbols(currjob);
}

void typeout_pc(struct job *j)
{
  long pc = ptrace(PTRACE_PEEKUSER, j->proc.pid, RIP * 8, NULL);
  long data = ptrace(PTRACE_PEEKDATA, j->proc.pid, pc, NULL);

  fprintf(stderr, "%lx)   ", pc);
  sch(data);
}
