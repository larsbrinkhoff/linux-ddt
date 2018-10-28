#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <termios.h>
#include "files.h"
#include "jobs.h"

char *_runame = 0;
char *_xuname = 0;

void logout (char *ignore)
{
  fputs("\r\n", stderr);
  if (_runame)
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
  else
    exit (0);
}

void login_as (char *name)
{
  if (_runame)
    logout(NULL);
  _runame = strdup(name);
  _xuname = strdup(name);
  fprintf (stderr, "\r\nWelcome, %s\r\n", _xuname);
}

char *runame(void)
{
  return _runame;
}
