/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpgpparams.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimpselection.h"
#include "core/gimpparamspecs.h"

#include "vectors/gimpvectors.h"

#include "libgimp/gimpgpparams.h"


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
_gimp_gp_param_def_to_param_spec (gpointer          gimp,
                                  const GPParamDef *param_def)
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
                                  g_type_from_name (param_def->meta.m_enum.type_name),
                                  param_def->meta.m_enum.default_val,
                                  flags);

      if (! strcmp (param_def->type_name, "GimpParamEnum"))
        return gimp_param_spec_enum (name, nick, blurb,
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

    case GP_PARAM_DEF_TYPE_PARAM_DEF:
      if (! strcmp (param_def->type_name, "GParamParam"))
        return g_param_spec_param (name, nick, blurb,
                                   g_type_from_name (param_def->meta.m_param_def.type_name),
                                   flags);
      break;
    }

  g_printerr ("%s: GParamSpec type '%s' is not handled\n",
              G_STRFUNC, param_def->type_name);

  return NULL;
}
