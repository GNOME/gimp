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

#include "paint-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimppattern.h"

#include "gimpclone.h"
#include "gimpcloneoptions.h"

#include "gimp-intl.h"


static void   gimp_clone_class_init       (GimpCloneClass   *klass);
static void   gimp_clone_init             (GimpClone        *clone);

static void   gimp_clone_paint            (GimpPaintCore    *paint_core,
                                           GimpDrawable     *drawable,
                                           GimpPaintOptions *paint_options,
                                           GimpPaintState    paint_state,
                                           guint32           time);
static void   gimp_clone_motion           (GimpPaintCore    *paint_core,
                                           GimpDrawable     *drawable,
                                           GimpPaintOptions *paint_options);

static void   gimp_clone_line_image       (GimpImage        *dest,
                                           GimpImage        *src,
                                           GimpDrawable     *d_drawable,
                                           GimpDrawable     *s_drawable,
                                           guchar           *s,
                                           guchar           *d,
                                           gint              has_alpha,
                                           gint              src_bytes,
                                           gint              dest_bytes,
                                           gint              width);
static void   gimp_clone_line_pattern     (GimpImage        *dest,
                                           GimpDrawable     *drawable,
                                           GimpPattern      *pattern,
                                           guchar           *d,
                                           gint              x,
                                           gint              y,
                                           gint              bytes,
                                           gint              width);

static void   gimp_clone_set_src_drawable (GimpClone        *clone,
                                           GimpDrawable     *drawable);


static GimpBrushCoreClass *parent_class = NULL;


void
gimp_clone_register (Gimp                      *gimp,
                     GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_CLONE,
                GIMP_TYPE_CLONE_OPTIONS,
                _("Clone"));
}

GType
gimp_clone_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpCloneClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_clone_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpClone),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_clone_init,
      };

      type = g_type_register_static (GIMP_TYPE_BRUSH_CORE,
                                     "GimpClone",
                                     &info, 0);
    }

  return type;
}

static void
gimp_clone_class_init (GimpCloneClass *klass)
{
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);
  GimpBrushCoreClass *brush_core_class = GIMP_BRUSH_CORE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  paint_core_class->paint                  = gimp_clone_paint;

  brush_core_class->handles_changing_brush = TRUE;
}

static void
gimp_clone_init (GimpClone *clone)
{
  clone->set_source   = FALSE;

  clone->src_drawable = NULL;
  clone->src_x        = 0.0;
  clone->src_y        = 0.0;

  clone->orig_src_x   = 0;
  clone->orig_src_y   = 0;

  clone->offset_x     = 0;
  clone->offset_y     = 0;
  clone->first_stroke = TRUE;
}

static void
gimp_clone_paint (GimpPaintCore    *paint_core,
                  GimpDrawable     *drawable,
                  GimpPaintOptions *paint_options,
                  GimpPaintState    paint_state,
                  guint32           time)
{
  GimpClone        *clone   = GIMP_CLONE (paint_core);
  GimpCloneOptions *options = GIMP_CLONE_OPTIONS (paint_options);
  GimpContext      *context = GIMP_CONTEXT (paint_options);

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      if (clone->set_source)
        {
          gimp_clone_set_src_drawable (clone, drawable);

          clone->src_x = paint_core->cur_coords.x;
          clone->src_y = paint_core->cur_coords.y;

          clone->first_stroke = TRUE;
        }
      else if (options->align_mode == GIMP_CLONE_ALIGN_NO)
        {
          clone->orig_src_x = clone->src_x;
          clone->orig_src_y = clone->src_y;

          clone->first_stroke = TRUE;
        }

      if (options->clone_type == GIMP_PATTERN_CLONE)
        if (! gimp_context_get_pattern (context))
          g_message (_("No patterns available for this operation."));
      break;

    case GIMP_PAINT_STATE_MOTION:
      if (clone->set_source)
        {
          /*  If the control key is down, move the src target and return */

          clone->src_x = paint_core->cur_coords.x;
          clone->src_y = paint_core->cur_coords.y;

          clone->first_stroke = TRUE;
        }
      else
        {
          /*  otherwise, update the target  */

          gint dest_x;
          gint dest_y;

          dest_x = paint_core->cur_coords.x;
          dest_y = paint_core->cur_coords.y;

          if (options->align_mode == GIMP_CLONE_ALIGN_REGISTERED)
            {
              clone->offset_x = 0;
              clone->offset_y = 0;
            }
          else if (clone->first_stroke)
            {
              clone->offset_x = clone->src_x - dest_x;
              clone->offset_y = clone->src_y - dest_y;

              clone->first_stroke = FALSE;
            }

          clone->src_x = dest_x + clone->offset_x;
          clone->src_y = dest_y + clone->offset_y;

          gimp_clone_motion (paint_core, drawable, paint_options);
        }
      break;

    case GIMP_PAINT_STATE_FINISH:
      if (options->align_mode == GIMP_CLONE_ALIGN_NO && ! clone->first_stroke)
        {
          clone->src_x = clone->orig_src_x;
          clone->src_y = clone->orig_src_y;
        }
      break;

    default:
      break;
    }
}

static void
gimp_clone_motion (GimpPaintCore    *paint_core,
                   GimpDrawable     *drawable,
                   GimpPaintOptions *paint_options)
{
  GimpClone           *clone            = GIMP_CLONE (paint_core);
  GimpCloneOptions    *options          = GIMP_CLONE_OPTIONS (paint_options);
  GimpContext         *context          = GIMP_CONTEXT (paint_options);
  GimpPressureOptions *pressure_options = paint_options->pressure_options;
  GimpImage           *gimage;
  GimpImage           *src_gimage = NULL;
  guchar              *s;
  guchar              *d;
  TempBuf             *area;
  gpointer             pr = NULL;
  gint                 y;
  gint                 x1, y1, x2, y2;
  gint                 has_alpha = -1;
  PixelRegion          srcPR, destPR;
  GimpPattern         *pattern = NULL;
  gdouble              opacity;
  gint                 offset_x;
  gint                 offset_y;

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  opacity = gimp_paint_options_get_fade (paint_options, gimage,
                                         paint_core->pixel_dist);
  if (opacity == 0.0)
    return;

  /*  make local copies because we change them  */
  offset_x = clone->offset_x;
  offset_y = clone->offset_y;

  /*  Make sure we still have a source if we are doing image cloning */
  if (options->clone_type == GIMP_IMAGE_CLONE)
    {
      if (! clone->src_drawable)
        return;

      if (! (src_gimage = gimp_item_get_image (GIMP_ITEM (clone->src_drawable))))
        return;

      /*  Determine whether the source image has an alpha channel  */
      has_alpha = gimp_drawable_has_alpha (clone->src_drawable);
    }

  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options);
  if (! area)
    return;

  switch (options->clone_type)
    {
    case GIMP_IMAGE_CLONE:
      /*  Set the paint area to transparent  */
      temp_buf_data_clear (area);

      x1 = CLAMP (area->x + offset_x,
                  0, gimp_item_width  (GIMP_ITEM (clone->src_drawable)));
      y1 = CLAMP (area->y + offset_y,
                  0, gimp_item_height (GIMP_ITEM (clone->src_drawable)));
      x2 = CLAMP (area->x + offset_x + area->width,
                  0, gimp_item_width  (GIMP_ITEM (clone->src_drawable)));
      y2 = CLAMP (area->y + offset_y + area->height,
                  0, gimp_item_height (GIMP_ITEM (clone->src_drawable)));

      if (!(x2 - x1) || !(y2 - y1))
        return;

      /*  If the source gimage is different from the destination,
       *  then we should copy straight from the destination image
       *  to the canvas.
       *  Otherwise, we need a call to get_orig_image to make sure
       *  we get a copy of the unblemished (offset) image
       */
      if (clone->src_drawable != drawable)
        {
          pixel_region_init (&srcPR, gimp_drawable_data (clone->src_drawable),
                             x1, y1, (x2 - x1), (y2 - y1), FALSE);
        }
      else
        {
          TempBuf *orig;

          /*  get the original image  */
          orig = gimp_paint_core_get_orig_image (paint_core, clone->src_drawable,
                                                 x1, y1, x2, y2);

          srcPR.bytes     = orig->bytes;
          srcPR.x         = 0;
          srcPR.y         = 0;
          srcPR.w         = x2 - x1;
          srcPR.h         = y2 - y1;
          srcPR.rowstride = srcPR.bytes * orig->width;
          srcPR.data      = temp_buf_data (orig);
        }

      offset_x = x1 - (area->x + offset_x);
      offset_y = y1 - (area->y + offset_y);

      /*  configure the destination  */
      destPR.bytes     = area->bytes;
      destPR.x         = 0;
      destPR.y         = 0;
      destPR.w         = srcPR.w;
      destPR.h         = srcPR.h;
      destPR.rowstride = destPR.bytes * area->width;
      destPR.data      = (temp_buf_data (area) +
                          offset_y * destPR.rowstride +
                          offset_x * destPR.bytes);

      pr = pixel_regions_register (2, &srcPR, &destPR);
      break;

    case GIMP_PATTERN_CLONE:
      pattern = gimp_context_get_pattern (context);

      if (!pattern)
        return;

      /*  configure the destination  */
      destPR.bytes     = area->bytes;
      destPR.x         = 0;
      destPR.y         = 0;
      destPR.w         = area->width;
      destPR.h         = area->height;
      destPR.rowstride = destPR.bytes * area->width;
      destPR.data      = temp_buf_data (area);

      pr = pixel_regions_register (1, &destPR);
      break;
    }

  for (; pr != NULL; pr = pixel_regions_process (pr))
    {
      s = srcPR.data;
      d = destPR.data;

      for (y = 0; y < destPR.h; y++)
        {
          switch (options->clone_type)
            {
            case GIMP_IMAGE_CLONE:
              gimp_clone_line_image (gimage, src_gimage,
                                     drawable, clone->src_drawable,
                                     s, d, has_alpha,
                                     srcPR.bytes, destPR.bytes, destPR.w);
              s += srcPR.rowstride;
              break;

            case GIMP_PATTERN_CLONE:
              gimp_clone_line_pattern (gimage, drawable,
                                       pattern, d,
                                       area->x + offset_x,
                                       area->y + y + offset_y,
                                       destPR.bytes, destPR.w);
              break;
            }

          d += destPR.rowstride;
        }
    }

  if (pressure_options->opacity)
    opacity *= PRESSURE_SCALE * paint_core->cur_coords.pressure;

  gimp_brush_core_paste_canvas (GIMP_BRUSH_CORE (paint_core), drawable,
                                MIN (opacity, GIMP_OPACITY_OPAQUE),
                                gimp_context_get_opacity (context),
                                gimp_context_get_paint_mode (context),
                                gimp_paint_options_get_brush_mode (paint_options),
                                GIMP_PAINT_CONSTANT);
}

static void
gimp_clone_line_image (GimpImage    *dest,
                       GimpImage    *src,
                       GimpDrawable *d_drawable,
                       GimpDrawable *s_drawable,
                       guchar       *s,
                       guchar       *d,
                       gint          has_alpha,
                       gint          src_bytes,
                       gint          dest_bytes,
                       gint          width)
{
  guchar rgba[MAX_CHANNELS];
  gint   alpha;

  alpha = dest_bytes - 1;

  while (width--)
    {
      gimp_image_get_color (src, gimp_drawable_type (s_drawable), s, rgba);
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

static void
gimp_clone_src_drawable_disconnect_cb (GimpDrawable *drawable,
                                       GimpClone    *clone)
{
  if (drawable == clone->src_drawable)
    {
      clone->src_drawable = NULL;
    }
}

static void
gimp_clone_set_src_drawable (GimpClone    *clone,
                             GimpDrawable *drawable)
{
  if (clone->src_drawable == drawable)
    return;

  if (clone->src_drawable)
    g_signal_handlers_disconnect_by_func (clone->src_drawable,
                                          gimp_clone_src_drawable_disconnect_cb,
                                          clone);

  clone->src_drawable = drawable;

  if (clone->src_drawable)
    {
      g_signal_connect (clone->src_drawable, "disconnect",
                        G_CALLBACK (gimp_clone_src_drawable_disconnect_cb),
                        clone);
    }
}
