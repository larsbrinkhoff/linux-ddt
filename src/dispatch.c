#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "term.h"

static const char *prompt = "*";

static int altmodes;
static char prefix[256];
static int nprefix;
static char character;
static int done;

static void (**fn) (void);
static void (*plain[256]) (void);
static void (*alt[256]) (void);

#define ALTMODE 033
#define RUBOUT 0177

static void echo (int ch)
{
  switch (ch)
    {
    case ALTMODE: fputc ('$', stderr); break;
    default: fputc (ch, stderr); break;
    }
}

static void unknown (void)
{
  fprintf (stderr, "?\n");
  done = 1;
}

static void arg (void)
{
  prefix[nprefix] = character;
  prefix[nprefix+1] = 0;
  nprefix++;
}

static void altmode (void)
{
  altmodes++;
  fn = alt; 
}

static void rubout (void)
{
  fprintf (stderr, "\010 \010");
}

static char *suffix (void)
{
  static char string[256];
  int n = 0;
  char ch;

  string[0] = 0;

  while ((ch = term_read ()) != '\r')
    {
      echo (ch);
      string[n] = ch;
      n++;
    }

  return string;
}

static void colon (void)
{
  fprintf (stderr, "\r\nColon command: %s\r\n", suffix ());
  done = 1;
}

static void logout (void)
{
  exit (0);
}

static void login (void)
{
  if (altmodes > 1)
    logout ();

  fprintf (stderr, "\r\nWelcome, %s\r\n", prefix);
  done = 1;
}

static void print_args (void)
{
  fprintf (stderr, "\n\rArgs: %s\r\n", prefix);
}

void dispatch_init (void)
{
  int i;

  for (i = 0; i < 256; i++)
    {
      plain[i] = unknown;
      alt[i] = unknown;
    }

  for (i = 'a'; i <= 'z'; i++)
    {
      plain[i] = arg;
    }

  for (i = '0'; i <= '9'; i++)
    {
      plain[i] = arg;
      alt[i] = arg;
    }

  plain[ALTMODE] = altmode;
  alt[ALTMODE] = altmode;
  plain[RUBOUT] = rubout;
  alt[RUBOUT] = rubout;

  plain[':'] = colon;
  alt['u'] = login;
  alt['?'] = print_args;
}

static void dispatch (int ch)
{
  done = 0;
  character = ch;
  fn[ch] ();
}

void prompt_and_execute (void)
{
  int ch;

  write (1, prompt, 1);
  altmodes = 0;
  nprefix = 0;
  fn = plain;

  do
    {
      ch = term_read ();
      echo (ch);
      dispatch (ch);
    }
  while (!done);
}
