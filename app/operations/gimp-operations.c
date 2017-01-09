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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "operations-types.h"

#include "core/gimp.h"

#include "gegl/gimp-gegl-config.h"

#include "gimp-operations.h"

#include "gimpoperationblend.h"
#include "gimpoperationborder.h"
#include "gimpoperationcagecoefcalc.h"
#include "gimpoperationcagetransform.h"
#include "gimpoperationcomposecrop.h"
#include "gimpoperationequalize.h"
#include "gimpoperationflood.h"
#include "gimpoperationgrow.h"
#include "gimpoperationhistogramsink.h"
#include "gimpoperationmaskcomponents.h"
#include "gimpoperationprofiletransform.h"
#include "gimpoperationscalarmultiply.h"
#include "gimpoperationsemiflatten.h"
#include "gimpoperationsetalpha.h"
#include "gimpoperationshapeburst.h"
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

#include "gimpbrightnesscontrastconfig.h"
#include "gimpcolorbalanceconfig.h"
#include "gimpcolorizeconfig.h"
#include "gimpcurvesconfig.h"
#include "gimphuesaturationconfig.h"
#include "gimplevelsconfig.h"

#include "gimpoperationpointlayermode.h"
#include "gimpoperationnormalmode.h"
#include "gimpoperationdissolvemode.h"
#include "gimpoperationbehindmode.h"
#include "gimpoperationmultiply.h"
#include "gimpoperationmultiplylegacy.h"
#include "gimpoperationscreenmode.h"
#include "gimpoperationoverlaymode.h"
#include "gimpoperationdifferencemode.h"
#include "gimpoperationadditionmode.h"
#include "gimpoperationsubtractmode.h"
#include "gimpoperationdarkenonlymode.h"
#include "gimpoperationlightenonlymode.h"
#include "gimpoperationhuemode.h"
#include "gimpoperationsaturationmode.h"
#include "gimpoperationcolormode.h"
#include "gimpoperationvaluemode.h"
#include "gimpoperationdividemode.h"
#include "gimpoperationdodgemode.h"
#include "gimpoperationburnmode.h"
#include "gimpoperationhardlightmode.h"
#include "gimpoperationsoftlightmode.h"
#include "gimpoperationgrainextractmode.h"
#include "gimpoperationgrainmergemode.h"
#include "gimpoperationcolorerasemode.h"
#include "gimpoperationlchhuemode.h"
#include "gimpoperationlchchromamode.h"
#include "gimpoperationlchcolormode.h"
#include "gimpoperationlchlightnessmode.h"
#include "gimpoperationerasemode.h"
#include "gimpoperationreplacemode.h"
#include "gimpoperationantierasemode.h"


void
gimp_operations_init (void)
{
  g_type_class_ref (GIMP_TYPE_OPERATION_BLEND);
  g_type_class_ref (GIMP_TYPE_OPERATION_BORDER);
  g_type_class_ref (GIMP_TYPE_OPERATION_CAGE_COEF_CALC);
  g_type_class_ref (GIMP_TYPE_OPERATION_CAGE_TRANSFORM);
  g_type_class_ref (GIMP_TYPE_OPERATION_COMPOSE_CROP);
  g_type_class_ref (GIMP_TYPE_OPERATION_EQUALIZE);
  g_type_class_ref (GIMP_TYPE_OPERATION_FLOOD);
  g_type_class_ref (GIMP_TYPE_OPERATION_GROW);
  g_type_class_ref (GIMP_TYPE_OPERATION_HISTOGRAM_SINK);
  g_type_class_ref (GIMP_TYPE_OPERATION_MASK_COMPONENTS);
  g_type_class_ref (GIMP_TYPE_OPERATION_PROFILE_TRANSFORM);
  g_type_class_ref (GIMP_TYPE_OPERATION_SCALAR_MULTIPLY);
  g_type_class_ref (GIMP_TYPE_OPERATION_SEMI_FLATTEN);
  g_type_class_ref (GIMP_TYPE_OPERATION_SET_ALPHA);
  g_type_class_ref (GIMP_TYPE_OPERATION_SHAPEBURST);
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

  g_type_class_ref (GIMP_TYPE_OPERATION_POINT_LAYER_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_NORMAL_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_DISSOLVE_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_BEHIND_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_MULTIPLY);
  g_type_class_ref (GIMP_TYPE_OPERATION_MULTIPLY_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_SCREEN_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_OVERLAY_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_DIFFERENCE_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_ADDITION_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_SUBTRACT_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_DARKEN_ONLY_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_LIGHTEN_ONLY_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_HUE_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_SATURATION_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_COLOR_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_VALUE_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_DIVIDE_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_DODGE_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_BURN_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_HARDLIGHT_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_SOFTLIGHT_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_GRAIN_EXTRACT_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_GRAIN_MERGE_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_COLOR_ERASE_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_LCH_HUE_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_LCH_CHROMA_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_LCH_COLOR_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_LCH_LIGHTNESS_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_ERASE_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_REPLACE_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_ANTI_ERASE_MODE);

  gimp_gegl_config_register ("gimp:brightness-contrast",
                             GIMP_TYPE_BRIGHTNESS_CONTRAST_CONFIG);
  gimp_gegl_config_register ("gimp:color-balance",
                             GIMP_TYPE_COLOR_BALANCE_CONFIG);
  gimp_gegl_config_register ("gimp:colorize",
                             GIMP_TYPE_COLORIZE_CONFIG);
  gimp_gegl_config_register ("gimp:curves",
                             GIMP_TYPE_CURVES_CONFIG);
  gimp_gegl_config_register ("gimp:hue-saturation",
                             GIMP_TYPE_HUE_SATURATION_CONFIG);
  gimp_gegl_config_register ("gimp:levels",
                             GIMP_TYPE_LEVELS_CONFIG);
}
