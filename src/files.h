struct file {
  char *name;
  int devfd;
  int dirfd;
  int fd;
};

void files_init(void);

void delete_file(char *name);
