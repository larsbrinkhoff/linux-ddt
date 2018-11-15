struct builtin {
  char *name;
  char *arghelp;
  char *desc;
  void (*fn) (char *arg);
};

void init_ccmd(void);
void ccmd(char *cmdline);
void set_ddtmode(char *);
void set_monmode(char *);

extern const char *prompt;
extern int monmode;
