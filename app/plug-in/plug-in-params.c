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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpcolor/gimpcolor.h"

#include "plug-in-types.h"

#include "core/gimpparamspecs.h"

#include "pdb/gimp-pdb-compat.h"

#include "plug-in-params.h"


GimpValueArray *
plug_in_params_to_args (GParamSpec **pspecs,
                        gint         n_pspecs,
                        GPParam     *params,
                        gint         n_params,
                        gboolean     return_values,
                        gboolean     full_copy)
{
  GimpValueArray *args;
  gint            i;

  g_return_val_if_fail ((pspecs != NULL && n_pspecs  > 0) ||
                        (pspecs == NULL && n_pspecs == 0), NULL);
  g_return_val_if_fail ((params != NULL && n_params  > 0) ||
                        (params == NULL && n_params == 0), NULL);

  args = gimp_value_array_new (n_params);

  for (i = 0; i < n_params; i++)
    {
      GValue value = G_VALUE_INIT;
      GType  type;
      gint   count;

      /*  first get the fallback compat GType that matches the pdb type  */
      type = gimp_pdb_compat_arg_type_to_gtype (params[i].type);

      /*  then try to try to be more specific by looking at the param
       *  spec (return values have one additional value (the status),
       *  skip that, it's not in the array of param specs)
       */
      if (i > 0 || ! return_values)
        {
          gint pspec_index = i;

          if (return_values)
            pspec_index--;

          /*  are there param specs left?  */
          if (pspec_index < n_pspecs)
            {
              GType          pspec_gtype;
              GimpPDBArgType pspec_arg_type;

              pspec_gtype    = G_PARAM_SPEC_VALUE_TYPE (pspecs[pspec_index]);
              pspec_arg_type = gimp_pdb_compat_arg_type_from_gtype (pspec_gtype);

              /*  if the param spec's GType, mapped to a pdb type, matches
               *  the passed pdb type, use the param spec's GType
               */
              if (pspec_arg_type == params[i].type)
                type = pspec_gtype;
            }
        }

      g_value_init (&value, type);

      switch (gimp_pdb_compat_arg_type_from_gtype (type))
        {
        case GIMP_PDB_INT32:
          if (G_VALUE_HOLDS_INT (&value))
            g_value_set_int (&value, params[i].data.d_int32);
          else if (G_VALUE_HOLDS_UINT (&value))
            g_value_set_uint (&value, params[i].data.d_int32);
          else if (G_VALUE_HOLDS_ENUM (&value))
            g_value_set_enum (&value, params[i].data.d_int32);
          else if (G_VALUE_HOLDS_BOOLEAN (&value))
            g_value_set_boolean (&value, params[i].data.d_int32 ? TRUE : FALSE);
          else
            {
              g_printerr ("%s: unhandled GIMP_PDB_INT32 type: %s\n",
                          G_STRFUNC, g_type_name (G_VALUE_TYPE (&value)));
              g_return_val_if_reached (args);
            }
          break;

        case GIMP_PDB_INT16:
          g_value_set_int (&value, params[i].data.d_int16);
          break;

        case GIMP_PDB_INT8:
          g_value_set_uint (&value, params[i].data.d_int8);
          break;

        case GIMP_PDB_FLOAT:
          g_value_set_double (&value, params[i].data.d_float);
          break;

        case GIMP_PDB_STRING:
          if (full_copy)
            g_value_set_string (&value, params[i].data.d_string);
          else
            g_value_set_static_string (&value, params[i].data.d_string);
          break;

        case GIMP_PDB_INT32ARRAY:
          count = g_value_get_int (gimp_value_array_index (args, i - 1));
          if (full_copy)
            gimp_value_set_int32array (&value,
                                       params[i].data.d_int32array,
                                       count);
          else
            gimp_value_set_static_int32array (&value,
                                              params[i].data.d_int32array,
                                              count);
          break;

        case GIMP_PDB_INT16ARRAY:
          count = g_value_get_int (gimp_value_array_index (args, i - 1));
          if (full_copy)
            gimp_value_set_int16array (&value,
                                       params[i].data.d_int16array,
                                       count);
          else
            gimp_value_set_static_int16array (&value,
                                              params[i].data.d_int16array,
                                              count);
          break;

        case GIMP_PDB_INT8ARRAY:
          count = g_value_get_int (gimp_value_array_index (args, i - 1));
          if (full_copy)
            gimp_value_set_int8array (&value,
                                      params[i].data.d_int8array,
                                      count);
          else
            gimp_value_set_static_int8array (&value,
                                             params[i].data.d_int8array,
                                             count);
          break;

        case GIMP_PDB_FLOATARRAY:
          count = g_value_get_int (gimp_value_array_index (args, i - 1));
          if (full_copy)
            gimp_value_set_floatarray (&value,
                                       params[i].data.d_floatarray,
                                       count);
          else
            gimp_value_set_static_floatarray (&value,
                                              params[i].data.d_floatarray,
                                              count);
          break;

        case GIMP_PDB_STRINGARRAY:
          count = g_value_get_int (gimp_value_array_index (args, i - 1));
          if (full_copy)
            gimp_value_set_stringarray (&value,
                                        (const gchar **) params[i].data.d_stringarray,
                                        count);
          else
            gimp_value_set_static_stringarray (&value,
                                               (const gchar **) params[i].data.d_stringarray,
                                               count);
          break;

        case GIMP_PDB_COLOR:
          gimp_value_set_rgb (&value, &params[i].data.d_color);
          break;

        case GIMP_PDB_ITEM:
          g_value_set_int (&value, params[i].data.d_item);
          break;

        case GIMP_PDB_DISPLAY:
          g_value_set_int (&value, params[i].data.d_display);
          break;

        case GIMP_PDB_IMAGE:
          g_value_set_int (&value, params[i].data.d_image);
          break;

        case GIMP_PDB_LAYER:
          g_value_set_int (&value, params[i].data.d_layer);
          break;

        case GIMP_PDB_CHANNEL:
          g_value_set_int (&value, params[i].data.d_channel);
          break;

        case GIMP_PDB_DRAWABLE:
          g_value_set_int (&value, params[i].data.d_drawable);
          break;

        case GIMP_PDB_SELECTION:
          g_value_set_int (&value, params[i].data.d_selection);
          break;

        case GIMP_PDB_COLORARRAY:
          count = g_value_get_int (gimp_value_array_index (args, i - 1));
          if (full_copy)
            gimp_value_set_colorarray (&value,
                                      params[i].data.d_colorarray,
                                      count);
          else
            gimp_value_set_static_colorarray (&value,
                                             params[i].data.d_colorarray,
                                             count);
          break;

        case GIMP_PDB_VECTORS:
          g_value_set_int (&value, params[i].data.d_vectors);
          break;

        case GIMP_PDB_PARASITE:
          if (full_copy)
            g_value_set_boxed (&value, &params[i].data.d_parasite);
          else
            g_value_set_static_boxed (&value, &params[i].data.d_parasite);
          break;

        case GIMP_PDB_STATUS:
          g_value_set_enum (&value, params[i].data.d_status);
          break;

        case GIMP_PDB_END:
          break;
        }

      gimp_value_array_append (args, &value);
      g_value_unset (&value);
    }

  return args;
}

GPParam *
plug_in_args_to_params (GimpValueArray *args,
                        gboolean        full_copy)
{
  GPParam *params;
  gint     length;
  gint     i;

  g_return_val_if_fail (args != NULL, NULL);

  params = g_new0 (GPParam, gimp_value_array_length (args));

  length = gimp_value_array_length (args);

  for (i = 0; i < length; i++)
    {
      GValue *value = gimp_value_array_index (args, i);

      params[i].type =
        gimp_pdb_compat_arg_type_from_gtype (G_VALUE_TYPE (value));

      switch (params[i].type)
        {
        case GIMP_PDB_INT32:
          if (G_VALUE_HOLDS_INT (value))
            params[i].data.d_int32 = g_value_get_int (value);
          else if (G_VALUE_HOLDS_UINT (value))
            params[i].data.d_int32 = g_value_get_uint (value);
          else if (G_VALUE_HOLDS_ENUM (value))
            params[i].data.d_int32 = g_value_get_enum (value);
          else if (G_VALUE_HOLDS_BOOLEAN (value))
            params[i].data.d_int32 = g_value_get_boolean (value);
          else
            {
              g_printerr ("%s: unhandled GIMP_PDB_INT32 type: %s\n",
                          G_STRFUNC, g_type_name (G_VALUE_TYPE (value)));
              g_return_val_if_reached (params);
            }
          break;

        case GIMP_PDB_INT16:
          params[i].data.d_int16 = g_value_get_int (value);
          break;

        case GIMP_PDB_INT8:
          params[i].data.d_int8 = g_value_get_uint (value);
          break;

        case GIMP_PDB_FLOAT:
          params[i].data.d_float = g_value_get_double (value);
          break;

        case GIMP_PDB_STRING:
          if (full_copy)
            params[i].data.d_string = g_value_dup_string (value);
          else
            params[i].data.d_string = (gchar *) g_value_get_string (value);
          break;

        case GIMP_PDB_INT32ARRAY:
          if (full_copy)
            params[i].data.d_int32array = gimp_value_dup_int32array (value);
          else
            params[i].data.d_int32array = (gint32 *) gimp_value_get_int32array (value);
          break;

        case GIMP_PDB_INT16ARRAY:
          if (full_copy)
            params[i].data.d_int16array = gimp_value_dup_int16array (value);
          else
            params[i].data.d_int16array = (gint16 *) gimp_value_get_int16array (value);
          break;

        case GIMP_PDB_INT8ARRAY:
          if (full_copy)
            params[i].data.d_int8array = gimp_value_dup_int8array (value);
          else
            params[i].data.d_int8array = (guint8 *) gimp_value_get_int8array (value);
          break;

        case GIMP_PDB_FLOATARRAY:
          if (full_copy)
            params[i].data.d_floatarray = gimp_value_dup_floatarray (value);
          else
            params[i].data.d_floatarray = (gdouble *) gimp_value_get_floatarray (value);
          break;

        case GIMP_PDB_STRINGARRAY:
          if (full_copy)
            params[i].data.d_stringarray = gimp_value_dup_stringarray (value);
          else
            params[i].data.d_stringarray = (gchar **) gimp_value_get_stringarray (value);
          break;

        case GIMP_PDB_COLOR:
          gimp_value_get_rgb (value, &params[i].data.d_color);
          break;

        case GIMP_PDB_ITEM:
          params[i].data.d_item = g_value_get_int (value);
          break;

        case GIMP_PDB_DISPLAY:
          params[i].data.d_display = g_value_get_int (value);
          break;

        case GIMP_PDB_IMAGE:
          params[i].data.d_image = g_value_get_int (value);
          break;

        case GIMP_PDB_LAYER:
          params[i].data.d_layer = g_value_get_int (value);
          break;

        case GIMP_PDB_CHANNEL:
          params[i].data.d_channel = g_value_get_int (value);
          break;

        case GIMP_PDB_DRAWABLE:
          params[i].data.d_drawable = g_value_get_int (value);
          break;

        case GIMP_PDB_SELECTION:
          params[i].data.d_selection = g_value_get_int (value);
          break;

        case GIMP_PDB_COLORARRAY:
          if (full_copy)
            params[i].data.d_colorarray = gimp_value_dup_colorarray (value);
          else
            params[i].data.d_colorarray = (GimpRGB *) gimp_value_get_colorarray (value);
          break;

        case GIMP_PDB_VECTORS:
          params[i].data.d_vectors = g_value_get_int (value);
          break;

        case GIMP_PDB_PARASITE:
          {
            GimpParasite *parasite = (full_copy ?
                                      g_value_dup_boxed (value) :
                                      g_value_get_boxed (value));

            if (parasite)
              {
                params[i].data.d_parasite.name  = parasite->name;
                params[i].data.d_parasite.flags = parasite->flags;
                params[i].data.d_parasite.size  = parasite->size;
                params[i].data.d_parasite.data  = parasite->data;

                if (full_copy)
                  {
                    parasite->name  = NULL;
                    parasite->flags = 0;
                    parasite->size  = 0;
                    parasite->data  = NULL;

                    gimp_parasite_free (parasite);
                  }
              }
            else
              {
                params[i].data.d_parasite.name  = NULL;
                params[i].data.d_parasite.flags = 0;
                params[i].data.d_parasite.size  = 0;
                params[i].data.d_parasite.data  = NULL;
              }
          }
          break;

        case GIMP_PDB_STATUS:
          params[i].data.d_status = g_value_get_enum (value);
          break;

        case GIMP_PDB_END:
          break;
        }
    }

  return params;
}
