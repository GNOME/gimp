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


void
gimp_image_mask_select_rectangle (GimpImage  *gimage,
                                  gint        x,
                                  gint        y,
                                  gint        w,
                                  gint        h,
                                  ChannelOps  op,
                                  gboolean    feather,
                                  gdouble     feather_radius_x,
                                  gdouble     feather_radius_y)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  if applicable, replace the current selection  */
  if (op == CHANNEL_OP_REPLACE)
    gimp_image_mask_clear (gimage);
  else
    gimp_image_mask_undo (gimage);

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather || op == CHANNEL_OP_INTERSECT)
    {
      GimpChannel *mask;

      mask = gimp_channel_new_mask (gimage, gimage->width, gimage->height);
      gimp_channel_combine_rect (mask, CHANNEL_OP_ADD, x, y, w, h);

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
}

void
gimp_image_mask_select_ellipse (GimpImage  *gimage,
                                gint        x,
                                gint        y,
                                gint        w,
                                gint        h,
                                ChannelOps  op,
                                gboolean    antialias,
                                gboolean    feather,
                                gdouble     feather_radius_x,
                                gdouble     feather_radius_y)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  if applicable, replace the current selection  */
  if (op == CHANNEL_OP_REPLACE)
    gimp_image_mask_clear (gimage);
  else
    gimp_image_mask_undo (gimage);

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather || op == CHANNEL_OP_INTERSECT)
    {
      GimpChannel *mask;

      mask = gimp_channel_new_mask (gimage, gimage->width, gimage->height);
      gimp_channel_combine_ellipse (mask, CHANNEL_OP_ADD, x, y, w, h, antialias);

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
}

void
gimp_image_mask_select_polygon (GimpImage   *gimage,
                                gint         n_points,
                                GimpVector2 *points,
                                ChannelOps   op,
                                gboolean     antialias,
                                gboolean     feather,
                                gdouble      feather_radius_x,
                                gdouble      feather_radius_y)
{
  GimpScanConvert *scan_convert;
  GimpChannel     *mask;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  if applicable, replace the current selection
   *  or insure that a floating selection is anchored down...
   */
  if (op == CHANNEL_OP_REPLACE)
    gimp_image_mask_clear (gimage);
  else
    gimp_image_mask_undo (gimage);

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
}

void
gimp_image_mask_select_channel (GimpImage    *gimage,
                                GimpDrawable *drawable,
                                gboolean      sample_merged,
                                GimpChannel  *channel,
                                ChannelOps    op, 
                                gboolean      feather,
                                gdouble       feather_radius_x,
                                gdouble       feather_radius_y)
{
  gint off_x, off_y;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail ((! drawable && ! sample_merged) || GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  /*  if applicable, replace the current selection  */
  if (op == CHANNEL_OP_REPLACE)
    gimp_image_mask_clear (gimage);
  else
    gimp_image_mask_undo (gimage);

  if (sample_merged)
    {
      off_x = off_y = 0;
    }
  else
    {
      gimp_drawable_offsets (drawable, &off_x, &off_y);
    }
  
  if (feather)
    gimp_channel_feather (channel,
			  feather_radius_x,
			  feather_radius_y,
                          FALSE /* no undo */);

  gimp_channel_combine_mask (gimp_image_get_mask (gimage), channel,
                             op, off_x, off_y);
}

void
gimp_image_mask_select_fuzzy (GimpImage    *gimage,
                              GimpDrawable *drawable,
                              gboolean      sample_merged,
                              gint          x,
                              gint          y,
                              gint          threshold,
                              gboolean      select_transparent,
                              ChannelOps    op,
                              gboolean      antialias,
                              gboolean      feather,
                              gdouble       feather_radius_x,
                              gdouble       feather_radius_y)
{
  GimpChannel *mask;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  mask = gimp_image_contiguous_region_by_seed (gimage, drawable,
                                               sample_merged,
                                               antialias,
                                               threshold,
                                               select_transparent,
                                               x, y);

  gimp_image_mask_select_channel (gimage,
                                  drawable,
                                  sample_merged,
                                  mask,
                                  op,
                                  feather,
                                  feather_radius_x,
                                  feather_radius_y);

  g_object_unref (G_OBJECT (mask));
}

void
gimp_image_mask_select_by_color (GimpImage     *gimage,
                                 GimpDrawable  *drawable,
                                 gboolean       sample_merged,
                                 const GimpRGB *color,
                                 gint           threshold,
                                 gboolean       select_transparent,
                                 ChannelOps     op,
                                 gboolean       antialias,
                                 gboolean       feather,
                                 gdouble        feather_radius_x,
                                 gdouble        feather_radius_y)
{
  GimpChannel *mask;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (color != NULL);

  mask = gimp_image_contiguous_region_by_color (gimage, drawable,
                                                sample_merged,
                                                antialias,
                                                threshold,
                                                select_transparent,
                                                color);

  gimp_image_mask_select_channel (gimage,
                                  drawable,
                                  sample_merged,
                                  mask,
                                  op,
                                  feather,
                                  feather_radius_x,
                                  feather_radius_y);

  g_object_unref (G_OBJECT (mask));
}
