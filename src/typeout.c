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
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include "typeout.h"

typeoutfunc *mperce = tmc;	/* tms */
typeoutfunc *mamper = tmc;	/* tmsq */
typeoutfunc *mdolla = tmc;	/* tms */
typeoutfunc *mprime = tmc;	/* tm6 */
typeoutfunc *mdquot = tma;
typeoutfunc *mnmsgn = tmch;
typeoutfunc *mch = tmc;
typeoutfunc *sch = tmc;

static void outchar(unsigned char c)
{
  if (c & 0x80)
    {
      fputc('$', stderr);
      c &= 0x7f;
    }
  if (c == 0x7f)
    {
      fputc('^', stderr);
      c = '?';
    }
  else if (iscntrl(c))
    {
      fputc('^', stderr);
      c += 64;
    }
  fputc(c, stderr);
}

void tma(uint64_t value)
{
  unsigned char *str = (char *)value;
  for (int i = 0; str[i]; i++)
    outchar(str[i]);
  fputs("   ", stderr);
}

void tmch(uint64_t value)
{
  char c = (char)(value & 0x7f);

  fputs("$1#", stderr);
  outchar(c);
  fputs("   ", stderr);
}

static char radix = 16;
static char tradix = 16;
static char radixchars[] = "0123456789abcdefghijklmnopqrstuvwxyz";

void resetradix(void)
{
  tradix = radix;
}

static void outradix(uint64_t value)
{
  int i = 64;
  char str[65];
  str[i] = 0;
  for (i = 64; i--;)
    {
      str[i] = radixchars[(value % tradix)];
      if (!(value /= tradix))
	break;
    }
  fputs(&str[i], stderr);
}

void resettypeo(void)
{
  sch = mch;
}

void settypeo(typeoutfunc *f, int perm)
{
  sch = f;
  if (perm)
    mch = sch;
}

void setradix(int r, int perm)
{
  tradix = (r % 36);
  if (perm > 0)
      radix = tradix;
  settypeo(tmc, perm);
}

void tmc(uint64_t value)
{
  outradix(value);
  fputs("   ", stderr);
}

union val {
  uint64_t i;
  double f;
};

void tmf(uint64_t value)
{
  union val v;
  v.i = value;
  fprintf(stderr, "%f", v.f);
  fputs("   ", stderr);
}

void tmh(uint64_t value)
{
  outradix(value >> 32);
  fputs(",,", stderr);
  outradix(value & 0xffffffff);
  fputs("   ", stderr);
}
