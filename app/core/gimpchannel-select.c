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

#include <glib-object.h>

#include "core-types.h"

#include "gimpchannel.h"
#include "gimpimage.h"
#include "gimpimage-contiguous-region.h"
#include "gimpimage-mask.h"
#include "gimpimage-mask-select.h"
#include "gimpscanconvert.h"

#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"


void
gimp_image_mask_select_rectangle (GimpImage      *gimage,
                                  gint            x,
                                  gint            y,
                                  gint            w,
                                  gint            h,
                                  GimpChannelOps  op,
                                  gboolean        feather,
                                  gdouble         feather_radius_x,
                                  gdouble         feather_radius_y)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  if applicable, replace the current selection  */
  if (op == GIMP_CHANNEL_OP_REPLACE)
    gimp_image_mask_clear (gimage);
  else
    gimp_image_mask_push_undo (gimage);

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather || op == GIMP_CHANNEL_OP_INTERSECT)
    {
      GimpChannel *mask;

      mask = gimp_channel_new_mask (gimage, gimage->width, gimage->height);
      gimp_channel_combine_rect (mask, GIMP_CHANNEL_OP_ADD, x, y, w, h);

      if (feather)
        gimp_channel_feather (mask,
                              feather_radius_x,
                              feather_radius_y,
                              FALSE /* no undo */);

      gimp_channel_combine_mask (gimp_image_get_mask (gimage), mask, op, 0, 0);
      g_object_unref (G_OBJECT (mask));
    }
  else
    {
      gimp_channel_combine_rect (gimp_image_get_mask (gimage), op, x, y, w, h);
    }

  gimp_image_mask_changed (gimage);
}

void
gimp_image_mask_select_ellipse (GimpImage      *gimage,
                                gint            x,
                                gint            y,
                                gint            w,
                                gint            h,
                                GimpChannelOps  op,
                                gboolean        antialias,
                                gboolean        feather,
                                gdouble         feather_radius_x,
                                gdouble         feather_radius_y)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  if applicable, replace the current selection  */
  if (op == GIMP_CHANNEL_OP_REPLACE)
    gimp_image_mask_clear (gimage);
  else
    gimp_image_mask_push_undo (gimage);

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather || op == GIMP_CHANNEL_OP_INTERSECT)
    {
      GimpChannel *mask;

      mask = gimp_channel_new_mask (gimage, gimage->width, gimage->height);
      gimp_channel_combine_ellipse (mask, GIMP_CHANNEL_OP_ADD, 
                                    x, y, w, h, antialias);

      if (feather)
        gimp_channel_feather (mask,
                              feather_radius_x,
                              feather_radius_y,
                              FALSE /* no undo */);

      gimp_channel_combine_mask (gimp_image_get_mask (gimage), mask, op, 0, 0);
      g_object_unref (G_OBJECT (mask));
    }
  else
    {
      gimp_channel_combine_ellipse (gimp_image_get_mask (gimage), op,
				    x, y, w, h, antialias);
    }

  gimp_image_mask_changed (gimage);
}

void
gimp_image_mask_select_polygon (GimpImage       *gimage,
                                gint             n_points,
                                GimpVector2     *points,
                                GimpChannelOps   op,
                                gboolean         antialias,
                                gboolean         feather,
                                gdouble          feather_radius_x,
                                gdouble          feather_radius_y)
{
  GimpScanConvert *scan_convert;
  GimpChannel     *mask;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  if applicable, replace the current selection
   *  or insure that a floating selection is anchored down...
   */
  if (op == GIMP_CHANNEL_OP_REPLACE)
    gimp_image_mask_clear (gimage);
  else
    gimp_image_mask_push_undo (gimage);

#define SUPERSAMPLE 3

  scan_convert = gimp_scan_convert_new (gimage->width,
                                        gimage->height,
                                        antialias ? SUPERSAMPLE : 1);
  gimp_scan_convert_add_points (scan_convert, n_points, points);

  mask = gimp_scan_convert_to_channel (scan_convert, gimage);

  gimp_scan_convert_free (scan_convert);

#undef SUPERSAMPLE

  if (mask)
    {
      if (feather)
	gimp_channel_feather (mask,
			      feather_radius_x,
			      feather_radius_y,
                              FALSE /* no undo */);

      gimp_channel_combine_mask (gimp_image_get_mask (gimage), mask, op, 0, 0);
      g_object_unref (G_OBJECT (mask));
    }

  gimp_image_mask_changed (gimage);
}

void
gimp_image_mask_select_vectors (GimpImage      *gimage,
                                GimpVectors    *vectors,
                                GimpChannelOps  op,
                                gboolean        antialias,
                                gboolean        feather,
                                gdouble         feather_radius_x,
                                gdouble         feather_radius_y)
{
  GimpStroke *stroke;
  GArray     *coords = NULL;
  gboolean    closed;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  gimp_stroke_interpolate() may return NULL, so iterate over the
   *  list of strokes until one returns coords
   */
  for (stroke = vectors->strokes; stroke; stroke = stroke->next)
    {
      coords = gimp_stroke_interpolate (stroke, 1.0, &closed);

      if (coords)
        break;
    }

  if (coords)
    {
      GimpVector2 *points;
      gint         i;

      points = g_new0 (GimpVector2, coords->len);

      for (i = 0; i < coords->len; i++)
        {
          points[i].x = g_array_index (coords, GimpCoords, i).x;
          points[i].y = g_array_index (coords, GimpCoords, i).y;
        }

      gimp_image_mask_select_polygon (GIMP_ITEM (vectors)->gimage,
                                      coords->len,
                                      points,
                                      op,
                                      antialias,
                                      feather,
                                      feather_radius_x,
                                      feather_radius_y);

      g_array_free (coords, TRUE);
      g_free (points);
    }
}

void
gimp_image_mask_select_channel (GimpImage      *gimage,
                                GimpChannel    *channel,
                                gint            offset_x,
                                gint            offset_y,
                                GimpChannelOps  op, 
                                gboolean        feather,
                                gdouble         feather_radius_x,
                                gdouble         feather_radius_y)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  /*  if applicable, replace the current selection  */
  if (op == GIMP_CHANNEL_OP_REPLACE)
    gimp_image_mask_clear (gimage);
  else
    gimp_image_mask_push_undo (gimage);

  if (feather)
    gimp_channel_feather (channel,
			  feather_radius_x,
			  feather_radius_y,
                          FALSE /* no undo */);

  gimp_channel_combine_mask (gimp_image_get_mask (gimage), channel,
                             op, offset_x, offset_y);

  gimp_image_mask_changed (gimage);
}

void
gimp_image_mask_select_fuzzy (GimpImage      *gimage,
                              GimpDrawable   *drawable,
                              gboolean        sample_merged,
                              gint            x,
                              gint            y,
                              gint            threshold,
                              gboolean        select_transparent,
                              GimpChannelOps  op,
                              gboolean        antialias,
                              gboolean        feather,
                              gdouble         feather_radius_x,
                              gdouble         feather_radius_y)
{
  GimpChannel *mask;
  gint         mask_x;
  gint         mask_y;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  mask = gimp_image_contiguous_region_by_seed (gimage, drawable,
                                               sample_merged,
                                               antialias,
                                               threshold,
                                               select_transparent,
                                               x, y);

  if (sample_merged)
    {
      mask_x = 0;
      mask_y = 0;
    }
  else
    {
      gimp_drawable_offsets (drawable, &mask_x, &mask_y);
    }

  gimp_image_mask_select_channel (gimage,
                                  mask,
                                  mask_x,
                                  mask_y,
                                  op,
                                  feather,
                                  feather_radius_x,
                                  feather_radius_y);

  g_object_unref (G_OBJECT (mask));
}

void
gimp_image_mask_select_by_color (GimpImage      *gimage,
                                 GimpDrawable   *drawable,
                                 gboolean        sample_merged,
                                 const GimpRGB  *color,
                                 gint            threshold,
                                 gboolean        select_transparent,
                                 GimpChannelOps  op,
                                 gboolean        antialias,
                                 gboolean        feather,
                                 gdouble         feather_radius_x,
                                 gdouble         feather_radius_y)
{
  GimpChannel *mask;
  gint         mask_x;
  gint         mask_y;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (color != NULL);

  mask = gimp_image_contiguous_region_by_color (gimage, drawable,
                                                sample_merged,
                                                antialias,
                                                threshold,
                                                select_transparent,
                                                color);

  if (sample_merged)
    {
      mask_x = 0;
      mask_y = 0;
    }
  else
    {
      gimp_drawable_offsets (drawable, &mask_x, &mask_y);
    }

  gimp_image_mask_select_channel (gimage,
                                  mask,
                                  mask_x,
                                  mask_y,
                                  op,
                                  feather,
                                  feather_radius_x,
                                  feather_radius_y);

  g_object_unref (G_OBJECT (mask));
}
