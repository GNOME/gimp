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
#include "layer-modes/gimpoperationnormal.h"
#include "layer-modes/gimpoperationdissolve.h"
#include "layer-modes/gimpoperationbehind.h"
#include "layer-modes/gimpoperationmultiply.h"
#include "layer-modes-legacy/gimpoperationmultiplylegacy.h"
#include "layer-modes/gimpoperationscreen.h"
#include "layer-modes-legacy/gimpoperationscreenlegacy.h"
#include "layer-modes/gimpoperationoverlay.h"
#include "layer-modes/gimpoperationdifference.h"
#include "layer-modes-legacy/gimpoperationdifferencelegacy.h"
#include "layer-modes/gimpoperationaddition.h"
#include "layer-modes-legacy/gimpoperationadditionlegacy.h"
#include "layer-modes/gimpoperationsubtract.h"
#include "layer-modes-legacy/gimpoperationsubtractlegacy.h"
#include "layer-modes/gimpoperationdarkenonly.h"
#include "layer-modes-legacy/gimpoperationdarkenonlylegacy.h"
#include "layer-modes/gimpoperationlightenonly.h"
#include "layer-modes-legacy/gimpoperationlightenonlylegacy.h"
#include "layer-modes/gimpoperationhsvhue.h"
#include "layer-modes/gimpoperationhsvsaturation.h"
#include "layer-modes/gimpoperationhsvcolor.h"
#include "layer-modes/gimpoperationhsvvalue.h"
#include "layer-modes-legacy/gimpoperationhsvhuelegacy.h"
#include "layer-modes-legacy/gimpoperationhsvsaturationlegacy.h"
#include "layer-modes-legacy/gimpoperationhsvcolorlegacy.h"
#include "layer-modes-legacy/gimpoperationhsvvaluelegacy.h"
#include "layer-modes/gimpoperationdivide.h"
#include "layer-modes-legacy/gimpoperationdividelegacy.h"
#include "layer-modes/gimpoperationdodge.h"
#include "layer-modes-legacy/gimpoperationdodgelegacy.h"
#include "layer-modes/gimpoperationburn.h"
#include "layer-modes-legacy/gimpoperationburnlegacy.h"
#include "layer-modes/gimpoperationhardlight.h"
#include "layer-modes-legacy/gimpoperationhardlightlegacy.h"
#include "layer-modes/gimpoperationsoftlight.h"
#include "layer-modes-legacy/gimpoperationsoftlightlegacy.h"
#include "layer-modes/gimpoperationgrainextract.h"
#include "layer-modes-legacy/gimpoperationgrainextractlegacy.h"
#include "layer-modes/gimpoperationgrainmerge.h"
#include "layer-modes-legacy/gimpoperationgrainmergelegacy.h"
#include "layer-modes/gimpoperationcolorerase.h"
#include "layer-modes/gimpoperationlchhue.h"
#include "layer-modes/gimpoperationlchchroma.h"
#include "layer-modes/gimpoperationlchcolor.h"
#include "layer-modes/gimpoperationlchlightness.h"
#include "layer-modes/gimpoperationerase.h"
#include "layer-modes/gimpoperationreplace.h"
#include "layer-modes/gimpoperationantierase.h"


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
  g_type_class_ref (GIMP_TYPE_OPERATION_NORMAL);
  g_type_class_ref (GIMP_TYPE_OPERATION_DISSOLVE);
  g_type_class_ref (GIMP_TYPE_OPERATION_BEHIND);
  g_type_class_ref (GIMP_TYPE_OPERATION_MULTIPLY);
  g_type_class_ref (GIMP_TYPE_OPERATION_MULTIPLY_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_SCREEN);
  g_type_class_ref (GIMP_TYPE_OPERATION_SCREEN_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_OVERLAY);
  g_type_class_ref (GIMP_TYPE_OPERATION_DIFFERENCE);
  g_type_class_ref (GIMP_TYPE_OPERATION_DIFFERENCE_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_ADDITION);
  g_type_class_ref (GIMP_TYPE_OPERATION_ADDITION_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_SUBTRACT);
  g_type_class_ref (GIMP_TYPE_OPERATION_SUBTRACT_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_DARKEN_ONLY);
  g_type_class_ref (GIMP_TYPE_OPERATION_DARKEN_ONLY_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_LIGHTEN_ONLY);
  g_type_class_ref (GIMP_TYPE_OPERATION_LIGHTEN_ONLY_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_HSV_HUE);
  g_type_class_ref (GIMP_TYPE_OPERATION_HSV_SATURATION);
  g_type_class_ref (GIMP_TYPE_OPERATION_HSV_COLOR);
  g_type_class_ref (GIMP_TYPE_OPERATION_HSV_VALUE);
  g_type_class_ref (GIMP_TYPE_OPERATION_HSV_HUE_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_HSV_SATURATION_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_HSV_COLOR_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_HSV_VALUE_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_DIVIDE);
  g_type_class_ref (GIMP_TYPE_OPERATION_DIVIDE_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_DODGE);
  g_type_class_ref (GIMP_TYPE_OPERATION_DODGE_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_BURN);
  g_type_class_ref (GIMP_TYPE_OPERATION_BURN_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_HARDLIGHT);
  g_type_class_ref (GIMP_TYPE_OPERATION_HARDLIGHT_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_SOFTLIGHT);
  g_type_class_ref (GIMP_TYPE_OPERATION_SOFTLIGHT_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_GRAIN_EXTRACT);
  g_type_class_ref (GIMP_TYPE_OPERATION_GRAIN_EXTRACT_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_GRAIN_MERGE);
  g_type_class_ref (GIMP_TYPE_OPERATION_GRAIN_MERGE_LEGACY);
  g_type_class_ref (GIMP_TYPE_OPERATION_COLOR_ERASE);
  g_type_class_ref (GIMP_TYPE_OPERATION_LCH_HUE);
  g_type_class_ref (GIMP_TYPE_OPERATION_LCH_CHROMA);
  g_type_class_ref (GIMP_TYPE_OPERATION_LCH_COLOR);
  g_type_class_ref (GIMP_TYPE_OPERATION_LCH_LIGHTNESS);
  g_type_class_ref (GIMP_TYPE_OPERATION_ERASE);
  g_type_class_ref (GIMP_TYPE_OPERATION_REPLACE);
  g_type_class_ref (GIMP_TYPE_OPERATION_ANTI_ERASE);

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
