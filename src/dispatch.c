#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "term.h"

#define PREFIX_MAXBUF 255
#define SUFFIX_MAXBUF 255

static const char *prompt = "*";

static int altmodes;
static char prefix[PREFIX_MAXBUF+1];
static int nprefix;
static char character;
static int done;

static void (**fn) (void);
static void (*plain[256]) (void);
static void (*alt[256]) (void);

#define BELL 07
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
  if (nprefix < PREFIX_MAXBUF)
    {
      prefix[nprefix++] = character;
      prefix[nprefix] = 0;
    }
  else
    fputc(BELL, stderr);
}

static void altmode (void)
{
  altmodes++;
  fn = alt; 
}

static void rubout (void)
{
  if (!(altmodes || nprefix)) {
    fputs("?? ", stderr);
    return;
  }
  fprintf (stderr, "\010 \010");
  if (altmodes)
    {
      if (!--altmodes)
	fn = plain;
    }
  else
      nprefix--;
}

static char *suffix (void)
{
  static char string[SUFFIX_MAXBUF+1];
  int n = 0;
  char ch;

  string[0] = 0;

  while ((ch = term_read ()) != '\r')
    if (n < SUFFIX_MAXBUF)
      switch (ch)
	{
	case RUBOUT:
	  if (n)
	    {
	      fprintf (stderr, "\010 \010");
	      string[--n] = 0;
	    }
	  else
	    return NULL;
	  break;
	default:
	  {
	    echo (ch);
	    string[n++] = ch;
	    string[n] = 0;
	  }
	}
    else
      fputc(BELL, stderr);

  return string;
}

static void colon (void)
{
  char *suff = suffix();
  if (suff != NULL)
    {
      fprintf (stderr, "\r\nColon command: %s\r\n", suffix ());
      done = 1;
    }
  else				/* user rubbed out : */
    fprintf (stderr, "\010 \010");
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
