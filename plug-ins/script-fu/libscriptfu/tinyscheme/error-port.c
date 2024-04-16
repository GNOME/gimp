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

#include "error-port.h"

/* Error port.
 *
 * TinyScheme now writes all errors to a separate output string-port
 * (instead of to the output port.)
 * Similar to the way UNIX has distinct streams stdout and stderr.
 *
 * A program (e.g. ScriptFu) that wraps the inner TinyScheme interpreter
 * can retrieve error messages.
 *
 * A wrapping program can also redirect the inner interpreter's non-error output
 * but differently, using ts_set_output_func.
 *
 * The events:
 * When the inner interpreter declares an error
 * it redirects its output to the error port,
 * writes the error message, and then returns -1 retcode.
 * Output means internal calls to putbytes.
 * Interpreter is aborting so won't interpret any more writes in the script.
 * The wrapping program then gets the error message,
 * which also clears the error port, ready for the next interpretation.
 *
 * When interpretation proceeds without error,
 * no error port is created.
 * Existence of error port means:
 *    - interpreter is in error state, after a final call to internal error
 *      Note that exception handling occurs before this,
 *      so a script can still intercept errors using *error-hook*.
 *    - subsequent writes by putbytes are to the error port,
 *      until the error port is retrieved and destroyed,
 *      by the outer interpreter.
 *      Subsequent writes by putbytes are all writing error message
 *      and args to (error ... )
 *
 * Unlike other fields in scheme struct,
 * errport is not type pointer i.e. does not point to a cell.
 * The error port is internal to the interpreter.  Scripts cannot access it.
 * It is a singleton.
 */


/* Initialize so there is no error port.
 *
 * Ensure the next interpretation is w/o redirected errors.
 * Note the string-port API has no way to empty an output port.
 * We create/destroy the error port.
 *
 * Any existing error port should already be disposed, else leaks.
 */
void
error_port_init (scheme *sc)
{
  sc->errport = NULL;
}

/* Is interpreter in error state and redirecting to the error port? */
gboolean
error_port_is_redirect_output (scheme *sc)
{
  return sc->errport != NULL;
}

/* Set the error port so subsequent putbytes (which every IO write uses)
 * is to the error port.
 * Even writes passing a port: (write <foo> <port>)
 * will write to the error port instead of the passed port.
 *
 * Requires no error port exist already. When it does, memory can leak.
 *
 * Ensure the port is kind string, direction output.
 */
void
error_port_redirect_output (scheme  *sc)
{
  g_debug ("%s", G_STRFUNC);

  if (sc->errport != NULL)
    g_warning ("%s error port exists already", G_STRFUNC);

  sc->errport = string_port_open_output_port (sc);

  g_assert (sc->errport->kind & (port_output | port_string));
}

/* Return the errport or NULL.
 * When non-null, interpreter is in an error state, is aborting.
 */
port*
error_port_get_port_rep (scheme  *sc)
{
  return sc->errport;
}

/* Get the content string of the error port and close the port.
 *
 * The returned string is owned by the caller and must be freed.
 *
 * This must be called exactly once per inner interpretation.
 * Destroys the error port so next interpretation is not writing to error port.
 *
 * Require the inner interpreter just returned to the caller
 * (the wrapping interpreter) with an error status.
 * Else the returned string is "Unknown" and not the actual error.
 */
const gchar *error_port_take_string_and_close (scheme *sc)
{
  gchar *result;
  port  *port = error_port_get_port_rep (sc);

  if (port != NULL)
    {
      result = g_strdup (port->rep.string.start);

      string_port_dispose_struct (sc, port);

      error_port_init (sc);
    }
  else
    {
      /* Not expected to happen.  Untranslated. */
      result = g_strdup ("Unknown error");
    }
  return result;
}
