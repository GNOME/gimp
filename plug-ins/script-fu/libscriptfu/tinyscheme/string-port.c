/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "scheme-private.h"
#include "string-port.h"
#include "scheme.h"  /* mk_port */

/* Uses GLib types but not Gimp.
 * No conditional compilation.
 */

#define BLOCK_SIZE 256

#define min(a, b)  ((a) <= (b) ? (a) : (b))

/* StringPort
 *
 * Methods dealing with a scheme string-port.
 * This encapsulates access to the struct string (a kind of port)
 * but the declaration of that struct is not actually hidden.
 */



/* Local
 *
 * Mostly untouched as extracted from scheme.c
 * FUTURE: refactor, bug fixes, and reformat style.
 */
static port*
port_rep_from_string(scheme *sc, char *start, char *past_the_end, int prop) {
  port *pt;
  pt=(port*)sc->malloc(sizeof(port));
  if(pt==0) {
    return 0;
  }
  pt->kind=port_string|prop;
  pt->rep.string.start=start;
  pt->rep.string.curr=start;
  pt->rep.string.past_the_end=past_the_end;
  return pt;
}

static pointer
port_from_string(scheme *sc, char *start, char *past_the_end, int prop) {
  port *pt;
  pt=port_rep_from_string(sc,start,past_the_end,prop);
  if(pt==0) {
    return sc->NIL;
  }
  return mk_port(sc,pt);
}

static port*
port_rep_from_scratch(scheme *sc) {
  port *pt;
  char *start;
  pt=(port*)sc->malloc(sizeof(port));
  if(pt==0) {
    return 0;
  }
  start=sc->malloc(BLOCK_SIZE);
  if(start==0) {
    return 0;
  }
  memset(start,' ',BLOCK_SIZE-1);
  start[BLOCK_SIZE-1]='\0';
  pt->kind=port_string|port_output|port_srfi6;
  pt->rep.string.start=start;
  pt->rep.string.curr=start;
  pt->rep.string.past_the_end=start+BLOCK_SIZE-1;
  return pt;
}

static pointer
port_from_scratch(scheme *sc) {
  port *pt;
  pt=port_rep_from_scratch(sc);
  if(pt==0) {
    return sc->NIL;
  }
  return mk_port(sc,pt);
}

static int
realloc_port_string(scheme *sc, port *p)
{
  char *start=p->rep.string.start;
  size_t new_size=p->rep.string.past_the_end-start+1+BLOCK_SIZE;
  char *str=sc->malloc(new_size);
  if(str) {
    memset(str,' ',new_size-1);
    str[new_size-1]='\0';
    strcpy(str,start);
    p->rep.string.start=str;
    p->rep.string.past_the_end=str+new_size-1;
    p->rep.string.curr-=start-str;
    sc->free(start);
    return 1;
  } else {
    return 0;
  }
}



/* Exported
 *
 * Naming follows gimp/glib convention: string_port_<method>
 */

gint
string_port_inbyte (port *pt)
{
  if (pt->rep.string.curr == pt->rep.string.past_the_end)
    return EOF;
  else
    /* Cast so byte is not sign extended to a negative int. */
    return (guint8) *pt->rep.string.curr++;
}

void
string_port_backbyte (port *pt)
{
  if (pt->rep.string.start != NULL &&
      pt->rep.string.curr > pt->rep.string.start)
    {
      /* !!! Not actually writing a byte.  Port contents are read-only. */
      pt->rep.string.curr--;
    }
}

void
string_port_put_bytes (scheme *sc, port *pt, const gchar *bytes, guint byte_count)

{
  int   free_bytes;     /* Space remaining in buffer (in bytes) */
  int   l;

  if (pt->rep.string.past_the_end != pt->rep.string.curr)
    {
       free_bytes = pt->rep.string.past_the_end - pt->rep.string.curr;
       l          = min (byte_count, free_bytes);
       memcpy (pt->rep.string.curr, bytes, l);
       pt->rep.string.curr += l;
    }
    else if(pt->kind&port_srfi6&&realloc_port_string(sc,pt))
    {
       free_bytes = pt->rep.string.past_the_end - pt->rep.string.curr;
       l          = min (byte_count, free_bytes);
       memcpy (pt->rep.string.curr, bytes, byte_count);
       pt->rep.string.curr += l;
    }
}



/* Constructor and destructor of string-port object. */


/* Implementation of Scheme OPEN-OUTPUT_STRING.
 *
 * Returns scheme pointer to port, or to NIL.
 *
 * This is a "new" method.
 * A port struct is allocated.
 * It, and any allocation owned by the port, should later be freed,
 * when the cell containing the port struct is reclaimed.
 *
 * FUTURE: scheme-string should be ignored.
 */
pointer
string_port_open_output_string (scheme *sc, pointer scheme_string)
{
  pointer result;
  gchar  *c_string            = scheme_string->_object._string._svalue;
  int     string_length_chars = scheme_string->_object._string._length;

  if(scheme_string == sc->NIL)
    result = port_from_scratch(sc);
  else
    result = port_from_string (sc,
                               c_string,
                               /* address arithmetic */
                               /* FIXME: this should be strlen(c_string) */
                               c_string+string_length_chars,
                               port_output);
  return result;
}


pointer
string_port_open_input_string (scheme *sc, pointer scheme_string, int prop)
{
  gchar *c_string            = scheme_string->_object._string._svalue;
  int    string_length_chars = scheme_string->_object._string._length;

  return port_from_string (sc,
                           c_string,
                           /* address arithmetic */
                           /* FIXME: this should be strlen(c_string) */
                           c_string + string_length_chars,
                           prop);
}

/* Free any heap allocation of the Scheme object.
 * Called during garbage collection, the cell itself is reclaimed.

 * Require port is-a string-port.
 */
void
string_port_dispose (scheme  *sc, pointer port)
{
  /* FIXME, not disposing any allocated buffer. */

  /* Free the allocated struct itself. */
  sc->free(port->_object._port);
}



/* Implementation of Scheme GET-OUTPUT-STRING.
 *
 * Returns scheme pointer to a scheme string or to sc->F.
 *
 * Requires port is-a string-port.
 * Unlike most Scheme, does not require port is kind output.
 */
pointer
string_port_get_output_string (scheme *sc, port *p)
{
  off_t size;
  char *str;

  size=p->rep.string.curr-p->rep.string.start+1;
  str=sc->malloc(size);
  if(str != NULL)
    {
      pointer s;

      /* FIXME, we don't need to copy, since mk_string does.
       * If it is invariant that there is a NUL in the port's string?
       */
      memcpy(str,p->rep.string.start,size-1);
      str[size-1]='\0';
      /* mk_string copies yet again. */
      s=mk_string(sc,str);
      sc->free(str);
      return s;
    }
  else
    {
      return sc->F;
    }
}

/* Initialize a static port struct to be an input string-port,
 * from the given command string.
 *
 * Specialized: assert port is load_stack[0], statically allocated.
 * It is not finalized or disposed.
 * In this case, the string-port contents are not allocated, but borrowed.
 *
 * The command string is:
 *    read-only
 *    owned by the caller
 *    its lifetime is the interpretation session.
 *    is NUL terminated.
 */
void
string_port_init_static_port (port *port, const gchar *command)
{
  /* Discard const qualifier.
   * Assert input string-port respects read-only.
   * No scheme write operations are allowed on input string-ports.
   */
  char *c_string = (char*) command;

  port->kind=port_input|port_string;
  port->rep.string.start= c_string;
  port->rep.string.past_the_end=c_string+strlen(c_string);
  port->rep.string.curr=c_string;
}