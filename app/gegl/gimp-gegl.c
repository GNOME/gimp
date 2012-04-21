/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl.c
 * Copyright (C) 2007 Øyvind Kolås <pippin@gimp.org>
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

#include <gegl.h>

#include "gimp-gegl-types.h"

#include "base/tile.h"

#include "config/gimpbaseconfig.h"

#include "core/gimp.h"

#include "gimp-gegl.h"

#include "gimpoperationborder.h"
#include "gimpoperationcagecoefcalc.h"
#include "gimpoperationcagetransform.h"
#include "gimpoperationequalize.h"
#include "gimpoperationgrow.h"
#include "gimpoperationhistogramsink.h"
#include "gimpoperationsetalpha.h"
#include "gimpoperationshapeburst.h"
#include "gimpoperationshrink.h"

#include "gimpoperationbrightnesscontrast.h"
#include "gimpoperationcolorbalance.h"
#include "gimpoperationcolorize.h"
#include "gimpoperationcurves.h"
#include "gimpoperationdesaturate.h"
#include "gimpoperationhuesaturation.h"
#include "gimpoperationlevels.h"
#include "gimpoperationposterize.h"
#include "gimpoperationthreshold.h"

#include "gimpoperationpointlayermode.h"
#include "gimpoperationnormalmode.h"
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

#include "gimp-intl.h"


static void  gimp_gegl_notify_tile_cache_size (GimpBaseConfig *config);


void
gimp_gegl_init (Gimp *gimp)
{
  GimpBaseConfig *config;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  config = GIMP_BASE_CONFIG (gimp->config);

  g_object_set (gegl_config (),
                "tile-width",  TILE_WIDTH,
                "tile-height", TILE_HEIGHT,
                "cache-size", (gint) MIN (config->tile_cache_size, G_MAXINT),
                NULL);

  /* turn down the precision of babl - permitting use of lookup tables for
   * gamma conversions, this precision is anyways high enough for both 8bit
   * and 16bit operation
   */
  g_object_set (gegl_config (),
                "babl-tolerance", 0.00015,
                NULL);

  g_signal_connect (config, "notify::tile-cache-size",
                    G_CALLBACK (gimp_gegl_notify_tile_cache_size),
                    NULL);

  babl_format_new ("name", "R' u8",
                   babl_model ("R'G'B'A"),
                   babl_type ("u8"),
                   babl_component ("R'"),
                   NULL);
  babl_format_new ("name", "G' u8",
                   babl_model ("R'G'B'A"),
                   babl_type ("u8"),
                   babl_component ("G'"),
                   NULL);
  babl_format_new ("name", "B' u8",
                   babl_model ("R'G'B'A"),
                   babl_type ("u8"),
                   babl_component ("B'"),
                   NULL);
  babl_format_new ("name", "A u8",
                   babl_model ("R'G'B'A"),
                   babl_type ("u8"),
                   babl_component ("A"),
                   NULL);

  babl_format_new ("name", "A float",
                   babl_model ("R'G'B'A"),
                   babl_type ("float"),
                   babl_component ("A"),
                   NULL);

  babl_format_new ("name", "A double",
                   babl_model ("R'G'B'A"),
                   babl_type ("double"),
                   babl_component ("A"),
                   NULL);

  g_type_class_ref (GIMP_TYPE_OPERATION_BORDER);
  g_type_class_ref (GIMP_TYPE_OPERATION_CAGE_COEF_CALC);
  g_type_class_ref (GIMP_TYPE_OPERATION_CAGE_TRANSFORM);
  g_type_class_ref (GIMP_TYPE_OPERATION_EQUALIZE);
  g_type_class_ref (GIMP_TYPE_OPERATION_GROW);
  g_type_class_ref (GIMP_TYPE_OPERATION_HISTOGRAM_SINK);
  g_type_class_ref (GIMP_TYPE_OPERATION_SET_ALPHA);
  g_type_class_ref (GIMP_TYPE_OPERATION_SHAPEBURST);
  g_type_class_ref (GIMP_TYPE_OPERATION_SHRINK);

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

static const struct
{
  const gchar *name;
  const gchar *description;
}
babl_descriptions[] =
{
  { "R'G'B' u8",  N_("RGB") },
  { "R'G'B'A u8", N_("RGB-alpha") },
  { "Y' u8",      N_("Grayscale") },
  { "Y'A u8",     N_("Grayscale-alpha") },
  { "R' u8",      N_("Red component") },
  { "G' u8",      N_("Green component") },
  { "B' u8",      N_("Blue component") },
  { "A u8",       N_("Alpha component") }
};

static GHashTable *babl_description_hash = NULL;

const gchar *
gimp_babl_get_description (const Babl *babl)
{
  const gchar *description;

  g_return_val_if_fail (babl != NULL, NULL);

  if (G_UNLIKELY (! babl_description_hash))
    {
      gint i;

      babl_description_hash = g_hash_table_new (g_str_hash,
                                                g_str_equal);

      for (i = 0; i < G_N_ELEMENTS (babl_descriptions); i++)
        g_hash_table_insert (babl_description_hash,
                             (gpointer) babl_descriptions[i].name,
                             gettext (babl_descriptions[i].description));
    }

  if (babl_format_is_palette (babl))
    {
      if (babl_format_has_alpha (babl))
        return _("Indexed-alpha");
      else
        return _("Indexed");
    }

  description = g_hash_table_lookup (babl_description_hash,
                                     babl_get_name (babl));

  if (description)
    return description;

  return g_strconcat ("ERROR: unknown Babl format ",
                      babl_get_name (babl), NULL);
}

static void
gimp_gegl_notify_tile_cache_size (GimpBaseConfig *config)
{
  g_object_set (gegl_config (),
                "cache-size", (gint) MIN (config->tile_cache_size, G_MAXINT),
                NULL);
}
