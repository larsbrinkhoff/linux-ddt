struct builtin {
  char *name;
  char *arghelp;
  char *desc;
  void (*fn) (void);
};

void ccmd(char *cmdline);

void version(void);
void clear(void);
