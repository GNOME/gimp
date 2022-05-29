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
#include "scheme-marshal.h"
#include "script-fu-errors.h"


/*
 * Marshal arguments to, and return values from, calls to PDB.
 * Convert Scheme constructs to/from a GValue.
 *
 * For each marshalling function:
 *   - a returned "pointer" is a scheme pointer to a foreign error or NULL.
 *   - marshal into a GValue holding a designated type,
 *     usually a GIMP type but also GLib types, e.g. GFile.
 *     The GValue's held type is already set, but value is uninitialized.
 *
 * When marshalling into a GimpObjectArray, arbitrarily say the contained type is GIMP_TYPE_DRAWABLE.
 * The actual contained type is opaque to the PDB calling mechanism.
 * Setting the GValue's value does not check the contained type.
 * But we do call gimp_drawable_get_by_id.
 * GIMP_TYPE_DRAWABLE is a superclass of most common uses.
 * But perhaps we should call gimp_item_get_by_id
 * and arbitrarily say GIMP_TYPE_ITEM, a superclass of drawable.
 */




/* Marshal single drawable ID from script into a single GObject. */
pointer
marshal_ID_to_drawable (scheme   *sc,
                        pointer   a,
                        gint      id,
                        GValue   *value)
{
  GimpDrawable *drawable;

  pointer error = get_drawable_from_script (sc, a, id, &drawable);
  if (error)
    return error;

  /* drawable is NULL or valid */

  /* Shallow copy, adding a reference while the GValue exists. */
  g_value_set_object (value, drawable);
  return NULL;  /* no error */
}

/* Marshal a vector of ID into GimpObjectArray of same length. */
pointer
marshal_vector_to_drawable_array (scheme   *sc,
                                  pointer   vector,
                                  GValue   *value)
{
  GimpDrawable **drawable_array;
  gint           id;
  pointer        error;

  guint num_elements = sc->vptr->vector_length (vector);
  g_debug ("vector has %d elements", num_elements);
  /* empty vector will produce empty GimpObjectArray */

  drawable_array = g_new0 (GimpDrawable*, num_elements);

  for (int j = 0; j < num_elements; ++j)
    {
      pointer element = sc->vptr->vector_elem (vector, j);

      if (!sc->vptr->is_number (element))
        {
          g_free (drawable_array);
          return script_error (sc, "Expected numeric in drawable vector", vector);
          /* FUTURE more detailed error msg:
           * return script_type_error_in_container (sc, "numeric", i, j, proc_name, vector);
           */
        }

      id = sc->vptr->ivalue (element);
      error = get_drawable_from_script (sc, element, id, &drawable_array[j]);
      if (error)
        {
          g_free (drawable_array);
          return error;
        }
    }

  /* Shallow copy. */
  gimp_value_set_object_array (value, GIMP_TYPE_DRAWABLE, (GObject**)drawable_array, num_elements);

  g_free (drawable_array);

  return NULL;  /* no error */
}


/* Marshal path string from script into a GValue holding type GFile */
void
marshal_path_string_to_gfile (scheme     *sc,
                              pointer     a,
                              GValue     *value)
{
  /* require sc->vptr->is_string (sc->vptr->pair_car (a)) */

  GFile *gfile = g_file_new_for_path (sc->vptr->string_value (sc->vptr->pair_car (a)));
  /* GLib docs say that g_file_new_for_path():
   * "never fails, but the returned object might not support any I/O operation if path is malformed."
   */

  g_value_set_object (value, gfile);
  g_debug ("gfile arg is '%s'\n", g_file_get_parse_name (gfile));
}


/* Marshal values returned from PDB call in a GValue, into a Scheme construct to a script. */


/* Marshal a GValue holding a GFile into a string.
 *
 * Returns NULL or a string that must be freed.
 */
 gchar *
 marshal_returned_gfile_to_string (GValue   *value)
{
  gchar * filepath = NULL;

  GObject *object = g_value_get_object (value);
  /* object can be NULL, the GValue's type only indicates what should have been returned. */
  if (object)
    {
      filepath = g_file_get_parse_name ((GFile *) object);
      /* GLib docs:
       * For local files with names that can safely be converted to UTF-8 the pathname is used,
       * otherwise the IRI is used (a form of URI that allows UTF-8 characters unescaped).
       */
     }
  return filepath;
}


/* Marshal a GimpObjectArray into a Scheme list of ID's.
 *
 * Before v3.0, PDB procedure's return type was say INT32ARRAY,
 * preceded by a type INT32 designating array length.
 * Now return type is GimpObjectArray preceded by length.
 *
 * Returns a vector, since most arrays in Scriptfu are returned as vectors.
 * An alternate implementation would be return list.
 *
 * Existing scheme plugins usually expect PDB to return values: len, vector.
 * If ever the PDB is changed to be more object-oriented,
 * scripts could use a scheme call: (vector-length vector)
 * to get the length of the vector.
 */
pointer
marshal_returned_object_array_to_vector (scheme   *sc,
                                         GValue   *value)
{
  GObject **object_array;
  gint32    n;
  pointer   vector;

  object_array = gimp_value_get_object_array (value);
  /* array knows own length, ignore length in preceding return value */
  n = ((GimpObjectArray*)g_value_get_boxed (value))->length;

  vector = sc->vptr->mk_vector (sc, n);

  /* Iterate starting at the back of the array, and prefix to container
   * so the order of objects is not changed.
   */
  for (int j = n - 1; j >= 0; j--)
    {
      gint     id;
      GObject *object = object_array[j];

      if (object)
        g_object_get (object, "id", &id, NULL); /* get property "id" */
      else
        /* Scriptfu language represents NULL object by ID of -1*/
        id = -1;

      sc->vptr->set_vector_elem (vector, j, sc->vptr->mk_integer (sc, id));
      /* Alt: list = sc->vptr->cons (sc, sc->vptr->mk_integer (sc, id), list); */
    }
  /* ensure container's len equals object array's len and all elements are ID's or -1 */
  return vector;
}


 /* From a script numeric (a drawable ID) set a handle to a drawable.
  * When ID is -1, sets drawable to NULL and returns no error.
  * When ID is valid, sets drawable and returns no error.
  * Otherwise (ID is not -1 and not valid ID of a drawable) returns error.
  */
pointer
get_drawable_from_script (scheme        *sc,
                          pointer        a,
                          gint           id,
                          GimpDrawable **drawable_handle)
{
  if (id == -1)
    {
      /* -1 is scriptfu language for NULL i.e. none for an optional */
      *drawable_handle = NULL;
    }
  else
    {
      *drawable_handle = gimp_drawable_get_by_id (id);
      if (! *drawable_handle)
          return script_error (sc, "Invalid drawable ID", a);
    }

  /* ensure *drawable_handle is NULL or a valid reference to a drawable */
  return NULL;  /* no error */
}
