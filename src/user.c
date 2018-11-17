#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <termios.h>
#include "jobs.h"

char *_runame = 0;
char *_xuname = 0;

char *runame(void)
{
  return _runame;
}

static void _logout(void)
{
  massacre(NULL);
  free(_runame);
  _runame = 0;
  if (_xuname)
    {
      free(_xuname);
      _xuname = 0;
    }
}

void logout (char *ignore)
{
  fputs("\r\n", stderr);
  if (_runame)
    _logout();
  exit (0);
}

void login_as (char *name)
{
  if (_runame)
    {
      fprintf(stderr, "\r\nAlready logged in.\r\n");
      return;
    }
  _runame = strdup(name);
  _xuname = strdup(name);
  clobrf = 1;
  genjfl = 1;
  fprintf (stderr, "\r\nWelcome, %s\r\n", _xuname);
}

void chuname (char *name)
{
  if (_runame)
    _logout();
  login_as(name);
}
