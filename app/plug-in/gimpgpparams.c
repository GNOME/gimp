/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpgpparams.c
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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpprotocol.h"

#include "plug-in-types.h"

#include "core/gimp.h"
#include "core/gimpparamspecs.h"

#include "gimpgpparams.h"


/*  public functions  */

/*  include the implementations of
 *
 *  _gimp_param_spec_to_gp_param_def()
 *  _gimp_gp_params_to_value_array()
 *  _gimp_value_array_to_gp_params()
 *
 *  from libgimp, they are identical.
 */
#include "../../libgimp/gimpgpparams-body.c"

GParamSpec *
_gimp_gp_param_def_to_param_spec (Gimp       *gimp,
                                  GPParamDef *param_def)
{
  const gchar *name  = param_def->name;
  const gchar *nick  = param_def->nick;
  const gchar *blurb = param_def->blurb;
  GParamFlags  flags = G_PARAM_READWRITE;

  switch (param_def->param_def_type)
    {
    case GP_PARAM_DEF_TYPE_DEFAULT:
      if (! strcmp (param_def->type_name, "GimpParamInt32Array"))
        return gimp_param_spec_int32_array (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GimpParamInt16Array"))
        return gimp_param_spec_int16_array (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GimpParamInt8Array"))
        return gimp_param_spec_int8_array (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GimpParamFloatArray"))
        return gimp_param_spec_float_array (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GimpParamStringArray"))
        return gimp_param_spec_string_array (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GimpParamRGBArray"))
        return gimp_param_spec_rgb_array (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GimpParamParasite"))
        return gimp_param_spec_parasite (name, nick, blurb, flags);
      break;

    case GP_PARAM_DEF_TYPE_INT:
      if (! strcmp (param_def->type_name, "GimpParamInt32"))
        return gimp_param_spec_int32 (name, nick, blurb,
                                      param_def->meta.m_int.min_val,
                                      param_def->meta.m_int.max_val,
                                      param_def->meta.m_int.default_val,
                                      flags);

      if (! strcmp (param_def->type_name, "GimpParamInt16"))
        return gimp_param_spec_int16 (name, nick, blurb,
                                      param_def->meta.m_int.min_val,
                                      param_def->meta.m_int.max_val,
                                      param_def->meta.m_int.default_val,
                                      flags);

      if (! strcmp (param_def->type_name, "GimpParamInt8"))
        return gimp_param_spec_int8 (name, nick, blurb,
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
                                  g_type_from_name (param_def->meta.m_enum.type_name),
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
      if (! strcmp (param_def->type_name, "GimpParamString"))
        return gimp_param_spec_string (name, nick, blurb,
                                       param_def->meta.m_string.allow_non_utf8,
                                       param_def->meta.m_string.null_ok,
                                       param_def->meta.m_string.non_empty,
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
      if (! strcmp (param_def->type_name, "GimpParamDisplayID"))
        return gimp_param_spec_display_id (name, nick, blurb,
                                           gimp,
                                           param_def->meta.m_id.none_ok,
                                           flags);

      if (! strcmp (param_def->type_name, "GimpParamImageID"))
        return gimp_param_spec_image_id (name, nick, blurb,
                                         gimp,
                                         param_def->meta.m_id.none_ok,
                                         flags);

      if (! strcmp (param_def->type_name, "GimpParamItemID"))
        return gimp_param_spec_item_id (name, nick, blurb,
                                        gimp,
                                        param_def->meta.m_id.none_ok,
                                        flags);

      if (! strcmp (param_def->type_name, "GimpParamDrawableID"))
        return gimp_param_spec_drawable_id (name, nick, blurb,
                                            gimp,
                                            param_def->meta.m_id.none_ok,
                                            flags);

      if (! strcmp (param_def->type_name, "GimpParamLayerID"))
        return gimp_param_spec_layer_id (name, nick, blurb,
                                         gimp,
                                         param_def->meta.m_id.none_ok,
                                         flags);

      if (! strcmp (param_def->type_name, "GimpParamChannelID"))
        return gimp_param_spec_channel_id (name, nick, blurb,
                                           gimp,
                                           param_def->meta.m_id.none_ok,
                                           flags);

      if (! strcmp (param_def->type_name, "GimpParamLayerMaskID"))
        return gimp_param_spec_layer_mask_id (name, nick, blurb,
                                              gimp,
                                              param_def->meta.m_id.none_ok,
                                              flags);

      if (! strcmp (param_def->type_name, "GimpParamSelectionID"))
        return gimp_param_spec_selection_id (name, nick, blurb,
                                             gimp,
                                             param_def->meta.m_id.none_ok,
                                             flags);

      if (! strcmp (param_def->type_name, "GimpParamVectorsID"))
        return gimp_param_spec_vectors_id (name, nick, blurb,
                                           gimp,
                                           param_def->meta.m_id.none_ok,
                                           flags);
      break;
    }

  g_printerr ("%s: GParamSpec type '%s' is not handled\n",
              G_STRFUNC, param_def->type_name);

  return NULL;
}
