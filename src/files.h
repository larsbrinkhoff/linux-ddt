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
struct file {
  char *name;
  int devfd;
  int dirfd;
  int fd;
};

void files_init(void);
struct file *findprog(char *name);
void delete_file(char *name);
void cwd(char *arg);
void nfdir(char *arg);
void ofdir(char *arg);
void print_file(char *arg);
void listf(char *arg);
void list_files(char *arg, int setdefp);

void typeout_fname(struct file *f);

int open_(int dirfd, char *path, int flags);

extern struct file devices[];
extern struct file hsname;
extern struct file msname;

#define DEVDSK 0
