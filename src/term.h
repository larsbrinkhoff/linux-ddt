void term_init (void);
void term_restore (void);
void term_raw (void);
int term_read (void);
void clear(char *);
int uquery(char *text);

extern struct winsize winsz;
