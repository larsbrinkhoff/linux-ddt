#include <stdlib.h>
#include "term.h"
#include "dispatch.h"

static void cleanup (void)
{
  term_restore ();
}

int main (int argc, char **argv)
{
  term_init ();
  atexit (cleanup);
  dispatch_init ();

  for (;;)
    prompt_and_execute ();

  return 0;
}
