/*
SPDX-License-Identifier: GPL-3.0-or-later

This file is part of Linux-ddt.

Linux-ddt is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

Linux-ddt is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Linux-ddt. If not, see <https://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include "term.h"
#include "dispatch.h"
#include "jobs.h"
#include "user.h"

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

  greet();

  for (;;)
    if (!fgwait())
      {
	check_jobs();
	prompt_and_execute ();
      }

  return 0;
}
