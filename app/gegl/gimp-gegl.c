/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl.c
 * Copyright (C) 2007 Øyvind Kolås <pippin@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gegl.h>

#include "gegl-types.h"

#include "base/tile.h"

#include "gimp-gegl.h"
#include "gimpoperationcolorbalance.h"
#include "gimpoperationcolorize.h"
#include "gimpoperationcurves.h"
#include "gimpoperationdesaturate.h"
#include "gimpoperationhuesaturation.h"
#include "gimpoperationlevels.h"
#include "gimpoperationposterize.h"
#include "gimpoperationthreshold.h"
#include "gimpoperationtilesink.h"
#include "gimpoperationtilesource.h"

#include "gimpoperationdissolvemode.h"
#include "gimpoperationbehindmode.h"
#include "gimpoperationmultiplymode.h"
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
#include "gimpoperationerasemode.h"
#include "gimpoperationreplacemode.h"
#include "gimpoperationantierasemode.h"


void
gimp_gegl_init (void)
{
  g_object_set (gegl_config (),
                "tile-width",  TILE_WIDTH,
                "tile-height", TILE_HEIGHT,
                NULL);

  g_type_class_ref (GIMP_TYPE_OPERATION_TILE_SINK);
  g_type_class_ref (GIMP_TYPE_OPERATION_TILE_SOURCE);

  g_type_class_ref (GIMP_TYPE_OPERATION_COLOR_BALANCE);
  g_type_class_ref (GIMP_TYPE_OPERATION_COLORIZE);
  g_type_class_ref (GIMP_TYPE_OPERATION_CURVES);
  g_type_class_ref (GIMP_TYPE_OPERATION_DESATURATE);
  g_type_class_ref (GIMP_TYPE_OPERATION_HUE_SATURATION);
  g_type_class_ref (GIMP_TYPE_OPERATION_LEVELS);
  g_type_class_ref (GIMP_TYPE_OPERATION_POSTERIZE);
  g_type_class_ref (GIMP_TYPE_OPERATION_THRESHOLD);

  g_type_class_ref (GIMP_TYPE_OPERATION_DISSOLVE_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_BEHIND_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_MULTIPLY_MODE);
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
  g_type_class_ref (GIMP_TYPE_OPERATION_ERASE_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_REPLACE_MODE);
  g_type_class_ref (GIMP_TYPE_OPERATION_ANTI_ERASE_MODE);
}
