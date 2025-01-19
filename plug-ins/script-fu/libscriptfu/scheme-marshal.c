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

/* When include scheme-private.h, must undef cons macro */
#undef cons

static pointer get_item_from_ID_in_script (scheme    *sc,
                                           pointer   a,
                                           gint      id,
                                           GObject **object_handle);


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
 * When marshalling into a GimpObjectArray:
 * Setting a GValue's value does not check the contained type.
 * But validation of a GimpObjectArray does check the contained type.
 * We call gimp_item_get_by_id
 * GIMP_TYPE_ITEM is a superclass of most common uses.
 * And then set the actual subclass type on the GimpObjectArray.
 */




/* Marshal single object ID from script into a single GObject.
 *
 * Requires ID's of instances of GimpItem, not for image, display, etc. also having ID.
 * The GValue may hold a more specific type, e.g. GimpDrawable.
 */
pointer
marshal_ID_to_item (scheme   *sc,
                    pointer   a,
                    gint      id,
                    GValue   *value)
{
  GObject *object;

  pointer error = get_item_from_ID_in_script (sc, a, id, &object);
  if (error)
    return error;

  /* object is NULL or valid */

  /* Shallow copy, adding a reference while the GValue exists. */
  g_value_set_object (value, object);
  return NULL;  /* no error */
}

/* Marshal a vector of ID into GimpObjectArray of same length.
 * Requires ID's of instances of GimpItem
 */
pointer
marshal_vector_to_item_array (scheme   *sc,
                              pointer   vector,
                              GValue   *value,
                              gchar   **strvalue)
{
  GObject **object_array;
  GString  *v = NULL;
  gint      id;
  pointer   error;

  guint num_elements = sc->vptr->vector_length (vector);
  g_debug ("vector has %d elements", num_elements);
  /* empty vector will produce empty GimpObjectArray */

  object_array = g_new0 (GObject *, num_elements + 1);
  if (strvalue)
    v = g_string_new ("");

  for (int j = 0; j < num_elements; ++j)
    {
      pointer element = sc->vptr->vector_elem (vector, j);

      if (!sc->vptr->is_number (element))
        {
          g_free (object_array);
          if (v)
            g_string_free (v, TRUE);
          return script_error (sc, "Expected numeric in vector of ID", vector);
          /* FUTURE more detailed error msg:
           * return script_type_error_in_container (sc, "numeric", i, j, proc_name, vector);
           */
        }

      id = sc->vptr->ivalue (element);
      error = get_item_from_ID_in_script (sc, element, id, &object_array[j]);
      if (error)
        {
          g_free (object_array);
          if (v)
            g_string_free (v, TRUE);
          return error;
        }

      if (v)
        g_string_append_printf (v, "%s%d", j == 0 ? "" : " ", id);
    }

  /* Shallow copy.
   * This sets the contained type on the array.
   * If the array is empty, contained type is GIMP_TYPE_ITEM.
   * !!! Seems like validation checks the contained type of empty arrays,
   * and fails if we default to G_TYPE_INVALID but passes if ITEM.
   */
  g_value_set_boxed (value, (gpointer) object_array);
  if (v)
    {
      *strvalue = g_strdup_printf ("(%s)", v->str);
      g_string_free (v, TRUE);
    }

  g_free (object_array);

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
marshal_returned_object_array_to_vector (scheme *sc,
                                         GValue *value)
{
  GObject **object_array;
  gint32    n;
  pointer   vector;

  object_array = g_value_get_boxed (value);
  n = gimp_core_object_array_get_length (object_array);

  vector = sc->vptr->mk_vector (sc, n);

  /* Iterate starting at the back of the array, and prefix to container
   * so the order of objects is not changed.
   */
  for (int j = n - 1; j >= 0; j--)
    {
      GObject *object = object_array[j];
      gint     id;

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


 /* From a script numeric (a object ID) set a handle to a object.
  * When ID is -1, sets handle to NULL and returns no error.
  * When ID is valid, sets handle and returns no error.
  * Otherwise (ID is not -1 and not valid ID of a object) returns error.
  *
  * Require ID is of a GIMP object, some subclass of GimpItem.
  */
static pointer
get_item_from_ID_in_script (scheme   *sc,
                            pointer   a,
                            gint      id,
                            GObject **object_handle)
{
  if (id == -1)
    {
      /* -1 is scriptfu language for NULL i.e. none for an optional */
      *object_handle = NULL;
    }
  else
    {
      *object_handle = (GObject *) gimp_item_get_by_id (id);
      if (! *object_handle)
          return script_error (sc, "Invalid ID of GIMP item", a);
    }

  /* ensure *object_handle is NULL or a valid reference to a object */
  return NULL;  /* no error */
}

/* Walk a Scheme list of numerics, clamping into a C array of bytes.
 * Expects one to four numerics.
 * Returns length of list, in range [0,4]
 * Length zero denotes an error: not a numeric, or list is empty.
 * A list longer than expected (>4) is not an error, but extra list is not used.
 */
static void
marshal_list_of_numeric_to_rgba (scheme   *sc,
                                 pointer   color_list,
                                 guchar    (*rgba)[4],  /* OUT */
                                 guint    *length,      /* OUT */
                                 gchar   **strvalue)
{
  GString *v = NULL;

  *length = 0;
  if (strvalue)
    v = g_string_new ("");

  for (guint i=0; i<4; i++)
    {
      if (sc->vptr->is_number (sc->vptr->pair_car (color_list)))
        {
          (*rgba)[i] = CLAMP (sc->vptr->ivalue (sc->vptr->pair_car (color_list)),
                              0, 255);
          *length = *length + 1;
          color_list = sc->vptr->pair_cdr (color_list);

          if (v)
            g_string_append_printf (v, "%s%d", i == 0 ? "" : " ", (*rgba)[i]);
        }
      else
        {
          /* Reached end of list or non-numeric. */
          if (v)
            {
              if (*length != 0)
                *strvalue = g_strdup_printf ("'(%s)", v->str);

              g_string_free (v, TRUE);
            }

          return;
        }
    }
  /* *length is in [0,4] and *rgba is filled in with same count. */

  if (v)
    {
      *strvalue = g_strdup_printf ("'(%s)", v->str);
      g_string_free (v, TRUE);
    }
}

/* Walk a C array of bytes (rgba) creating Scheme list of numeric.
 * Returns list whose length is in range [0,4].
 * The list is memory managed by scheme.
 */
static pointer
marshal_rgba_to_list_of_numeric (scheme   *sc,
                                 guchar   rgba[4],
                                 guint    length)
{
  pointer result_list = sc->NIL;

  /* Walk rgba in reverse. */
  for (gint i=length-1; i>=0; i--)
    {
      /* Prepend to list, returning new list. */
      result_list = sc->vptr->cons (sc,
                      sc->vptr->mk_integer (sc, rgba[i]),
                      result_list);
    }

  /* list is numerics from rgba and length (list) == length. */
  return result_list;
}

/* Returns a GeglColor from a scheme list.
 * Caller owns the returned GeglColor.
 * Returns NULL on failure:
 *  - list wrong length, not >1
 *  - list elements not numbers.
 */
GeglColor *
marshal_component_list_to_color (scheme   *sc,
                                 pointer   color_list,
                                 gchar   **strvalue)
{
  GeglColor *color_result;
  guchar     rgba[4];
  guint      list_length;

  marshal_list_of_numeric_to_rgba (sc, color_list, &rgba, &list_length, strvalue);
  /* list_length is the count of numerics used. */

  /* Dispatch on list length and create different format colors */
  if (list_length == 3 || list_length == 4)
    {
      /* RGBA */
      if (list_length == 3)
        {
          /* gegl has no three byte rgb. * ScriptFu sets alpha to 1.0 */
          rgba[3] = 255;
        }
      /* Assert rgba is full */
      color_result = gegl_color_new ("black");
      gegl_color_set_rgba_with_space (color_result,
                                  (gdouble) rgba[0] / 255.0,
                                  (gdouble) rgba[1] / 255.0,
                                  (gdouble) rgba[2] / 255.0,
                                  (gdouble) rgba[3] / 255.0,
                                  NULL);  /* NULL defaults to sRGB */
    }
  else if (list_length == 1)
    {
      /* GRAY */
      GBytes *bytes = g_bytes_new (rgba, 1);

      color_result = gegl_color_new ("black");
      gegl_color_set_bytes (color_result, babl_format ("Y' u8"), bytes);
      g_free (bytes);
    }
  else if (list_length == 2)
    {
      /* GRAYA */
      GBytes *bytes = g_bytes_new (rgba, 2);

      color_result = gegl_color_new ("black");
      gegl_color_set_bytes (color_result, babl_format ("Y'A u8"), bytes);
      g_free (bytes);
    }
  else
    {
      color_result = NULL;
    }

  return color_result;
}

/* Returns a Scheme list of integers or empty list,
 * the same length as the count of components in the color.
 * List is length:
 *   1 GRAY
 *   2 GRAYA
 *   3 RGB
 *   4 RGBA
 * Returns NIL when color is NULL.
 */
pointer
marshal_color_to_component_list (scheme    *sc,
                                 GeglColor *color)
{
  pointer result;
  guint   count_components;
  guchar  rgba[4] = { 0 };

  if (color == NULL)
    {
      result = sc->NIL;
    }
  else
    {
      count_components = babl_format_get_n_components (gegl_color_get_format (color));
      switch (count_components)
      {
      case 3:
        gegl_color_get_pixel (color, babl_format ("R'G'B' u8"), rgba);
        result = marshal_rgba_to_list_of_numeric (sc, rgba, 3);
        break;
      case 4:
        gegl_color_get_pixel (color, babl_format ("R'G'B'A u8"), rgba);
        result = marshal_rgba_to_list_of_numeric (sc, rgba, 4);
        break;
      case 1:
        /* Grayscale with TRC from space */
        gegl_color_get_pixel (color, babl_format ("Y' u8"), rgba);
        result = marshal_rgba_to_list_of_numeric (sc, rgba, 1);
        break;
      case 2:
          /* Grayscale with TRC from space, separate alpha. */
          gegl_color_get_pixel (color, babl_format ("Y'A u8"), rgba);
          result = marshal_rgba_to_list_of_numeric (sc, rgba, 2);
          break;
      default:
          g_warning ("%s unhandled count of color components: %d", G_STRFUNC, count_components);
          result = sc->NIL;
      }
    }

  return result;
}

/* ColorArray */

/* Returns a vector of lists, where each list is a pixel i.e.
 * list of numerics, each the intensity component in a channel.
 *
 * array is a null-terminated array of pointers to GeglColor which is-a GObject.
 *
 * The returned vector can be empty, when the passed array is empty.
 */
pointer
marshal_color_array_to_vector (scheme         *sc,
                               GimpColorArray  array)
{
  guint   array_length = gimp_color_array_get_length (array);
  pointer result       = sc->vptr->mk_vector (sc, array_length);

  for (guint j = 0; j < array_length; j++)
    {
      pointer component_list = marshal_color_to_component_list (sc, array[j]);

      sc->vptr->set_vector_elem (result, j, component_list);
    }

  return result;
}
