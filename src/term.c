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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>

struct termios def_termios;
static struct termios new_termios;
struct winsize winsz;

void term_restore (void)
{
  tcsetattr (0, TCSADRAIN, &def_termios);
}

void term_raw (void)
{
  tcsetattr (0, TCSANOW, &new_termios);
}

static void setwinsz(void)
{
  winsz.ws_row = 24;
  winsz.ws_col = 80;
  winsz.ws_xpixel = 0;
  winsz.ws_ypixel = 0;
  errno = 0;
  ioctl(STDIN_FILENO, TIOCGWINSZ, &winsz);
}

static void winch_handler(int sig)
{
  int terrno = errno;
  setwinsz();
  errno = terrno;
}

void term_init (void)
{

  if (!isatty(0))
    {
      fprintf(stderr, "ddt currently needs interactive tty\r\n");
      exit(1);
    }

  pid_t pgid = 0;

  while (tcgetpgrp(0) != (pgid = getpgrp()))
    kill (-pgid, SIGTTIN);

  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  signal(SIGCHLD, SIG_IGN);

  pgid = getpid();
  if (setpgid(pgid, pgid) < 0)
    {
      perror("term_init setpgid");
      exit(1);
    }
  tcsetpgrp(0, pgid);

  tcgetattr (0, &def_termios);

  new_termios = def_termios;
  cfmakeraw (&new_termios);
  term_raw ();

  setwinsz();
  if (errno)
    perror("tiocgwinsz");
}

int term_read (void)
{
  char ch;
  int n;

  errno = 0;
  while ((n = read (0, &ch, 1)) == -1)
    if (errno == EINTR)
      {
	errno = 0;
	continue;
      }
    else
      {
	perror("read");
	fprintf (stderr, "Bye!\n");
	exit (0);
      }

  return ch;
}

void clear(char *arg)
{
  fprintf(stderr, "\033[2J\033[H");
}

char morwarn = 0;

int uquery(char *text)
{
  fprintf(stderr, "--%s--", text);
  if (morwarn)
    fputs(" (Space=yes, Rubout=no)", stderr);
  return (term_read() == ' ');
}
