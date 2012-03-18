/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <stdlib.h>
#include <string.h>

#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "paint-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "gegl/gimp-gegl-utils.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpdynamics.h"
#include "core/gimpdynamicsoutput.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimppattern.h"
#include "core/gimppickable.h"

#include "gimpclone.h"
#include "gimpcloneoptions.h"

#include "gimp-intl.h"


static gboolean gimp_clone_start        (GimpPaintCore    *paint_core,
                                         GimpDrawable     *drawable,
                                         GimpPaintOptions *paint_options,
                                         const GimpCoords *coords,
                                         GError          **error);

static void     gimp_clone_motion       (GimpSourceCore   *source_core,
                                         GimpDrawable     *drawable,
                                         GimpPaintOptions *paint_options,
                                         const GimpCoords *coords,
                                         gdouble           opacity,
                                         GimpPickable     *src_pickable,
                                         PixelRegion      *srcPR,
                                         gint              src_offset_x,
                                         gint              src_offset_y,
                                         TempBuf          *paint_area,
                                         gint              paint_area_offset_x,
                                         gint              paint_area_offset_y,
                                         gint              paint_area_width,
                                         gint              paint_area_height);

static void     gimp_clone_line_pattern (GimpImage        *dest_image,
                                         const Babl       *fish,
                                         GimpPattern      *pattern,
                                         guchar           *d,
                                         gint              x,
                                         gint              y,
                                         gint              dest_bytes,
                                         gint              width);


G_DEFINE_TYPE (GimpClone, gimp_clone, GIMP_TYPE_SOURCE_CORE)

#define parent_class gimp_clone_parent_class


void
gimp_clone_register (Gimp                      *gimp,
                     GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_CLONE,
                GIMP_TYPE_CLONE_OPTIONS,
                "gimp-clone",
                _("Clone"),
                "gimp-tool-clone");
}

static void
gimp_clone_class_init (GimpCloneClass *klass)
{
  GimpPaintCoreClass  *paint_core_class  = GIMP_PAINT_CORE_CLASS (klass);
  GimpSourceCoreClass *source_core_class = GIMP_SOURCE_CORE_CLASS (klass);

  paint_core_class->start   = gimp_clone_start;

  source_core_class->motion = gimp_clone_motion;
}

static void
gimp_clone_init (GimpClone *clone)
{
}

static gboolean
gimp_clone_start (GimpPaintCore     *paint_core,
                  GimpDrawable      *drawable,
                  GimpPaintOptions  *paint_options,
                  const GimpCoords  *coords,
                  GError           **error)
{
  GimpCloneOptions *options = GIMP_CLONE_OPTIONS (paint_options);

  if (! GIMP_PAINT_CORE_CLASS (parent_class)->start (paint_core, drawable,
                                                     paint_options, coords,
                                                     error))
    {
      return FALSE;
    }

  if (options->clone_type == GIMP_PATTERN_CLONE)
    {
      if (! gimp_context_get_pattern (GIMP_CONTEXT (options)))
        {
          g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			       _("No patterns available for use with this tool."));
          return FALSE;
        }
    }

  return TRUE;
}

static void
gimp_clone_motion (GimpSourceCore   *source_core,
                   GimpDrawable     *drawable,
                   GimpPaintOptions *paint_options,
                   const GimpCoords *coords,
                   gdouble           opacity,
                   GimpPickable     *src_pickable,
                   PixelRegion      *srcPR,
                   gint              src_offset_x,
                   gint              src_offset_y,
                   TempBuf          *paint_area,
                   gint              paint_area_offset_x,
                   gint              paint_area_offset_y,
                   gint              paint_area_width,
                   gint              paint_area_height)
{
  GimpPaintCore      *paint_core     = GIMP_PAINT_CORE (source_core);
  GimpCloneOptions   *options        = GIMP_CLONE_OPTIONS (paint_options);
  GimpSourceOptions  *source_options = GIMP_SOURCE_OPTIONS (paint_options);
  GimpContext        *context        = GIMP_CONTEXT (paint_options);
  GimpImage          *image          = gimp_item_get_image (GIMP_ITEM (drawable));
  GimpDynamicsOutput *force_output;
  gdouble             fade_point;
  gdouble             force;

  switch (options->clone_type)
    {
    case GIMP_IMAGE_CLONE:
      {
        const Babl  *fish;
        PixelRegion  destPR;
        gpointer     pr;

        fish = babl_fish (gimp_pickable_get_format_with_alpha (src_pickable),
                          gimp_drawable_get_format_with_alpha (drawable));

        pixel_region_init_temp_buf (&destPR, paint_area,
                                    paint_area_offset_x, paint_area_offset_y,
                                    paint_area_width, paint_area_height);

        pr = pixel_regions_register (2, srcPR, &destPR);

        for (; pr != NULL; pr = pixel_regions_process (pr))
          {
            guchar *s = srcPR->data;
            guchar *d = destPR.data;
            gint    y;

            for (y = 0; y < destPR.h; y++)
              {
                babl_process (fish, s, d, destPR.w);

                s += srcPR->rowstride;
                d += destPR.rowstride;
              }
          }
      }
      break;

    case GIMP_PATTERN_CLONE:
      {
        GimpPattern *pattern = gimp_context_get_pattern (context);
        const Babl  *fish;
        PixelRegion  destPR;
        gpointer     pr;

        fish = babl_fish (gimp_bpp_to_babl_format (pattern->mask->bytes, TRUE),
                          gimp_drawable_get_format_with_alpha (drawable));

        pixel_region_init_temp_buf (&destPR, paint_area,
                                    0, 0,
                                    paint_area->width, paint_area->height);

        pr = pixel_regions_register (1, &destPR);

        for (; pr != NULL; pr = pixel_regions_process (pr))
          {
            guchar *d = destPR.data;
            gint    y;

            for (y = 0; y < destPR.h; y++)
              {
                gimp_clone_line_pattern (image, fish,
                                         pattern, d,
                                         paint_area->x     + src_offset_x,
                                         paint_area->y + y + src_offset_y,
                                         destPR.bytes, destPR.w);

                d += destPR.rowstride;
              }
          }
      }
      break;
    }

  force_output = gimp_dynamics_get_output (GIMP_BRUSH_CORE (paint_core)->dynamics,
                                           GIMP_DYNAMICS_OUTPUT_FORCE);

  fade_point = gimp_paint_options_get_fade (paint_options, image,
                                            paint_core->pixel_dist);

  force = gimp_dynamics_output_get_linear_value (force_output,
                                                 coords,
                                                 paint_options,
                                                 fade_point);

  gimp_brush_core_paste_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
                                coords,
                                MIN (opacity, GIMP_OPACITY_OPAQUE),
                                gimp_context_get_opacity (context),
                                gimp_context_get_paint_mode (context),
                                gimp_paint_options_get_brush_mode (paint_options),
                                force,

                                /* In fixed mode, paint incremental so the
                                 * individual brushes are properly applied
                                 * on top of each other.
                                 * Otherwise the stuff we paint is seamless
                                 * and we don't need intermediate masking.
                                 */
                                source_options->align_mode ==
                                GIMP_SOURCE_ALIGN_FIXED ?
                                GIMP_PAINT_INCREMENTAL : GIMP_PAINT_CONSTANT);
}

static void
gimp_clone_line_pattern (GimpImage     *dest_image,
                         const Babl    *fish,
                         GimpPattern   *pattern,
                         guchar        *d,
                         gint           x,
                         gint           y,
                         gint           dest_bytes,
                         gint           width)
{
  guchar *pat, *p;
  gint    pat_bytes = pattern->mask->bytes;
  gint    i;

  /*  Make sure x, y are positive  */
  while (x < 0)
    x += pattern->mask->width;
  while (y < 0)
    y += pattern->mask->height;

  /*  Get a pointer to the appropriate scanline of the pattern buffer  */
  pat = temp_buf_get_data (pattern->mask) +
    (y % pattern->mask->height) * pattern->mask->width * pat_bytes;

  for (i = 0; i < width; i++)
    {
      p = pat + ((i + x) % pattern->mask->width) * pat_bytes;

      babl_process (fish, p, d, 1);

      d += dest_bytes;
    }
}
