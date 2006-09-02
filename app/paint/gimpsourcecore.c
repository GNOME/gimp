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


enum
{
  PROP_0,
  PROP_SRC_DRAWABLE,
  PROP_SRC_X,
  PROP_SRC_Y
};


static void   gimp_clone_set_property     (GObject          *object,
                                           guint             property_id,
                                           const GValue     *value,
                                           GParamSpec       *pspec);
static void   gimp_clone_get_property     (GObject          *object,
                                           guint             property_id,
                                           GValue           *value,
                                           GParamSpec       *pspec);

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
                                           GimpPickable     *s_pickable,
                                           guchar           *s,
                                           guchar           *d,
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


G_DEFINE_TYPE (GimpClone, gimp_clone, GIMP_TYPE_BRUSH_CORE)


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
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  GimpPaintCoreClass *paint_core_class = GIMP_PAINT_CORE_CLASS (klass);
  GimpBrushCoreClass *brush_core_class = GIMP_BRUSH_CORE_CLASS (klass);

  object_class->set_property               = gimp_clone_set_property;
  object_class->get_property               = gimp_clone_get_property;

  paint_core_class->paint                  = gimp_clone_paint;

  brush_core_class->handles_changing_brush = TRUE;

  g_object_class_install_property (object_class, PROP_SRC_DRAWABLE,
                                   g_param_spec_object ("src-drawable",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DRAWABLE,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SRC_X,
                                   g_param_spec_double ("src-x", NULL, NULL,
                                                        0, GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SRC_Y,
                                   g_param_spec_double ("src-y", NULL, NULL,
                                                        0, GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_clone_init (GimpClone *clone)
{
  clone->set_source   = FALSE;

  clone->src_drawable = NULL;
  clone->src_x        = 0.0;
  clone->src_y        = 0.0;

  clone->orig_src_x   = 0.0;
  clone->orig_src_y   = 0.0;

  clone->offset_x     = 0.0;
  clone->offset_y     = 0.0;
  clone->first_stroke = TRUE;
}

static void
gimp_clone_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  GimpClone *clone = GIMP_CLONE (object);

  switch (property_id)
    {
    case PROP_SRC_DRAWABLE:
      gimp_clone_set_src_drawable (clone, g_value_get_object (value));
      break;
    case PROP_SRC_X:
      clone->src_x = g_value_get_double (value);
      break;
    case PROP_SRC_Y:
      clone->src_y = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_clone_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  GimpClone *clone = GIMP_CLONE (object);

  switch (property_id)
    {
    case PROP_SRC_DRAWABLE:
      g_value_set_object (value, clone->src_drawable);
      break;
    case PROP_SRC_X:
      g_value_set_int (value, clone->src_x);
      break;
    case PROP_SRC_Y:
      g_value_set_int (value, clone->src_y);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
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
          else if (options->align_mode == GIMP_CLONE_ALIGN_FIXED)
            {
              clone->offset_x = clone->src_x - dest_x;
              clone->offset_y = clone->src_y - dest_y;
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

  g_object_notify (G_OBJECT (clone), "src-x");
  g_object_notify (G_OBJECT (clone), "src-y");
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
  GimpImage           *image;
  GimpImage           *src_image       = NULL;
  GimpPickable        *src_pickable     = NULL;
  guchar              *s;
  guchar              *d;
  TempBuf             *area;
  gpointer             pr = NULL;
  gint                 y;
  gint                 x1, y1, x2, y2;
  TileManager         *src_tiles;
  PixelRegion          srcPR, destPR;
  GimpPattern         *pattern = NULL;
  gdouble              opacity;
  gint                 offset_x;
  gint                 offset_y;

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  opacity = gimp_paint_options_get_fade (paint_options, image,
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

      src_pickable = GIMP_PICKABLE (clone->src_drawable);
      src_image    = gimp_pickable_get_image (src_pickable);

      if (options->sample_merged)
        {
          gint off_x, off_y;

          src_pickable = GIMP_PICKABLE (src_image->projection);

          gimp_item_offsets (GIMP_ITEM (clone->src_drawable), &off_x, &off_y);

          offset_x += off_x;
          offset_y += off_y;
        }

      gimp_pickable_flush (src_pickable);
    }

  area = gimp_paint_core_get_paint_area (paint_core, drawable, paint_options);
  if (! area)
    return;

  switch (options->clone_type)
    {
    case GIMP_IMAGE_CLONE:
      /*  Set the paint area to transparent  */
      temp_buf_data_clear (area);

      src_tiles = gimp_pickable_get_tiles (src_pickable);

      x1 = CLAMP (area->x + offset_x,
                  0, tile_manager_width  (src_tiles));
      y1 = CLAMP (area->y + offset_y,
                  0, tile_manager_height (src_tiles));
      x2 = CLAMP (area->x + offset_x + area->width,
                  0, tile_manager_width  (src_tiles));
      y2 = CLAMP (area->y + offset_y + area->height,
                  0, tile_manager_height (src_tiles));

      if (!(x2 - x1) || !(y2 - y1))
        return;

      /*  If the source image is different from the destination,
       *  then we should copy straight from the destination image
       *  to the canvas.
       *  Otherwise, we need a call to get_orig_image to make sure
       *  we get a copy of the unblemished (offset) image
       */
      if ((  options->sample_merged && (src_image           != image)) ||
          (! options->sample_merged && (clone->src_drawable != drawable)))
        {
          pixel_region_init (&srcPR, src_tiles,
                             x1, y1, x2 - x1, y2 - y1, FALSE);
        }
      else
        {
          TempBuf *orig;

          /*  get the original image  */
          if (options->sample_merged)
            orig = gimp_paint_core_get_orig_proj (paint_core,
                                                  src_pickable,
                                                  x1, y1, x2, y2);
          else
            orig = gimp_paint_core_get_orig_image (paint_core,
                                                   GIMP_DRAWABLE (src_pickable),
                                                   x1, y1, x2, y2);

          pixel_region_init_temp_buf (&srcPR, orig,
                                      0, 0, x2 - x1, y2 - y1);
        }

      offset_x = x1 - (area->x + offset_x);
      offset_y = y1 - (area->y + offset_y);

      /*  configure the destination  */
      pixel_region_init_temp_buf (&destPR, area,
                                  offset_x, offset_y, srcPR.w, srcPR.h);

      pr = pixel_regions_register (2, &srcPR, &destPR);
      break;

    case GIMP_PATTERN_CLONE:
      pattern = gimp_context_get_pattern (context);

      if (!pattern)
        return;

      /*  configure the destination  */
      pixel_region_init_temp_buf (&destPR, area,
                                  0, 0, area->width, area->height);

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
              gimp_clone_line_image (image, src_image,
                                     drawable, src_pickable,
                                     s, d,
                                     srcPR.bytes, destPR.bytes, destPR.w);
              s += srcPR.rowstride;
              break;

            case GIMP_PATTERN_CLONE:
              gimp_clone_line_pattern (image, drawable,
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

                                /* In fixed mode, paint incremental so the
                                 * individual brushes are properly applied
                                 * on top of each other.
                                 * Otherwise the stuff we paint is seamless
                                 * and we don't need intermediate masking.
                                 */
                                options->align_mode == GIMP_CLONE_ALIGN_FIXED ?
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

static void
gimp_clone_src_drawable_removed (GimpDrawable *drawable,
                                 GimpClone    *clone)
{
  if (drawable == clone->src_drawable)
    {
      clone->src_drawable = NULL;
    }

  g_signal_handlers_disconnect_by_func (drawable,
                                        gimp_clone_src_drawable_removed,
                                        clone);
}

static void
gimp_clone_set_src_drawable (GimpClone    *clone,
                             GimpDrawable *drawable)
{
  if (clone->src_drawable == drawable)
    return;

  if (clone->src_drawable)
    g_signal_handlers_disconnect_by_func (clone->src_drawable,
                                          gimp_clone_src_drawable_removed,
                                          clone);

  clone->src_drawable = drawable;

  if (clone->src_drawable)
    g_signal_connect (clone->src_drawable, "removed",
                      G_CALLBACK (gimp_clone_src_drawable_removed),
                      clone);

  g_object_notify (G_OBJECT (clone), "src-drawable");
}
