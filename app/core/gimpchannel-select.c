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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-mask-combine.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpchannel-select.h"
#include "gimpchannel-combine.h"
#include "gimpcontainer.h"
#include "gimpimage.h"
#include "gimpimage-new.h"
#include "gimppickable.h"
#include "gimppickable-contiguous-region.h"
#include "gimpscanconvert.h"

#include "vectors/gimppath.h"
#include "vectors/gimpstroke.h"

#include "gimp-intl.h"


/*  basic selection functions  */

void
gimp_channel_select_rectangle (GimpChannel    *channel,
                               gint            x,
                               gint            y,
                               gint            w,
                               gint            h,
                               GimpChannelOps  op,
                               gboolean        feather,
                               gdouble         feather_radius_x,
                               gdouble         feather_radius_y,
                               gboolean        push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));

  if (push_undo)
    gimp_channel_push_undo (channel, C_("undo-type", "Rectangle Select"));

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather)
    {
      GimpItem   *item = GIMP_ITEM (channel);
      GeglBuffer *add_on;

      add_on = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                gimp_item_get_width  (item),
                                                gimp_item_get_height (item)),
                                babl_format ("Y float"));

      gimp_gegl_mask_combine_rect (add_on, GIMP_CHANNEL_OP_REPLACE, x, y, w, h);

      gimp_gegl_apply_feather (add_on, NULL, NULL, add_on, NULL,
                               feather_radius_x,
                               feather_radius_y,
                               TRUE);

      gimp_channel_combine_buffer (channel, add_on, op, 0, 0);
      g_object_unref (add_on);
    }
  else
    {
      gimp_channel_combine_rect (channel, op, x, y, w, h);
    }
}

void
gimp_channel_select_ellipse (GimpChannel    *channel,
                             gint            x,
                             gint            y,
                             gint            w,
                             gint            h,
                             GimpChannelOps  op,
                             gboolean        antialias,
                             gboolean        feather,
                             gdouble         feather_radius_x,
                             gdouble         feather_radius_y,
                             gboolean        push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));

  if (push_undo)
    gimp_channel_push_undo (channel, C_("undo-type", "Ellipse Select"));

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather)
    {
      GimpItem   *item = GIMP_ITEM (channel);
      GeglBuffer *add_on;

      add_on = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                gimp_item_get_width  (item),
                                                gimp_item_get_height (item)),
                                babl_format ("Y float"));

      gimp_gegl_mask_combine_ellipse (add_on, GIMP_CHANNEL_OP_REPLACE,
                                      x, y, w, h, antialias);

      gimp_gegl_apply_feather (add_on, NULL, NULL, add_on, NULL,
                               feather_radius_x,
                               feather_radius_y,
                               TRUE);

      gimp_channel_combine_buffer (channel, add_on, op, 0, 0);
      g_object_unref (add_on);
    }
  else
    {
      gimp_channel_combine_ellipse (channel, op, x, y, w, h, antialias);
    }
}

void
gimp_channel_select_round_rect (GimpChannel         *channel,
                                gint                 x,
                                gint                 y,
                                gint                 w,
                                gint                 h,
                                gdouble              corner_radius_x,
                                gdouble              corner_radius_y,
                                GimpChannelOps       op,
                                gboolean             antialias,
                                gboolean             feather,
                                gdouble              feather_radius_x,
                                gdouble              feather_radius_y,
                                gboolean             push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));

  if (push_undo)
    gimp_channel_push_undo (channel, C_("undo-type", "Rounded Rectangle Select"));

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather)
    {
      GimpItem   *item = GIMP_ITEM (channel);
      GeglBuffer *add_on;

      add_on = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                gimp_item_get_width  (item),
                                                gimp_item_get_height (item)),
                                babl_format ("Y float"));

      gimp_gegl_mask_combine_ellipse_rect (add_on, GIMP_CHANNEL_OP_REPLACE,
                                           x, y, w, h,
                                           corner_radius_x, corner_radius_y,
                                           antialias);

      gimp_gegl_apply_feather (add_on, NULL, NULL, add_on, NULL,
                               feather_radius_x,
                               feather_radius_y,
                               TRUE);

      gimp_channel_combine_buffer (channel, add_on, op, 0, 0);
      g_object_unref (add_on);
    }
  else
    {
      gimp_channel_combine_ellipse_rect (channel, op, x, y, w, h,
                                         corner_radius_x, corner_radius_y,
                                         antialias);
    }
}

/*  select by GimpScanConvert functions  */

void
gimp_channel_select_scan_convert (GimpChannel     *channel,
                                  const gchar     *undo_desc,
                                  GimpScanConvert *scan_convert,
                                  gint             offset_x,
                                  gint             offset_y,
                                  GimpChannelOps   op,
                                  gboolean         antialias,
                                  gboolean         feather,
                                  gdouble          feather_radius_x,
                                  gdouble          feather_radius_y,
                                  gboolean         push_undo)
{
  GimpItem   *item;
  GeglBuffer *add_on;

  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (scan_convert != NULL);

  if (push_undo)
    gimp_channel_push_undo (channel, undo_desc);

  item = GIMP_ITEM (channel);

  add_on = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                            gimp_item_get_width  (item),
                                            gimp_item_get_height (item)),
                            babl_format ("Y float"));

  gimp_scan_convert_render (scan_convert, add_on,
                            offset_x, offset_y, antialias);

  if (feather)
    gimp_gegl_apply_feather (add_on, NULL, NULL, add_on, NULL,
                             feather_radius_x,
                             feather_radius_y,
                             TRUE);

  gimp_channel_combine_buffer (channel, add_on, op, 0, 0);
  g_object_unref (add_on);
}

void
gimp_channel_select_polygon (GimpChannel       *channel,
                             const gchar       *undo_desc,
                             gint               n_points,
                             const GimpVector2 *points,
                             GimpChannelOps     op,
                             gboolean           antialias,
                             gboolean           feather,
                             gdouble            feather_radius_x,
                             gdouble            feather_radius_y,
                             gboolean           push_undo)
{
  GimpScanConvert *scan_convert;

  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));
  g_return_if_fail (undo_desc != NULL);

  scan_convert = gimp_scan_convert_new ();

  gimp_scan_convert_add_polyline (scan_convert, n_points, points, TRUE);

  gimp_channel_select_scan_convert (channel, undo_desc, scan_convert, 0, 0,
                                    op, antialias, feather,
                                    feather_radius_x, feather_radius_y,
                                    push_undo);

  gimp_scan_convert_free (scan_convert);
}

void
gimp_channel_select_path (GimpChannel    *channel,
                          const gchar    *undo_desc,
                          GimpPath       *vectors,
                          GimpChannelOps  op,
                          gboolean        antialias,
                          gboolean        feather,
                          gdouble         feather_radius_x,
                          gdouble         feather_radius_y,
                          gboolean        push_undo)
{
  const GimpBezierDesc *bezier;

  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (GIMP_IS_PATH (vectors));

  bezier = gimp_path_get_bezier (vectors);

  if (bezier && bezier->num_data > 4)
    {
      GimpScanConvert *scan_convert;

      scan_convert = gimp_scan_convert_new ();
      gimp_scan_convert_add_bezier (scan_convert, bezier);

      gimp_channel_select_scan_convert (channel, undo_desc, scan_convert, 0, 0,
                                        op, antialias, feather,
                                        feather_radius_x, feather_radius_y,
                                        push_undo);

      gimp_scan_convert_free (scan_convert);
    }
}


/*  select by GimpChannel functions  */

void
gimp_channel_select_buffer (GimpChannel    *channel,
                            const gchar    *undo_desc,
                            GeglBuffer     *add_on,
                            gint            offset_x,
                            gint            offset_y,
                            GimpChannelOps  op,
                            gboolean        feather,
                            gdouble         feather_radius_x,
                            gdouble         feather_radius_y)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (GEGL_IS_BUFFER (add_on));

  gimp_channel_push_undo (channel, undo_desc);

  if (feather)
    {
      GimpItem   *item = GIMP_ITEM (channel);
      GeglBuffer *add_on2;

      add_on2 = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                 gimp_item_get_width  (item),
                                                 gimp_item_get_height (item)),
                                 babl_format ("Y float"));

      gimp_gegl_mask_combine_buffer (add_on2, add_on,
                                     GIMP_CHANNEL_OP_REPLACE,
                                     offset_x, offset_y);

      gimp_gegl_apply_feather (add_on2, NULL, NULL, add_on2, NULL,
                               feather_radius_x,
                               feather_radius_y,
                               TRUE);

      gimp_channel_combine_buffer (channel, add_on2, op, 0, 0);
      g_object_unref (add_on2);
    }
  else
    {
      gimp_channel_combine_buffer (channel, add_on, op, offset_x, offset_y);
    }
}

void
gimp_channel_select_channel (GimpChannel    *channel,
                             const gchar    *undo_desc,
                             GimpChannel    *add_on,
                             gint            offset_x,
                             gint            offset_y,
                             GimpChannelOps  op,
                             gboolean        feather,
                             gdouble         feather_radius_x,
                             gdouble         feather_radius_y)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (GIMP_IS_CHANNEL (add_on));

  gimp_channel_select_buffer (channel, undo_desc,
                              gimp_drawable_get_buffer (GIMP_DRAWABLE (add_on)),
                              offset_x, offset_y, op,
                              feather,
                              feather_radius_x, feather_radius_y);
}

void
gimp_channel_select_alpha (GimpChannel    *channel,
                           GimpDrawable   *drawable,
                           GimpChannelOps  op,
                           gboolean        feather,
                           gdouble         feather_radius_x,
                           gdouble         feather_radius_y)
{
  GimpItem    *item;
  GimpChannel *add_on;
  gint         off_x, off_y;
  const gchar *undo_desc = NULL;

  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  item = GIMP_ITEM (channel);

  if (gimp_drawable_has_alpha (drawable))
    {
      add_on = gimp_channel_new_from_alpha (gimp_item_get_image (item),
                                            drawable, NULL, NULL);
    }
  else
    {
      /*  no alpha is equivalent to completely opaque alpha,
       *  so simply select the whole layer's extents.  --mitch
       */
      add_on = gimp_channel_new_mask (gimp_item_get_image (item),
                                      gimp_item_get_width  (GIMP_ITEM (drawable)),
                                      gimp_item_get_height (GIMP_ITEM (drawable)));
      gimp_channel_all (add_on, FALSE);
    }

  switch (op)
    {
    case GIMP_CHANNEL_OP_ADD:
      undo_desc = C_("undo-type", "Add Alpha to Selection");
      break;
    case GIMP_CHANNEL_OP_SUBTRACT:
      undo_desc = C_("undo-type", "Subtract Alpha from Selection");
      break;
    case GIMP_CHANNEL_OP_REPLACE:
      undo_desc = C_("undo-type", "Alpha to Selection");
      break;
    case GIMP_CHANNEL_OP_INTERSECT:
      undo_desc = C_("undo-type", "Intersect Alpha with Selection");
      break;
    }

  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

  gimp_channel_select_channel (channel, undo_desc, add_on,
                               off_x, off_y,
                               op, feather,
                               feather_radius_x,
                               feather_radius_y);
  g_object_unref (add_on);
}

void
gimp_channel_select_component (GimpChannel     *channel,
                               GimpChannelType  component,
                               GimpChannelOps   op,
                               gboolean         feather,
                               gdouble          feather_radius_x,
                               gdouble          feather_radius_y)
{
  GimpItem    *item;
  GimpChannel *add_on;
  const gchar *desc;
  gchar       *undo_desc;

  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));

  item = GIMP_ITEM (channel);

  add_on = gimp_channel_new_from_component (gimp_item_get_image (item),
                                            component, NULL, NULL);

  if (feather)
    gimp_channel_feather (add_on,
                          feather_radius_x,
                          feather_radius_y,
                          TRUE,
                          FALSE /* no undo */);

  gimp_enum_get_value (GIMP_TYPE_CHANNEL_TYPE, component,
                       NULL, NULL, &desc, NULL);

  undo_desc = g_strdup_printf (C_("undo-type", "%s Channel to Selection"), desc);

  gimp_channel_select_channel (channel, undo_desc, add_on,
                               0, 0, op,
                               FALSE, 0.0, 0.0);

  g_free (undo_desc);
  g_object_unref (add_on);
}

void
gimp_channel_select_fuzzy (GimpChannel         *channel,
                           GimpDrawable        *drawable,
                           gboolean             sample_merged,
                           gint                 x,
                           gint                 y,
                           gfloat               threshold,
                           gboolean             select_transparent,
                           GimpSelectCriterion  select_criterion,
                           gboolean             diagonal_neighbors,
                           GimpChannelOps       op,
                           gboolean             antialias,
                           gboolean             feather,
                           gdouble              feather_radius_x,
                           gdouble              feather_radius_y)
{
  GimpPickable *pickable;
  GeglBuffer   *add_on;
  gint          add_on_x = 0;
  gint          add_on_y = 0;

  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  if (sample_merged)
    pickable = GIMP_PICKABLE (gimp_item_get_image (GIMP_ITEM (drawable)));
  else
    pickable = GIMP_PICKABLE (drawable);

  add_on = gimp_pickable_contiguous_region_by_seed (pickable,
                                                    antialias,
                                                    threshold,
                                                    select_transparent,
                                                    select_criterion,
                                                    diagonal_neighbors,
                                                    x, y);

  if (! sample_merged)
    gimp_item_get_offset (GIMP_ITEM (drawable), &add_on_x, &add_on_y);

  gimp_channel_select_buffer (channel, C_("undo-type", "Fuzzy Select"),
                              add_on, add_on_x, add_on_y,
                              op,
                              feather,
                              feather_radius_x,
                              feather_radius_y);
  g_object_unref (add_on);
}

void
gimp_channel_select_by_color (GimpChannel         *channel,
                              GList               *drawables,
                              gboolean             sample_merged,
                              GeglColor           *color,
                              gfloat               threshold,
                              gboolean             select_transparent,
                              GimpSelectCriterion  select_criterion,
                              GimpChannelOps       op,
                              gboolean             antialias,
                              gboolean             feather,
                              gdouble              feather_radius_x,
                              gdouble              feather_radius_y)
{
  GimpPickable *pickable;
  GeglBuffer   *add_on;
  GimpImage    *image;
  GimpImage    *sel_image = NULL;
  gint          add_on_x  = 0;
  gint          add_on_y  = 0;

  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));
  g_return_if_fail (drawables != NULL);
  g_return_if_fail (color != NULL);

  image = gimp_item_get_image (drawables->data);
  if (sample_merged)
    {
      pickable = GIMP_PICKABLE (image);
    }
  else
    {
      if (g_list_length (drawables) == 1)
        {
          pickable = GIMP_PICKABLE (drawables->data);
        }
      else
        {
          sel_image = gimp_image_new_from_drawables (image->gimp, drawables, FALSE, FALSE);
          gimp_container_remove (image->gimp->images, GIMP_OBJECT (sel_image));

          pickable = GIMP_PICKABLE (sel_image);
          gimp_pickable_flush (pickable);
        }
    }

  add_on = gimp_pickable_contiguous_region_by_color (pickable,
                                                     antialias,
                                                     threshold,
                                                     select_transparent,
                                                     select_criterion,
                                                     color);

  if (! sample_merged && ! sel_image)
    gimp_item_get_offset (GIMP_ITEM (drawables->data), &add_on_x, &add_on_y);

  gimp_channel_select_buffer (channel, C_("undo-type", "Select by Color"),
                              add_on, add_on_x, add_on_y,
                              op,
                              feather,
                              feather_radius_x,
                              feather_radius_y);

  g_object_unref (add_on);
  if (sel_image)
    g_object_unref (sel_image);

}

void
gimp_channel_select_by_index (GimpChannel    *channel,
                              GimpDrawable   *drawable,
                              gint            index,
                              GimpChannelOps  op,
                              gboolean        feather,
                              gdouble         feather_radius_x,
                              gdouble         feather_radius_y)
{
  GeglBuffer *add_on;
  gint        add_on_x = 0;
  gint        add_on_y = 0;

  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (channel)));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_drawable_is_indexed (drawable));

  add_on = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                            gimp_item_get_width  (GIMP_ITEM (drawable)),
                                            gimp_item_get_height (GIMP_ITEM (drawable))),
                            babl_format ("Y float"));

  gimp_gegl_index_to_mask (gimp_drawable_get_buffer (drawable), NULL,
                           gimp_drawable_get_format_without_alpha (drawable),
                           add_on, NULL,
                           index);

  gimp_item_get_offset (GIMP_ITEM (drawable), &add_on_x, &add_on_y);

  gimp_channel_select_buffer (channel, C_("undo-type", "Select by Indexed Color"),
                              add_on, add_on_x, add_on_y,
                              op,
                              feather,
                              feather_radius_x,
                              feather_radius_y);
  g_object_unref (add_on);
}
