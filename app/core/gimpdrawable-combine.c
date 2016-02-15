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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gegl/gimpapplicator.h"
#include "gegl/gimp-babl-compat.h"
#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpdrawable-combine.h"
#include "gimpdrawableundo.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimptempbuf.h"


void
gimp_drawable_real_apply_buffer (GimpDrawable         *drawable,
                                 GeglBuffer           *buffer,
                                 const GeglRectangle  *buffer_region,
                                 gboolean              push_undo,
                                 const gchar          *undo_desc,
                                 gdouble               opacity,
                                 GimpLayerModeEffects  mode,
                                 GeglBuffer           *base_buffer,
                                 gint                  base_x,
                                 gint                  base_y)
{
  GimpItem          *item  = GIMP_ITEM (drawable);
  GimpImage         *image = gimp_item_get_image (item);
  GimpChannel       *mask  = gimp_image_get_mask (image);
  GimpApplicator    *applicator;
  gint               x, y, width, height;
  gint               offset_x, offset_y;

  /*  don't apply the mask to itself and don't apply an empty mask  */
  if (GIMP_DRAWABLE (mask) == drawable || gimp_channel_is_empty (mask))
    mask = NULL;

  if (! base_buffer)
    base_buffer = gimp_drawable_get_buffer (drawable);

  /*  get the layer offsets  */
  gimp_item_get_offset (item, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within drawable bounds  */
  gimp_rectangle_intersect (base_x, base_y,
                            buffer_region->width, buffer_region->height,
                            0, 0,
                            gimp_item_get_width  (item),
                            gimp_item_get_height (item),
                            &x, &y, &width, &height);

  if (mask)
    {
      GimpItem *mask_item = GIMP_ITEM (mask);

      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      gimp_rectangle_intersect (x, y, width, height,
                                -offset_x, -offset_y,
                                gimp_item_get_width  (mask_item),
                                gimp_item_get_height (mask_item),
                                &x, &y, &width, &height);
    }

  if (push_undo)
    {
      GimpDrawableUndo *undo;

      gimp_drawable_push_undo (drawable, undo_desc,
                               NULL, x, y, width, height);

      undo = GIMP_DRAWABLE_UNDO (gimp_image_undo_get_fadeable (image));

      if (undo)
        {
          undo->paint_mode = mode;
          undo->opacity    = opacity;

          undo->applied_buffer =
            gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                             gegl_buffer_get_format (buffer));

          gegl_buffer_copy (buffer,
                            GEGL_RECTANGLE (buffer_region->x + (x - base_x),
                                            buffer_region->y + (y - base_y),
                                            width, height),
                            GEGL_ABYSS_NONE,
                            undo->applied_buffer,
                            GEGL_RECTANGLE (0, 0, width, height));
        }
    }

  applicator = gimp_applicator_new (NULL, gimp_drawable_get_linear (drawable),
                                    FALSE, FALSE);

  if (mask)
    {
      GeglBuffer *mask_buffer;

      mask_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

      gimp_applicator_set_mask_buffer (applicator, mask_buffer);
      gimp_applicator_set_mask_offset (applicator, -offset_x, -offset_y);
    }

  gimp_applicator_set_src_buffer (applicator, base_buffer);
  gimp_applicator_set_dest_buffer (applicator,
                                   gimp_drawable_get_buffer (drawable));

  gimp_applicator_set_apply_buffer (applicator, buffer);
  gimp_applicator_set_apply_offset (applicator,
                                    base_x - buffer_region->x,
                                    base_y - buffer_region->y);

  gimp_applicator_set_mode (applicator, opacity, mode);
  gimp_applicator_set_affect (applicator,
                              gimp_drawable_get_active_mask (drawable));

  gimp_applicator_blit (applicator, GEGL_RECTANGLE (x, y, width, height));

  g_object_unref (applicator);
}

/*  Similar to gimp_drawable_apply_region but works in "replace" mode (i.e.
 *  transparent pixels in src2 make the result transparent rather than
 *  opaque.
 *
 * Takes an additional mask pixel region as well.
 */
void
gimp_drawable_real_replace_buffer (GimpDrawable        *drawable,
                                   GeglBuffer          *buffer,
                                   const GeglRectangle *buffer_region,
                                   gboolean             push_undo,
                                   const gchar         *undo_desc,
                                   gdouble              opacity,
                                   GeglBuffer          *mask_buffer,
                                   const GeglRectangle *mask_buffer_region,
                                   gint                 dest_x,
                                   gint                 dest_y)
{
  GimpItem        *item  = GIMP_ITEM (drawable);
  GimpImage       *image = gimp_item_get_image (item);
  GimpChannel     *mask  = gimp_image_get_mask (image);
  GeglBuffer      *drawable_buffer;
  gint             x, y, width, height;
  gint             offset_x, offset_y;
  gboolean         active_components[MAX_CHANNELS];

  /*  don't apply the mask to itself and don't apply an empty mask  */
  if (GIMP_DRAWABLE (mask) == drawable || gimp_channel_is_empty (mask))
    mask = NULL;

  /*  configure the active channel array  */
  gimp_drawable_get_active_components (drawable, active_components);

  /*  get the layer offsets  */
  gimp_item_get_offset (item, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within drawable bounds  */
  gimp_rectangle_intersect (dest_x, dest_y,
                            buffer_region->width, buffer_region->height,
                            0, 0,
                            gimp_item_get_width  (item),
                            gimp_item_get_height (item),
                            &x, &y, &width, &height);

  if (mask)
    {
      GimpItem *mask_item = GIMP_ITEM (mask);

      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      gimp_rectangle_intersect (x, y, width, height,
                                -offset_x, -offset_y,
                                gimp_item_get_width  (mask_item),
                                gimp_item_get_height (mask_item),
                                &x, &y, &width, &height);
    }

  /*  If the calling procedure specified an undo step...  */
  if (push_undo)
    gimp_drawable_push_undo (drawable, undo_desc,
                             NULL, x, y, width, height);

  drawable_buffer = gimp_drawable_get_buffer (drawable);

  if (mask)
    {
      GeglBuffer *src_buffer;
      GeglBuffer *dest_buffer;

      src_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

      dest_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                     gegl_buffer_get_format (src_buffer));

      gegl_buffer_copy (src_buffer,
                        GEGL_RECTANGLE (x + offset_x, y + offset_y,
                                        width, height),
                        GEGL_ABYSS_NONE,
                        dest_buffer,
                        GEGL_RECTANGLE (0, 0, 0, 0));

      gimp_gegl_combine_mask (mask_buffer, mask_buffer_region,
                              dest_buffer, GEGL_RECTANGLE (0, 0, width, height),
                              1.0);

      gimp_gegl_replace (buffer,          buffer_region,
                         drawable_buffer, GEGL_RECTANGLE (x, y, width, height),
                         dest_buffer,     GEGL_RECTANGLE (0, 0, width, height),
                         drawable_buffer, GEGL_RECTANGLE (x, y, width, height),
                         opacity,
                         active_components);

      g_object_unref (dest_buffer);
    }
  else
    {
      gimp_gegl_replace (buffer,          buffer_region,
                         drawable_buffer, GEGL_RECTANGLE (x, y, width, height),
                         mask_buffer,     mask_buffer_region,
                         drawable_buffer, GEGL_RECTANGLE (x, y, width, height),
                         opacity,
                         active_components);
    }
}
