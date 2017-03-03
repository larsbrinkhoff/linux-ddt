#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

static struct termios old_termios;

void term_init (void)
{
  struct termios new_termios;

  tcgetattr (0, &old_termios);

  new_termios = old_termios;
  cfmakeraw (&new_termios);
  tcsetattr (0, TCSANOW, &new_termios);
}

void term_restore (void)
{
  tcsetattr (0, TCSANOW, &old_termios);

}

int term_read (void)
{
  char ch;
  int n;

  n = read (0, &ch, 1);
  if (n != 1)
    {
      fprintf (stderr, "Bye!\n");
      exit (0);
    }

  return ch;
}
