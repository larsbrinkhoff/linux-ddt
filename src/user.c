#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "jobs.h"

char *username = 0;

void logout (char *ignore)
{
  if (username)
    {
      massacre(NULL);
      free(username);
      username = 0;
    }
  else
    {
      fputs("\r\n", stderr);
      exit (0);
    }
}

void login_as (char *name)
{
  if (username)
    {
      logout(NULL);
    }
  username = strdup(name);
  fprintf (stderr, "\r\nWelcome, %s\r\n", username);
}

char *login_name(void)
{
  return username;
}
