
#include <stdio.h>
#include <stdarg.h>

#include "trace.h"

/* temporary logging facilities */


static char indent[] = "                                                 ";
static int level = 48;

void 
trace_enter  (
              char * func
              )
{
  printf ("%s%s () {\n", indent+level, func);
  level -= 2;
}


void 
trace_exit  (
             void
             )
{
  level += 2;
  printf ("%s}\n", indent+level);
}


void 
trace_begin  (
              char * format,
              ...
              )
{
  va_list args, args2;
  char *buf;
  extern char * g_vsprintf ();
    
  va_start (args, format);
  va_start (args2, format);
  buf = g_vsprintf (format, &args, &args2);
  va_end (args);
  va_end (args2);

  fputs (indent+level, stdout);
  fputs (buf, stdout);
  fputs (": {\n", stdout);

  level -= 2;
}


void 
trace_end  (
            void
            )
{
  level += 2;
  printf ("%s}\n", indent+level);
}


void 
trace_printf  (
               char * format,
               ...
               )
{
  va_list args, args2;
  char *buf;
  extern char * g_vsprintf ();

  va_start (args, format);
  va_start (args2, format);
  buf = g_vsprintf (format, &args, &args2);
  va_end (args);
  va_end (args2);

  fputs (indent+level, stdout);
  fputs (buf, stdout);
  fputc ('\n', stdout);
}


  

