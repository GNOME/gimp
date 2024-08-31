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
#include "scheme.h"  /* mk_port */

#include "string-port.h"

/* Uses GLib types but not Gimp.
 * No conditional compilation.
 */

/* Minimum size of allocations for
 * Allocations can be larger.
 *
 * A string-port buffer is NUL-terminated.
 * Invariant that the last byte is also NUL, for safety.
 * It doesn't need to be because the contents of the buffer are NUL terminated.
 * A string-port holds one less byte of content than the size of allocation.
 */
#define STRING_PORT_MIN_ALLOCATION 256


/* StringPort
 *
 * Methods dealing with a scheme string-port.
 * This encapsulates access to the struct string (a kind of port)
 * but the declaration of that struct is not actually hidden.
 *
 * We no longer implement on top of scheme strings,
 * nor use the port_srfi6 enum to mark ports as such.
 * SRFI6 is an alternative implementation in scheme instead of C.
 *
 * We don't implement input-output kind of string port
 * (not sure if the old implementation was tested or used.)
 */



/* Local */

/* Called on failure to malloc. */
static void
no_memory (scheme *sc, const gchar *allocation_reason)
{
  /* Make the eval cycle end. */
  sc->no_memory = 1;
  /* report what we were trying to allocate. */
  g_warning ("%s", allocation_reason);
}

/* Init a port struct for a string-port.
 *
 * The passed buffer is a byte buffer.
 * The contents are read-only.
 * Requires the last byte is NUL.
 *
 * This knows:
 *    - how to convert from buffer and size to char*.
 *    - initial read/write pointer (named curr) is always the start of buffer.)
 *    - invariant: last byte is NUL
 */
static void
init_port_struct (port *port, unsigned int kind, gchar *buffer, guint size)
{
  port->kind                    = kind;

  port->rep.string.start        = buffer;
  /* address arithmetic in byte pointers. */
  port->rep.string.past_the_end = buffer + size - 1;
  port->rep.string.curr         = buffer;
  /* Ensure:
   *   - start points to the first byte
   *   - past_the_end points to last byte
   *   - last byte is NUL
   * !!! past_the_end is not past the buffer,
   * but past the last byte which read/writes.
   */
  g_assert (*port->rep.string.past_the_end == '\0');
}

/* Reset a port struct newly created by expansion.
 * Make the port own a new buffer, different than it now owns.
 * The new buffer has same contents as old, but has a larger allocation.
 *
 * Requires the buffer is size.
 * Requires the last byte of buffer is NUL.
 * Requires curr_read points to a NUL in the middle of the buffer.
 */
static void
reset_output_port_struct (port *pt, gchar *buffer, size_t size, gchar * curr_read)
{
  init_port_struct (pt, port_string | port_output, buffer, size);

  /* curr is pointing to start, make it point into the middle. */
  pt->rep.string.curr = curr_read;

  /* byte at start is not NULL, but byte at curr is */
  g_assert (*pt->rep.string.curr == '\0');
}

/* Create a input port struct from a NUL-terminated C string.
 *
 * Requires passed C string is NUL terminated contents of a Scheme string.
 * The bytes are copied, as the original scheme string may go out of scope.
 */
static port*
input_port_struct_from_string (scheme *sc, char *start) {
  port  *port_struct;
  gchar *copy;
  guint  string_size_bytes = strlen (start);
  /* !!! The buffer size is plus one for a NUL. */
  guint  buffer_size_bytes = string_size_bytes + 1;

  /* Allocate port struct. */
  port_struct = (port*)sc->malloc (sizeof (port));
  if (port_struct == NULL)
    {
      no_memory (sc, "input port struct");
      return NULL;
    }


  /* Allocate and copy contents. */
  copy = sc->malloc (buffer_size_bytes);
  if (copy == NULL)
    {
      no_memory (sc, "input port buffer");
      return NULL;
    }

  /* Assert strcpy writes NUL at end of buffer. */
  strcpy (copy, start);

  init_port_struct (port_struct, port_string | port_input, copy, buffer_size_bytes);

  return port_struct;
}

/* Create a port of kind input from a NUL-terminated, UTF-8 encoded string.
 *
 * Returns a pointer to a Scheme string-port object, or NIL when no memory.
 *
 * This knows: create cell, and its contained port struct.
 */
static pointer
input_port_from_string (scheme *sc, char *start) {
  port *port_struct;

  port_struct = input_port_struct_from_string (sc, start);
  if (port_struct == NULL)
    return sc->NIL;
  else
    return mk_port (sc, port_struct);
}

/* Create a port struct of kind output having a fresh allocated buffer.
 *
 * Scratch means:
 * ensure buffer is:
 *    of default size.
 *    init to all zeroes.
 * and ensure port struct ready for writing at start of buffer.
 *
 * Returns NULL on no memory.
 */
static port*
output_port_struct_from_scratch (scheme *sc)
{
  port *pt;
  char *start;

  pt = (port*) sc->malloc (sizeof (port));
  if (pt == NULL)
    {
      no_memory (sc, "output port struct");
      return NULL;
    }

  start = sc->malloc (STRING_PORT_MIN_ALLOCATION);
  if (start == NULL)
    {
      no_memory (sc, "output port bytes");
      return NULL;
    }

  memset (start, '\0', STRING_PORT_MIN_ALLOCATION);

  init_port_struct (pt, port_string|port_output, start, STRING_PORT_MIN_ALLOCATION);

  return pt;
}

/* Return scheme string-port object of kind output, or NIL.
 *
 * Returned string-port is empty.
 *
 * Returns NIL on no memory.
 */
static pointer
output_port_from_scratch (scheme *sc)
{
  port *pt = output_port_struct_from_scratch (sc);
  if (pt == NULL)
    {
      /* no-memory already called. */
      return sc->NIL;
    }
  else
    return mk_port (sc, pt);
}

/* Will the current allocated buffer of the port hold byte_count bytes?
 * Requires port is string-port of kind output.
 */
static gboolean
output_port_fits_bytes (port *pt, guint byte_count)
{
  /* Free bytes equals pointer to last byte of buffer minus current write pointer.
   * past_the_end is the last byte of buffer, a NUL, not really past it.
   */
  return pt->rep.string.past_the_end - pt->rep.string.curr >= byte_count;
}

/* Write bytes to a string-port.
 * Requires string-port of kind output.
 * Requires buffer has free space of byte_count.
 */
static void
output_port_write_bytes (port *pt, const gchar *bytes, guint byte_count)
{
  memcpy (pt->rep.string.curr, bytes, byte_count);
  pt->rep.string.curr += byte_count;

  /* Maintain invariant: NUL terminated.
   * curr is pointing inside the buffer after the byte we just wrote,
   * or to the last byte of the buffer, already NUL.
   */
  *pt->rep.string.curr = '\0';
}

/* Expand the buffer of a string-port of kind output by at least byte-count.
 * Current contents of the port are kept.
 *
 * Returns FALSE on no memory.
 */
static gboolean
output_port_expand_by_at_least (scheme *sc, port *p, size_t byte_count)
{
  gchar  *current_contents           = p->rep.string.start;
  size_t  current_content_size_bytes = strlen (p->rep.string.start);
  gchar  *new_buffer;

  /* We don't care how many bytes are free,
   * just allocate max (byte_count or STRING_PORT_MIN_ALLOCATION) + 1
   *
   * Plus one for a terminating NUL.
   *
   * We allocate more than enough, by definition of buffer:
   * prevent overhead on a series of frequent small writes.
   * Frequent writes larger than STRING_PORT_MIN_ALLOCATION will mostly allocate.
   * Frequent writes smaller than STRING_PORT_MIN_ALLOCATION will not all allocate.
   */
  /* GLib MAX */
  size_t new_size = current_content_size_bytes + MAX (byte_count, STRING_PORT_MIN_ALLOCATION) + 1;

  g_debug ("%s byte_count %" G_GSIZE_FORMAT, G_STRFUNC, byte_count);
  g_debug ("%s current contents %" G_GSIZE_FORMAT " new size %" G_GSIZE_FORMAT, G_STRFUNC, current_content_size_bytes, new_size);

  new_buffer = sc->malloc (new_size);
  if (new_buffer)
    {
      /* address arithmetic */
      gchar *new_curr = new_buffer + current_content_size_bytes;

      /* NUL the whole thing.
       * Not optimized re soon copying.
       * Alternatively just NUL the free area that we don't copy.
       */
      memset (new_buffer, '\0', new_size);

      /* This copies the terminating NUL. */
      strcpy (new_buffer, current_contents);

      reset_output_port_struct (p, new_buffer, new_size, new_curr);

      /* Free old buffer, an allocation whose length is known by allocator. */
      sc->free (current_contents);

      return TRUE;
    }
  else
    {
      return FALSE;
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
  /* Else a backbyte when no byte has been read? */
}

/* Write a sequence of bytes to a string-port of kind output.
 *
 * The bytes need not be NUL terminated, they are counted.
 *
 * Ensures that a NUL is written after the bytes.
 *
 * The bytes usually, but not always, represent a character.
 *
 * Fails quietly, on no memory.
 */
void
string_port_put_bytes (scheme *sc, port *pt, const gchar *bytes, guint byte_count)

{
  if (output_port_fits_bytes (pt, byte_count))
    output_port_write_bytes (pt, bytes, byte_count);
  else if (output_port_expand_by_at_least (sc, pt, byte_count))
    output_port_write_bytes (pt, bytes, byte_count);
  else
    no_memory (sc, "expand output port");
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
 * !!! scheme-string is ignored.
 * Whatever the scheme-string did in the original TinyScheme is no longer so.
 * The scheme-string is NOT the buffer for the output port,
 * nor the name of the port, as in some Schemes.
 */
pointer
string_port_open_output_string (scheme *sc, pointer scheme_string)
{
  return output_port_from_scratch (sc);
}

/* Returns C pointer to a port struct, or NULL.
 *
 * Used for internal ports of the interpreter
 * (not for port objects known by a script.)
 */
port*
string_port_open_output_port (scheme *sc)
{
  return output_port_struct_from_scratch (sc);
}

/* Create a string-port of kind input from a Scheme string.
 *
 * Ensures the port contents do not depend on lifetime of Scheme string.
 * Other implementations of string-port use the Scheme string itself as the contents.
 *
 * Requires the Scheme string is NUL-terminated.
 *
 * Prop is kind of port.
 * Relic of older API supporting kind input OR input-output.
 * This TinyScheme no longer supports kind input-output.
 * Prop is ignored, and the string-port is kind input.
 */
pointer
string_port_open_input_string (scheme *sc, pointer scheme_string, int prop)
{
  gchar *c_string = scheme_string->_object._string._svalue;

  return input_port_from_string (sc, c_string);
}


/* Free heap allocation of a port struct.
 *
 * Require port is-a struct port.
 * The port* must not be used again.
 */
void
string_port_dispose_struct (scheme  *sc, port *port)
{
  g_debug ("%s content size %" G_GSIZE_FORMAT, G_STRFUNC,
           strlen (port->rep.string.start) + 1);

  /* Free allocated buffer. */
  sc->free (port->rep.string.start);

  /* Free the allocated struct itself. */
  sc->free (port);
}

/* Free heap allocation of the Scheme object.
 * Called during garbage collection, the cell itself is being reclaimed.
 *
 * Require port is-a string-port.
 */
void
string_port_dispose (scheme  *sc, pointer port_cell)
{
  string_port_dispose_struct (sc, port_cell->_object._port);
  /* The cell still has a reference, but it is invalid. */
}

/* Implementation of Scheme GET-OUTPUT-STRING.
 *
 * Returns scheme pointer to a scheme string or to sc->F.
 * Returns sc->F on no memory.
 *
 * Returned scheme string has lifetime independent of the port (is a copy.)
 *
 * Requires port is-a string-port.
 * Does not require (check that) port is kind output, but that is required earlier.
 */
pointer
string_port_get_output_string (scheme *sc, port *p)
{
  pointer result;

  /* Invariant there is a NUL in the buffer at the write pointer (curr.)
   * Require that mk_string copies the contents.
   */
  result = mk_string (sc, p->rep.string.start);

  if (result == sc->NIL)
    {
      no_memory (sc, "get output string");
      result = sc->F;
    }
  return result;
}

/* Initialize a static port struct to be an input string-port,
 * from the given command string.
 *
 * Specialized: assert port is load_stack[0], statically allocated.
 * It is never finalized or disposed.
 * In this case, the string-port contents are not allocated, but borrowed.
 *
 * The command string is:
 *    read-only C string
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

  port->kind = port_input|port_string;

  port->rep.string.start        = c_string;
  /* address arithmetic */
  port->rep.string.past_the_end = c_string + strlen (c_string);
  port->rep.string.curr         = c_string;
}
