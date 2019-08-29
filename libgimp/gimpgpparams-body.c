/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpgpparams-body.c
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
  param_def->flags          = pspec->flags;

  pspec_type = G_PARAM_SPEC_TYPE (pspec);

  if (pspec_type == G_TYPE_PARAM_INT)
    {
      GParamSpecInt *ispec = G_PARAM_SPEC_INT (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_INT;

      param_def->meta.m_int.min_val     = ispec->minimum;
      param_def->meta.m_int.max_val     = ispec->maximum;
      param_def->meta.m_int.default_val = ispec->default_value;
    }
  else if (pspec_type == G_TYPE_PARAM_UINT)
    {
      GParamSpecUInt *uspec = G_PARAM_SPEC_UINT (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_INT;

      param_def->meta.m_int.min_val     = uspec->minimum;
      param_def->meta.m_int.max_val     = uspec->maximum;
      param_def->meta.m_int.default_val = uspec->default_value;
    }
  else if (pspec_type == G_TYPE_PARAM_UCHAR)
    {
      GParamSpecUChar *uspec = G_PARAM_SPEC_UCHAR (pspec);

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
  else if (G_IS_PARAM_SPEC_ENUM (pspec))
    {
      GParamSpecEnum *espec     = G_PARAM_SPEC_ENUM (pspec);
      GType           enum_type = pspec->value_type;

      param_def->param_def_type = GP_PARAM_DEF_TYPE_ENUM;

      param_def->meta.m_enum.type_name    = (gchar *) g_type_name (enum_type);
      param_def->meta.m_enum.default_val = espec->default_value;
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
  else if (G_IS_PARAM_SPEC_STRING (pspec))
    {
      GParamSpecString *gsspec = G_PARAM_SPEC_STRING (pspec);

      if (! strcmp (param_def->type_name, "GimpParamString"))
        param_def->type_name = "GParamString";

      param_def->param_def_type = GP_PARAM_DEF_TYPE_STRING;

      param_def->meta.m_string.default_val = gsspec->default_value;

    }
  else if (pspec_type == GIMP_TYPE_PARAM_RGB)
    {
      param_def->param_def_type = GP_PARAM_DEF_TYPE_COLOR;

      param_def->meta.m_color.has_alpha =
        gimp_param_spec_rgb_has_alpha (pspec);

      gimp_param_spec_rgb_get_default (pspec,
                                       &param_def->meta.m_color.default_val);
    }
  else if (pspec_type == GIMP_TYPE_PARAM_IMAGE)
    {
      GimpParamSpecImage *ispec = GIMP_PARAM_SPEC_IMAGE (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_ID;

      param_def->meta.m_id.none_ok = ispec->none_ok;
    }
  else if (GIMP_IS_PARAM_SPEC_ITEM (pspec))
    {
      GimpParamSpecItem *ispec = GIMP_PARAM_SPEC_ITEM (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_ID;

      param_def->meta.m_id.none_ok = ispec->none_ok;
    }
  else if (pspec_type == GIMP_TYPE_PARAM_DISPLAY)
    {
      GimpParamSpecDisplay *ispec = GIMP_PARAM_SPEC_DISPLAY (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_ID;

      param_def->meta.m_id.none_ok = ispec->none_ok;
    }
  else if (G_IS_PARAM_SPEC_PARAM (pspec))
    {
      param_def->param_def_type = GP_PARAM_DEF_TYPE_PARAM_DEF;

      param_def->meta.m_param_def.type_name =
        (gchar *) g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec));
    }
}

static GimpImage *
get_image_by_id (gpointer gimp,
                 gint     id)
{
#ifdef LIBGIMP_COMPILATION
  return gimp_image_get_by_id (id);
#else
  return gimp_image_get_by_id (gimp, id);
#endif
}

static GimpItem *
get_item_by_id (gpointer gimp,
                gint     id)
{
#ifdef LIBGIMP_COMPILATION
  return gimp_item_get_by_id (id);
#else
  return gimp_item_get_by_id (gimp, id);
#endif
}

static GObject *
get_display_by_id (gpointer gimp,
                   gint     id)
{
#ifdef LIBGIMP_COMPILATION
  return (GObject *) gimp_display_get_by_id (id);
#else
  return (GObject *) gimp_get_display_by_id (gimp, id);
#endif
}

void
_gimp_gp_param_to_value (gpointer        gimp,
                         const GPParam  *param,
                         GType           type,
                         GValue         *value,
                         gboolean        full_copy)
{
  g_return_if_fail (param != NULL);
  g_return_if_fail (value != NULL);

  if (type == G_TYPE_NONE)
    type = g_type_from_name (param->type_name);

  g_value_init (value, type);

  if (type == G_TYPE_INT ||
      type == GIMP_TYPE_UNIT)
    {
      g_value_set_int (value, param->data.d_int);
    }
  else if (G_VALUE_HOLDS_UINT (value))
    {
      g_value_set_uint (value, param->data.d_int);
    }
  else if (G_VALUE_HOLDS_ENUM (value))
    {
      g_value_set_enum (value, param->data.d_int);
    }
  else if (G_VALUE_HOLDS_BOOLEAN (value))
    {
      g_value_set_boolean (value, param->data.d_int ? TRUE : FALSE);
    }
  else if (G_VALUE_HOLDS_DOUBLE (value))
    {
      g_value_set_double (value, param->data.d_float);
    }
  else if (G_VALUE_HOLDS_STRING (value))
    {
      if (full_copy)
        g_value_set_string (value, param->data.d_string);
      else
        g_value_set_static_string (value, param->data.d_string);
    }
  else if (GIMP_VALUE_HOLDS_RGB (value))
    {
      gimp_value_set_rgb (value, &param->data.d_color);
    }
  else if (GIMP_VALUE_HOLDS_PARASITE (value))
    {
      if (full_copy)
        g_value_set_boxed (value, &param->data.d_parasite);
      else
        g_value_set_static_boxed (value, &param->data.d_parasite);
    }
  else if (GIMP_VALUE_HOLDS_INT32_ARRAY (value))
    {
      if (full_copy)
        gimp_value_set_int32_array (value,
                                    (gint32 *) param->data.d_array.data,
                                    param->data.d_array.size /
                                    sizeof (gint32));
      else
        gimp_value_set_static_int32_array (value,
                                           (gint32 *) param->data.d_array.data,
                                           param->data.d_array.size /
                                           sizeof (gint32));
    }
  else if (GIMP_VALUE_HOLDS_INT16_ARRAY (value))
    {
      if (full_copy)
        gimp_value_set_int16_array (value,
                                    (gint16 *) param->data.d_array.data,
                                    param->data.d_array.size /
                                    sizeof (gint16));
      else
        gimp_value_set_static_int16_array (value,
                                           (gint16 *) param->data.d_array.data,
                                           param->data.d_array.size /
                                           sizeof (gint16));
    }
  else if (GIMP_VALUE_HOLDS_UINT8_ARRAY (value))
    {
      if (full_copy)
        gimp_value_set_uint8_array (value,
                                    param->data.d_array.data,
                                    param->data.d_array.size /
                                    sizeof (guint8));
      else
        gimp_value_set_static_uint8_array (value,
                                           param->data.d_array.data,
                                           param->data.d_array.size /
                                           sizeof (guint8));
    }
  else if (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value))
    {
      if (full_copy)
        gimp_value_set_float_array (value,
                                    (const gdouble *)
                                    param->data.d_array.data,
                                    param->data.d_array.size /
                                    sizeof (gdouble));
      else
        gimp_value_set_static_float_array (value,
                                           (const gdouble *)
                                           param->data.d_array.data,
                                           param->data.d_array.size /
                                           sizeof (gdouble));
    }
  else if (GIMP_VALUE_HOLDS_STRING_ARRAY (value))
    {
      if (full_copy)
        gimp_value_set_string_array (value,
                                     (const gchar **)
                                     param->data.d_string_array.data,
                                     param->data.d_string_array.size);
      else
        gimp_value_set_static_string_array (value,
                                            (const gchar **)
                                            param->data.d_string_array.data,
                                            param->data.d_string_array.size);
    }
  else if (GIMP_VALUE_HOLDS_RGB_ARRAY (value))
    {
      if (full_copy)
        gimp_value_set_rgb_array (value,
                                  (GimpRGB *)
                                  param->data.d_array.data,
                                  param->data.d_array.size /
                                  sizeof (GimpRGB));
      else
        gimp_value_set_static_rgb_array (value,
                                         (GimpRGB *)
                                         param->data.d_array.data,
                                         param->data.d_array.size /
                                         sizeof (GimpRGB));
    }
  else if (GIMP_VALUE_HOLDS_IMAGE (value))
    {
      g_value_set_object (value, get_image_by_id (gimp, param->data.d_int));
    }
  else if (GIMP_VALUE_HOLDS_ITEM (value))
    {
      g_value_set_object (value, get_item_by_id (gimp, param->data.d_int));
    }
  else if (GIMP_VALUE_HOLDS_DISPLAY (value))
    {
      g_value_set_object (value, get_display_by_id (gimp, param->data.d_int));
    }
  else if (G_VALUE_HOLDS_PARAM (value))
    {
      GParamSpec *pspec =
        _gimp_gp_param_def_to_param_spec (gimp, &param->data.d_param_def);

      g_value_take_param (value, pspec);
    }
}

GimpValueArray *
_gimp_gp_params_to_value_array (gpointer        gimp,
                                GParamSpec    **pspecs,
                                gint            n_pspecs,
                                const GPParam  *params,
                                gint            n_params,
                                gboolean        return_values,
                                gboolean        full_copy)
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
                  else if (type == G_TYPE_INT)
                    {
                      if (g_type_is_a (pspec_type, G_TYPE_ENUM))
                        {
                          /*  if the param spec's type is enum, but an
                           *  int was passed, use the enum type.
                           */
                          type = pspec_type;
                        }
                      else if (g_type_is_a (pspec_type, G_TYPE_BOOLEAN))
                        {
                          /*  if the param spec's type is boolean, but
                           *  an int was passed, use the boolean
                           *  type.
                           */
                          type = pspec_type;
                        }
                    }
                }
            }
        }

      _gimp_gp_param_to_value (gimp, &params[i], type, &value, full_copy);

      gimp_value_array_append (args, &value);
      g_value_unset (&value);
    }

  return args;
}

void
_gimp_value_to_gp_param (const GValue *value,
                         GPParam      *param,
                         gboolean      full_copy)
{
  GType type;

  g_return_if_fail (value != NULL);
  g_return_if_fail (param != NULL);

  type = G_VALUE_TYPE (value);

  param->param_type = -1;

  if (full_copy)
    param->type_name = g_strdup (g_type_name (type));
  else
    param->type_name = (gchar *) g_type_name (type);

  if (type == G_TYPE_INT ||
      type == GIMP_TYPE_UNIT)
    {
      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = g_value_get_int (value);
    }
  else if (type == G_TYPE_UINT)
    {
      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = g_value_get_uint (value);
    }
  else if (type == G_TYPE_UCHAR)
    {
      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = g_value_get_uchar (value);
    }
  else if (G_VALUE_HOLDS_ENUM (value))
    {
      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = g_value_get_enum (value);
    }
  else if (G_VALUE_HOLDS_BOOLEAN (value))
    {
      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = g_value_get_boolean (value);
    }
  else if (G_VALUE_HOLDS_DOUBLE (value))
    {
      param->param_type = GP_PARAM_TYPE_FLOAT;

      param->data.d_float = g_value_get_double (value);
    }
  else if (G_VALUE_HOLDS_STRING (value))
    {
      param->param_type = GP_PARAM_TYPE_STRING;

      if (full_copy)
        param->data.d_string = g_value_dup_string (value);
      else
        param->data.d_string = (gchar *) g_value_get_string (value);
    }
  else if (GIMP_VALUE_HOLDS_RGB (value))
    {
      param->param_type = GP_PARAM_TYPE_COLOR;

      gimp_value_get_rgb (value, &param->data.d_color);
    }
  else if (GIMP_VALUE_HOLDS_PARASITE (value))
    {
      GimpParasite *parasite = (full_copy ?
                                g_value_dup_boxed (value) :
                                g_value_get_boxed (value));

      param->param_type = GP_PARAM_TYPE_PARASITE;

      if (parasite)
        {
          param->data.d_parasite.name  = parasite->name;
          param->data.d_parasite.flags = parasite->flags;
          param->data.d_parasite.size  = parasite->size;
          param->data.d_parasite.data  = parasite->data;

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
          param->data.d_parasite.name  = NULL;
          param->data.d_parasite.flags = 0;
          param->data.d_parasite.size  = 0;
          param->data.d_parasite.data  = NULL;
        }
    }
  else if (GIMP_VALUE_HOLDS_INT32_ARRAY (value) ||
           GIMP_VALUE_HOLDS_INT16_ARRAY (value) ||
           GIMP_VALUE_HOLDS_UINT8_ARRAY (value) ||
           GIMP_VALUE_HOLDS_FLOAT_ARRAY (value) ||
           GIMP_VALUE_HOLDS_RGB_ARRAY (value))
    {
      GimpArray *array = g_value_get_boxed (value);

      if (array)
        {
          param->param_type = GP_PARAM_TYPE_ARRAY;

          param->data.d_array.size = array->length;

          if (full_copy)
            param->data.d_array.data = g_memdup (array->data,
                                                 array->length);
          else
            param->data.d_array.data = array->data;
        }
      else
        {
          param->data.d_array.data = NULL;
        }
    }
  else if (GIMP_VALUE_HOLDS_STRING_ARRAY (value))
    {
      GimpArray *array = g_value_get_boxed (value);

      if (array)
        {
          param->param_type = GP_PARAM_TYPE_STRING_ARRAY;

          param->data.d_string_array.size = array->length;

          if (full_copy)
            param->data.d_string_array.data =
              gimp_value_dup_string_array (value);
          else
            param->data.d_string_array.data =
              (gchar **) gimp_value_get_string_array (value);
        }
      else
        {
          param->data.d_string_array.data = NULL;
        }
    }
  else if (GIMP_VALUE_HOLDS_IMAGE (value))
    {
      GimpImage *image = g_value_get_object (value);

      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = image ? gimp_image_get_id (image) : -1;
    }
  else if (GIMP_VALUE_HOLDS_ITEM (value))
    {
      GimpItem *item = g_value_get_object (value);

      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = item ? gimp_item_get_id (item) : -1;
    }
  else if (GIMP_VALUE_HOLDS_DISPLAY (value))
    {
      GObject *display = g_value_get_object (value);
      gint     id      = -1;

#if 0
      if (full_copy)
        {
          g_free (param->type_name);
          param->type_name = "GObject";
        }
      else
        param->type_name = (gchar *) "GObject";
#endif

      param->param_type = GP_PARAM_TYPE_INT;

      if (display)
        g_object_get (display, "id", &id, NULL);

      param->data.d_int = id;
    }
  else if (G_VALUE_HOLDS_PARAM (value))
    {
      param->param_type = GP_PARAM_TYPE_PARAM_DEF;

      _gimp_param_spec_to_gp_param_def (g_value_get_param (value),
                                        &param->data.d_param_def);
    }

  if (param->param_type == -1)
    g_printerr ("%s: GValue contains unsupported type '%s'\n",
                G_STRFUNC, param->type_name);
}

GPParam *
_gimp_value_array_to_gp_params (const GimpValueArray  *args,
                                gboolean               full_copy)
{
  GPParam *params;
  gint     length;
  gint     i;

  g_return_val_if_fail (args != NULL, NULL);

  length = gimp_value_array_length (args);

  params = g_new0 (GPParam, length);

  for (i = 0; i < length; i++)
    {
      GValue *value = gimp_value_array_index (args, i);

      _gimp_value_to_gp_param (value, &params[i], full_copy);
    }

  return params;
}
