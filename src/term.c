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
