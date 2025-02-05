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

#include <glib.h>
#include <glib-object.h>

#include "tinyscheme/scheme-private.h"
#include "script-fu-errors.h"


/* Enable logging by "export G_MESSAGES_DEBUG=scriptfu" in the env */

/* Used by debug_in_arg().
 * FUTURE: conditional compile out when debug not enabled.
 */
/* These three #defines are from Tinyscheme (tinyscheme/scheme.c) */
#define T_MASKTYPE  31
#define typeflag(p) ((p)->_flag)
#define type(p)     (typeflag(p)&T_MASKTYPE)

  static const char *ts_types[] =
  {
    "T_NONE",
    "T_STRING",    "T_NUMBER",     "T_SYMBOL",       "T_PROC",
    "T_PAIR",      "T_CLOSURE",    "T_CONTINUATION", "T_FOREIGN",
    "T_CHARACTER", "T_PORT",       "T_VECTOR",       "T_MACRO",
    "T_PROMISE",   "T_ENVIRONMENT","T_ARRAY",        "T_ARG_SLOT"
  };


/* Called on event: language error in author's script,
 * in the Scheme arg spec for a PDB registered argument.
 * Returns an error which the caller must return to its caller.
 */
pointer registration_error (scheme        *sc,
                            const gchar   *error)
{
  gchar  error_message[1024];

  g_debug ("%s", error);
  /* Prefix with name of SF function that discovered error. */
  g_snprintf (error_message, sizeof (error_message),
              "script-fu-register: %s",
              error);
  return foreign_error (sc, error_message, 0);
}


/*
 * Called on event: language error in the author's script.
 * Logs the error and returns a foreign_error.
 * Not all foreign_error are errors in script, some are scriptfu implementation
 * errors or implementation errors in called procedures.
 *
 * This should specialize foreign_error by emphasizing script error.
 * For now, it just specializes by also logging.
 * foreign error does not do logging, since the caller usually logs.

 * Returns a value which the caller must return to its caller.
 */
pointer
script_error (scheme        *sc,
              const gchar   *error_message,
              const pointer  a)
{
  /* Logs to domain "scriptfu" since G_LOG_DOMAIN is set to that. */
  g_debug ("%s", error_message);

  /* Return message that will cross the GimpProtocol in a GError in return values
   * to be displayed to GUI user.
   */
   /* FUTURE prefix with "ScriptFu: in script," */
  return foreign_error (sc, error_message, a);
}


/* Specialized calls to script_error. */

/* Arg has wrong type. */
pointer
script_type_error (scheme       *sc,
                   const gchar  *expected_type,
                   const guint   arg_index,
                   const gchar  *proc_name)
{
  gchar  error_message[1024];

  g_snprintf (error_message, sizeof (error_message),
              "in script, expected type: %s for argument %d to %s ",
              expected_type, arg_index+1, proc_name );

  return script_error (sc, error_message, 0);
}

/* Arg is container (list or vector) having an element of wrong type. */
pointer
script_type_error_in_container (scheme        *sc,
                                const gchar   *expected_type,
                                const guint    arg_index,
                                const guint    element_index,
                                const gchar   *proc_name,
                                const pointer  container)
{
  gchar     error_message[1024];

  /* convert zero based indices to ordinals */
  g_snprintf (error_message, sizeof (error_message),
              "in script, expected type: %s for element %d of argument %d to %s ",
              expected_type, element_index+1, arg_index+1, proc_name );

  /* pass container to foreign_error */
  return script_error (sc, error_message, container);
}

/* Arg is vector of wrong length. !!! Arg is not a list.  */
pointer
script_length_error_in_vector (scheme       *sc,
                               const guint   arg_index,
                               const gchar  *proc_name,
                               const guint   expected_length,
                               const pointer vector)
{
  gchar    error_message[1024];

  /* vector_length returns signed long (???) but expected_length is unsigned */
  g_snprintf (error_message, sizeof (error_message),
              "in script, vector (argument %d) for function %s has "
              "length %ld but expected length %u",
              arg_index+1, proc_name,
              sc->vptr->vector_length (vector), expected_length);

  /* not pass vector to foreign_error */
  return script_error (sc, error_message, 0);
}

/* Arg of type int is out of range in call to PDB. */
pointer
script_int_range_error (scheme       *sc,
                        const guint   arg_index,
                        const gchar  *proc_name,
                        const gint    expected_min,
                        const gint    expected_max,
                        const gint    value)
{
  gchar error_message[1024];

  g_snprintf (error_message, sizeof (error_message),
              "argument %d in call to %s has value %d out of range: "
              "%d to %d",
              arg_index+1, proc_name,
              value, expected_min, expected_max);
  return script_error (sc, error_message, 0);
}

/* Arg of type double is out of range in call to PDB. */
pointer
script_float_range_error (scheme       *sc,
                          const guint   arg_index,
                          const gchar  *proc_name,
                          const double  expected_min,
                          const double  expected_max,
                          const double  value)
{
  gchar error_message[1024];

  g_snprintf (error_message, sizeof (error_message),
              "argument %d in call to %s has value %f out of range: "
              "%f to %f",
              arg_index+1, proc_name,
              value, expected_min, expected_max);
  return script_error (sc, error_message, 0);
}


/* Thin wrapper around foreign_error.
 * Does logging.
 * Names a kind of error: in ScriptFu code, or in external code.
 * Same as script_error, but FUTURE distinguish the message with a prefix.
 */
pointer
implementation_error (scheme       *sc,
                      const gchar  *error_message,
                      const pointer a)
{
  g_debug ("%s", error_message);
  return foreign_error (sc, error_message, a);
}


/* Debug helpers.
 * Enabled by G_MESSAGES_DEBUG=scriptfu env var.
 * FUTURE: For performance, return early if not debugging.
 * Or conditionally compile.
 */

void
debug_vector (scheme        *sc,
              const pointer  vector,
              const char    *format)
{
  glong count = sc->vptr->vector_length (vector);

  g_debug ("vector has %ld elements", count);
  if (count > 0)
    {
      for (int j = 0; j < count; ++j)
        {
          if (strcmp (format, "%f")==0)
            /* real i.e. float */
            g_debug (format,
                     sc->vptr->rvalue ( sc->vptr->vector_elem (vector, j) ));
          else
            /* integer */
            g_debug (format,
                     sc->vptr->ivalue ( sc->vptr->vector_elem (vector, j) ));
          /* FUTURE vectors of strings or other formats? */
        }
    }
}

/* TinyScheme has no polymorphic length(), elem() methods on containers.
 * Must walk a list with car/cdr.
 *
 * Unlike vectors, lists have a guint length, not gulong
 *
 * !!! Only for lists of strings.
 */
void
debug_list (scheme       *sc,
            pointer       list,
            const char   *format,
            const guint   num_elements)
{
  g_return_if_fail (num_elements == sc->vptr->list_length (sc, list));
  g_debug ("list has %d elements", num_elements);
  if (num_elements > 0)
    {
      for (int j = 0; j < num_elements; ++j)
        {
          pointer v_element = sc->vptr->pair_car (list);

          g_debug (format,
                   sc->vptr->string_value ( v_element ));
          list = sc->vptr->pair_cdr (list);
        }
    }
}

/* Understands the adapted type system: Scheme interpreter type system.
 * Log types of formal and actual args.
 * Scheme type names, and enum of actual type.
 */
void
debug_in_arg (scheme           *sc,
              const pointer     a,
              const guint       arg_index,
              const gchar      *type_name )
{
  pointer arg_val;

  if (sc->vptr->is_pair (a))
    arg_val = sc->vptr->pair_car (a);
  else
    arg_val = a;

  g_debug ("param:%d, formal C type:%s, actual scheme type:%s (%d)",
           arg_index + 1,
           type_name,
           ts_types[ type (arg_val) ],
           type (arg_val));
}

/* Log GValue: its value and its GType
 * FUTURE: for Gimp types, gimp_item_get_id (GIMP_ITEM (<value>)));
 */
void
debug_gvalue (const GValue     *value)
{
  char        *contents_str;
  const char  *type_name;

  type_name = G_VALUE_TYPE_NAME(value);
  contents_str = g_strdup_value_contents (value);
  g_debug ("Value: %s Type: %s", contents_str, type_name);
  g_free (contents_str);
}
