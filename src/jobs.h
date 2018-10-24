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
void listj(char *);
void show_currjob(char *arg);
void kill_currjob(char *);

void set_currjname(char *jname);
void list_currjob(void);
void next_job(void);
void jcl(char *argstr);
void jclprt(char *);
void massacre(char *);
void load(char *name);
