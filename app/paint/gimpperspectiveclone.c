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

#include "libgimpmath/gimpmatrix.h"

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

#include "gimpperspectiveclone.h"
#include "gimpperspectivecloneoptions.h"

#include "gimp-intl.h"


#define MIN4(a,b,c,d) MIN(MIN(a,b),MIN(c,d))
#define MAX4(a,b,c,d) MAX(MAX(a,b),MAX(c,d))


static void     gimp_perspective_clone_finalize   (GObject          *object);

static void     gimp_perspective_clone_paint      (GimpPaintCore    *paint_core,
                                                   GimpDrawable     *drawable,
                                                   GimpPaintOptions *paint_options,
                                                   GimpPaintState    paint_state,
                                                   guint32           time);

static gboolean gimp_perspective_clone_get_source (GimpSourceCore   *source_core,
                                                   GimpDrawable     *drawable,
                                                   GimpPaintOptions *paint_options,
                                                   GimpPickable     *src_pickable,
                                                   gint              src_offset_x,
                                                   gint              src_offset_y,
                                                   TempBuf          *paint_area,
                                                   gint             *paint_area_offset_x,
                                                   gint             *paint_area_offset_y,
                                                   gint             *paint_area_width,
                                                   gint             *paint_area_height,
                                                   PixelRegion      *srcPR);


G_DEFINE_TYPE (GimpPerspectiveClone, gimp_perspective_clone,
               GIMP_TYPE_CLONE)

#define parent_class gimp_perspective_clone_parent_class


void
gimp_perspective_clone_register (Gimp                      *gimp,
                                 GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_PERSPECTIVE_CLONE,
                GIMP_TYPE_PERSPECTIVE_CLONE_OPTIONS,
                "gimp-perspective-clone",
                _("Perspective Clone"),
                "gimp-tool-perspective-clone");
}

static void
gimp_perspective_clone_class_init (GimpPerspectiveCloneClass *klass)
{
  GObjectClass        *object_class      = G_OBJECT_CLASS (klass);
  GimpPaintCoreClass  *paint_core_class  = GIMP_PAINT_CORE_CLASS (klass);
  GimpSourceCoreClass *source_core_class = GIMP_SOURCE_CORE_CLASS (klass);

  object_class->finalize        = gimp_perspective_clone_finalize;

  paint_core_class->paint       = gimp_perspective_clone_paint;

  source_core_class->get_source = gimp_perspective_clone_get_source;
}

static void
gimp_perspective_clone_init (GimpPerspectiveClone *clone)
{
  clone->dest_x    = 0.0;    /* coords where the stroke starts */
  clone->dest_y    = 0.0;

  clone->src_x_fv  = 0.0;    /* source coords in front_view perspective */
  clone->src_y_fv  = 0.0;

  clone->dest_x_fv = 0.0;    /* destination coords in front_view perspective */
  clone->dest_y_fv = 0.0;

  gimp_matrix3_identity (&clone->transform);
  gimp_matrix3_identity (&clone->transform_inv);
}

static void
gimp_perspective_clone_finalize (GObject *object)
{
  GimpPerspectiveClone *clone = GIMP_PERSPECTIVE_CLONE (object);

  if (clone->src_area)
    {
      temp_buf_free (clone->src_area);
      clone->src_area = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_perspective_clone_paint (GimpPaintCore    *paint_core,
                              GimpDrawable     *drawable,
                              GimpPaintOptions *paint_options,
                              GimpPaintState    paint_state,
                              guint32           time)
{
  GimpSourceCore       *source_core = GIMP_SOURCE_CORE (paint_core);
  GimpPerspectiveClone *clone       = GIMP_PERSPECTIVE_CLONE (paint_core);
  GimpSourceOptions    *options     = GIMP_SOURCE_OPTIONS (paint_options);

  switch (paint_state)
    {
    case GIMP_PAINT_STATE_INIT:
      if (source_core->set_source)
        {
          g_object_set (source_core, "src-drawable", drawable, NULL);

          source_core->src_x = paint_core->cur_coords.x;
          source_core->src_y = paint_core->cur_coords.y;

          /* get source coordinates in front view perspective */
          gimp_matrix3_transform_point (&clone->transform_inv,
                                        source_core->src_x,
                                        source_core->src_y,
                                        &clone->src_x_fv,
                                        &clone->src_y_fv);

          source_core->first_stroke = TRUE;
        }
      else if (options->align_mode == GIMP_SOURCE_ALIGN_NO)
        {
          source_core->orig_src_x = source_core->src_x;
          source_core->orig_src_y = source_core->src_y;

          source_core->first_stroke = TRUE;
        }
      break;

    case GIMP_PAINT_STATE_MOTION:
      if (source_core->set_source)
        {
          /*  If the control key is down, move the src target and return */

          source_core->src_x = paint_core->cur_coords.x;
          source_core->src_y = paint_core->cur_coords.y;

          /* get source coordinates in front view perspective */
          gimp_matrix3_transform_point (&clone->transform_inv,
                                        source_core->src_x,
                                        source_core->src_y,
                                        &clone->src_x_fv,
                                        &clone->src_y_fv);

          source_core->first_stroke = TRUE;
        }
      else
        {
          /*  otherwise, update the target  */

          gint dest_x;
          gint dest_y;

          dest_x = paint_core->cur_coords.x;
          dest_y = paint_core->cur_coords.y;

          if (options->align_mode == GIMP_SOURCE_ALIGN_REGISTERED)
            {
              source_core->offset_x = 0;
              source_core->offset_y = 0;
            }
          else if (options->align_mode == GIMP_SOURCE_ALIGN_FIXED)
            {
              source_core->offset_x = source_core->src_x - dest_x;
              source_core->offset_y = source_core->src_y - dest_y;
            }
          else if (source_core->first_stroke)
            {
              source_core->offset_x = source_core->src_x - dest_x;
              source_core->offset_y = source_core->src_y - dest_y;

              clone->dest_x = dest_x;       /* cooords where start the destination stroke */
              clone->dest_y = dest_y;

              /* get destination coordinates in front view perspective */
              gimp_matrix3_transform_point (&clone->transform_inv,
                                            clone->dest_x,
                                            clone->dest_y,
                                            &clone->dest_x_fv,
                                            &clone->dest_y_fv);

              source_core->first_stroke = FALSE;
            }

          gimp_source_core_motion (source_core, drawable, paint_options);
        }
      break;

    case GIMP_PAINT_STATE_FINISH:
      break;

    default:
      break;
    }

  g_object_notify (G_OBJECT (clone), "src-x");
  g_object_notify (G_OBJECT (clone), "src-y");
}

static gboolean
gimp_perspective_clone_get_source (GimpSourceCore   *source_core,
                                   GimpDrawable     *drawable,
                                   GimpPaintOptions *paint_options,
                                   GimpPickable     *src_pickable,
                                   gint              src_offset_x,
                                   gint              src_offset_y,
                                   TempBuf          *paint_area,
                                   gint             *paint_area_offset_x,
                                   gint             *paint_area_offset_y,
                                   gint             *paint_area_width,
                                   gint             *paint_area_height,
                                   PixelRegion      *srcPR)
{
  GimpPerspectiveClone *clone      = GIMP_PERSPECTIVE_CLONE (source_core);
  GimpPaintCore        *paint_core = GIMP_PAINT_CORE (source_core);
  GimpSourceOptions    *options    = GIMP_SOURCE_OPTIONS (paint_options);
  GimpImage            *src_image;
  GimpImage            *image;
  gint                  x1d, y1d, x2d, y2d;                      /* Coordinates of the destination area to paint */
  gdouble               x1s, y1s, x2s, y2s, x3s, y3s, x4s, y4s;  /* Coordinates of the boundary box to copy pixels to the tempbuf and after apply perspective transform */
  gint                  xmin, ymin, xmax, ymax;
  TileManager          *src_tiles;
  guchar               *src_data;
  guchar               *dest_data;
  gint                  i, j;
  TempBuf              *orig;

  if (gimp_drawable_is_indexed (drawable))
    {
      g_message (_("Indexed images are not currently supported."));
      return FALSE;
    }

  src_image = gimp_pickable_get_image (src_pickable);
  image     = gimp_item_get_image (GIMP_ITEM (drawable));

  src_tiles = gimp_pickable_get_tiles (src_pickable);

  /* Destination coordinates that will be painted */
  x1d = paint_area->x;
  y1d = paint_area->y;
  x2d = paint_area->x + paint_area->width;
  y2d = paint_area->y + paint_area->height;

  /* Boundary box for source pixels to copy: Convert all the vertex of
   * the box to paint in destination area to its correspondent in
   * source area bearing in mind perspective
   */
  gimp_perspective_clone_get_source_point (clone, x1d, y1d, &x1s, &y1s);
  gimp_perspective_clone_get_source_point (clone, x1d, y2d, &x2s, &y2s);
  gimp_perspective_clone_get_source_point (clone, x2d, y1d, &x3s, &y3s);
  gimp_perspective_clone_get_source_point (clone, x2d, y2d, &x4s, &y4s);

  xmin = floor (MIN4 (x1s, x2s, x3s, x4s));
  ymin = floor (MIN4 (y1s, y2s, y3s, y4s));
  xmax = ceil  (MAX4 (x1s, x2s, x3s, x4s));
  ymax = ceil  (MAX4 (y1s, y2s, y3s, y4s));

  xmin = CLAMP (xmin - 1, 0, tile_manager_width  (src_tiles));
  ymin = CLAMP (ymin - 1, 0, tile_manager_height (src_tiles));
  xmax = CLAMP (xmax + 1, 0, tile_manager_width  (src_tiles));
  ymax = CLAMP (ymax + 1, 0, tile_manager_height (src_tiles));

  /* if the source area is completely out of the image */
  if (!(xmax - xmin) || !(ymax - ymin))
    return FALSE;

  /*  get the original image  */
  if (options->sample_merged)
    orig = gimp_paint_core_get_orig_proj (paint_core,
                                          src_pickable,
                                          xmin, ymin, xmax, ymax);
  else
    orig = gimp_paint_core_get_orig_image (paint_core,
                                           GIMP_DRAWABLE (src_pickable),
                                           xmin, ymin, xmax, ymax);

  /* note: orig is a TempBuf where are all the pixels that I need to copy */
  /* copy from orig to temp_buf, this buffer has the size of the destination area */

  /* Also allocate memory for alpha channel */
  clone->src_area = temp_buf_resize (clone->src_area,
                                     orig->bytes + 1, /* FIXME */
                                     0, 0,
                                     x2d - x1d, y2d - y1d);

  src_data  = temp_buf_data (orig);
  dest_data = temp_buf_data (clone->src_area);

  for (i = x1d; i < x2d; i++)
    {
      for (j = y1d; j < y2d; j++)
        {
          guchar  *dest_pixel;
          gdouble  temp_x, temp_y;
          gint     itemp_x, itemp_y;

          gimp_perspective_clone_get_source_point (clone,
                                                   i, j,
                                                   &temp_x, &temp_y);

          itemp_x = (gint) temp_x;
          itemp_y = (gint) temp_y;

          /* Points to the dest pixel in clone->src_area */
          dest_pixel = (dest_data +
                        clone->src_area->bytes *
                        ((j - y1d) * clone->src_area->width +
                         (i - x1d)));

          /* Check if the source pixel is inside the image */
          if (itemp_x > 0 && itemp_x < tile_manager_width  (src_tiles) - 1 &&
              itemp_y > 0 && itemp_y < tile_manager_height (src_tiles) - 1)
            {
              guchar  *src_pixel;
              guchar  *color1 = g_alloca (clone->src_area->bytes - 1);
              guchar  *color2 = g_alloca (clone->src_area->bytes - 1);
              guchar  *color3 = g_alloca (clone->src_area->bytes - 1);
              guchar  *color4 = g_alloca (clone->src_area->bytes - 1);
              gdouble  dx, dy;
              gint     k;

              dx = temp_x - itemp_x;
              dy = temp_y - itemp_y;

              /* linear interpolation
               *   (i,j)     * (1-dx)*(1-dy) +
               *   (i+1,j)   * (dx*(1-dy))   +
               *   (i,j+1)   * ((1-dx)*dy)   +
               *   (i+1,j+1) * (dx*dy)
               */
              src_pixel = (src_data +
                           orig->bytes *
                           ((itemp_y - ymin) * orig->width +
                            (itemp_x - xmin)));
              for (k = 0 ; k < clone->src_area->bytes - 1; k++)
                color1[k] = *src_pixel++;

              src_pixel = (src_data +
                           orig->bytes *
                           ((itemp_y     - ymin) * orig->width +
                            (itemp_x + 1 - xmin)));
              for (k = 0 ; k < clone->src_area->bytes - 1; k++)
                color2[k] = *src_pixel++;

              src_pixel = (src_data +
                           orig->bytes *
                           ((itemp_y + 1 - ymin) * orig->width +
                            (itemp_x     - xmin)));
              for (k = 0 ; k < clone->src_area->bytes - 1; k++)
                color3[k] = *src_pixel++;

              src_pixel = (src_data +
                           orig->bytes *
                           ((itemp_y + 1 - ymin) * orig->width +
                            (itemp_x + 1 - xmin)));
              for (k = 0 ; k < clone->src_area->bytes - 1; k++)
                color4[k] = *src_pixel++;

              /* copy the pixel interpolated to clone->src_area */
              for (k = 0 ; k < clone->src_area->bytes - 1; k++)
                *dest_pixel++ = (color1[k] * ((1-dx) * (1-dy)) +
                                 color2[k] * (dx     * (1-dy)) +
                                 color3[k] * ((1-dx) * dy)     +
                                 color4[k] * (dx     * dy));

              /* FIXME: If the pixel is inside the image set the alpha
               * channel visible
               */
              *dest_pixel = 255;
            }
          else
            {
              /* Pixels with source out-of-image are transparent */
              dest_pixel[clone->src_area->bytes - 1] = 0;
            }
        }
    }

  pixel_region_init_temp_buf (srcPR, clone->src_area,
                              0, 0, x2d - x1d, y2d - y1d);

  return TRUE;
}

void
gimp_perspective_clone_get_source_point (GimpPerspectiveClone *perspective_clone,
                                         gdouble               x,
                                         gdouble               y,
                                         gdouble              *newx,
                                         gdouble              *newy)
{
  gdouble temp_x, temp_y;
  gdouble offset_x_fv, offset_y_fv;

  gimp_matrix3_transform_point (&perspective_clone->transform_inv,
                                x, y, &temp_x, &temp_y);

  /* Get the offset of each pixel in destination area from the
   * destination pixel in front view perspective
   */
  offset_x_fv = temp_x - perspective_clone->dest_x_fv;
  offset_y_fv = temp_y - perspective_clone->dest_y_fv;

  /* Get the source pixel in front view perspective */
  temp_x = offset_x_fv + perspective_clone->src_x_fv;
  temp_y = offset_y_fv + perspective_clone->src_y_fv;

  /* Convert the source pixel to perspective view */
  gimp_matrix3_transform_point (&perspective_clone->transform,
                                temp_x, temp_y, newx, newy);
}
