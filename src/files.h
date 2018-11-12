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
void nfdir(char *arg);
void ofdir(char *arg);
void print_file(char *arg);

void typeout_fname(struct file *f);

extern struct file devices[];
extern struct file hsname;
extern struct file msname;

#define DEVDSK 0
