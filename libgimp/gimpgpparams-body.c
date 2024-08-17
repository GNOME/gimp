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

      if (! strcmp (param_def->type_name, "GimpParamFloatArray"))
        return gimp_param_spec_float_array (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GimpParamParasite"))
        return gimp_param_spec_parasite (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GParamParam"))
        return g_param_spec_param (name, nick, blurb,
                                   g_type_from_name (param_def->value_type_name),
                                   flags);

      if (! strcmp (param_def->type_name, "GParamObject") &&
          ! strcmp (param_def->value_type_name, "GFile"))
        return g_param_spec_object (name, nick, blurb, G_TYPE_FILE, flags);

      if (! strcmp (param_def->type_name, "GParamBoxed") &&
          ! strcmp (param_def->value_type_name, "GStrv"))
        return g_param_spec_boxed (name, nick, blurb, G_TYPE_STRV, flags);

      if (! strcmp (param_def->type_name, "GParamBoxed") &&
          ! strcmp (param_def->value_type_name, "GBytes"))
        return g_param_spec_boxed (name, nick, blurb, G_TYPE_BYTES, flags);

      if (! strcmp (param_def->type_name, "GParamBoxed") &&
          ! strcmp (param_def->value_type_name, "GimpColorArray"))
        return g_param_spec_boxed (name, nick, blurb, GIMP_TYPE_COLOR_ARRAY, flags);

      break;

    case GP_PARAM_DEF_TYPE_CHOICE:
      if (! strcmp (param_def->type_name, "GimpParamChoice"))
        {
          return gimp_param_spec_choice (name, nick, blurb,
                                         g_object_ref (param_def->meta.m_choice.choice),
                                         param_def->meta.m_choice.default_val,
                                         flags);
        }

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
                                     gimp_unit_get_by_id (param_def->meta.m_unit.default_val),
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

    case GP_PARAM_DEF_TYPE_GEGL_COLOR:
      if (! strcmp (param_def->type_name, "GeglParamColor") ||
          ! strcmp (param_def->type_name, "GimpParamColor"))
        {
          GeglColor *default_color = NULL;

          if (param_def->meta.m_gegl_color.default_val)
            {
              GPParamColor *default_val = param_def->meta.m_gegl_color.default_val;
              const Babl   *format      = NULL;
              const Babl   *space       = NULL;
              gint          bpp;

              default_color = gegl_color_new ("black");

              if (default_val->profile_data)
                {
                  GimpColorProfile *profile;

                  profile = gimp_color_profile_new_from_icc_profile (default_val->profile_data,
                                                                     default_val->profile_size,
                                                                     NULL);
                  if (profile)
                    {
                      GError *error = NULL;

                      space = gimp_color_profile_get_space (profile,
                                                            GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                            &error);

                      if (! space)
                        {
                          g_printerr ("%s: failed to create Babl space from profile: %s\n",
                                      G_STRFUNC, error->message);
                          g_clear_error (&error);
                        }
                      g_object_unref (profile);
                    }
                }

              format = babl_format_with_space (default_val->encoding, space);
              bpp    = babl_format_get_bytes_per_pixel (format);

              if (bpp != default_val->size)
                g_printerr ("%s: encoding \"%s\" expects %d bpp but data size is %d bpp.\n",
                            G_STRFUNC, default_val->encoding, bpp, default_val->size);
              else
                gegl_color_set_pixel (default_color, format, default_val->data);
            }

          return gimp_param_spec_color (name, nick, blurb,
                                        param_def->meta.m_gegl_color.has_alpha,
                                        default_color, flags);
        }
      break;

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

      if (! strcmp (param_def->type_name, "GimpParamTextLayer"))
        return gimp_param_spec_text_layer (name, nick, blurb,
                                           param_def->meta.m_id.none_ok,
                                           flags);

      if (! strcmp (param_def->type_name, "GimpParamGroupLayer"))
        return gimp_param_spec_group_layer (name, nick, blurb,
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

      if (! strcmp (param_def->type_name, "GimpParamPath"))
        return gimp_param_spec_path (name, nick, blurb,
                                     param_def->meta.m_id.none_ok,
                                     flags);

      if (! strcmp (param_def->type_name, "GimpParamResource"))
        return gimp_param_spec_resource (name, nick, blurb,
                                         param_def->meta.m_id.none_ok, flags);

      if (! strcmp (param_def->type_name, "GimpParamBrush"))
        return gimp_param_spec_brush (name, nick, blurb,
                                      param_def->meta.m_id.none_ok, flags);

      if (! strcmp (param_def->type_name, "GimpParamFont"))
        return gimp_param_spec_font (name, nick, blurb,
                                     param_def->meta.m_id.none_ok, flags);

      if (! strcmp (param_def->type_name, "GimpParamGradient"))
        return gimp_param_spec_gradient (name, nick, blurb,
                                         param_def->meta.m_id.none_ok, flags);

      if (! strcmp (param_def->type_name, "GimpParamPalette"))
        return gimp_param_spec_palette (name, nick, blurb,
                                        param_def->meta.m_id.none_ok, flags);

      if (! strcmp (param_def->type_name, "GimpParamPattern"))
        return gimp_param_spec_pattern (name, nick, blurb,
                                        param_def->meta.m_id.none_ok, flags);
      break;

    case GP_PARAM_DEF_TYPE_ID_ARRAY:
      if (! strcmp (param_def->type_name, "GimpParamObjectArray"))
        return gimp_param_spec_object_array (name, nick, blurb,
                                             g_type_from_name (param_def->meta.m_id_array.type_name),
                                             flags);

    case GP_PARAM_DEF_TYPE_EXPORT_OPTIONS:
      if (! strcmp (param_def->type_name, "GimpParamExportOptions"))
        {
          return gimp_param_spec_export_options (name, nick, blurb,
                                                 param_def->meta.m_export_options.capabilities,
                                                 flags);
        }
      break;
    }

  g_warning ("%s: GParamSpec type unsupported '%s'", G_STRFUNC,
             param_def->type_name);

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
      GimpParamSpecUnit *uspec = GIMP_PARAM_SPEC_UNIT (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_UNIT;

      param_def->meta.m_unit.allow_pixels  = uspec->allow_pixel;
      param_def->meta.m_unit.allow_percent = uspec->allow_percent;
      param_def->meta.m_unit.default_val   = gimp_unit_get_id (uspec->default_value);
    }
  else if (G_IS_PARAM_SPEC_ENUM (pspec))
    {
      GParamSpecEnum *espec = G_PARAM_SPEC_ENUM (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_ENUM;

      param_def->meta.m_enum.default_val = espec->default_value;
    }
  else if (pspec_type == GIMP_TYPE_PARAM_CHOICE)
    {
      GimpParamSpecChoice *cspec = GIMP_PARAM_SPEC_CHOICE (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_CHOICE;

      param_def->meta.m_choice.default_val = cspec->default_value;
      param_def->meta.m_choice.choice      = cspec->choice;
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
  else if (GEGL_IS_PARAM_SPEC_COLOR (pspec) ||
           GIMP_IS_PARAM_SPEC_COLOR (pspec) ||
           (pspec_type == G_TYPE_PARAM_OBJECT && value_type == GEGL_TYPE_COLOR))
    {
      GPParamColor *default_val = NULL;
      GeglColor    *default_color;

      param_def->param_def_type              = GP_PARAM_DEF_TYPE_GEGL_COLOR;
      param_def->meta.m_gegl_color.has_alpha = TRUE;

      if (GEGL_IS_PARAM_SPEC_COLOR (pspec))
        {
          default_color = gegl_param_spec_color_get_default (pspec);
        }
      else if (GIMP_IS_PARAM_SPEC_COLOR (pspec))
        {
          default_color = gimp_param_spec_color_get_default (pspec);
          param_def->meta.m_gegl_color.has_alpha = gimp_param_spec_color_has_alpha (pspec);
        }
      else
        {
          const GValue *value = g_param_spec_get_default_value (pspec);

          default_color = g_value_get_object (value);
          param_def->type_name = "GeglParamColor";
        }

      if (default_color != NULL)
        {
          const Babl *format;

          format      = gegl_color_get_format (default_color);
          default_val = g_new0 (GPParamColor, 1);

          default_val->size     = babl_format_get_bytes_per_pixel (format);
          default_val->encoding = (gchar *) g_strdup (babl_format_get_encoding (format));

          default_val->profile_data = NULL;
          default_val->profile_size = 0;
          if (babl_format_get_space (format) != babl_space ("sRGB"))
            {
              const char *icc;
              int         icc_length;

              icc = babl_space_get_icc (babl_format_get_space (format), &icc_length);

              if (icc_length > 0)
                {
                  default_val->profile_data = g_new0 (guint8, icc_length);
                  memcpy (default_val->profile_data, icc, icc_length);
                }
              default_val->profile_size = icc_length;
            }
        }
      param_def->meta.m_gegl_color.default_val = default_val;
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
  else if (GIMP_IS_PARAM_SPEC_RESOURCE (pspec))
    {
      GimpParamSpecResource *rspec = GIMP_PARAM_SPEC_RESOURCE (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_ID;

      param_def->meta.m_id.none_ok = rspec->none_ok;
    }
  else if (GIMP_IS_PARAM_SPEC_OBJECT_ARRAY (pspec))
    {
      param_def->param_def_type = GP_PARAM_DEF_TYPE_ID_ARRAY;

      param_def->meta.m_id_array.type_name =
        (gchar *) g_type_name (GIMP_PARAM_SPEC_OBJECT_ARRAY (pspec)->object_type);
    }
  else if (pspec_type == GIMP_TYPE_PARAM_EXPORT_OPTIONS)
    {
      GimpParamSpecExportOptions *eospec = GIMP_PARAM_SPEC_EXPORT_OPTIONS (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_EXPORT_OPTIONS;

      param_def->meta.m_export_options.capabilities = eospec->capabilities;
    }
  else if (pspec_type == G_TYPE_PARAM_OBJECT &&
           value_type != G_TYPE_FILE &&
           value_type != GEGL_TYPE_COLOR)
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
      else if (value_type == GIMP_TYPE_LAYER)
        {
          type_name = "GimpParamLayer";
        }
      else if (value_type == GIMP_TYPE_TEXT_LAYER)
        {
          type_name = "GimpParamTextLayer";
        }
      else if (value_type == GIMP_TYPE_GROUP_LAYER)
        {
          type_name = "GimpParamGroupLayer";
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
      else if (value_type == GIMP_TYPE_PATH)
        {
          type_name = "GimpParamPath";
        }
      else if (value_type == GIMP_TYPE_RESOURCE)
        {
          type_name = "GimpParamResource";
        }
      else if (value_type == GIMP_TYPE_BRUSH)
        {
          type_name = "GimpParamBrush";
        }
      else if (value_type == GIMP_TYPE_FONT)
        {
          type_name = "GimpParamFont";
        }
      else if (value_type == GIMP_TYPE_GRADIENT)
        {
          type_name = "GimpParamGradient";
        }
      else if (value_type == GIMP_TYPE_PALETTE)
        {
          type_name = "GimpParamPalette";
        }
      else if (value_type == GIMP_TYPE_PATTERN)
        {
          type_name = "GimpParamPattern";
        }

      if (type_name)
        {
          param_def->param_def_type    = GP_PARAM_DEF_TYPE_ID;
          param_def->type_name         = (gchar *) type_name;
          param_def->meta.m_id.none_ok = TRUE;
        }
      else
        {
          g_warning ("%s: GParamSpecObject for unsupported type '%s:%s'",
                     G_STRFUNC,
                     param_def->type_name, param_def->value_type_name);
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

static GObject *
get_resource_by_id (gint id)
{
#ifdef LIBGIMP_COMPILATION
  return (GObject *) gimp_resource_get_by_id (id);
#else
  return (GObject *) gimp_data_get_by_id (id);
#endif
}

static gint
get_resource_id (GObject *resource)
{
#ifdef LIBGIMP_COMPILATION
  return gimp_resource_get_id (GIMP_RESOURCE (resource));
#else
  return gimp_data_get_id (GIMP_DATA (resource));
#endif
}

static GimpUnit *
get_unit_by_id (gpointer gimp,
                gint     id)
{
  return gimp_unit_get_by_id (id);
}


/* Deserialize a gp_param (from the wire) to an instance of object or
 * primitive type.
 *
 * This is used on both the core and plugin (libgimp) side,
 * each having its own class definitions for a same named class.
 * Thus this creates different objects, depending on which side it is.
 * See the conditionally compiled constructors/fetchers above.
 */
static void
gimp_gp_param_to_value (gpointer        gimp,
                        const GPParam  *param,
                        GType           type,
                        GParamSpec     *pspec,
                        GValue         *value)
{
  g_return_if_fail (param != NULL);
  g_return_if_fail (value != NULL);
  /* pspec is nullable. */

  /* See also changing of types by caller. */
  if (type == G_TYPE_NONE || type == G_TYPE_INVALID)
    {
      type = g_type_from_name (param->type_name);
      if (type == 0)
        {
          if (! strcmp (param->type_name, "GimpResource"))
            type = g_type_from_name ("GimpData");
          else if (! strcmp (param->type_name, "GimpData"))
            type = g_type_from_name ("GimpResource");
          else
            g_critical ("%s: type name %s is not registered", G_STRFUNC,
                        param->type_name);
        }
    }
  /* assert type is not G_TYPE_NONE and type is not G_TYPE_INVALID. */

  g_value_init (value, type);

  if (type == G_TYPE_INT)
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
  else if (G_VALUE_HOLDS (value, G_TYPE_STRV))
    {
      g_value_set_boxed (value, param->data.d_strv);
    }
  else if (G_VALUE_HOLDS (value, G_TYPE_BYTES))
    {
      g_value_set_boxed (value, param->data.d_bytes);
    }
  else if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_FILE))
    {
      g_value_take_object (value, (param->data.d_string ?
                                   g_file_new_for_uri (param->data.d_string) :
                                   NULL));
    }
  else if (GIMP_VALUE_HOLDS_COLOR (value))
    {
      GeglColor        *color;
      const Babl       *format = NULL;
      const Babl       *space  = NULL;
      const gchar      *encoding;
      GimpColorProfile *profile;
      gint              bpp;

      encoding = param->data.d_gegl_color.encoding;
      profile = gimp_color_profile_new_from_icc_profile (param->data.d_gegl_color.profile_data,
                                                         param->data.d_gegl_color.profile_size,
                                                         NULL);

      if (profile)
        {
          GError *error = NULL;

          space = gimp_color_profile_get_space (profile,
                                                GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                &error);

          if (! space)
            {
              g_printerr ("%s: failed to create Babl space from profile: %s\n",
                          G_STRFUNC, error->message);
              g_clear_error (&error);
            }
          g_object_unref (profile);
        }

      format = babl_format_with_space (encoding, space);
      color  = gegl_color_new ("black");

      bpp = babl_format_get_bytes_per_pixel (format);
      if (bpp != param->data.d_gegl_color.size)
        g_printerr ("%s: encoding \"%s\" expects %d bpp but data size is %d bpp.\n",
                    G_STRFUNC, encoding, bpp, param->data.d_gegl_color.size);
      else
        gegl_color_set_pixel (color, format, param->data.d_gegl_color.data);

      g_value_take_object (value, color);
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
  else if (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value))
    {
      gimp_value_set_float_array (value,
                                  (const gdouble *)
                                  param->data.d_array.data,
                                  param->data.d_array.size /
                                  sizeof (gdouble));
    }
  else if (GIMP_VALUE_HOLDS_COLOR_ARRAY (value))
    {
      GeglColor **colors;

      colors = g_new0 (GeglColor *, param->data.d_color_array.size + 1);

      for (gint i = 0; i < param->data.d_color_array.size; i++)
        {
          GeglColor        *color;
          const Babl       *format  = NULL;
          const Babl       *space   = NULL;
          GimpColorProfile *profile = NULL;
          const gchar      *encoding;
          gint              bpp;

          encoding = param->data.d_color_array.colors[i].encoding;
          if (param->data.d_color_array.colors[i].profile_size > 0)
            profile = gimp_color_profile_new_from_icc_profile (param->data.d_color_array.colors[i].profile_data,
                                                               param->data.d_color_array.colors[i].profile_size,
                                                               NULL);

          if (profile)
            {
              GError *error = NULL;

              space = gimp_color_profile_get_space (profile,
                                                    GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                    &error);

              if (! space)
                {
                  g_printerr ("%s: failed to create Babl space from profile: %s\n",
                              G_STRFUNC, error->message);
                  g_clear_error (&error);
                }
              g_object_unref (profile);
            }
          format = babl_format_with_space (encoding, space);
          color  = gegl_color_new ("black");

          bpp = babl_format_get_bytes_per_pixel (format);
          if (bpp != param->data.d_color_array.colors[i].size)
            g_printerr ("%s: encoding \"%s\" expects %d bpp but data size is %d bpp.\n",
                        G_STRFUNC, encoding, bpp, param->data.d_color_array.colors[i].size);
          else
            gegl_color_set_pixel (color, format, param->data.d_color_array.colors[i].data);

          colors[i] = color;
        }

      g_value_take_boxed (value, colors);
    }
  else if (GIMP_VALUE_HOLDS_OBJECT_ARRAY (value))
    {
      GType     object_type = G_TYPE_INVALID;
      GObject **objects;
      gint      i;

      /* Get type of the array elements. */
      if (param->data.d_id_array.type_name != NULL)
        {
          /* An empty array from a NULL has an arbitrary type_name set earlier. */
          object_type = g_type_from_name (param->data.d_id_array.type_name);
        }
      else if (pspec != NULL)
        {
          object_type = GIMP_PARAM_SPEC_OBJECT_ARRAY (pspec)->object_type;
        }
      /* Else object-type is G_TYPE_INVALID*/

      /* GimpObjectArray requires declared element type derived from GObject.
       * Even when empty.
       * When not, return without setting the gvalue.
       * Not necessarily an error, when the plugin does not use the gvalue.
       */
      if (!g_type_is_a (object_type, G_TYPE_OBJECT))
        {
          g_warning ("%s returning NULL for GimpObjectArray", G_STRFUNC);
          return;
        }

      /* size might be zero. */

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
          else if (g_type_is_a (object_type, GIMP_TYPE_RESOURCE))
            {
              objects[i] = (GObject *) get_resource_by_id (id);
            }

          if (objects[i])
            g_object_ref (objects[i]);
        }

      /* Even when size is zero, set gvalue to an empty GimpObjectArray object,
       * having a valid but possibly arbitrary type of its elements.
       */
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
  else if (GIMP_VALUE_HOLDS_RESOURCE (value))
    {
      g_value_set_object (value, get_resource_by_id (param->data.d_int));
    }
  else if (GIMP_VALUE_HOLDS_UNIT (value))
    {
      g_value_set_object (value, get_unit_by_id (gimp, param->data.d_int));
    }
  else if (g_type_is_a (G_VALUE_TYPE (value), GIMP_TYPE_EXPORT_OPTIONS))
    {
      GimpExportOptions *options;

      options = g_object_new (GIMP_TYPE_EXPORT_OPTIONS,
                              "capabilities", param->data.d_export_options.capabilities,
                              NULL);

      g_value_set_object (value, options);

      g_object_unref (options);
    }
  else if (G_VALUE_HOLDS_PARAM (value))
    {
      GParamSpec *pspec =
        _gimp_gp_param_def_to_param_spec (&param->data.d_param_def);

      g_value_take_param (value, pspec);
    }
  else
    {
      g_warning ("%s: unsupported deserialization to GValue of type '%s'\n",
                 G_STRFUNC, param->type_name);
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
      GParamSpec *pspec = NULL;
      GValue      value = G_VALUE_INIT;
      GType       type;

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

              pspec = pspecs[pspec_index];

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

      gimp_gp_param_to_value (gimp, &params[i], type, pspec, &value);

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

  if (type == G_TYPE_INT)
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
  else if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_FILE))
    {
      GFile *file = g_value_get_object (value);

      param->param_type = GP_PARAM_TYPE_FILE;

      param->data.d_string = file ? g_file_get_uri (file) : NULL;
    }
  else if (GIMP_VALUE_HOLDS_COLOR (value))
    {
      GeglColor  *color;
      const Babl *format;
      int         icc_length = 0;

      param->param_type = GP_PARAM_TYPE_GEGL_COLOR;

      color  = g_value_get_object (value);
      format = gegl_color_get_format (color);

      /* TODO: For indexed colors, we'll convert to R'G'B'(A) u8 as that
       * is currently what we support. As indexed format support improves,
       * we'll want to make this space and encoding agnostic. */
      if (babl_format_is_palette (format))
        {
          const Babl *indexed_format = NULL;
          gint        bpp;

          bpp = babl_format_get_bytes_per_pixel (format);
          if (bpp == 1)
            indexed_format = babl_format_with_space ("R'G'B' u8",
                                                     babl_format_get_space (format));
          else if (bpp == 2)
            indexed_format = babl_format_with_space ("R'G'B'A u8",
                                                     babl_format_get_space (format));

          /* TODO: This is to notify us in the future when indexed image can have more than
           * 256 colors - we'll need to update encoding support accordingly */
          g_return_if_fail (indexed_format != NULL);

          format = indexed_format;
        }

      param->data.d_gegl_color.size = babl_format_get_bytes_per_pixel (format);
      gegl_color_get_pixel (color, format, &param->data.d_gegl_color.data);

      param->data.d_gegl_color.encoding     = (gchar *) babl_format_get_encoding (format);
      param->data.d_gegl_color.profile_data = (guint8 *) babl_space_get_icc (babl_format_get_space (format),
                                                                             &icc_length);
      param->data.d_gegl_color.profile_size = icc_length;
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
           GIMP_VALUE_HOLDS_FLOAT_ARRAY (value))
    {
      GimpArray *array = g_value_get_boxed (value);

      param->param_type = GP_PARAM_TYPE_ARRAY;

      if (array)
        {
          param->data.d_array.size = array->length;

          if (full_copy)
            param->data.d_array.data = g_memdup2 (array->data,
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
  else if (GIMP_VALUE_HOLDS_COLOR_ARRAY (value))
    {
      GeglColor **colors = g_value_get_boxed (value);

      param->param_type = GP_PARAM_TYPE_COLOR_ARRAY;

      if (colors != NULL)
        {
          param->data.d_color_array.size = gimp_color_array_get_length (colors);
          param->data.d_color_array.colors = g_new0 (GPParamColor,
                                                     param->data.d_color_array.size);
          for (gint i = 0; i < param->data.d_color_array.size; i++)
            {
              const Babl *format;
              int         icc_length = 0;

              format = gegl_color_get_format (colors[i]);

              param->data.d_color_array.colors[i].size = babl_format_get_bytes_per_pixel (format);
              gegl_color_get_pixel (colors[i], format, &param->data.d_color_array.colors[i].data);

              param->data.d_color_array.colors[i].encoding = (gchar *) babl_format_get_encoding (format);

              if (babl_format_get_space (format) != babl_space ("sRGB"))
                param->data.d_color_array.colors[i].profile_data = (guint8 *) babl_space_get_icc (babl_format_get_space (format),
                                                                                                  &icc_length);
              param->data.d_gegl_color.profile_size = icc_length;
            }
        }
      else
        {
          param->data.d_color_array.size = 0;
          param->data.d_color_array.colors = NULL;
        }
    }
  else if (G_VALUE_HOLDS (value, G_TYPE_BYTES))
    {
      GBytes *bytes = g_value_get_boxed (value);

      param->param_type = GP_PARAM_TYPE_BYTES;

      if (bytes != NULL)
        {
          if (full_copy)
            param->data.d_bytes = g_bytes_new (g_bytes_get_data (bytes, NULL),
                                               g_bytes_get_size (bytes));
          else
            param->data.d_bytes = g_bytes_new_static (g_bytes_get_data (bytes, NULL),
                                                      g_bytes_get_size (bytes));
        }
    }
  else if (G_VALUE_HOLDS (value, G_TYPE_STRV))
    {
      gchar **array = g_value_get_boxed (value);

      param->param_type = GP_PARAM_TYPE_STRV;

      if (full_copy)
        param->data.d_strv = g_strdupv (array);
      else
        param->data.d_strv = array;
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
              else if (GIMP_IS_RESOURCE (array->data[i]))
                {
                  param->data.d_id_array.data[i] = get_resource_id (array->data[i]);
                }
              else
                {
                  param->data.d_id_array.data[i] = -1;
                }
            }
        }
      else
        {
          /* GValue intended to hold GimpObjectArray is NULL.
           * For convenience, we allow this, meaning empty.
           */
          param->data.d_id_array.size = 0;
          param->data.d_id_array.data = NULL;
          /* Arbitrarily say elements are type Drawable.
           * We must have a type to create an empty GimpObjectArray.
           */
          if (full_copy)
            param->data.d_id_array.type_name = g_strdup ("GimpDrawable");
          else
            param->data.d_id_array.type_name = "GimpDrawable";
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
  else if (GIMP_VALUE_HOLDS_RESOURCE (value))
    {
      GObject *resource = g_value_get_object (value);

      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = resource ? get_resource_id (resource) : -1;
    }
  else if (GIMP_VALUE_HOLDS_UNIT (value))
    {
      GimpUnit *unit = g_value_get_object (value);

      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = unit ? gimp_unit_get_id (unit) : -1;
    }
  else if (g_type_is_a (G_VALUE_TYPE (value), GIMP_TYPE_EXPORT_OPTIONS))
    {
      GimpExportOptions *options      = g_value_get_object (value);
      gint               capabilities = 0;

      param->param_type = GP_PARAM_TYPE_INT;

      if (options)
        g_object_get (options,
                      "capabilities", &capabilities,
                      NULL);

      param->data.d_export_options.capabilities = capabilities;
    }
  else if (G_VALUE_HOLDS_PARAM (value))
    {
      param->param_type = GP_PARAM_TYPE_PARAM_DEF;

      _gimp_param_spec_to_gp_param_def (g_value_get_param (value),
                                        &param->data.d_param_def);
    }

  if (param->param_type == -1)
    g_warning ("%s: GValue holds unsupported type '%s'", G_STRFUNC, param->type_name);
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

        case GP_PARAM_TYPE_GEGL_COLOR:
          break;

        case GP_PARAM_TYPE_COLOR_ARRAY:
          g_free (params[i].data.d_color_array.colors);
          break;

        case GP_PARAM_TYPE_ARRAY:
          if (full_copy)
            g_free (params[i].data.d_array.data);
          break;

        case GP_PARAM_TYPE_STRV:
          if (full_copy)
            g_strfreev (params[i].data.d_strv);
          break;

        case GP_PARAM_TYPE_BYTES:
          g_bytes_unref (params[i].data.d_bytes);
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

        case GP_PARAM_TYPE_EXPORT_OPTIONS:
          break;

        case GP_PARAM_TYPE_PARAM_DEF:
          break;
        }
    }

  g_free (params);
}
