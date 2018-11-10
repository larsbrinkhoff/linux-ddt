#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include "files.h"
#include "jobs.h"

#define PATH_MAX 4096

struct file dsk = {"dsk", -1, -1, -1};
#define QTY_SYSDIRS 4
struct file sysdirs[QTY_SYSDIRS] = {
				    {"bin", -1, -1, -1},
				    {"sbin", -1, -1, -1},
				    {"usr/bin", -1, -1, -1},
				    {"usr/sbin", -1, -1, -1}
};

struct file hsname = { 0, -1, -1, -1 };
struct file msname = { 0, -1, -1, -1 };

void files_init(void)
{
  int fd;
  errno = 0;
  fd = openat(AT_FDCWD, "/",
	      O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  dsk.fd = dsk.dirfd = dsk.devfd = fd;
  if (errno)
    errout("/");

  for (int i = 0; i < QTY_SYSDIRS; i++)
    {
      errno = 0;
      sysdirs[i].fd = openat(dsk.fd, sysdirs[i].name,
			     O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
      if (errno)
	{
	  errout(sysdirs[i].name);
	  continue;
	}
      sysdirs[i].devfd = dsk.fd;
      sysdirs[i].dirfd = dsk.fd;
    }
  
  msname.name = malloc(PATH_MAX);
  errno = 0;
  if (getcwd(msname.name, PATH_MAX) == 0)
    {
      errout("getcwd");
      exit(1);
    }
  errno = 0;
  msname.fd = openat(dsk.fd, msname.name,
		     O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (errno)
    {
      errout("openat msname");
      exit(1);
    }

  hsname.name = strdup(msname.name);
  hsname.devfd = dsk.fd;
  hsname.dirfd = msname.dirfd;
  errno = 0;
  hsname.fd = openat(dsk.fd, hsname.name,
		     O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (errno)
    {
      errout("openat hsname");
      exit(1);
    }
}

struct file *findprog(char *name)
{
  int fd;
  for (int i = 0; i < QTY_SYSDIRS; i++)
    {
      if (sysdirs[i].fd == -1)
	continue;
      if ((fd = faccessat(sysdirs[i].fd, name, X_OK, 0)) != -1)
	return &(sysdirs[i]);
    }
  return 0;
}

static inline char *skip_ws(char *buf)
{
  while (*buf == ' ')
    buf++;
  return buf;
}

char *parse_fname(struct file *f, char *str)
{
  char *eow;
  str = skip_ws(str);

  while ((eow = strpbrk(str, ":; ,")) != NULL)
    {
      switch (*eow)
	{
	case ':':
	  *eow = 0;
	  if (strcmp(str, "dsk") != 0)
	    {
	      fprintf(stderr, " %s unknown device? ", str);
	      return NULL;
	    }
	  f->devfd = dsk.fd;
	  break;
	case ';':
	  *eow = 0;
	  fprintf(stderr, " found dir %s; (NSY) ", str);
	  f->dirfd = -1;
	  return NULL;
	  break;
	case ' ':
	  *eow = 0;
	  if (f->name) free(f->name);
	  f->name = strdup(str);
	  break;
	case ',':
	  *eow = 0;
	  if ((eow - str) > 1)
	    {
	      if (f->name) free(f->name);
	      f->name = strdup(str);
	    }
	  return ++eow;
	  break;
	}
      str = ++eow;
      str = skip_ws(str);
    }

  if (*str)
    {
      if (f->name) free(f->name);
      f->name = strdup(str);
    }

  return str + strlen(str);
}

void delete_file(char *name)
{
  struct file parsed = { 0, dsk.fd, msname.fd, -1 };
  char *p = parse_fname(&parsed, name);
  fputs("\r\n", stderr);
  if (p == NULL)
    return;
  if (parsed.name == NULL)
    {
      fprintf(stderr, " no defaulting yet\r\n");
      return;
    }
  errno = 0;
  if (unlinkat(parsed.dirfd, parsed.name, 0) == -1)
    errout(0);
  free(parsed.name);
}

void cwd(char *arg)
{
  struct file parsed = { 0, dsk.fd, -1, -1 };

  fputs("\r\n", stderr);

  char *p = parse_fname(&parsed, arg);
  if (p == NULL)
    return;
      
  if (parsed.name == NULL)
    {
      parsed.name = strdup(hsname.name);
      parsed.devfd = hsname.devfd;
      parsed.dirfd = hsname.dirfd;
    }

  int fd;
  errno = 0;
  fd = openat(msname.fd, parsed.name,
	      O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (!errno)
    {
      if (msname.name) free(msname.name);
      msname.name = parsed.name;
      msname.devfd = parsed.devfd;
      msname.dirfd = parsed.dirfd;
      if (msname.fd != -1) close(msname.fd);
      msname.fd = fd;
    }
  else
    errout("cwd");
}
