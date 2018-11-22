#include <stdlib.h>
#include "term.h"
#include "dispatch.h"
#include "jobs.h"

static void cleanup (void)
{
  term_restore ();
}

int main (int argc, char **argv)
{
  jobs_init ();
  files_init();
  term_init ();
  atexit (cleanup);
  dispatch_init ();

  for (;;)
    if (!fgwait())
      {
	check_jobs();
	prompt_and_execute ();
      }

  return 0;
}
