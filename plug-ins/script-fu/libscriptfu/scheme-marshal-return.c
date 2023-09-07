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
#include "libgimp/gimp.h"
#include "tinyscheme/scheme-private.h"
#include "script-fu-errors.h"
#include "scheme-marshal.h"
#include "scheme-marshal-return.h"

/* When include scheme-private.h, must undef cons macro */
#undef cons




/* Marshall a set of values returned by a PDB procedure.
 * From a GValueArray into scheme list.
 *
 * Returns a scheme "pointer" type referencing the scheme list.
 *
 * Either returns a non-null scheme value and sets error to null,
 * or sets error and returns a null scheme value.
 * IOW, error is an OUT argument.
 *
 * The returned scheme value is scheme type list.
 * The list can be non-homogenous (elements of different scheme types.)
 *
 * The returned list may be empty or have only a single element.
 * FUTURE:
 * When a PDB procedure returns a single value (which can be a container)
 * do not wrap it in a list.
 * It will be an error to call this function
 * for PDB procedures that return a single value or return void.
 * IOW, for PDB procedures of return arity < 2.
 */
pointer
marshal_returned_PDB_values (scheme         *sc,
                             GimpValueArray *values,
                             pointer        *error)
{
  /* Result is empty list. */
  pointer result = sc->NIL;

  *error = NULL;

  /* Counting down, i.e. traversing in reverse.
  * i+1 is the current index.  i is the preceding value.
  * When at the current index is an array, preceding value (at i) is array length.
  */
  for (gint i = gimp_value_array_length (values) - 2; i >= 0; --i)
    {
      GValue *value = gimp_value_array_index (values, i + 1);
      pointer scheme_value;
      pointer single_error = NULL;
      gint32  array_length = 0;

      g_debug ("Return value %d is type %s", i+1, G_VALUE_TYPE_NAME (value));

      /* In some cases previous value is array_length. */
      if (   GIMP_VALUE_HOLDS_INT32_ARRAY (value)
          || GIMP_VALUE_HOLDS_FLOAT_ARRAY (value)
          || GIMP_VALUE_HOLDS_RGB_ARRAY   (value))
        {
          array_length = GIMP_VALUES_GET_INT (values, i);
        }

      scheme_value = marshal_returned_PDB_value (sc, value, array_length, &single_error);

      if (single_error == NULL)
        {
          /* Prepend to scheme list of returned values and continue iteration. */
          result = sc->vptr->cons (sc, scheme_value, result);
        }
      else
        {
          /* Error marshalling a single return value.
          * Any scheme values already marshalled will be garbage collected.
          */
          /* Propagate error to caller. */
          *error = single_error;
          /* null C pointer not the same as pointer to scheme NIL */
          result = NULL;
          break;
        }
    }

  g_assert (   (result == NULL && *error != NULL)
            || (result != NULL && *error == NULL));
  /* result can be sc->NIL i.e. empty list. */
  return result;
}


/* This function is only used in this file.
 * FUTURE: call when the PDB procedure returns arity 1.
 */

/* The below code for array results is not safe.
 * It implicitly requires, but does not explicitly check,
 * that the returned length equals the actual length of the returned array,
 * and iterates over the returned array assuming it has the returned length.
 * It could read past the end of the array.
 */

/* Convert a GValue from C type to Scheme type.
 *
 * Returns a scheme "pointer" type referencing the scheme value.
 *
 * When the value has C type an array type,
 * array_length must be its length,
 * otherwise array_length is not used.
 *
 * Either returns a non-null scheme value and sets error to null,
 * or sets error and returns a null scheme value.
 * IOW, error is an OUT argument.
 *
 * The returned scheme value is an atom or a container (list or vector.)
 * Returned containers are homogeneous (elements all the same type.)
 * Returned atoms are scheme type number or string.
 * Currently, does not return atoms of scheme type byte or char
 * (no PDB procedure returns those types.)
 *
 * !!! Returns a scheme number (0 or 1) for C type boolean.
 * FUTURE: return atoms #f and #t.
 */
pointer
marshal_returned_PDB_value    (scheme        *sc,
                               GValue        *value,
                               guint          array_length,
                               pointer       *error)
{
  pointer  result = sc->NIL;
  gint     j;
  gchar    error_str[1024];

  *error = NULL;

  /* Order is important.
    * GFile before other objects.
    * GIMP resource objects before GIMP Image, Drawable, etc. objects.
    * Alternatively, more specific tests.
    */
  if (G_VALUE_TYPE (value) == G_TYPE_FILE)
    {
      gchar *parsed_filepath = marshal_returned_gfile_to_string (value);

      if (parsed_filepath)
        {
          g_debug ("PDB procedure returned GFile '%s'", parsed_filepath);
          /* copy string into interpreter state. */
          result = sc->vptr->mk_string (sc, parsed_filepath);
          g_free (parsed_filepath);
        }
      else
        {
          g_warning ("PDB procedure failed to return a valid GFile");
          result = sc->vptr->mk_string (sc, "");
        }
      /* Ensure result holds a string, possibly empty. */
    }
  else if (GIMP_VALUE_HOLDS_RESOURCE (value))
    {
      /* ScriptFu represents resource objects by ther unique string ID's. */
      GObject *object = g_value_get_object (value);
      gchar   *name   = NULL;

      if (object)
        name = gimp_resource_get_name (GIMP_RESOURCE(object));

      if (! name)
        g_warning("PDB procedure returned NULL object.");

      result = sc->vptr->mk_string (sc, name);

      g_free (name);
    }
  else if (G_VALUE_HOLDS_OBJECT (value))
    {
      /* G_VALUE_HOLDS_OBJECT only ensures value derives from GObject.
        * Could be a GIMP or a GLib type.
        * Here we handle GIMP types, which all have an id property.
        * Resources have a string ID and Images and Drawables etc. have an int ID.
        */
      GObject *object = g_value_get_object (value);
      gint     id     = -1;

      /* expect a GIMP opaque object having an "id" property */
      if (object)
        g_object_get (object, "id", &id, NULL);

      /* id is -1 when the gvalue had no GObject*,
        * or the referenced object had no property "id".
        * This can be an undetected fault in the called procedure.
        * It is not necessarily an error in the script.
        */
      if (id == -1)
        g_warning("PDB procedure returned NULL GIMP object.");

      g_debug ("PDB procedure returned object ID: %i", id);

      /* Scriptfu stores object IDs as int. */
      result = sc->vptr->mk_integer (sc, id);
    }
  else if (G_VALUE_HOLDS_INT (value))
    {
      gint v = g_value_get_int (value);
      result = sc->vptr->mk_integer (sc, v);
    }
  else if (G_VALUE_HOLDS_UINT (value))
    {
      guint v = g_value_get_uint (value);
      result = sc->vptr->mk_integer (sc, v);
    }
  else if (G_VALUE_HOLDS_DOUBLE (value))
    {
      gdouble v = g_value_get_double (value);
      result = sc->vptr->mk_real (sc, v);
    }
  else if (G_VALUE_HOLDS_ENUM (value))
    {
      gint v = g_value_get_enum (value);
      result = sc->vptr->mk_integer (sc, v);
    }
  else if (G_VALUE_HOLDS_BOOLEAN (value))
    {
      gboolean v = g_value_get_boolean (value);
      result = sc->vptr->mk_integer (sc, v);
    }
  else if (G_VALUE_HOLDS_STRING (value))
    {
      const gchar *v = g_value_get_string (value);

      if (! v)
        v = "";

      result = sc->vptr->mk_string (sc, v);
    }
  else if (GIMP_VALUE_HOLDS_INT32_ARRAY (value))
    {
      const gint32 *v      = gimp_value_get_int32_array (value);
      pointer       vector = sc->vptr->mk_vector (sc, array_length);

      for (j = 0; j < array_length; j++)
        {
          sc->vptr->set_vector_elem (vector, j,
                                      sc->vptr->mk_integer (sc, v[j]));
        }

      result = vector;
    }
  else if (G_VALUE_HOLDS (value, G_TYPE_BYTES))
    {
      GBytes       *v_bytes  = g_value_get_boxed (value);
      const guint8 *v        = g_bytes_get_data (v_bytes, NULL);
      gsize         n        = g_bytes_get_size (v_bytes);
      pointer       vector   = sc->vptr->mk_vector (sc, n);

      for (j = 0; j < n; j++)
        {
          sc->vptr->set_vector_elem (vector, j,
                                      sc->vptr->mk_integer (sc, v[j]));
        }

      result = vector;
    }
  else if (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value))
    {
      const gdouble *v      = gimp_value_get_float_array (value);
      pointer        vector = sc->vptr->mk_vector (sc, array_length);

      for (j = 0; j < array_length; j++)
        {
          sc->vptr->set_vector_elem (vector, j,
                                      sc->vptr->mk_real (sc, v[j]));
        }

      result = vector;
    }
  else if (G_VALUE_HOLDS (value, G_TYPE_STRV))
    {
      gint32         n    = 0;
      const gchar  **v    = g_value_get_boxed (value);
      pointer        list = sc->NIL;

      n = (v)? g_strv_length ((char **) v) : 0;
      for (j = n - 1; j >= 0; j--)
        {
          list = sc->vptr->cons (sc,
                                  sc->vptr->mk_string (sc,
                                                      v[j] ?
                                                      v[j] : ""),
                                  list);
        }

      result = list;
    }
  else if (GIMP_VALUE_HOLDS_RGB (value))
    {
      GimpRGB  v;
      guchar   r, g, b;
      gpointer temp_val;

      gimp_value_get_rgb (value, &v);
      gimp_rgb_get_uchar (&v, &r, &g, &b);

      temp_val = sc->vptr->cons
                    (sc,
                    sc->vptr->mk_integer (sc, r),
                    sc->vptr->cons
                      (sc,
                        sc->vptr->mk_integer (sc, g),
                        sc->vptr->cons
                          (sc,
                          sc->vptr->mk_integer (sc, b),
                          sc->NIL)));

      result = temp_val;
    }
  else if (GIMP_VALUE_HOLDS_RGB_ARRAY (value))
    {
      const GimpRGB  *v = gimp_value_get_rgb_array (value);
      pointer  vector   = sc->vptr->mk_vector (sc, array_length);

      for (j = 0; j < array_length; j++)
        {
          guchar  r, g, b;
          pointer temp_val;

          gimp_rgb_get_uchar (&v[j], &r, &g, &b);

          temp_val = sc->vptr->cons
                      (sc,
                        sc->vptr->mk_integer (sc, r),
                        sc->vptr->cons
                          (sc,
                          sc->vptr->mk_integer (sc, g),
                          sc->vptr->cons
                            (sc,
                              sc->vptr->mk_integer (sc, b),
                              sc->NIL)));
          sc->vptr->set_vector_elem (vector, j, temp_val);
        }

      result = vector;
    }
  else if (GIMP_VALUE_HOLDS_PARASITE (value))
    {
      GimpParasite *v = g_value_get_boxed (value);

      if (v->name == NULL)
        {
          /* Wrongly passed a Parasite that appears to be null, or other error. */
          *error = implementation_error (sc, "Error: null parasite", 0);
        }
      else
        {
          gchar   *data = g_strndup (v->data, v->size);
          gint     char_cnt = g_utf8_strlen (data, v->size);
          pointer  temp_val;

          /* don't move the mk_foo() calls outside this function call,
            * otherwise they might be garbage collected away!
            */
          temp_val = sc->vptr->cons
                        (sc,
                        sc->vptr->mk_string (sc, v->name),
                        sc->vptr->cons
                          (sc,
                            sc->vptr->mk_integer (sc, v->flags),
                            sc->vptr->cons
                              (sc,
                              sc->vptr->mk_counted_string (sc,
                                                            data,
                                                            char_cnt),
                              sc->NIL)));

          result = temp_val;
          g_free (data);

          g_debug ("name '%s'", v->name);
          g_debug ("flags %d", v->flags);
          g_debug ("size %d", v->size);
          g_debug ("data '%.*s'", v->size, (gchar *) v->data);
        }
    }
  else if (GIMP_VALUE_HOLDS_OBJECT_ARRAY (value))
    {
      result = marshal_returned_object_array_to_vector (sc, value);
    }
  else if (G_VALUE_TYPE (&value) == GIMP_TYPE_PDB_STATUS_TYPE)
    {
      /* Called procedure implemented incorrectly. */
      *error = implementation_error (sc, "Procedure execution returned multiple status values", 0);
    }
  else
    {
      /* Missing cases here. */
      g_snprintf (error_str, sizeof (error_str),
                  "Unhandled return type %s",
                  G_VALUE_TYPE_NAME (value));
      *error = implementation_error (sc, error_str, 0);
    }

  g_assert (   (result == NULL && *error != NULL)
            || (result != NULL && *error == NULL));

  return result;
}