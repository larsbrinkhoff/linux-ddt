#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include "files.h"
#include "jobs.h"

#define PATH_MAX 4096

#define QTY_DEVICES 1
#define QTY_SYSDIRS 4
#define QTY_FDIRS 8

struct file devices[QTY_DEVICES] = { {"dsk", -1, -1, -1} };
struct file sysdirs[QTY_SYSDIRS] = { {"bin", -1, -1, -1},
				     {"sbin", -1, -1, -1},
				     {"usr/bin", -1, -1, -1},
				     {"usr/sbin", -1, -1, -1}
};

struct file *finddirs[QTY_FDIRS] = { 0 };

struct file hsname = { 0, -1, -1, -1 };
struct file msname = { 0, -1, -1, -1 };
struct file deffile = { 0, -1, -1, -1 };

int open_ro(int dirfd, char *path)
{
  int fd;

  errno = 0;

  while ((fd = openat(dirfd, path, 0, O_RDONLY)) == -1)
    if (errno == EINTR)
      {
	errno = 0;
	continue;
      }
    else
      return -1;
  return fd;
}

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
  devices[DEVDSK].fd = devices[DEVDSK].dirfd = devices[DEVDSK].devfd = fd;

  for (int i = 0; i < QTY_SYSDIRS; i++)
    {
      sysdirs[i].fd = open_dirpath(devices[DEVDSK].fd, sysdirs[i].name);
      if (errno)
	{
	  errout(sysdirs[i].name);
	  continue;
	}
      sysdirs[i].devfd = devices[DEVDSK].fd;
      sysdirs[i].dirfd = devices[DEVDSK].fd;
    }
  
  msname.name = malloc(PATH_MAX);
  errno = 0;
  if (getcwd(msname.name, PATH_MAX) == 0)
    {
      errout("getcwd");
      exit(1);
    }

  msname.fd = open_dirpath(devices[DEVDSK].fd, msname.name);
  if (errno)
    {
      errout(msname.name);
      exit(1);
    }

  hsname.name = strdup(msname.name);
  hsname.devfd = devices[DEVDSK].fd;
  hsname.dirfd = msname.dirfd;

  hsname.fd = open_dirpath(devices[DEVDSK].fd, hsname.name);
  if (errno)
    {
      errout(hsname.name);
      exit(1);
    }

  deffile.name = strdup(".foo.");
  deffile.devfd = devices[DEVDSK].fd;
  deffile.dirfd = hsname.fd;
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
  for (int i = 0; i < QTY_FDIRS; i++)
    {
      if (finddirs[i] == NULL)
	break;
      if ((fd = faccessat(finddirs[i]->fd, name, X_OK, 0)) != -1)
	{
	  fprintf(stderr, " %s;\r\n", finddirs[i]->name);
	  return finddirs[i];
	}
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
	  f->devfd = devices[DEVDSK].fd;
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

static void setdeffile(struct file *f)
{
  if (f->name)
    {
      if (deffile.name) free(deffile.name);
      deffile.name = f->name;
    }

  deffile.devfd = f->devfd;
  deffile.dirfd = f->dirfd;
}

void delete_file(char *name)
{
  struct file parsed = { strdup(deffile.name), deffile.devfd, deffile.dirfd, -1 };
  char *p = parse_fname(&parsed, name);
  fputs("\r\n", stderr);
  if (p == NULL)
    {
      free(parsed.name);
      return;
    }

  errno = 0;
  if (unlinkat(parsed.dirfd, parsed.name, 0) == -1)
    {
      errout(parsed.name);
      free(parsed.name);
    }
  else
    setdeffile(&parsed);
}

void cwd(char *arg)
{
  struct file parsed = { 0, devices[DEVDSK].fd, -1, -1 };

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

static int equivdirs(struct file *a, struct file *b)
{
  return (strcmp(a->name, b->name) == 0
	  && a->dirfd == b->dirfd
	  && a->devfd == b->devfd);
}

static void delete_fdir(struct file *fdir)
{
  int i;
  for (i = 0; i < QTY_FDIRS; i++)
    {
      if (equivdirs(fdir, finddirs[i]))
	break;
    }
  if (i == QTY_FDIRS)
    return;

  for (i++; i < QTY_FDIRS; i++)
    {
      finddirs[i-1] = finddirs[i];
    }
  finddirs[i-1] = 0;
}

static void insert_fdir(struct file *fdir)
{
  struct file *ins = NULL;
  struct file *t = fdir;
  for (int i = 0; i < QTY_FDIRS; i++)
    {
      ins = t;
      t = finddirs[i];
      finddirs[i] = ins;
      if (t == 0) break;
      if (equivdirs(t, fdir))
	break;
    }
}

static void parse_fnames(struct file *parsed[], int n, char *arg)
{
  char *p;
  for (int i = 0; i < n; i++) {
    parsed[i] = (struct file *)malloc(sizeof(struct file));

    parsed[i]->name = 0;
    parsed[i]->devfd = devices[DEVDSK].fd;
    parsed[i]->dirfd = msname.fd;
    parsed[i]->fd = -1;

    p = parse_fname(parsed[i], arg);
    if (p == NULL)
      {
	free(parsed[i]);
	parsed[i] = NULL;
	continue;
      }

    if (parsed[i]->name == NULL)
      {
	fprintf(stderr, " arg %d noname? ", i+1);
	free(parsed[i]);
	parsed[i] = NULL;
	continue;
      }
    if (*p)
      arg = p;
    else
      break;
  }
  if (*p)
    fprintf(stderr, " ign args >%d? ", n);
}

void ofdir(char *arg)
{
  struct file *parsed[QTY_FDIRS] = { 0 };

  fputs("\r\n", stderr);

  parse_fnames(parsed, QTY_FDIRS, arg);

  for (int i = 0; i < QTY_FDIRS; i++)
    if (parsed[i] != NULL)
      delete_fdir(parsed[i]);
}

void nfdir(char *arg)
{
  struct file *parsed[QTY_FDIRS] = { 0 };

  fputs("\r\n", stderr);

  parse_fnames(parsed, QTY_FDIRS, arg);

  for (int i = QTY_FDIRS; --i >= 0; )
    {
      if (parsed[i] == NULL)
	continue;

      int fd;
      fd = open_dirpath(parsed[i]->dirfd, parsed[i]->name);
      if (errno)
	{
	  errout(parsed[i]->name);
	  continue;
	}
      parsed[i]->fd = fd;
      insert_fdir(parsed[i]);
    }
}

void typeout_fname(struct file *f)
{
  int i;
  for (int i = 0; i < QTY_DEVICES; i++)
    {
      if (f->devfd == devices[i].fd)
	{
	  fprintf(stderr, "%s: ", devices[i].name);
	  break;
	}
    }
  if (i == QTY_DEVICES)
    fprintf(stderr, "%d: ", devices[i].fd);

  fprintf(stderr, "%d; %s (%d)",
	  f->dirfd,
	  f->name,
	  f->fd);
}

void print_file(char *arg)
{
  struct file parsed = { strdup(deffile.name), deffile.devfd, deffile.dirfd, -1 };

  fputs("\r\n", stderr);

  if (arg && *arg)
    if (parse_fname(&parsed, arg) == NULL)
      goto error;

  if ((parsed.fd = open_ro(parsed.dirfd, parsed.name)) == -1)
    goto error;

  struct stat fstatus;
  errno = 0;
  if (fstat(parsed.fd, &fstatus) == -1)
    goto close1;

  if ((fstatus.st_mode & S_IFMT) != S_IFREG)
    {
      fprintf(stderr, "%s - regular files only\r\n", parsed.name);
      goto close1;
    }

  fprintf(stderr, "Would print %s (%ld bytes)\r\n", parsed.name, fstatus.st_size);

  setdeffile(&parsed);
  parsed.name = 0;

  int terrno;
 close1:
  terrno = errno;
  close(parsed.fd);
  errno = terrno;

 error:
  if (errno)
    errout(parsed.name);

  free(parsed.name);
}

void list_files(char *arg, int setdefp)
{
  struct file parsed = { 0, deffile.devfd, deffile.dirfd, -1 };

  fputs("\r\n", stderr);

  if (arg && *arg)
    if (parse_fname(&parsed, arg) == NULL)
      goto error;

  typeout_fname(&parsed);
  fputs("\r\n", stderr);

  if ((parsed.fd = open_ro(parsed.dirfd, ".")) == -1)
    goto error;

  struct stat fstatus;
  errno = 0;
  if (fstat(parsed.fd, &fstatus) == -1)
    goto close1;

  if ((fstatus.st_mode & S_IFMT) != S_IFDIR)
    {
      fprintf(stderr, " directories only\r\n");
      goto close1;
    }

  struct dirent **namelist;
  int n;
  if ((n = scandirat(parsed.dirfd, ".", &namelist, NULL, versionsort)) == -1)
    goto error;

  for (int i = 0; i < n; i++) {
    errno = 0;
    if (fstatat(parsed.dirfd, namelist[i]->d_name, &fstatus, 0) == -1)
      {
	continue;
      }

    char ftypec;
    switch (fstatus.st_mode & S_IFMT)
      {
      case S_IFBLK:  ftypec = 'b'; break;
      case S_IFCHR:  ftypec = 'c'; break;
      case S_IFDIR:  ftypec = 'd'; break;
      case S_IFIFO:  ftypec = 'p'; break;
      case S_IFLNK:  ftypec = 'l'; break;
      case S_IFREG:  ftypec = 'f'; break;
      case S_IFSOCK: ftypec = 's'; break;
      default: ftypec = '?'; break;
      }

    fprintf(stderr, " %c %-16s ", ftypec, namelist[i]->d_name);
    if ((fstatus.st_mode & S_IFMT) == S_IFLNK)
      fprintf(stderr, "symlinkhere\r\n");
    else
      {
	struct tm *t = localtime(&(fstatus.st_mtim.tv_sec));
	fprintf(stderr, "%-6ld %02d/%02d/%04d %02d:%02d:%02d\r\n",
		fstatus.st_blocks,
		t->tm_mon+1, t->tm_mday, t->tm_year + 1900,
		t->tm_hour, t->tm_min, t->tm_sec);
      }
    free(namelist[i]);
  }
  free(namelist);

  if (setdefp)
    setdeffile(&parsed);

  int terrno;
 close1:
  terrno = errno;
  close(parsed.fd);
  errno = terrno;

 error:
  if (errno)
    errout(parsed.name);
}

void listf(char *arg)
{
  list_files(arg, 1);
}
