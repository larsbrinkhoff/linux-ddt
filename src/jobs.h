struct process {
  // struct process *next;
  struct file ufname;
  char **argv;
  char **env;
  pid_t pid;
  int status;
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
void stop_currjob(void);

void select_job(char *jname);
void listj(char *);
void show_currjob(char *arg);
void kill_currjob(char *);

void set_currjname(char *jname);
void jcl(char *argstr);
void jclprt(char *);
void massacre(char *);
void load_prog(char *name);
void go(char *addr);
void gzp(char *addr);
void contin(char *);
void proced(char *);
void lfile(char *);
void forget(char *);
void self(char *);
void retry_job(char *jname, char *arg);

void errout(char *arg);

extern struct termios def_termios;
extern struct job *currjob;
extern int clobrf;
extern int genjfl;
