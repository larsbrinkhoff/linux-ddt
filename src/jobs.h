struct process {
  // struct process *next;
  char *prog;
  char **argv;
  char **env;
  pid_t pid;
  int status;
  int dirfd;
};

struct job {
  char *jname;
  char *jcl;
  char state;
  char slot;
  struct process proc;
};

void job(char *jname);
void kill_currjob(void);
