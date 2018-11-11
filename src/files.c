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

int open_dirpath(int dirfd, char *path)
{
  int fd;

  errno = 0;

  while ((fd = openat(dirfd, path, O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW)) == -1)
    if (errno == EINTR)
      {
	errno = 0;
	continue;
      }
    else
      return -1;
  return fd;
}

void files_init(void)
{
  int fd;

  fd = open_dirpath(AT_FDCWD, "/");
  if (errno)
    {
     errout("/");
     exit(1);
    }
  dsk.fd = dsk.dirfd = dsk.devfd = fd;

  for (int i = 0; i < QTY_SYSDIRS; i++)
    {
      sysdirs[i].fd = open_dirpath(dsk.fd, sysdirs[i].name);
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

  msname.fd = open_dirpath(dsk.fd, msname.name);
  if (errno)
    {
      errout(msname.name);
      exit(1);
    }

  hsname.name = strdup(msname.name);
  hsname.devfd = dsk.fd;
  hsname.dirfd = msname.dirfd;

  hsname.fd = open_dirpath(dsk.fd, hsname.name);
  if (errno)
    {
      errout(hsname.name);
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
  if ((fd = faccessat(msname.fd, name, X_OK, 0)) != -1)
    return &msname;
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
  fd = open_dirpath(msname.fd, parsed.name);
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
    errout(parsed.name);
}
