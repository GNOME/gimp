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

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimpchannel.h"
#include "gimpchannel-combine.h"
#include "gimpimage.h"
#include "gimpimage-contiguous-region.h"
#include "gimpimage-mask-select.h"
#include "gimplayer.h"
#include "gimpscanconvert.h"

#include "vectors/gimpstroke.h"
#include "vectors/gimpvectors.h"

#include "gimp-intl.h"


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
  GimpChannel *selection;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  selection = gimp_image_get_mask (gimage);

  /*  if applicable, replace the current selection  */
  if (op == GIMP_CHANNEL_OP_REPLACE)
    gimp_channel_clear (selection, _("Rect Select"), TRUE);
  else
    gimp_channel_push_undo (selection, _("Rect Select"));

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

      gimp_channel_combine_mask (selection, mask, op, 0, 0);
      g_object_unref (mask);
    }
  else
    {
      gimp_channel_combine_rect (selection, op, x, y, w, h);
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
  GimpChannel *selection;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  selection = gimp_image_get_mask (gimage);

  /*  if applicable, replace the current selection  */
  if (op == GIMP_CHANNEL_OP_REPLACE)
    gimp_channel_clear (selection, _("Ellipse Select"), TRUE);
  else
    gimp_channel_push_undo (selection, _("Ellipse Select"));

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

      gimp_channel_combine_mask (selection, mask, op, 0, 0);
      g_object_unref (mask);
    }
  else
    {
      gimp_channel_combine_ellipse (selection, op, x, y, w, h, antialias);
    }

  gimp_image_mask_changed (gimage);
}

void
gimp_image_mask_select_polygon (GimpImage      *gimage,
                                const gchar    *undo_desc,
                                gint            n_points,
                                GimpVector2    *points,
                                GimpChannelOps  op,
                                gboolean        antialias,
                                gboolean        feather,
                                gdouble         feather_radius_x,
                                gdouble         feather_radius_y)
{
  GimpChannel     *selection;
  GimpScanConvert *scan_convert;
  GimpChannel     *mask;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  selection = gimp_image_get_mask (gimage);

  /*  if applicable, replace the current selection
   *  or insure that a floating selection is anchored down...
   */
  if (op == GIMP_CHANNEL_OP_REPLACE)
    gimp_channel_clear (selection, undo_desc, TRUE);
  else
    gimp_channel_push_undo (selection, undo_desc);

#define SUPERSAMPLE 3

  scan_convert = gimp_scan_convert_new (gimage->width,
                                        gimage->height,
                                        antialias ? SUPERSAMPLE : 1);
  gimp_scan_convert_add_points (scan_convert, n_points, points, FALSE);

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

      gimp_channel_combine_mask (selection, mask, op, 0, 0);
      g_object_unref (mask);
    }

  gimp_image_mask_changed (gimage);
}

void
gimp_image_mask_select_vectors (GimpImage      *gimage,
                                const gchar    *undo_desc,
                                GimpVectors    *vectors,
                                GimpChannelOps  op,
                                gboolean        antialias,
                                gboolean        feather,
                                gdouble         feather_radius_x,
                                gdouble         feather_radius_y)
{
  GimpChannel     *selection;
  GimpScanConvert *scan_convert;
  GimpChannel     *mask = NULL;
  GList           *stroke;
  GArray          *coords = NULL;
  gboolean         closed;
  gint             num_coords = 0;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_VECTORS (vectors));

  selection = gimp_image_get_mask (gimage);

  /*  if applicable, replace the current selection
   *  or insure that a floating selection is anchored down...
   */
  if (op == GIMP_CHANNEL_OP_REPLACE)
    gimp_channel_clear (selection, undo_desc, TRUE);
  else
    gimp_channel_push_undo (selection, undo_desc);

#define SUPERSAMPLE 3

  scan_convert = gimp_scan_convert_new (gimage->width,
                                        gimage->height,
                                        antialias ? SUPERSAMPLE : 1);

#undef SUPERSAMPLE

  for (stroke = vectors->strokes; stroke; stroke = stroke->next)
    {
      coords = gimp_stroke_interpolate (GIMP_STROKE (stroke->data),
                                        1.0, &closed);

      if (coords)
        {
          GimpVector2 *points;
          gint         i;

          points = g_new0 (GimpVector2, coords->len);

          for (i = 0; i < coords->len; i++)
            {
              points[i].x = g_array_index (coords, GimpCoords, i).x;
              points[i].y = g_array_index (coords, GimpCoords, i).y;
              num_coords++;
            }

          gimp_scan_convert_add_points (scan_convert, coords->len,
                                        points, TRUE);

          g_array_free (coords, TRUE);
          g_free (points);
        }
    }

  if (num_coords)
    mask = gimp_scan_convert_to_channel (scan_convert, gimage);

  gimp_scan_convert_free (scan_convert);

  if (mask)
    {
      if (feather)
        gimp_channel_feather (mask,
                              feather_radius_x,
                              feather_radius_y,
                              FALSE /* no undo */);

      gimp_channel_combine_mask (selection, mask, op, 0, 0);
      g_object_unref (mask);
    }

  gimp_image_mask_changed (gimage);
}

void
gimp_image_mask_select_channel (GimpImage      *gimage,
                                const gchar    *undo_desc,
                                GimpChannel    *channel,
                                gint            offset_x,
                                gint            offset_y,
                                GimpChannelOps  op, 
                                gboolean        feather,
                                gdouble         feather_radius_x,
                                gdouble         feather_radius_y)
{
  GimpChannel *selection;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  selection = gimp_image_get_mask (gimage);

  /*  if applicable, replace the current selection  */
  if (op == GIMP_CHANNEL_OP_REPLACE)
    gimp_channel_clear (selection, undo_desc, TRUE);
  else
    gimp_channel_push_undo (selection, undo_desc);

  if (feather || op == GIMP_CHANNEL_OP_INTERSECT)
    {
      GimpChannel *mask;

      mask = gimp_channel_new_mask (gimage, gimage->width, gimage->height);
      gimp_channel_combine_mask (mask, channel, GIMP_CHANNEL_OP_ADD,
                                 offset_x, offset_y);

      if (feather)
        gimp_channel_feather (mask,
                              feather_radius_x,
                              feather_radius_y,
                              FALSE /* no undo */);

      gimp_channel_combine_mask (selection, mask, op, 0, 0);
      g_object_unref (mask);
    }
  else
    {
      gimp_channel_combine_mask (selection, channel, op, offset_x, offset_y);
    }

  gimp_image_mask_changed (gimage);
}

void
gimp_image_mask_select_alpha (GimpImage      *gimage,
                              GimpLayer      *layer,
                              GimpChannelOps  op,
                              gboolean        feather,
                              gdouble         feather_radius_x,
                              gdouble         feather_radius_y)
{
  GimpChannel *channel;
  GimpRGB      color;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_LAYER (layer));

  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);

  channel = gimp_channel_new_from_alpha (gimage, layer, NULL, &color);

  if (feather)
    gimp_channel_feather (channel,
                          feather_radius_x,
                          feather_radius_y,
                          FALSE /* no undo */);

  gimp_image_mask_select_channel (gimage, _("Alpha to Selection"), channel,
                                  0, 0, op,
                                  FALSE, 0.0, 0.0);

  g_object_unref (channel);
}

void
gimp_image_mask_select_component (GimpImage       *gimage,
                                  GimpChannelType  component,
                                  GimpChannelOps   op,
                                  gboolean         feather,
                                  gdouble          feather_radius_x,
                                  gdouble          feather_radius_y)
{
  GimpChannel *channel;
  GimpRGB      color;
  GEnumClass  *enum_class;
  GEnumValue  *enum_value;
  gchar       *name;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimp_rgba_set (&color, 0.0, 0.0, 0.0, 1.0);

  channel = gimp_channel_new_from_component (gimage, component, NULL, &color);

  if (feather)
    gimp_channel_feather (channel,
                          feather_radius_x,
                          feather_radius_y,
                          FALSE /* no undo */);

  enum_class = g_type_class_ref (GIMP_TYPE_CHANNEL_TYPE);
  enum_value = g_enum_get_value (enum_class, component);
  g_type_class_unref (enum_class);

  name = g_strdup_printf (_("%s Channel to Selection"),
                          gettext (enum_value->value_name));

  gimp_image_mask_select_channel (gimage, name, channel,
                                  0, 0, op,
                                  FALSE, 0.0, 0.0);

  g_free (name);
  g_object_unref (channel);
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
      gimp_item_offsets (GIMP_ITEM (drawable), &mask_x, &mask_y);
    }

  gimp_image_mask_select_channel (gimage,
                                  _("Fuzzy Select"),
                                  mask,
                                  mask_x,
                                  mask_y,
                                  op,
                                  feather,
                                  feather_radius_x,
                                  feather_radius_y);

  g_object_unref (mask);
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
      gimp_item_offsets (GIMP_ITEM (drawable), &mask_x, &mask_y);
    }

  gimp_image_mask_select_channel (gimage,
                                  _("Select by Color"),
                                  mask,
                                  mask_x,
                                  mask_y,
                                  op,
                                  feather,
                                  feather_radius_x,
                                  feather_radius_y);

  g_object_unref (mask);
}
