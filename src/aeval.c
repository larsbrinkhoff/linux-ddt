#include <stdint.h>
#include <stdio.h>

char *evalfactor(char *expr, uint64_t *value)
{
  if (*expr == '1')
    {
      *value = 1;
      return ++expr;
    }
  return NULL;
}

static char *logictail(char *expr, uint64_t *value)
{
  uint64_t result;
  switch (*expr)
    {
    case '#':
      if ((expr = evalfactor(++expr, &result)) == NULL)
	return expr;
      *value = *value ^ result;
      expr = logictail(expr, value);
      break;
    case '&':
      if ((expr = evalfactor(++expr, &result)) == NULL)
	return expr;
      *value = *value & result;
      expr = logictail(expr, value);
      break;
    }
  return expr;
}

char *evallogic(char *expr, uint64_t *value)
{
  uint64_t result = *value;
  
  if ((expr = evalfactor(expr, &result)) == NULL)
    return expr;
  expr = logictail(expr, &result);

  *value = result;

  return expr;

  uint64_t left, right;
  char *r = evalfactor(expr, &left);
  if (r)
    *value = left;
  
  return r;
}

static char *termtail(char *expr, uint64_t *value)
{
  uint64_t result;
  switch (*expr)
    {
    case '*':
      if ((expr = evallogic(++expr, &result)) == NULL)
	return expr;
      *value = *value * result;
      expr = termtail(expr, value);
      break;
    case '!':
      if ((expr = evallogic(++expr, &result)) == NULL)
	return expr;
      *value = *value / result;
      expr = termtail(expr, value);
      break;
    }
  return expr;
}

char *evalterm(char *expr, uint64_t *value)
{
  uint64_t result = *value;
  
  if ((expr = evallogic(expr, &result)) == NULL)
    return expr;
  expr = termtail(expr, &result);

  *value = result;

  return expr;
}

static char *exprtail(char *expr, uint64_t *value)
{
  uint64_t result;
  switch (*expr)
    {
    case '+':
      if ((expr = evalterm(++expr, &result)) == NULL)
	return expr;
      *value = *value + result;
      expr = exprtail(expr, value);
      break;
    case '-':
      if ((expr = evalterm(++expr, &result)) == NULL)
	return expr;
      *value = *value - result;
      expr = exprtail(expr, value);
      break;
    }
  return expr;
}

char *evalexpr(char *expr, uint64_t *value)
{
  uint64_t result = *value;

  if ((expr = evalterm(expr, &result)) == NULL)
    return expr;
  expr = exprtail(expr, &result);

  *value = result;

  return expr;
}
