struct builtin {
  char *name;
  char *arghelp;
  char *desc;
  void (*fn) (char *arg);
};

void init_ccmd(void);
void ccmd(char *cmdline);
void clear(char *);

