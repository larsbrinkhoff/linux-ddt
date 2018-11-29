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

void typeout_pc(struct job *j);
void step_job(struct job *j);
int ptrace_seize(pid_t pid);
int ptrace_detach(pid_t pid);
int ptrace_interrupt(pid_t pid);
int ptrace_setopts(pid_t pid, int opts);
int ptrace_cont(pid_t pid);

void load_symbols(struct job *j);
void unload_symbols(struct job *j);
void listp(char *);
void lists(char *);
void symlod(char *arg);

void tmc(uint64_t value);
void tma(uint64_t value);
void tmch(uint64_t value);
void tmf(uint64_t value);
void tmh(uint64_t value);

void resetradix(void);
void setradix(int r, int perm);
void settypeo(typeoutfunc *f, int perm);

extern typeoutfunc *mdquot;
extern typeoutfunc *mnmsgn;
extern typeoutfunc *mdolla;
extern typeoutfunc *mperce;
extern typeoutfunc *mamper;
extern typeoutfunc *mprime;
extern typeoutfunc *tch;

extern uint64_t qreg;
