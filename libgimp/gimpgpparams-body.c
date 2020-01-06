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

GParamSpec *
_gimp_gp_param_def_to_param_spec (const GPParamDef *param_def)
{
  const gchar *name  = param_def->name;
  const gchar *nick  = param_def->nick;
  const gchar *blurb = param_def->blurb;
  GParamFlags  flags = param_def->flags;

  flags &= ~G_PARAM_STATIC_STRINGS;

  switch (param_def->param_def_type)
    {
    case GP_PARAM_DEF_TYPE_DEFAULT:
      if (! strcmp (param_def->type_name, "GimpParamInt32Array"))
        return gimp_param_spec_int32_array (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GimpParamInt16Array"))
        return gimp_param_spec_int16_array (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GimpParamUInt8Array"))
        return gimp_param_spec_uint8_array (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GimpParamFloatArray"))
        return gimp_param_spec_float_array (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GimpParamStringArray"))
        return gimp_param_spec_string_array (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GimpParamRGBArray"))
        return gimp_param_spec_rgb_array (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GimpParamParasite"))
        return gimp_param_spec_parasite (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GParamParam"))
        return g_param_spec_param (name, nick, blurb,
                                   g_type_from_name (param_def->value_type_name),
                                   flags);

      if (! strcmp (param_def->type_name, "GParamObject") &&
          ! strcmp (param_def->value_type_name, "GFile"))
        return g_param_spec_object (name, nick, blurb, G_TYPE_FILE, flags);
      break;

    case GP_PARAM_DEF_TYPE_INT:
      if (! strcmp (param_def->type_name, "GParamInt"))
        return g_param_spec_int (name, nick, blurb,
                                 param_def->meta.m_int.min_val,
                                 param_def->meta.m_int.max_val,
                                 param_def->meta.m_int.default_val,
                                 flags);

      if (! strcmp (param_def->type_name, "GParamUInt"))
        return g_param_spec_uint (name, nick, blurb,
                                  param_def->meta.m_int.min_val,
                                  param_def->meta.m_int.max_val,
                                  param_def->meta.m_int.default_val,
                                  flags);

      if (! strcmp (param_def->type_name, "GParamUChar"))
        return g_param_spec_uchar (name, nick, blurb,
                                   param_def->meta.m_int.min_val,
                                   param_def->meta.m_int.max_val,
                                   param_def->meta.m_int.default_val,
                                   flags);
      break;

    case GP_PARAM_DEF_TYPE_UNIT:
      if (! strcmp (param_def->type_name, "GimpParamUnit"))
        return gimp_param_spec_unit (name, nick, blurb,
                                     param_def->meta.m_unit.allow_pixels,
                                     param_def->meta.m_unit.allow_percent,
                                     param_def->meta.m_unit.default_val,
                                     flags);
      break;

    case GP_PARAM_DEF_TYPE_ENUM:
      if (! strcmp (param_def->type_name, "GParamEnum"))
        return g_param_spec_enum (name, nick, blurb,
                                  g_type_from_name (param_def->value_type_name),
                                  param_def->meta.m_enum.default_val,
                                  flags);

      if (! strcmp (param_def->type_name, "GimpParamEnum"))
        /* FIXME GimpParamEnum */
        return g_param_spec_enum (name, nick, blurb,
                                  g_type_from_name (param_def->value_type_name),
                                  param_def->meta.m_enum.default_val,
                                  flags);
      break;

    case GP_PARAM_DEF_TYPE_BOOLEAN:
      if (! strcmp (param_def->type_name, "GParamBoolean"))
        return g_param_spec_boolean (name, nick, blurb,
                                     param_def->meta.m_boolean.default_val,
                                     flags);
      break;

    case GP_PARAM_DEF_TYPE_FLOAT:
      if (! strcmp (param_def->type_name, "GParamDouble"))
        return g_param_spec_double (name, nick, blurb,
                                    param_def->meta.m_float.min_val,
                                    param_def->meta.m_float.max_val,
                                    param_def->meta.m_float.default_val,
                                    flags);
      break;

    case GP_PARAM_DEF_TYPE_STRING:
      if (! strcmp (param_def->type_name, "GParamString"))
        return g_param_spec_string (name, nick, blurb,
                                    param_def->meta.m_string.default_val,
                                    flags);
      break;

    case GP_PARAM_DEF_TYPE_COLOR:
      if (! strcmp (param_def->type_name, "GimpParamRGB"))
        return gimp_param_spec_rgb (name, nick, blurb,
                                    param_def->meta.m_color.has_alpha,
                                    &param_def->meta.m_color.default_val,
                                    flags);

    case GP_PARAM_DEF_TYPE_ID:
      if (! strcmp (param_def->type_name, "GimpParamDisplay"))
        return gimp_param_spec_display (name, nick, blurb,
                                        param_def->meta.m_id.none_ok,
                                        flags);

      if (! strcmp (param_def->type_name, "GimpParamImage"))
        return gimp_param_spec_image (name, nick, blurb,
                                      param_def->meta.m_id.none_ok,
                                      flags);

      if (! strcmp (param_def->type_name, "GimpParamItem"))
        return gimp_param_spec_item (name, nick, blurb,
                                     param_def->meta.m_id.none_ok,
                                     flags);

      if (! strcmp (param_def->type_name, "GimpParamDrawable"))
        return gimp_param_spec_drawable (name, nick, blurb,
                                         param_def->meta.m_id.none_ok,
                                         flags);

      if (! strcmp (param_def->type_name, "GimpParamLayer"))
        return gimp_param_spec_layer (name, nick, blurb,
                                      param_def->meta.m_id.none_ok,
                                      flags);

      if (! strcmp (param_def->type_name, "GimpParamChannel"))
        return gimp_param_spec_channel (name, nick, blurb,
                                        param_def->meta.m_id.none_ok,
                                        flags);

      if (! strcmp (param_def->type_name, "GimpParamLayerMask"))
        return gimp_param_spec_layer_mask (name, nick, blurb,
                                           param_def->meta.m_id.none_ok,
                                           flags);

      if (! strcmp (param_def->type_name, "GimpParamSelection"))
        return gimp_param_spec_selection (name, nick, blurb,
                                          param_def->meta.m_id.none_ok,
                                          flags);

      if (! strcmp (param_def->type_name, "GimpParamVectors"))
        return gimp_param_spec_vectors (name, nick, blurb,
                                        param_def->meta.m_id.none_ok,
                                        flags);
      break;

    case GP_PARAM_DEF_TYPE_ID_ARRAY:
      if (! strcmp (param_def->type_name, "GimpParamObjectArray"))
        return gimp_param_spec_object_array (name, nick, blurb,
                                             g_type_from_name (param_def->meta.m_id_array.type_name),
                                             flags);
      break;
    }

  g_printerr ("%s: GParamSpec type '%s' is not handled\n",
              G_STRFUNC, param_def->type_name);

  return NULL;
}

void
_gimp_param_spec_to_gp_param_def (GParamSpec *pspec,
                                  GPParamDef *param_def)
{
  GType pspec_type = G_PARAM_SPEC_TYPE (pspec);
  GType value_type = G_PARAM_SPEC_VALUE_TYPE (pspec);

  param_def->param_def_type  = GP_PARAM_DEF_TYPE_DEFAULT;
  param_def->type_name       = (gchar *) g_type_name (pspec_type);
  param_def->value_type_name = (gchar *) g_type_name (value_type);
  param_def->name            = (gchar *) g_param_spec_get_name (pspec);
  param_def->nick            = (gchar *) g_param_spec_get_nick (pspec);
  param_def->blurb           = (gchar *) g_param_spec_get_blurb (pspec);
  param_def->flags           = pspec->flags;

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
      GParamSpecEnum *espec = G_PARAM_SPEC_ENUM (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_ENUM;

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
  else if (GIMP_IS_PARAM_SPEC_OBJECT_ARRAY (pspec))
    {
      param_def->param_def_type = GP_PARAM_DEF_TYPE_ID_ARRAY;

      param_def->meta.m_id_array.type_name =
        (gchar *) g_type_name (GIMP_PARAM_SPEC_OBJECT_ARRAY (pspec)->object_type);
    }
  else if (pspec_type == G_TYPE_PARAM_OBJECT &&
           value_type != G_TYPE_FILE)
    {
      const gchar *type_name  = NULL;

      if (g_type_is_a (value_type, GIMP_TYPE_DISPLAY))
        {
          /* g_type_is_a() because the core has a GimpDisplay subclasses */
          type_name = "GimpParamDisplay";
        }
      else if (value_type == GIMP_TYPE_IMAGE)
        {
          type_name = "GimpParamImage";
        }
      else if (value_type == GIMP_TYPE_ITEM)
        {
          type_name = "GimpParamItem";
        }
      else if (value_type == GIMP_TYPE_DRAWABLE)
        {
          type_name = "GimpParamDrawable";
        }
      else if (g_type_is_a (value_type, GIMP_TYPE_LAYER))
        {
          /* g_type_is_a() because the core has layer subclasses */
          type_name = "GimpParamLayer";
        }
      else if (value_type == GIMP_TYPE_CHANNEL)
        {
          type_name = "GimpParamChannel";
        }
      else if (value_type == GIMP_TYPE_LAYER_MASK)
        {
          type_name = "GimpParamLayerMask";
        }
      else if (value_type == GIMP_TYPE_SELECTION)
        {
          type_name = "GimpParamSelection";
        }
      else if (value_type == GIMP_TYPE_VECTORS)
        {
          type_name = "GimpParamVectors";
        }

      if (type_name)
        {
          param_def->param_def_type    = GP_PARAM_DEF_TYPE_ID;
          param_def->type_name         = (gchar *) type_name;
          param_def->meta.m_id.none_ok = TRUE;
        }
      else
        {
          g_printerr ("%s: GParamSpecObject for unsupported object type '%s'\n",
                      G_STRFUNC, param_def->type_name);
        }
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

static GimpDisplay *
get_display_by_id (gpointer gimp,
                   gint     id)
{
#ifdef LIBGIMP_COMPILATION
  return gimp_display_get_by_id (id);
#else
  return gimp_display_get_by_id (gimp, id);
#endif
}

static void
gimp_gp_param_to_value (gpointer        gimp,
                        const GPParam  *param,
                        GType           type,
                        GValue         *value)
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
      g_value_set_string (value, param->data.d_string);
    }
  else if (G_VALUE_TYPE (value) == G_TYPE_FILE)
    {
      g_value_take_object (value, (param->data.d_string ?
                                   g_file_new_for_uri (param->data.d_string) :
                                   NULL));
    }
  else if (GIMP_VALUE_HOLDS_RGB (value))
    {
      gimp_value_set_rgb (value, &param->data.d_color);
    }
  else if (GIMP_VALUE_HOLDS_PARASITE (value))
    {
      g_value_set_boxed (value, &param->data.d_parasite);
    }
  else if (GIMP_VALUE_HOLDS_INT32_ARRAY (value))
    {
      gimp_value_set_int32_array (value,
                                  (gint32 *) param->data.d_array.data,
                                  param->data.d_array.size /
                                  sizeof (gint32));
    }
  else if (GIMP_VALUE_HOLDS_INT16_ARRAY (value))
    {
      gimp_value_set_int16_array (value,
                                  (gint16 *) param->data.d_array.data,
                                  param->data.d_array.size /
                                  sizeof (gint16));
    }
  else if (GIMP_VALUE_HOLDS_UINT8_ARRAY (value))
    {
      gimp_value_set_uint8_array (value,
                                  param->data.d_array.data,
                                  param->data.d_array.size /
                                  sizeof (guint8));
    }
  else if (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value))
    {
      gimp_value_set_float_array (value,
                                  (const gdouble *)
                                  param->data.d_array.data,
                                  param->data.d_array.size /
                                  sizeof (gdouble));
    }
  else if (GIMP_VALUE_HOLDS_STRING_ARRAY (value))
    {
      gimp_value_set_string_array (value,
                                   (const gchar **)
                                   param->data.d_string_array.data,
                                   param->data.d_string_array.size);
    }
  else if (GIMP_VALUE_HOLDS_RGB_ARRAY (value))
    {
      gimp_value_set_rgb_array (value,
                                (GimpRGB *)
                                param->data.d_array.data,
                                param->data.d_array.size /
                                sizeof (GimpRGB));
    }
  else if (GIMP_VALUE_HOLDS_OBJECT_ARRAY (value))
    {
      GType     object_type;
      GObject **objects;
      gint      i;

      object_type = g_type_from_name (param->data.d_id_array.type_name);

      objects = g_new (GObject *, param->data.d_id_array.size);

      for (i = 0; i < param->data.d_id_array.size; i++)
        {
          gint id = param->data.d_id_array.data[i];

          if (object_type == GIMP_TYPE_IMAGE)
            {
              objects[i] = (GObject *) get_image_by_id (gimp, id);
            }
          else if (g_type_is_a (object_type, GIMP_TYPE_ITEM))
            {
              objects[i] = (GObject *) get_item_by_id (gimp, id);
            }
          else if (g_type_is_a (object_type, GIMP_TYPE_DISPLAY))
            {
              objects[i] = (GObject *) get_display_by_id (gimp, id);
            }

          if (objects[i])
            g_object_ref (objects[i]);
        }

      gimp_value_take_object_array (value, object_type, objects,
                                    param->data.d_id_array.size);
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
        _gimp_gp_param_def_to_param_spec (&param->data.d_param_def);

      g_value_take_param (value, pspec);
    }
}

GimpValueArray *
_gimp_gp_params_to_value_array (gpointer        gimp,
                                GParamSpec    **pspecs,
                                gint            n_pspecs,
                                const GPParam  *params,
                                gint            n_params,
                                gboolean        return_values)
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

      gimp_gp_param_to_value (gimp, &params[i], type, &value);

      gimp_value_array_append (args, &value);
      g_value_unset (&value);
    }

  return args;
}

static void
gimp_value_to_gp_param (const GValue *value,
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
  else if (G_VALUE_TYPE (value) == G_TYPE_FILE)
    {
      GFile *file = g_value_get_object (value);

      param->param_type = GP_PARAM_TYPE_FILE;

      param->data.d_string = file ? g_file_get_uri (file) : NULL;
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

      param->param_type = GP_PARAM_TYPE_ARRAY;

      if (array)
        {
          param->data.d_array.size = array->length;

          if (full_copy)
            param->data.d_array.data = g_memdup (array->data,
                                                 array->length);
          else
            param->data.d_array.data = array->data;
        }
      else
        {
          param->data.d_array.size = 0;
          param->data.d_array.data = NULL;
        }
    }
  else if (GIMP_VALUE_HOLDS_STRING_ARRAY (value))
    {
      GimpStringArray *array = g_value_get_boxed (value);

      param->param_type = GP_PARAM_TYPE_STRING_ARRAY;

      if (array)
        {
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
          param->data.d_string_array.size = 0,
          param->data.d_string_array.data = NULL;
        }
    }
  else if (GIMP_VALUE_HOLDS_OBJECT_ARRAY (value))
    {
      GimpObjectArray *array = g_value_get_boxed (value);

      param->param_type = GP_PARAM_TYPE_ID_ARRAY;

      if (array)
        {
          gint i;

          if (full_copy)
            param->data.d_id_array.type_name =
              g_strdup (g_type_name (array->object_type));
          else
            param->data.d_id_array.type_name =
              (gchar *) g_type_name (array->object_type);

          param->data.d_id_array.size = array->length;

          /* must be free'd also for full_copy == FALSE */
          param->data.d_id_array.data = g_new (gint32, array->length);

          for (i = 0; i < array->length; i++)
            {
              if (GIMP_IS_IMAGE (array->data[i]))
                {
                  param->data.d_id_array.data[i] =
                    gimp_image_get_id (GIMP_IMAGE (array->data[i]));
                }
              else if (GIMP_IS_ITEM (array->data[i]))
                {
                  param->data.d_id_array.data[i] =
                    gimp_item_get_id (GIMP_ITEM (array->data[i]));
                }
              else if (GIMP_IS_DISPLAY (array->data[i]))
                {
                  param->data.d_id_array.data[i] =
                    gimp_display_get_id (GIMP_DISPLAY (array->data[i]));
                }
              else
                {
                  param->data.d_id_array.data[i] = -1;
                }
            }
        }
      else
        {
          param->data.d_id_array.size = 0;
          param->data.d_id_array.data = NULL;
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
      GimpDisplay *display = g_value_get_object (value);

      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = display ? gimp_display_get_id (display) : -1;
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

      gimp_value_to_gp_param (value, &params[i], full_copy);
    }

  return params;
}

void
_gimp_gp_params_free (GPParam  *params,
                      gint      n_params,
                      gboolean  full_copy)
{
  gint i;

  for (i = 0; i < n_params; i++)
    {
      if (full_copy)
        g_free (params[i].type_name);

      switch (params[i].param_type)
        {
        case GP_PARAM_TYPE_INT:
        case GP_PARAM_TYPE_FLOAT:
          break;

        case GP_PARAM_TYPE_STRING:
          if (full_copy)
            g_free (params[i].data.d_string);
          break;

        case GP_PARAM_TYPE_FILE:
          /* always free the uri */
          g_free (params[i].data.d_string);
          break;

        case GP_PARAM_TYPE_COLOR:
          break;

        case GP_PARAM_TYPE_ARRAY:
          if (full_copy)
            g_free (params[i].data.d_array.data);
          break;

        case GP_PARAM_TYPE_STRING_ARRAY:
          if (full_copy                              &&
              params[i].data.d_string_array.size > 0 &&
              params[i].data.d_string_array.data)
            {
              gint j;

              for (j = 0; j < params[i].data.d_string_array.size; j++)
                g_free (params[i].data.d_string_array.data[j]);

              g_free (params[i].data.d_string_array.data);
            }
          break;

        case GP_PARAM_TYPE_ID_ARRAY:
          if (full_copy)
            g_free (params[i].data.d_id_array.type_name);

          /* always free the array */
          g_free (params[i].data.d_id_array.data);
          break;

        case GP_PARAM_TYPE_PARASITE:
          if (full_copy)
            {
              g_free (params[i].data.d_parasite.name);
              g_free (params[i].data.d_parasite.data);
            }
          break;

        case GP_PARAM_TYPE_PARAM_DEF:
          break;
        }
    }

  g_free (params);
}
