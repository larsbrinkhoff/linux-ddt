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
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "aeval.h"

union val {
  uint64_t i;
  double f;
};

// TODO: errout needs to be moved out of jobs.c
#include "jobs.h"

#define ALT_(c)	((c)+0x80)

int base = 10;

char *evalfactor(char *expr, uint64_t *value)
{
  union val v;
  if (*expr == '-')
    {
      expr++;
      expr = evalexpr(expr, value);
      *value = -*value;
    }
  else if (isdigit(*expr))
    {
      // uint64_t v;
      char *end;
      errno = 0;
      v.i = strtoull(expr, &end, base);
      if (errno)
	{
	  errout("evalfactor");
	}
      else if (*end == '.' || *end == 'E' || *end == 'e')
      	{
      	  v.f = strtod(expr, &end);
      	  errno = 0;
      	  if (errno)
      	    errout("evalfactor");
      	  expr = end;
      	  *value = v.i;
	  fprintf(stderr, "(%f)", v.f);
      	}
      else
	{
	  *value = v.i;
	  expr = end;
	}
    }
  else
    expr = NULL;

  return expr;
}

static char *logictail(char *expr, uint64_t *value)
{
  uint64_t result = 0;
  switch (*expr)
    {
    case '#':
      if ((expr = evalfactor(++expr, &result)) == NULL)
	return expr;
      *value = *value ^ result;
      break;
    case '&':
      if ((expr = evalfactor(++expr, &result)) == NULL)
	return expr;
      *value = *value & result;
      break;
    case '|':
      if ((expr = evalfactor(++expr, &result)) == NULL)
	return expr;
      *value = *value | result;
      break;
    default:
      return expr;
    }
  return logictail(expr, value);
}

char *evallogic(char *expr, uint64_t *value)
{
  uint64_t result = 0;
  
  if ((expr = evalfactor(expr, &result)) == NULL)
    return expr;
  if ((expr = logictail(expr, &result)) == NULL)
    return expr;

  *value = result;

  return expr;
}

static char *termtail(char *expr, uint64_t *value)
{
  union val v;
  union val result;
  result.i = 0;
  switch ((unsigned char)*expr)
    {
    case '*':
      if ((expr = evallogic(++expr, &result.i)) == NULL)
	return expr;
      *value = *value * result.i;
      break;
    case '!':
      if ((expr = evallogic(++expr, &result.i)) == NULL)
	return expr;
      *value = *value / result.i;
      break;
    case ALT_('*'):
      if ((expr = evallogic(++expr, &result.i)) == NULL)
    	return expr;
      v.i = *value;
      v.f = v.f * result.f;
      *value = v.i;
      break;
    case ALT_('!'):
      if ((expr = evallogic(++expr, &result.i)) == NULL)
	return expr;
      v.i = *value;
      v.f = v.f / result.f;
      *value = v.i;
      break;
    default:
      return expr;
    }
  return termtail(expr, value);
}

char *evalterm(char *expr, uint64_t *value)
{
  uint64_t result = 0;
  
  if ((expr = evallogic(expr, &result)) == NULL)
    return expr;
  if ((expr = termtail(expr, &result)) == NULL)
    return expr;

  *value = result;

  return expr;
}

static char *exprtail(unsigned char *expr, uint64_t *value)
{
  union val v;
  union val result;
  result.i = 0;

  switch (*expr)
    {
    case '+':
      if ((expr = evalterm(++expr, &result.i)) == NULL)
	return expr;
      *value = *value + result.i;
      break;
    case '-':
      if ((expr = evalterm(++expr, &result.i)) == NULL)
	return expr;
      *value = *value - result.i;
      break;
    case ALT_('+'):
      if ((expr = evallogic(++expr, &result.i)) == NULL)
    	return expr;
      v.i = *value;
      v.f = v.f + result.f;
      *value = v.i;
      break;
    case ALT_('-'):
      if ((expr = evallogic(++expr, &result.i)) == NULL)
	return expr;
      v.i = *value;
      v.f = v.f - result.f;
      *value = v.i;
      break;
    default:
      return expr;
    }
  return exprtail(expr, value);
}

char *evalexpr(char *expr, uint64_t *value)
{
  uint64_t result = 0;

  if ((expr = evalterm(expr, &result)) == NULL)
    return expr;
  if ((expr = exprtail(expr, &result)) == NULL)
    return expr;

  *value = result;

  return expr;
}
