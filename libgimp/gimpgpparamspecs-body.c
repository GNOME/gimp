/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpgpparamspecs-body.c
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

/*  this file is included by both libgimp/gimpgpparamspecs.c
 *  and app/plug-in/gimpgpparamspecs.c
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
