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
      errout("cwd");
      exit(1);
    }
  errno = 0;
  msname.fd = openat(AT_FDCWD, msname.name,
		     O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (errno)
    errout("openat cwd");
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

void delete_file(char *name)
{
  fputs("\r\n", stderr);
  errno = 0;
  if (unlinkat(msname.fd, name, 0) == -1)
    errout(0);
}
