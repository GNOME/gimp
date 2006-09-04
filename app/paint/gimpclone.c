/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "paint-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimppattern.h"
#include "core/gimppickable.h"

#include "gimpclone.h"
#include "gimpcloneoptions.h"

#include "gimp-intl.h"


static void   gimp_clone_paint        (GimpPaintCore    *paint_core,
                                       GimpDrawable     *drawable,
                                       GimpPaintOptions *paint_options,
                                       GimpPaintState    paint_state,
                                       guint32           time);

static void   gimp_clone_motion       (GimpSourceCore   *source_core,
                                       GimpDrawable     *drawable,
                                       GimpPaintOptions *paint_options,
                                       gdouble           opacity,
                                       GimpImage        *src_image,
                                       GimpPickable     *src_pickable,
                                       PixelRegion      *srcPR,
                                       TempBuf          *paint_area,
                                       gint              paint_area_offset_x,
                                       gint              paint_area_offset_y);

static void   gimp_clone_line_image   (GimpImage        *dest,
                                       GimpImage        *src,
                                       GimpDrawable     *d_drawable,
                                       GimpPickable     *s_pickable,
                                       guchar           *s,
                                       guchar           *d,
                                       gint              src_bytes,
                                       gint              dest_bytes,
                                       gint              width);
static void   gimp_clone_line_pattern (GimpImage        *dest,
                                       GimpDrawable     *drawable,
                                       GimpPattern      *pattern,
                                       guchar           *d,
                                       gint              x,
                                       gint              y,
                                       gint              bytes,
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

  paint_core_class->paint   = gimp_clone_paint;

  source_core_class->motion = gimp_clone_motion;
}

static void
gimp_clone_init (GimpClone *clone)
{
}

static void
gimp_clone_paint (GimpPaintCore    *paint_core,
                  GimpDrawable     *drawable,
                  GimpPaintOptions *paint_options,
                  GimpPaintState    paint_state,
                  guint32           time)
{
  GimpCloneOptions *options = GIMP_CLONE_OPTIONS (paint_options);

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      if (options->clone_type == GIMP_PATTERN_CLONE)
        if (! gimp_context_get_pattern (GIMP_CONTEXT (options)))
          g_message (_("No patterns available for this operation."));
      break;

    default:
      break;
    }

  GIMP_PAINT_CORE_CLASS (parent_class)->paint (paint_core, drawable,
                                               paint_options, paint_state,
                                               time);
}

static void
gimp_clone_motion (GimpSourceCore   *source_core,
                   GimpDrawable     *drawable,
                   GimpPaintOptions *paint_options,
                   gdouble           opacity,
                   GimpImage        *src_image,
                   GimpPickable     *src_pickable,
                   PixelRegion      *srcPR,
                   TempBuf          *paint_area,
                   gint              paint_area_offset_x,
                   gint              paint_area_offset_y)
{
  GimpPaintCore       *paint_core     = GIMP_PAINT_CORE (source_core);
  GimpCloneOptions    *options        = GIMP_CLONE_OPTIONS (paint_options);
  GimpSourceOptions   *source_options = GIMP_SOURCE_OPTIONS (paint_options);
  GimpContext         *context        = GIMP_CONTEXT (paint_options);
  GimpImage           *image;
  gpointer             pr = NULL;
  gint                 y;
  PixelRegion          destPR;
  GimpPattern         *pattern = NULL;

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /*  configure the destination  */
  switch (options->clone_type)
    {
    case GIMP_IMAGE_CLONE:
      pixel_region_init_temp_buf (&destPR, paint_area,
                                  paint_area_offset_x, paint_area_offset_y,
                                  srcPR->w, srcPR->h);

      pr = pixel_regions_register (2, srcPR, &destPR);
      break;

    case GIMP_PATTERN_CLONE:
      pattern = gimp_context_get_pattern (context);
      if (! pattern)
        return;

      pixel_region_init_temp_buf (&destPR, paint_area,
                                  0, 0,
                                  paint_area->width, paint_area->height);

      pr = pixel_regions_register (1, &destPR);
      break;
    }

  for (; pr != NULL; pr = pixel_regions_process (pr))
    {
      guchar *s = srcPR->data;
      guchar *d = destPR.data;

      for (y = 0; y < destPR.h; y++)
        {
          switch (options->clone_type)
            {
            case GIMP_IMAGE_CLONE:
              gimp_clone_line_image (image, src_image,
                                     drawable, src_pickable,
                                     s, d,
                                     srcPR->bytes, destPR.bytes, destPR.w);
              s += srcPR->rowstride;
              break;

            case GIMP_PATTERN_CLONE:
              gimp_clone_line_pattern (image, drawable,
                                       pattern, d,
                                       paint_area->x     + paint_area_offset_x,
                                       paint_area->y + y + paint_area_offset_y,
                                       destPR.bytes, destPR.w);
              break;
            }

          d += destPR.rowstride;
        }
    }

  if (paint_options->pressure_options->opacity)
    opacity *= PRESSURE_SCALE * paint_core->cur_coords.pressure;

  gimp_brush_core_paste_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
                                MIN (opacity, GIMP_OPACITY_OPAQUE),
                                gimp_context_get_opacity (context),
                                gimp_context_get_paint_mode (context),
                                gimp_paint_options_get_brush_mode (paint_options),

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
gimp_clone_line_image (GimpImage    *dest,
                       GimpImage    *src,
                       GimpDrawable *d_drawable,
                       GimpPickable *s_pickable,
                       guchar       *s,
                       guchar       *d,
                       gint          src_bytes,
                       gint          dest_bytes,
                       gint          width)
{
  guchar rgba[MAX_CHANNELS];
  gint   alpha;

  alpha = dest_bytes - 1;

  while (width--)
    {
      gimp_image_get_color (src, gimp_pickable_get_image_type (s_pickable),
                            s, rgba);
      gimp_image_transform_color (dest, d_drawable, d, GIMP_RGB, rgba);

      d[alpha] = rgba[ALPHA_PIX];

      s += src_bytes;
      d += dest_bytes;
    }
}

static void
gimp_clone_line_pattern (GimpImage    *dest,
                         GimpDrawable *drawable,
                         GimpPattern  *pattern,
                         guchar       *d,
                         gint          x,
                         gint          y,
                         gint          bytes,
                         gint          width)
{
  guchar            *pat, *p;
  GimpImageBaseType  color_type;
  gint               alpha;
  gint               pat_bytes;
  gint               i;

  pat_bytes = pattern->mask->bytes;

  /*  Make sure x, y are positive  */
  while (x < 0)
    x += pattern->mask->width;
  while (y < 0)
    y += pattern->mask->height;

  /*  Get a pointer to the appropriate scanline of the pattern buffer  */
  pat = temp_buf_data (pattern->mask) +
    (y % pattern->mask->height) * pattern->mask->width * pat_bytes;

  color_type = (pat_bytes == 3 ||
                pat_bytes == 4) ? GIMP_RGB : GIMP_GRAY;

  alpha = bytes - 1;

  for (i = 0; i < width; i++)
    {
      p = pat + ((i + x) % pattern->mask->width) * pat_bytes;

      gimp_image_transform_color (dest, drawable, d, color_type, p);

      if (pat_bytes == 2 || pat_bytes == 4)
        d[alpha] = p[pat_bytes - 1];
      else
        d[alpha] = OPAQUE_OPACITY;

      d += bytes;
    }
}
