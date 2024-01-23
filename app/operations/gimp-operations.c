/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-operations.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "operations-types.h"

#include "core/gimp.h"

#include "gimp-operations.h"

#include "gimpoperationborder.h"
#include "gimpoperationbuffersourcevalidate.h"
#include "gimpoperationcagecoefcalc.h"
#include "gimpoperationcagetransform.h"
#include "gimpoperationcomposecrop.h"
#include "gimpoperationequalize.h"
#include "gimpoperationfillsource.h"
#include "gimpoperationflood.h"
#include "gimpoperationgradient.h"
#include "gimpoperationgrow.h"
#include "gimpoperationhistogramsink.h"
#include "gimpoperationmaskcomponents.h"
#include "gimpoperationoffset.h"
#include "gimpoperationprofiletransform.h"
#include "gimpoperationscalarmultiply.h"
#include "gimpoperationsemiflatten.h"
#include "gimpoperationsetalpha.h"
#include "gimpoperationshrink.h"
#include "gimpoperationthresholdalpha.h"

#include "gimpoperationbrightnesscontrast.h"
#include "gimpoperationcolorbalance.h"
#include "gimpoperationcolorize.h"
#include "gimpoperationcurves.h"
#include "gimpoperationdesaturate.h"
#include "gimpoperationhuesaturation.h"
#include "gimpoperationlevels.h"
#include "gimpoperationposterize.h"
#include "gimpoperationthreshold.h"

#include "gimp-operation-config.h"
#include "gimpbrightnesscontrastconfig.h"
#include "gimpcolorbalanceconfig.h"
#include "gimpcurvesconfig.h"
#include "gimphuesaturationconfig.h"
#include "gimplevelsconfig.h"

#include "layer-modes-legacy/gimpoperationadditionlegacy.h"
#include "layer-modes-legacy/gimpoperationburnlegacy.h"
#include "layer-modes-legacy/gimpoperationdarkenonlylegacy.h"
#include "layer-modes-legacy/gimpoperationdifferencelegacy.h"
#include "layer-modes-legacy/gimpoperationdividelegacy.h"
#include "layer-modes-legacy/gimpoperationdodgelegacy.h"
#include "layer-modes-legacy/gimpoperationgrainextractlegacy.h"
#include "layer-modes-legacy/gimpoperationgrainmergelegacy.h"
#include "layer-modes-legacy/gimpoperationhardlightlegacy.h"
#include "layer-modes-legacy/gimpoperationhslcolorlegacy.h"
#include "layer-modes-legacy/gimpoperationhsvhuelegacy.h"
#include "layer-modes-legacy/gimpoperationhsvsaturationlegacy.h"
#include "layer-modes-legacy/gimpoperationhsvvaluelegacy.h"
#include "layer-modes-legacy/gimpoperationlightenonlylegacy.h"
#include "layer-modes-legacy/gimpoperationmultiplylegacy.h"
#include "layer-modes-legacy/gimpoperationscreenlegacy.h"
#include "layer-modes-legacy/gimpoperationsoftlightlegacy.h"
#include "layer-modes-legacy/gimpoperationsubtractlegacy.h"

#include "layer-modes/gimp-layer-modes.h"
#include "layer-modes/gimpoperationantierase.h"
#include "layer-modes/gimpoperationbehind.h"
#include "layer-modes/gimpoperationdissolve.h"
#include "layer-modes/gimpoperationerase.h"
#include "layer-modes/gimpoperationmerge.h"
#include "layer-modes/gimpoperationnormal.h"
#include "layer-modes/gimpoperationpassthrough.h"
#include "layer-modes/gimpoperationreplace.h"
#include "layer-modes/gimpoperationsplit.h"


static void
set_compat_file (GType        type,
                 const gchar *basename)
{
  GFile *file  = gimp_directory_file ("tool-options", basename, NULL);
  GQuark quark = g_quark_from_static_string ("compat-file");

  g_type_set_qdata (type, quark, file);
}

static void
set_settings_folder (GType        type,
                     const gchar *basename)
{
  GFile *file  = gimp_directory_file (basename, NULL);
  GQuark quark = g_quark_from_static_string ("settings-folder");

  g_type_set_qdata (type, quark, file);
}

void
gimp_operations_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_layer_modes_init ();

  g_type_class_ref (GIMP_TYPE_OPERATION_BORDER);
  g_type_class_ref (GIMP_TYPE_OPERATION_BUFFER_SOURCE_VALIDATE);
  g_type_class_ref (GIMP_TYPE_OPERATION_CAGE_COEF_CALC);
  g_type_class_ref (GIMP_TYPE_OPERATION_CAGE_TRANSFORM);
  g_type_class_ref (GIMP_TYPE_OPERATION_COMPOSE_CROP);
  g_type_class_ref (GIMP_TYPE_OPERATION_EQUALIZE);
  g_type_class_ref (GIMP_TYPE_OPERATION_FILL_SOURCE);
  g_type_class_ref (GIMP_TYPE_OPERATION_FLOOD);
  g_type_class_ref (GIMP_TYPE_OPERATION_GRADIENT);
  g_type_class_ref (GIMP_TYPE_OPERATION_GROW);
  g_type_class_ref (GIMP_TYPE_OPERATION_HISTOGRAM_SINK);
  g_type_class_ref (GIMP_TYPE_OPERATION_MASK_COMPONENTS);
  g_type_class_ref (GIMP_TYPE_OPERATION_OFFSET);
  g_type_class_ref (GIMP_TYPE_OPERATION_PROFILE_TRANSFORM);
  g_type_class_ref (GIMP_TYPE_OPERATION_SCALAR_MULTIPLY);
  g_type_class_ref (GIMP_TYPE_OPERATION_SEMI_FLATTEN);
  g_type_class_ref (GIMP_TYPE_OPERATION_SET_ALPHA);
  g_type_class_ref (GIMP_TYPE_OPERATION_SHRINK);
  g_type_class_ref (GIMP_TYPE_OPERATION_THRESHOLD_ALPHA);

  g_type_class_ref (GIMP_TYPE_OPERATION_BRIGHTNESS_CONTRAST);
  g_type_class_ref (GIMP_TYPE_OPERATION_COLOR_BALANCE);
  g_type_class_ref (GIMP_TYPE_OPERATION_COLORIZE);
  g_type_class_ref (GIMP_TYPE_OPERATION_CURVES);
  g_type_class_ref (GIMP_TYPE_OPERATION_DESATURATE);
  g_type_class_ref (GIMP_TYPE_OPERATION_HUE_SATURATION);
  g_type_class_ref (GIMP_TYPE_OPERATION_LEVELS);
  g_type_class_ref (GIMP_TYPE_OPERATION_POSTERIZE);
  g_type_class_ref (GIMP_TYPE_OPERATION_THRESHOLD);

  g_type_class_ref (GIMP_TYPE_OPERATION_NORMAL);
  g_type_class_ref (GIMP_TYPE_OPERATION_DISSOLVE);
  g_type_class_ref (GIMP_TYPE_OPERATION_BEHIND);
  g_type_class_ref (GIMP_TYPE_OPERATION_MULTIPLY_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_SCREEN_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_DIFFERENCE_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_ADDITION_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_SUBTRACT_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_DARKEN_ONLY_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_LIGHTEN_ONLY_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_HSV_HUE_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_HSV_SATURATION_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_HSL_COLOR_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_HSV_VALUE_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_DIVIDE_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_DODGE_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_BURN_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_HARDLIGHT_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_SOFTLIGHT_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_GRAIN_EXTRACT_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_GRAIN_MERGE_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_ERASE);
  g_type_class_ref (GIMP_TYPE_OPERATION_MERGE);
  g_type_class_ref (GIMP_TYPE_OPERATION_SPLIT);
  g_type_class_ref (GIMP_TYPE_OPERATION_PASS_THROUGH);
  g_type_class_ref (GIMP_TYPE_OPERATION_REPLACE);
  g_type_class_ref (GIMP_TYPE_OPERATION_ANTI_ERASE);

  gimp_operation_config_init_start (gimp);

  gimp_operation_config_register (gimp,
                                  "gimp:brightness-contrast",
                                  GIMP_TYPE_BRIGHTNESS_CONTRAST_CONFIG);
  set_compat_file (GIMP_TYPE_BRIGHTNESS_CONTRAST_CONFIG,
                   "gimp-brightness-contrast-tool.settings");
  set_settings_folder (GIMP_TYPE_BRIGHTNESS_CONTRAST_CONFIG,
                       "brightness-contrast");

  gimp_operation_config_register (gimp,
                                  "gimp:color-balance",
                                  GIMP_TYPE_COLOR_BALANCE_CONFIG);
  set_compat_file (GIMP_TYPE_COLOR_BALANCE_CONFIG,
                   "gimp-color-balance-tool.settings");
  set_settings_folder (GIMP_TYPE_COLOR_BALANCE_CONFIG,
                       "color-balance");

  gimp_operation_config_register (gimp,
                                  "gimp:curves",
                                  GIMP_TYPE_CURVES_CONFIG);
  set_compat_file (GIMP_TYPE_CURVES_CONFIG,
                   "gimp-curves-tool.settings");
  set_settings_folder (GIMP_TYPE_CURVES_CONFIG,
                       "curves");

  gimp_operation_config_register (gimp,
                                  "gimp:hue-saturation",
                                  GIMP_TYPE_HUE_SATURATION_CONFIG);
  set_compat_file (GIMP_TYPE_HUE_SATURATION_CONFIG,
                   "gimp-hue-saturation-tool.settings");
  set_settings_folder (GIMP_TYPE_HUE_SATURATION_CONFIG,
                       "hue-saturation");

  gimp_operation_config_register (gimp,
                                  "gimp:levels",
                                  GIMP_TYPE_LEVELS_CONFIG);
  set_compat_file (GIMP_TYPE_LEVELS_CONFIG,
                   "gimp-levels-tool.settings");
  set_settings_folder (GIMP_TYPE_LEVELS_CONFIG,
                       "levels");

  gimp_operation_config_init_end (gimp);
}

void
gimp_operations_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_layer_modes_exit ();
  gimp_operation_config_exit (gimp);
}
