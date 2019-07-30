/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpgpparams-body.c
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

/*  this file is included by both
 *
 *  libgimp/gimpgpparams.c
 *  app/plug-in/gimpgpparams.c
 */

void
_gimp_param_spec_to_gp_param_def (GParamSpec *pspec,
                                  GPParamDef *param_def)
{
  GType pspec_type;

  param_def->param_def_type = GP_PARAM_DEF_TYPE_DEFAULT;
  param_def->type_name      = (gchar *) G_PARAM_SPEC_TYPE_NAME (pspec);
  param_def->name           = (gchar *) g_param_spec_get_name (pspec);
  param_def->nick           = (gchar *) g_param_spec_get_nick (pspec);
  param_def->blurb          = (gchar *) g_param_spec_get_blurb (pspec);

  pspec_type = G_PARAM_SPEC_TYPE (pspec);

  if (pspec_type == GIMP_TYPE_PARAM_INT32 ||
      pspec_type == GIMP_TYPE_PARAM_INT16)
    {
      GParamSpecInt *ispec = G_PARAM_SPEC_INT (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_INT;

      param_def->meta.m_int.min_val     = ispec->minimum;
      param_def->meta.m_int.max_val     = ispec->maximum;
      param_def->meta.m_int.default_val = ispec->default_value;
    }
  else if (pspec_type == GIMP_TYPE_PARAM_INT8)
    {
      GParamSpecUInt *uspec = G_PARAM_SPEC_UINT (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_INT;

      param_def->meta.m_int.min_val     = uspec->minimum;
      param_def->meta.m_int.max_val     = uspec->maximum;
      param_def->meta.m_int.default_val = uspec->default_value;
    }
  else if (pspec_type == GIMP_TYPE_PARAM_UNIT)
    {
      GParamSpecInt     *ispec = G_PARAM_SPEC_INT (pspec);
      GimpParamSpecUnit *uspec = GIMP_PARAM_SPEC_UNIT (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_UNIT;

      param_def->meta.m_unit.allow_pixels  = (ispec->minimum < GIMP_UNIT_INCH);
      param_def->meta.m_unit.allow_percent = uspec->allow_percent;
      param_def->meta.m_unit.default_val   = ispec->default_value;
    }
  else if (pspec_type == G_TYPE_PARAM_ENUM)
    {
      GParamSpecEnum *espec     = G_PARAM_SPEC_ENUM (pspec);
      GType           enum_type = pspec->value_type;

      param_def->param_def_type = GP_PARAM_DEF_TYPE_ENUM;

      param_def->meta.m_enum.type_name    = (gchar *) g_type_name (enum_type);
      param_def->meta.m_float.default_val = espec->default_value;
    }
  else if (pspec_type == G_TYPE_PARAM_BOOLEAN)
    {
      GParamSpecBoolean *bspec = G_PARAM_SPEC_BOOLEAN (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_BOOLEAN;

      param_def->meta.m_boolean.default_val = bspec->default_value;
    }
  else if (pspec_type == G_TYPE_PARAM_DOUBLE)
    {
      GParamSpecDouble *dspec = G_PARAM_SPEC_DOUBLE (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_FLOAT;

      param_def->meta.m_float.min_val     = dspec->minimum;
      param_def->meta.m_float.max_val     = dspec->maximum;
      param_def->meta.m_float.default_val = dspec->default_value;
    }
  else if (pspec_type == GIMP_TYPE_PARAM_STRING)
    {
      GParamSpecString    *gsspec = G_PARAM_SPEC_STRING (pspec);
      GimpParamSpecString *sspec  = GIMP_PARAM_SPEC_STRING (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_STRING;

      param_def->meta.m_string.allow_non_utf8 = sspec->allow_non_utf8;
      param_def->meta.m_string.null_ok        = ! gsspec->ensure_non_null;
      param_def->meta.m_string.non_empty      = sspec->non_empty;
      param_def->meta.m_string.default_val    = gsspec->default_value;
    }
  else if (pspec_type == GIMP_TYPE_PARAM_RGB)
    {
      param_def->param_def_type = GP_PARAM_DEF_TYPE_COLOR;

      param_def->meta.m_color.has_alpha =
        gimp_param_spec_rgb_has_alpha (pspec);

      gimp_param_spec_rgb_get_default (pspec,
                                       &param_def->meta.m_color.default_val);
    }
  else if (pspec_type == GIMP_TYPE_PARAM_DISPLAY_ID)
    {
      GimpParamSpecDisplayID *ispec = GIMP_PARAM_SPEC_DISPLAY_ID (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_ID;

      param_def->meta.m_id.none_ok = ispec->none_ok;
    }
  else if (pspec_type == GIMP_TYPE_PARAM_IMAGE_ID)
    {
      GimpParamSpecImageID *ispec = GIMP_PARAM_SPEC_IMAGE_ID (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_ID;

      param_def->meta.m_id.none_ok = ispec->none_ok;
    }
  else if (GIMP_IS_PARAM_SPEC_ITEM_ID (pspec))
    {
      GimpParamSpecItemID *ispec = GIMP_PARAM_SPEC_ITEM_ID (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_ID;

      param_def->meta.m_id.none_ok = ispec->none_ok;
    }
}

GimpValueArray *
_gimp_gp_params_to_value_array (GParamSpec **pspecs,
                                gint         n_pspecs,
                                GPParam     *params,
                                gint         n_params,
                                gboolean     return_values,
                                gboolean     full_copy)
{
  GimpValueArray *args;
  gint            i;

  g_return_val_if_fail ((params != NULL && n_params  > 0) ||
                        (params == NULL && n_params == 0), NULL);

  args = gimp_value_array_new (n_params);

  for (i = 0; i < n_params; i++)
    {
      GValue value = G_VALUE_INIT;
      GType  type;

      /*  first get the GType from the passed GPParam  */
      type = g_type_from_name (params[i].type_name);

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
              GType pspec_type = G_PARAM_SPEC_VALUE_TYPE (pspecs[pspec_index]);

              if (type != pspec_type)
                {
                  if (g_type_is_a (pspec_type, type))
                    {
                      /*  if the param spec's GType is more specific
                       *  than the one from the GPParam, use the param
                       *  spec's GType.
                       */
                      type = pspec_type;
                    }
                  else if (type == GIMP_TYPE_INT32)
                    {
                      if (g_type_is_a (pspec_type, G_TYPE_ENUM))
                        {
                          /*  if the param spec's type is enum, but an
                           *  int32 was passed, use the enum type.
                           */
                          type = pspec_type;
                        }
                      else if (g_type_is_a (pspec_type, G_TYPE_BOOLEAN))
                        {
                          /*  if the param spec's type is boolean, but
                           *  an int32 was passed, use the boolean
                           *  type.
                           */
                          type = pspec_type;
                        }
                    }
                }
            }
        }

      g_value_init (&value, type);

      if (type == G_TYPE_INT      ||
          type == GIMP_TYPE_INT32 ||
          type == GIMP_TYPE_INT16 ||
          type == GIMP_TYPE_UNIT)
        {
          g_value_set_int (&value, params[i].data.d_int);
        }
      else if (G_VALUE_HOLDS_UINT (&value))
        {
          g_value_set_uint (&value, params[i].data.d_int);
        }
      else if (G_VALUE_HOLDS_ENUM (&value))
        {
          g_value_set_enum (&value, params[i].data.d_int);
        }
      else if (G_VALUE_HOLDS_BOOLEAN (&value))
        {
          g_value_set_boolean (&value, params[i].data.d_int ? TRUE : FALSE);
        }
      else if (G_VALUE_HOLDS_DOUBLE (&value))
        {
          g_value_set_double (&value, params[i].data.d_float);
        }
      else if (G_VALUE_HOLDS_STRING (&value))
        {
          if (full_copy)
            g_value_set_string (&value, params[i].data.d_string);
          else
            g_value_set_static_string (&value, params[i].data.d_string);
        }
      else if (GIMP_VALUE_HOLDS_RGB (&value))
        {
          gimp_value_set_rgb (&value, &params[i].data.d_color);
        }
      else if (GIMP_VALUE_HOLDS_PARASITE (&value))
        {
          if (full_copy)
            g_value_set_boxed (&value, &params[i].data.d_parasite);
          else
            g_value_set_static_boxed (&value, &params[i].data.d_parasite);
        }
      else if (GIMP_VALUE_HOLDS_INT32_ARRAY (&value))
        {
          if (full_copy)
            gimp_value_set_int32_array (&value,
                                        (gint32 *) params[i].data.d_array.data,
                                        params[i].data.d_array.size /
                                        sizeof (gint32));
          else
            gimp_value_set_static_int32_array (&value,
                                               (gint32 *) params[i].data.d_array.data,
                                               params[i].data.d_array.size /
                                               sizeof (gint32));
        }
      else if (GIMP_VALUE_HOLDS_INT16_ARRAY (&value))
        {
          if (full_copy)
            gimp_value_set_int16_array (&value,
                                        (gint16 *) params[i].data.d_array.data,
                                        params[i].data.d_array.size /
                                        sizeof (gint16));
          else
            gimp_value_set_static_int16_array (&value,
                                               (gint16 *) params[i].data.d_array.data,
                                               params[i].data.d_array.size /
                                               sizeof (gint16));
        }
      else if (GIMP_VALUE_HOLDS_INT8_ARRAY (&value))
        {
          if (full_copy)
            gimp_value_set_int8_array (&value,
                                       params[i].data.d_array.data,
                                       params[i].data.d_array.size /
                                       sizeof (guint8));
          else
            gimp_value_set_static_int8_array (&value,
                                              params[i].data.d_array.data,
                                              params[i].data.d_array.size /
                                              sizeof (guint8));
        }
      else if (GIMP_VALUE_HOLDS_FLOAT_ARRAY (&value))
        {
          if (full_copy)
            gimp_value_set_float_array (&value,
                                        (const gdouble *)
                                        params[i].data.d_array.data,
                                        params[i].data.d_array.size /
                                        sizeof (gdouble));
          else
            gimp_value_set_static_float_array (&value,
                                               (const gdouble *)
                                               params[i].data.d_array.data,
                                               params[i].data.d_array.size /
                                               sizeof (gdouble));
        }
      else if (GIMP_VALUE_HOLDS_STRING_ARRAY (&value))
        {
          if (full_copy)
            gimp_value_set_string_array (&value,
                                         (const gchar **)
                                         params[i].data.d_string_array.data,
                                         params[i].data.d_string_array.size);
          else
            gimp_value_set_static_string_array (&value,
                                                (const gchar **)
                                                params[i].data.d_string_array.data,
                                                params[i].data.d_string_array.size);
        }
      else if (GIMP_VALUE_HOLDS_RGB_ARRAY (&value))
        {
          if (full_copy)
            gimp_value_set_rgb_array (&value,
                                      (GimpRGB *)
                                      params[i].data.d_array.data,
                                      params[i].data.d_array.size /
                                      sizeof (GimpRGB));
          else
            gimp_value_set_static_rgb_array (&value,
                                             (GimpRGB *)
                                             params[i].data.d_array.data,
                                             params[i].data.d_array.size /
                                             sizeof (GimpRGB));
        }
      else if (GIMP_VALUE_HOLDS_DISPLAY_ID    (&value) ||
               GIMP_VALUE_HOLDS_IMAGE_ID      (&value) ||
               GIMP_VALUE_HOLDS_ITEM_ID       (&value) ||
               GIMP_VALUE_HOLDS_DRAWABLE_ID   (&value) ||
               GIMP_VALUE_HOLDS_LAYER_ID      (&value) ||
               GIMP_VALUE_HOLDS_CHANNEL_ID    (&value) ||
               GIMP_VALUE_HOLDS_LAYER_MASK_ID (&value) ||
               GIMP_VALUE_HOLDS_SELECTION_ID  (&value) ||
               GIMP_VALUE_HOLDS_VECTORS_ID    (&value))
        {
          g_value_set_int (&value, params[i].data.d_int);
        }

      gimp_value_array_append (args, &value);
      g_value_unset (&value);
    }

  return args;
}

GPParam *
_gimp_value_array_to_gp_params (GimpValueArray  *args,
                                gboolean         full_copy)
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
      GType   type  = G_VALUE_TYPE (value);

      params[i].param_type = -1;

      if (full_copy)
        params[i].type_name = g_strdup (g_type_name (type));
      else
        params[i].type_name = (gchar *) g_type_name (type);

      if (type == G_TYPE_INT      ||
          type == GIMP_TYPE_INT32 ||
          type == GIMP_TYPE_INT16 ||
          type == GIMP_TYPE_UNIT)
        {
          params[i].param_type = GP_PARAM_TYPE_INT;

          params[i].data.d_int = g_value_get_int (value);
        }
      else if (type == G_TYPE_UINT ||
               type == GIMP_TYPE_INT8)
        {
          params[i].param_type = GP_PARAM_TYPE_INT;

          params[i].data.d_int = g_value_get_uint (value);
        }
      else if (G_VALUE_HOLDS_ENUM (value))
        {
          params[i].param_type = GP_PARAM_TYPE_INT;

          params[i].data.d_int = g_value_get_enum (value);
        }
      else if (G_VALUE_HOLDS_BOOLEAN (value))
        {
          params[i].param_type = GP_PARAM_TYPE_INT;

          params[i].data.d_int = g_value_get_boolean (value);
        }
      else if (G_VALUE_HOLDS_DOUBLE (value))
        {
          params[i].param_type = GP_PARAM_TYPE_FLOAT;

          params[i].data.d_float = g_value_get_double (value);
        }
      else if (G_VALUE_HOLDS_STRING (value))
        {
          params[i].param_type = GP_PARAM_TYPE_STRING;

          if (full_copy)
            params[i].data.d_string = g_value_dup_string (value);
          else
            params[i].data.d_string = (gchar *) g_value_get_string (value);
        }
      else if (GIMP_VALUE_HOLDS_RGB (value))
        {
          params[i].param_type = GP_PARAM_TYPE_COLOR;

          gimp_value_get_rgb (value, &params[i].data.d_color);
        }
      else if (GIMP_VALUE_HOLDS_PARASITE (value))
        {
          GimpParasite *parasite = (full_copy ?
                                    g_value_dup_boxed (value) :
                                    g_value_get_boxed (value));

          params[i].param_type = GP_PARAM_TYPE_PARASITE;

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
      else if (GIMP_VALUE_HOLDS_INT32_ARRAY (value) ||
               GIMP_VALUE_HOLDS_INT16_ARRAY (value) ||
               GIMP_VALUE_HOLDS_INT8_ARRAY  (value) ||
               GIMP_VALUE_HOLDS_FLOAT_ARRAY (value) ||
               GIMP_VALUE_HOLDS_RGB_ARRAY (value))
        {
          GimpArray *array = g_value_get_boxed (value);

          params[i].param_type = GP_PARAM_TYPE_ARRAY;

          params[i].data.d_array.size = array->length;

          if (full_copy)
            params[i].data.d_array.data = g_memdup (array->data,
                                                    array->length);
          else
            params[i].data.d_array.data = array->data;
        }
      else if (GIMP_VALUE_HOLDS_STRING_ARRAY (value))
        {
          GimpArray *array = g_value_get_boxed (value);

          params[i].param_type = GP_PARAM_TYPE_STRING_ARRAY;

          params[i].data.d_string_array.size = array->length;

          if (full_copy)
            params[i].data.d_string_array.data =
              gimp_value_dup_string_array (value);
          else
            params[i].data.d_string_array.data =
              (gchar **) gimp_value_get_string_array (value);
        }
      else if (GIMP_VALUE_HOLDS_DISPLAY_ID (value)    ||
               GIMP_VALUE_HOLDS_IMAGE_ID (value)      ||
               GIMP_VALUE_HOLDS_ITEM_ID (value)       ||
               GIMP_VALUE_HOLDS_DRAWABLE_ID (value)   ||
               GIMP_VALUE_HOLDS_LAYER_ID (value)      ||
               GIMP_VALUE_HOLDS_CHANNEL_ID (value)    ||
               GIMP_VALUE_HOLDS_LAYER_MASK_ID (value) ||
               GIMP_VALUE_HOLDS_SELECTION_ID (value)  ||
               GIMP_VALUE_HOLDS_VECTORS_ID (value))
        {
          params[i].param_type = GP_PARAM_TYPE_INT;

          params[i].data.d_int = g_value_get_int (value);
        }

      if (params[i].param_type == -1)
        g_printerr ("%s: GValue contains unsupported type '%s'\n",
                    G_STRFUNC, params[i].type_name);
    }

  return params;
}
