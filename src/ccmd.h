struct builtin {
  char *name;
  char *arghelp;
  char *desc;
  void (*fn) (char *arg);
};

void ccmd(char *cmdline);
void clear(char *);
