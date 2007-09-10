/* GIMP - The GNU Image Manipulation Program
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
#include "libgimpmath/gimpmath.h"

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
#include "core/gimp-transform-region.h"

#include "gimpperspectiveclone.h"
#include "gimpperspectivecloneoptions.h"

#include "gimp-intl.h"


#define MIN4(a,b,c,d) MIN(MIN(a,b),MIN(c,d))
#define MAX4(a,b,c,d) MAX(MAX(a,b),MAX(c,d))


static void     gimp_perspective_clone_finalize   (GObject          *object);

static gboolean gimp_perspective_clone_start      (GimpPaintCore    *paint_core,
                                                   GimpDrawable     *drawable,
                                                   GimpPaintOptions *paint_options,
                                                   GimpCoords       *coords,
                                                   GError          **error);
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

  paint_core_class->start       = gimp_perspective_clone_start;
  paint_core_class->paint       = gimp_perspective_clone_paint;

  source_core_class->get_source = gimp_perspective_clone_get_source;
}

static void
gimp_perspective_clone_init (GimpPerspectiveClone *clone)
{
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

static gboolean
gimp_perspective_clone_start (GimpPaintCore     *paint_core,
                              GimpDrawable      *drawable,
                              GimpPaintOptions  *paint_options,
                              GimpCoords        *coords,
                              GError           **error)
{
  GimpSourceCore *source_core = GIMP_SOURCE_CORE (paint_core);

  if (! GIMP_PAINT_CORE_CLASS (parent_class)->start (paint_core, drawable,
                                                     paint_options, coords,
                                                     error))
    {
      return FALSE;
    }

  if (! source_core->set_source && gimp_drawable_is_indexed (drawable))
    {
      g_set_error (error, 0, 0,
                   _("Perspective Clone does not operate on indexed layers."));
      return FALSE;
    }

  return TRUE;
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

              /* get destination coordinates in front view perspective */
              gimp_matrix3_transform_point (&clone->transform_inv,
                                            dest_x,
                                            dest_y,
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
  GimpImageType         src_type;
  gint                  x1d, y1d, x2d, y2d;
  gdouble               x1s, y1s, x2s, y2s, x3s, y3s, x4s, y4s;
  gint                  xmin, ymin, xmax, ymax;
  TileManager          *src_tiles;
  TileManager          *orig_tiles;
  PixelRegion           origPR;
  PixelRegion           destPR;
  GimpMatrix3           matrix;
  gint                  bytes;

  src_image = gimp_pickable_get_image (src_pickable);
  image     = gimp_item_get_image (GIMP_ITEM (drawable));

  src_type  = gimp_pickable_get_image_type (src_pickable);
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

  /*  If the source image is different from the destination,
   *  then we should copy straight from the source image
   *  to the canvas.
   *  Otherwise, we need a call to get_orig_image to make sure
   *  we get a copy of the unblemished (offset) image
   */
  if ((  options->sample_merged && (src_image                 != image)) ||
      (! options->sample_merged && (source_core->src_drawable != drawable)))
    {
      pixel_region_init (&origPR, src_tiles,
                         xmin, ymin, xmax - xmin, ymax - ymin, FALSE);
    }
  else
    {
      TempBuf *orig;

      /*  get the original image  */
      if (options->sample_merged)
        orig = gimp_paint_core_get_orig_proj (paint_core,
                                              src_pickable,
                                              xmin, ymin, xmax, ymax);
      else
        orig = gimp_paint_core_get_orig_image (paint_core,
                                               GIMP_DRAWABLE (src_pickable),
                                               xmin, ymin, xmax, ymax);

      pixel_region_init_temp_buf (&origPR, orig,
                                  0, 0, xmax - xmin, ymax - ymin);
    }

  /*  copy the original image to a tile manager, adding alpha if needed  */

  bytes = GIMP_IMAGE_TYPE_BYTES (GIMP_IMAGE_TYPE_WITH_ALPHA (src_type));

  orig_tiles = tile_manager_new (xmax - xmin, ymax - ymin, bytes);

  tile_manager_set_offsets (orig_tiles, xmin, ymin);

  pixel_region_init (&destPR, orig_tiles,
                     0, 0, xmax - xmin, ymax - ymin,
                     TRUE);

  if (bytes > origPR.bytes)
    add_alpha_region (&origPR, &destPR);
  else
    copy_region (&origPR, &destPR);

  clone->src_area = temp_buf_resize (clone->src_area,
                                     tile_manager_bpp (orig_tiles),
                                     0, 0,
                                     x2d - x1d, y2d - y1d);

  pixel_region_init_temp_buf (&destPR, clone->src_area,
                              0, 0,
                              x2d - x1d, y2d - y1d);

  gimp_perspective_clone_get_matrix (clone, &matrix);

  gimp_transform_region (src_pickable,
                         GIMP_CONTEXT (paint_options),
                         orig_tiles,
                         &destPR,
                         x1d, y1d, x2d, y2d,
                         &matrix,
                         GIMP_INTERPOLATION_LINEAR, 0, NULL);

  tile_manager_unref (orig_tiles);

  pixel_region_init_temp_buf (srcPR, clone->src_area,
                              0, 0, x2d - x1d, y2d - y1d);

  return TRUE;
}

void
gimp_perspective_clone_get_source_point (GimpPerspectiveClone *clone,
                                         gdouble               x,
                                         gdouble               y,
                                         gdouble              *newx,
                                         gdouble              *newy)
{
  gdouble temp_x, temp_y;

  gimp_matrix3_transform_point (&clone->transform_inv,
                                x, y, &temp_x, &temp_y);

#if 0
  /* Get the offset of each pixel in destination area from the
   * destination pixel in front view perspective
   */
  offset_x_fv = temp_x - clone->dest_x_fv;
  offset_y_fv = temp_y - clone->dest_y_fv;

  /* Get the source pixel in front view perspective */
  temp_x = offset_x_fv + clone->src_x_fv;
  temp_y = offset_y_fv + clone->src_y_fv;
#endif

  temp_x += clone->src_x_fv - clone->dest_x_fv;
  temp_y += clone->src_y_fv - clone->dest_y_fv;

  /* Convert the source pixel to perspective view */
  gimp_matrix3_transform_point (&clone->transform,
                                temp_x, temp_y, newx, newy);
}

void
gimp_perspective_clone_get_matrix (GimpPerspectiveClone *clone,
                                   GimpMatrix3          *matrix)
{
  GimpMatrix3 temp;

  gimp_matrix3_identity (&temp);
  gimp_matrix3_translate (&temp,
                          clone->dest_x_fv - clone->src_x_fv,
                          clone->dest_y_fv - clone->src_y_fv);

  *matrix = clone->transform_inv;
  gimp_matrix3_mult (&temp, matrix);
  gimp_matrix3_mult (&clone->transform, matrix);
}
