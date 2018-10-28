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
  struct termios tmode;
  struct process proc;
};

void jobs_init(void);
int fgwait(void);
void check_jobs(void);
void list_currjob(void);
void next_job(void);

void job(char *jname);
void listj(char *);
void show_currjob(char *arg);
void kill_currjob(char *);

void set_currjname(char *jname);
void jcl(char *argstr);
void jclprt(char *);
void massacre(char *);
void load(char *name);
void go(char *addr);
void gzp(char *addr);
void contin(char *);
void proced(char *);
void errout(char *arg);

extern struct termios def_termios;
