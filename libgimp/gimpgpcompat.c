/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpgpcompat.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"

#include "gimpgpcompat.h"


/*  local function prototypes  */

static gchar * _gimp_pdb_arg_type_to_string (GimpPDBArgType type);


/*  public functions  */

GParamSpec *
_gimp_gp_compat_param_spec (GimpPDBArgType  arg_type,
                            const gchar    *name,
                            const gchar    *nick,
                            const gchar    *blurb)
{
  GParamSpec *pspec = NULL;

  g_return_val_if_fail (name != NULL, NULL);

  switch (arg_type)
    {
    case GIMP_PDB_INT32:
      pspec = g_param_spec_int (name, nick, blurb,
                                G_MININT32, G_MAXINT32, 0,
                                G_PARAM_READWRITE);
      break;

    case GIMP_PDB_INT16:
      pspec = g_param_spec_int (name, nick, blurb,
                                G_MININT16, G_MAXINT16, 0,
                                G_PARAM_READWRITE);
      break;

    case GIMP_PDB_INT8:
      pspec = g_param_spec_uchar (name, nick, blurb,
                                  0, G_MAXUINT8, 0,
                                  G_PARAM_READWRITE);
      break;

    case GIMP_PDB_FLOAT:
      pspec = g_param_spec_double (name, nick, blurb,
                                   -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                                   G_PARAM_READWRITE);
      break;

    case GIMP_PDB_STRING:
      pspec = g_param_spec_string (name, nick, blurb,
                                   NULL,
                                   G_PARAM_READWRITE);
      break;

    case GIMP_PDB_INT32ARRAY:
      pspec = gimp_param_spec_int32_array (name, nick, blurb,
                                           G_PARAM_READWRITE);
      break;

    case GIMP_PDB_INT16ARRAY:
      pspec = gimp_param_spec_int16_array (name, nick, blurb,
                                           G_PARAM_READWRITE);
      break;

    case GIMP_PDB_INT8ARRAY:
      pspec = gimp_param_spec_uint8_array (name, nick, blurb,
                                           G_PARAM_READWRITE);
      break;

    case GIMP_PDB_FLOATARRAY:
      pspec = gimp_param_spec_float_array (name, nick, blurb,
                                           G_PARAM_READWRITE);
      break;

    case GIMP_PDB_STRINGARRAY:
      pspec = gimp_param_spec_string_array (name, nick, blurb,
                                            G_PARAM_READWRITE);
      break;

    case GIMP_PDB_COLOR:
      pspec = gimp_param_spec_rgb (name, nick, blurb,
                                   TRUE, NULL,
                                   G_PARAM_READWRITE);
      break;

    case GIMP_PDB_ITEM:
      pspec = gimp_param_spec_item (name, nick, blurb,
                                    TRUE,
                                    G_PARAM_READWRITE);
      break;

    case GIMP_PDB_DISPLAY:
      pspec = gimp_param_spec_display (name, nick, blurb,
                                       TRUE,
                                       G_PARAM_READWRITE);
      break;

    case GIMP_PDB_IMAGE:
      pspec = gimp_param_spec_image (name, nick, blurb,
                                     TRUE,
                                     G_PARAM_READWRITE);
      break;

    case GIMP_PDB_LAYER:
      pspec = gimp_param_spec_layer (name, nick, blurb,
                                     TRUE,
                                     G_PARAM_READWRITE);
      break;

    case GIMP_PDB_CHANNEL:
      pspec = gimp_param_spec_channel (name, nick, blurb,
                                       TRUE,
                                       G_PARAM_READWRITE);
      break;

    case GIMP_PDB_DRAWABLE:
      pspec = gimp_param_spec_drawable (name, nick, blurb,
                                        TRUE,
                                        G_PARAM_READWRITE);
      break;

    case GIMP_PDB_SELECTION:
      pspec = gimp_param_spec_selection (name, nick, blurb,
                                         TRUE,
                                         G_PARAM_READWRITE);
      break;

    case GIMP_PDB_COLORARRAY:
      pspec = gimp_param_spec_rgb_array (name, nick, blurb,
                                         G_PARAM_READWRITE);
      break;

    case GIMP_PDB_VECTORS:
      pspec = gimp_param_spec_vectors (name, nick, blurb,
                                       TRUE,
                                       G_PARAM_READWRITE);
      break;

    case GIMP_PDB_PARASITE:
      pspec = gimp_param_spec_parasite (name, nick, blurb,
                                        G_PARAM_READWRITE);
      break;

    case GIMP_PDB_STATUS:
      pspec = g_param_spec_enum (name, nick, blurb,
                                 GIMP_TYPE_PDB_STATUS_TYPE,
                                 GIMP_PDB_EXECUTION_ERROR,
                                 G_PARAM_READWRITE);
      break;

    case GIMP_PDB_END:
      break;
    }

  if (! pspec)
    g_warning ("%s: returning NULL for %s (%s)",
               G_STRFUNC, name, _gimp_pdb_arg_type_to_string (arg_type));

  return pspec;
}

GType
_gimp_pdb_arg_type_to_gtype (GimpPDBArgType  type)
{
  switch (type)
    {
    case GIMP_PDB_INT32:
      return G_TYPE_INT;

    case GIMP_PDB_INT16:
      return G_TYPE_INT;

    case GIMP_PDB_INT8:
      return G_TYPE_UCHAR;

    case GIMP_PDB_FLOAT:
      return G_TYPE_DOUBLE;

    case GIMP_PDB_STRING:
      return G_TYPE_STRING;

    case GIMP_PDB_INT32ARRAY:
      return GIMP_TYPE_INT32_ARRAY;

    case GIMP_PDB_INT16ARRAY:
      return GIMP_TYPE_INT16_ARRAY;

    case GIMP_PDB_INT8ARRAY:
      return GIMP_TYPE_UINT8_ARRAY;

    case GIMP_PDB_FLOATARRAY:
      return GIMP_TYPE_FLOAT_ARRAY;

    case GIMP_PDB_STRINGARRAY:
      return GIMP_TYPE_STRING_ARRAY;

    case GIMP_PDB_COLOR:
      return GIMP_TYPE_RGB;

    case GIMP_PDB_ITEM:
      return GIMP_TYPE_ITEM;

    case GIMP_PDB_DISPLAY:
      return GIMP_TYPE_DISPLAY;

    case GIMP_PDB_IMAGE:
      return GIMP_TYPE_IMAGE;

    case GIMP_PDB_LAYER:
      return GIMP_TYPE_LAYER;

    case GIMP_PDB_CHANNEL:
      return GIMP_TYPE_CHANNEL;

    case GIMP_PDB_DRAWABLE:
      return GIMP_TYPE_DRAWABLE;

    case GIMP_PDB_SELECTION:
      return GIMP_TYPE_SELECTION;

    case GIMP_PDB_COLORARRAY:
      return GIMP_TYPE_RGB_ARRAY;

    case GIMP_PDB_VECTORS:
      return GIMP_TYPE_VECTORS;

    case GIMP_PDB_PARASITE:
      return GIMP_TYPE_PARASITE;

    case GIMP_PDB_STATUS:
      return GIMP_TYPE_PDB_STATUS_TYPE;

    case GIMP_PDB_END:
      break;
    }

  g_warning ("%s: returning G_TYPE_NONE for %d (%s)",
             G_STRFUNC, type, _gimp_pdb_arg_type_to_string (type));

  return G_TYPE_NONE;
}

GimpPDBArgType
_gimp_pdb_gtype_to_arg_type (GType type)
{
  static GQuark  pdb_type_quark = 0;
  GimpPDBArgType pdb_type;

  if (! pdb_type_quark)
    {
      struct
      {
        GType          g_type;
        GimpPDBArgType pdb_type;
      }
      type_mapping[] =
      {
        { G_TYPE_INT,                GIMP_PDB_INT32       },
        { G_TYPE_UINT,               GIMP_PDB_INT32       },
        { G_TYPE_ENUM,               GIMP_PDB_INT32       },
        { G_TYPE_BOOLEAN,            GIMP_PDB_INT32       },

        { G_TYPE_UCHAR,              GIMP_PDB_INT8        },
        { G_TYPE_DOUBLE,             GIMP_PDB_FLOAT       },

        { G_TYPE_STRING,             GIMP_PDB_STRING      },

        { GIMP_TYPE_RGB,             GIMP_PDB_COLOR       },

        { GIMP_TYPE_INT32_ARRAY,     GIMP_PDB_INT32ARRAY  },
        { GIMP_TYPE_INT16_ARRAY,     GIMP_PDB_INT16ARRAY  },
        { GIMP_TYPE_UINT8_ARRAY,     GIMP_PDB_INT8ARRAY   },
        { GIMP_TYPE_FLOAT_ARRAY,     GIMP_PDB_FLOATARRAY  },
        { GIMP_TYPE_STRING_ARRAY,    GIMP_PDB_STRINGARRAY },
        { GIMP_TYPE_RGB_ARRAY,       GIMP_PDB_COLORARRAY  },

        { GIMP_TYPE_ITEM,            GIMP_PDB_ITEM        },
        { GIMP_TYPE_DISPLAY,         GIMP_PDB_DISPLAY     },
        { GIMP_TYPE_IMAGE,           GIMP_PDB_IMAGE       },
        { GIMP_TYPE_LAYER,           GIMP_PDB_LAYER       },
        { GIMP_TYPE_CHANNEL,         GIMP_PDB_CHANNEL     },
        { GIMP_TYPE_DRAWABLE,        GIMP_PDB_DRAWABLE    },
        { GIMP_TYPE_SELECTION,       GIMP_PDB_SELECTION   },
        { GIMP_TYPE_LAYER_MASK,      GIMP_PDB_CHANNEL     },
        { GIMP_TYPE_VECTORS,         GIMP_PDB_VECTORS     },

        { GIMP_TYPE_PARASITE,        GIMP_PDB_PARASITE    },

        { GIMP_TYPE_PDB_STATUS_TYPE, GIMP_PDB_STATUS      }
      };

      gint i;

      pdb_type_quark = g_quark_from_static_string ("gimp-pdb-type");

      for (i = 0; i < G_N_ELEMENTS (type_mapping); i++)
        g_type_set_qdata (type_mapping[i].g_type, pdb_type_quark,
                          GINT_TO_POINTER (type_mapping[i].pdb_type));
    }

  pdb_type = GPOINTER_TO_INT (g_type_get_qdata (type, pdb_type_quark));

#if 0
  g_printerr ("%s: arg_type = %p (%s)  ->  %d (%s)\n",
              G_STRFUNC,
              (gpointer) type, g_type_name (type),
              pdb_type, _gimp_pdb_arg_type_to_string (pdb_type));
#endif

  return pdb_type;
}

GimpValueArray *
_gimp_params_to_value_array (const GimpParam  *params,
                             gint              n_params,
                             gboolean          full_copy)
{
  GimpValueArray *args;
  gint            i;

  g_return_val_if_fail ((params != NULL && n_params  > 0) ||
                        (params == NULL && n_params == 0), NULL);

  args = gimp_value_array_new (n_params);

  for (i = 0; i < n_params; i++)
    {
      GValue value = G_VALUE_INIT;
      GType  type  = _gimp_pdb_arg_type_to_gtype (params[i].type);
      gint   count;

      g_value_init (&value, type);

      switch (_gimp_pdb_gtype_to_arg_type (type))
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
          g_value_set_uchar (&value, params[i].data.d_int8);
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
          count = GIMP_VALUES_GET_INT (args, i - 1);
          if (full_copy)
            gimp_value_set_int32_array (&value,
                                        params[i].data.d_int32array,
                                        count);
          else
            gimp_value_set_static_int32_array (&value,
                                               params[i].data.d_int32array,
                                               count);
          break;

        case GIMP_PDB_INT16ARRAY:
          count = GIMP_VALUES_GET_INT (args, i - 1);
          if (full_copy)
            gimp_value_set_int16_array (&value,
                                        params[i].data.d_int16array,
                                        count);
          else
            gimp_value_set_static_int16_array (&value,
                                               params[i].data.d_int16array,
                                               count);
          break;

        case GIMP_PDB_INT8ARRAY:
          count = GIMP_VALUES_GET_INT (args, i - 1);
          if (full_copy)
            gimp_value_set_uint8_array (&value,
                                        params[i].data.d_int8array,
                                        count);
          else
            gimp_value_set_static_uint8_array (&value,
                                               params[i].data.d_int8array,
                                               count);
          break;

        case GIMP_PDB_FLOATARRAY:
          count = GIMP_VALUES_GET_INT (args, i - 1);
          if (full_copy)
            gimp_value_set_float_array (&value,
                                        params[i].data.d_floatarray,
                                        count);
          else
            gimp_value_set_static_float_array (&value,
                                               params[i].data.d_floatarray,
                                               count);
          break;

        case GIMP_PDB_STRINGARRAY:
          count = GIMP_VALUES_GET_INT (args, i - 1);
          if (full_copy)
            gimp_value_set_string_array (&value,
                                        (const gchar **) params[i].data.d_stringarray,
                                        count);
          else
            gimp_value_set_static_string_array (&value,
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
          count = GIMP_VALUES_GET_INT (args, i - 1);
          if (full_copy)
            gimp_value_set_rgb_array (&value,
                                      params[i].data.d_colorarray,
                                      count);
          else
            gimp_value_set_static_rgb_array (&value,
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

GimpParam *
_gimp_value_array_to_params (GimpValueArray *args,
                             gboolean        full_copy)
{
  GimpParam *params;
  gint       length;
  gint       i;

  g_return_val_if_fail (args != NULL, NULL);

  params = g_new0 (GimpParam, gimp_value_array_length (args));

  length = gimp_value_array_length (args);

  for (i = 0; i < length; i++)
    {
      GValue *value = gimp_value_array_index (args, i);

      params[i].type = _gimp_pdb_gtype_to_arg_type (G_VALUE_TYPE (value));

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
          params[i].data.d_int8 = g_value_get_uchar (value);
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
            params[i].data.d_int32array = gimp_value_dup_int32_array (value);
          else
            params[i].data.d_int32array = (gint32 *) gimp_value_get_int32_array (value);
          break;

        case GIMP_PDB_INT16ARRAY:
          if (full_copy)
            params[i].data.d_int16array = gimp_value_dup_int16_array (value);
          else
            params[i].data.d_int16array = (gint16 *) gimp_value_get_int16_array (value);
          break;

        case GIMP_PDB_INT8ARRAY:
          if (full_copy)
            params[i].data.d_int8array = gimp_value_dup_uint8_array (value);
          else
            params[i].data.d_int8array = (guint8 *) gimp_value_get_uint8_array (value);
          break;

        case GIMP_PDB_FLOATARRAY:
          if (full_copy)
            params[i].data.d_floatarray = gimp_value_dup_float_array (value);
          else
            params[i].data.d_floatarray = (gdouble *) gimp_value_get_float_array (value);
          break;

        case GIMP_PDB_STRINGARRAY:
          if (full_copy)
            params[i].data.d_stringarray = gimp_value_dup_string_array (value);
          else
            params[i].data.d_stringarray = (gchar **) gimp_value_get_string_array (value);
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
            params[i].data.d_colorarray = gimp_value_dup_rgb_array (value);
          else
            params[i].data.d_colorarray = (GimpRGB *) gimp_value_get_rgb_array (value);
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


/*  private functions  */

gchar *
_gimp_pdb_arg_type_to_string (GimpPDBArgType type)
{
  const gchar *name;

  if (! gimp_enum_get_value (GIMP_TYPE_PDB_ARG_TYPE, type,
                             &name, NULL, NULL, NULL))
    {
      return g_strdup_printf ("(PDB type %d unknown)", type);
    }

  return g_strdup (name);
}
