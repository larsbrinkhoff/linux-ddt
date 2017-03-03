#include <stdlib.h>
#include "term.h"
#include "dispatch.h"

static void cleanup (void)
{
  term_restore ();
}

int main (int argc, char **argv)
{
  atexit (cleanup);
  term_init ();
  dispatch_init ();

  for (;;)
    prompt_and_execute ();

  return 0;
}
