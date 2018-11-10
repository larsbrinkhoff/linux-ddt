struct file {
  char *name;
  int devfd;
  int dirfd;
  int fd;
};

void files_init(void);
struct file *findprog(char *name);
void delete_file(char *name);
void cwd(char *arg);

extern struct file dsk;
extern struct file hsname;
extern struct file msname;
