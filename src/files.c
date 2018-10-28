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

struct file hsname = { 0, -1, -1, -1 };
struct file msname = { 0, -1, -1, -1 };

void files_init(void)
{
  msname.name = malloc(PATH_MAX);
  errno = 0;
  if (getcwd(msname.name, PATH_MAX) == 0)
    {
      errout("getcwd");
      exit(1);
    }
  errno = 0;
  msname.dirfd = openat(AT_FDCWD, "/",
			O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (errno)
    errout("openat /");
  errno = 0;
  msname.fd = openat(msname.dirfd, msname.name,
		     O_PATH | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (errno)
    errout("openat cwd");
}

void delete_file(char *name)
{
  fputs("\r\n", stderr);
  errno = 0;
  if (unlinkat(msname.fd, name, 0) == -1)
    errout(0);
}
