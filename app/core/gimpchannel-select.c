/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "gegl/ligma-gegl-apply-operation.h"
#include "gegl/ligma-gegl-loops.h"
#include "gegl/ligma-gegl-mask-combine.h"

#include "ligma.h"
#include "ligmachannel.h"
#include "ligmachannel-select.h"
#include "ligmachannel-combine.h"
#include "ligmacontainer.h"
#include "ligmaimage.h"
#include "ligmaimage-new.h"
#include "ligmapickable.h"
#include "ligmapickable-contiguous-region.h"
#include "ligmascanconvert.h"

#include "vectors/ligmastroke.h"
#include "vectors/ligmavectors.h"

#include "ligma-intl.h"


/*  basic selection functions  */

void
ligma_channel_select_rectangle (LigmaChannel    *channel,
                               gint            x,
                               gint            y,
                               gint            w,
                               gint            h,
                               LigmaChannelOps  op,
                               gboolean        feather,
                               gdouble         feather_radius_x,
                               gdouble         feather_radius_y,
                               gboolean        push_undo)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)));

  if (push_undo)
    ligma_channel_push_undo (channel, C_("undo-type", "Rectangle Select"));

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather)
    {
      LigmaItem   *item = LIGMA_ITEM (channel);
      GeglBuffer *add_on;

      add_on = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                ligma_item_get_width  (item),
                                                ligma_item_get_height (item)),
                                babl_format ("Y float"));

      ligma_gegl_mask_combine_rect (add_on, LIGMA_CHANNEL_OP_REPLACE, x, y, w, h);

      ligma_gegl_apply_feather (add_on, NULL, NULL, add_on, NULL,
                               feather_radius_x,
                               feather_radius_y,
                               TRUE);

      ligma_channel_combine_buffer (channel, add_on, op, 0, 0);
      g_object_unref (add_on);
    }
  else
    {
      ligma_channel_combine_rect (channel, op, x, y, w, h);
    }
}

void
ligma_channel_select_ellipse (LigmaChannel    *channel,
                             gint            x,
                             gint            y,
                             gint            w,
                             gint            h,
                             LigmaChannelOps  op,
                             gboolean        antialias,
                             gboolean        feather,
                             gdouble         feather_radius_x,
                             gdouble         feather_radius_y,
                             gboolean        push_undo)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)));

  if (push_undo)
    ligma_channel_push_undo (channel, C_("undo-type", "Ellipse Select"));

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather)
    {
      LigmaItem   *item = LIGMA_ITEM (channel);
      GeglBuffer *add_on;

      add_on = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                ligma_item_get_width  (item),
                                                ligma_item_get_height (item)),
                                babl_format ("Y float"));

      ligma_gegl_mask_combine_ellipse (add_on, LIGMA_CHANNEL_OP_REPLACE,
                                      x, y, w, h, antialias);

      ligma_gegl_apply_feather (add_on, NULL, NULL, add_on, NULL,
                               feather_radius_x,
                               feather_radius_y,
                               TRUE);

      ligma_channel_combine_buffer (channel, add_on, op, 0, 0);
      g_object_unref (add_on);
    }
  else
    {
      ligma_channel_combine_ellipse (channel, op, x, y, w, h, antialias);
    }
}

void
ligma_channel_select_round_rect (LigmaChannel         *channel,
                                gint                 x,
                                gint                 y,
                                gint                 w,
                                gint                 h,
                                gdouble              corner_radius_x,
                                gdouble              corner_radius_y,
                                LigmaChannelOps       op,
                                gboolean             antialias,
                                gboolean             feather,
                                gdouble              feather_radius_x,
                                gdouble              feather_radius_y,
                                gboolean             push_undo)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)));

  if (push_undo)
    ligma_channel_push_undo (channel, C_("undo-type", "Rounded Rectangle Select"));

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather)
    {
      LigmaItem   *item = LIGMA_ITEM (channel);
      GeglBuffer *add_on;

      add_on = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                ligma_item_get_width  (item),
                                                ligma_item_get_height (item)),
                                babl_format ("Y float"));

      ligma_gegl_mask_combine_ellipse_rect (add_on, LIGMA_CHANNEL_OP_REPLACE,
                                           x, y, w, h,
                                           corner_radius_x, corner_radius_y,
                                           antialias);

      ligma_gegl_apply_feather (add_on, NULL, NULL, add_on, NULL,
                               feather_radius_x,
                               feather_radius_y,
                               TRUE);

      ligma_channel_combine_buffer (channel, add_on, op, 0, 0);
      g_object_unref (add_on);
    }
  else
    {
      ligma_channel_combine_ellipse_rect (channel, op, x, y, w, h,
                                         corner_radius_x, corner_radius_y,
                                         antialias);
    }
}

/*  select by LigmaScanConvert functions  */

void
ligma_channel_select_scan_convert (LigmaChannel     *channel,
                                  const gchar     *undo_desc,
                                  LigmaScanConvert *scan_convert,
                                  gint             offset_x,
                                  gint             offset_y,
                                  LigmaChannelOps   op,
                                  gboolean         antialias,
                                  gboolean         feather,
                                  gdouble          feather_radius_x,
                                  gdouble          feather_radius_y,
                                  gboolean         push_undo)
{
  LigmaItem   *item;
  GeglBuffer *add_on;

  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (scan_convert != NULL);

  if (push_undo)
    ligma_channel_push_undo (channel, undo_desc);

  item = LIGMA_ITEM (channel);

  add_on = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                            ligma_item_get_width  (item),
                                            ligma_item_get_height (item)),
                            babl_format ("Y float"));

  ligma_scan_convert_render (scan_convert, add_on,
                            offset_x, offset_y, antialias);

  if (feather)
    ligma_gegl_apply_feather (add_on, NULL, NULL, add_on, NULL,
                             feather_radius_x,
                             feather_radius_y,
                             TRUE);

  ligma_channel_combine_buffer (channel, add_on, op, 0, 0);
  g_object_unref (add_on);
}

void
ligma_channel_select_polygon (LigmaChannel       *channel,
                             const gchar       *undo_desc,
                             gint               n_points,
                             const LigmaVector2 *points,
                             LigmaChannelOps     op,
                             gboolean           antialias,
                             gboolean           feather,
                             gdouble            feather_radius_x,
                             gdouble            feather_radius_y,
                             gboolean           push_undo)
{
  LigmaScanConvert *scan_convert;

  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)));
  g_return_if_fail (undo_desc != NULL);

  scan_convert = ligma_scan_convert_new ();

  ligma_scan_convert_add_polyline (scan_convert, n_points, points, TRUE);

  ligma_channel_select_scan_convert (channel, undo_desc, scan_convert, 0, 0,
                                    op, antialias, feather,
                                    feather_radius_x, feather_radius_y,
                                    push_undo);

  ligma_scan_convert_free (scan_convert);
}

void
ligma_channel_select_vectors (LigmaChannel    *channel,
                             const gchar    *undo_desc,
                             LigmaVectors    *vectors,
                             LigmaChannelOps  op,
                             gboolean        antialias,
                             gboolean        feather,
                             gdouble         feather_radius_x,
                             gdouble         feather_radius_y,
                             gboolean        push_undo)
{
  const LigmaBezierDesc *bezier;

  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (LIGMA_IS_VECTORS (vectors));

  bezier = ligma_vectors_get_bezier (vectors);

  if (bezier && bezier->num_data > 4)
    {
      LigmaScanConvert *scan_convert;

      scan_convert = ligma_scan_convert_new ();
      ligma_scan_convert_add_bezier (scan_convert, bezier);

      ligma_channel_select_scan_convert (channel, undo_desc, scan_convert, 0, 0,
                                        op, antialias, feather,
                                        feather_radius_x, feather_radius_y,
                                        push_undo);

      ligma_scan_convert_free (scan_convert);
    }
}


/*  select by LigmaChannel functions  */

void
ligma_channel_select_buffer (LigmaChannel    *channel,
                            const gchar    *undo_desc,
                            GeglBuffer     *add_on,
                            gint            offset_x,
                            gint            offset_y,
                            LigmaChannelOps  op,
                            gboolean        feather,
                            gdouble         feather_radius_x,
                            gdouble         feather_radius_y)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (GEGL_IS_BUFFER (add_on));

  ligma_channel_push_undo (channel, undo_desc);

  if (feather)
    {
      LigmaItem   *item = LIGMA_ITEM (channel);
      GeglBuffer *add_on2;

      add_on2 = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                 ligma_item_get_width  (item),
                                                 ligma_item_get_height (item)),
                                 babl_format ("Y float"));

      ligma_gegl_mask_combine_buffer (add_on2, add_on,
                                     LIGMA_CHANNEL_OP_REPLACE,
                                     offset_x, offset_y);

      ligma_gegl_apply_feather (add_on2, NULL, NULL, add_on2, NULL,
                               feather_radius_x,
                               feather_radius_y,
                               TRUE);

      ligma_channel_combine_buffer (channel, add_on2, op, 0, 0);
      g_object_unref (add_on2);
    }
  else
    {
      ligma_channel_combine_buffer (channel, add_on, op, offset_x, offset_y);
    }
}

void
ligma_channel_select_channel (LigmaChannel    *channel,
                             const gchar    *undo_desc,
                             LigmaChannel    *add_on,
                             gint            offset_x,
                             gint            offset_y,
                             LigmaChannelOps  op,
                             gboolean        feather,
                             gdouble         feather_radius_x,
                             gdouble         feather_radius_y)
{
  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (LIGMA_IS_CHANNEL (add_on));

  ligma_channel_select_buffer (channel, undo_desc,
                              ligma_drawable_get_buffer (LIGMA_DRAWABLE (add_on)),
                              offset_x, offset_y, op,
                              feather,
                              feather_radius_x, feather_radius_y);
}

void
ligma_channel_select_alpha (LigmaChannel    *channel,
                           LigmaDrawable   *drawable,
                           LigmaChannelOps  op,
                           gboolean        feather,
                           gdouble         feather_radius_x,
                           gdouble         feather_radius_y)
{
  LigmaItem    *item;
  LigmaChannel *add_on;
  gint         off_x, off_y;
  const gchar *undo_desc = NULL;

  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)));
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));

  item = LIGMA_ITEM (channel);

  if (ligma_drawable_has_alpha (drawable))
    {
      add_on = ligma_channel_new_from_alpha (ligma_item_get_image (item),
                                            drawable, NULL, NULL);
    }
  else
    {
      /*  no alpha is equivalent to completely opaque alpha,
       *  so simply select the whole layer's extents.  --mitch
       */
      add_on = ligma_channel_new_mask (ligma_item_get_image (item),
                                      ligma_item_get_width  (LIGMA_ITEM (drawable)),
                                      ligma_item_get_height (LIGMA_ITEM (drawable)));
      ligma_channel_all (add_on, FALSE);
    }

  switch (op)
    {
    case LIGMA_CHANNEL_OP_ADD:
      undo_desc = C_("undo-type", "Add Alpha to Selection");
      break;
    case LIGMA_CHANNEL_OP_SUBTRACT:
      undo_desc = C_("undo-type", "Subtract Alpha from Selection");
      break;
    case LIGMA_CHANNEL_OP_REPLACE:
      undo_desc = C_("undo-type", "Alpha to Selection");
      break;
    case LIGMA_CHANNEL_OP_INTERSECT:
      undo_desc = C_("undo-type", "Intersect Alpha with Selection");
      break;
    }

  ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

  ligma_channel_select_channel (channel, undo_desc, add_on,
                               off_x, off_y,
                               op, feather,
                               feather_radius_x,
                               feather_radius_y);
  g_object_unref (add_on);
}

void
ligma_channel_select_component (LigmaChannel     *channel,
                               LigmaChannelType  component,
                               LigmaChannelOps   op,
                               gboolean         feather,
                               gdouble          feather_radius_x,
                               gdouble          feather_radius_y)
{
  LigmaItem    *item;
  LigmaChannel *add_on;
  const gchar *desc;
  gchar       *undo_desc;

  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)));

  item = LIGMA_ITEM (channel);

  add_on = ligma_channel_new_from_component (ligma_item_get_image (item),
                                            component, NULL, NULL);

  if (feather)
    ligma_channel_feather (add_on,
                          feather_radius_x,
                          feather_radius_y,
                          TRUE,
                          FALSE /* no undo */);

  ligma_enum_get_value (LIGMA_TYPE_CHANNEL_TYPE, component,
                       NULL, NULL, &desc, NULL);

  undo_desc = g_strdup_printf (C_("undo-type", "%s Channel to Selection"), desc);

  ligma_channel_select_channel (channel, undo_desc, add_on,
                               0, 0, op,
                               FALSE, 0.0, 0.0);

  g_free (undo_desc);
  g_object_unref (add_on);
}

void
ligma_channel_select_fuzzy (LigmaChannel         *channel,
                           LigmaDrawable        *drawable,
                           gboolean             sample_merged,
                           gint                 x,
                           gint                 y,
                           gfloat               threshold,
                           gboolean             select_transparent,
                           LigmaSelectCriterion  select_criterion,
                           gboolean             diagonal_neighbors,
                           LigmaChannelOps       op,
                           gboolean             antialias,
                           gboolean             feather,
                           gdouble              feather_radius_x,
                           gdouble              feather_radius_y)
{
  LigmaPickable *pickable;
  GeglBuffer   *add_on;
  gint          add_on_x = 0;
  gint          add_on_y = 0;

  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)));
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));

  if (sample_merged)
    pickable = LIGMA_PICKABLE (ligma_item_get_image (LIGMA_ITEM (drawable)));
  else
    pickable = LIGMA_PICKABLE (drawable);

  add_on = ligma_pickable_contiguous_region_by_seed (pickable,
                                                    antialias,
                                                    threshold,
                                                    select_transparent,
                                                    select_criterion,
                                                    diagonal_neighbors,
                                                    x, y);

  if (! sample_merged)
    ligma_item_get_offset (LIGMA_ITEM (drawable), &add_on_x, &add_on_y);

  ligma_channel_select_buffer (channel, C_("undo-type", "Fuzzy Select"),
                              add_on, add_on_x, add_on_y,
                              op,
                              feather,
                              feather_radius_x,
                              feather_radius_y);
  g_object_unref (add_on);
}

void
ligma_channel_select_by_color (LigmaChannel         *channel,
                              GList               *drawables,
                              gboolean             sample_merged,
                              const LigmaRGB       *color,
                              gfloat               threshold,
                              gboolean             select_transparent,
                              LigmaSelectCriterion  select_criterion,
                              LigmaChannelOps       op,
                              gboolean             antialias,
                              gboolean             feather,
                              gdouble              feather_radius_x,
                              gdouble              feather_radius_y)
{
  LigmaPickable *pickable;
  GeglBuffer   *add_on;
  LigmaImage    *image;
  LigmaImage    *sel_image = NULL;
  gint          add_on_x  = 0;
  gint          add_on_y  = 0;

  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)));
  g_return_if_fail (drawables != NULL);
  g_return_if_fail (color != NULL);

  image = ligma_item_get_image (drawables->data);
  if (sample_merged)
    {
      pickable = LIGMA_PICKABLE (image);
    }
  else
    {
      if (g_list_length (drawables) == 1)
        {
          pickable = LIGMA_PICKABLE (drawables->data);
        }
      else
        {
          sel_image = ligma_image_new_from_drawables (image->ligma, drawables, FALSE, FALSE);
          ligma_container_remove (image->ligma->images, LIGMA_OBJECT (sel_image));

          pickable = LIGMA_PICKABLE (sel_image);
          ligma_pickable_flush (pickable);
        }
    }

  add_on = ligma_pickable_contiguous_region_by_color (pickable,
                                                     antialias,
                                                     threshold,
                                                     select_transparent,
                                                     select_criterion,
                                                     color);

  if (! sample_merged && ! sel_image)
    ligma_item_get_offset (LIGMA_ITEM (drawables->data), &add_on_x, &add_on_y);

  ligma_channel_select_buffer (channel, C_("undo-type", "Select by Color"),
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
ligma_channel_select_by_index (LigmaChannel    *channel,
                              LigmaDrawable   *drawable,
                              gint            index,
                              LigmaChannelOps  op,
                              gboolean        feather,
                              gdouble         feather_radius_x,
                              gdouble         feather_radius_y)
{
  GeglBuffer *add_on;
  gint        add_on_x = 0;
  gint        add_on_y = 0;

  g_return_if_fail (LIGMA_IS_CHANNEL (channel));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (channel)));
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_drawable_is_indexed (drawable));

  add_on = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                            ligma_item_get_width  (LIGMA_ITEM (drawable)),
                                            ligma_item_get_height (LIGMA_ITEM (drawable))),
                            babl_format ("Y float"));

  ligma_gegl_index_to_mask (ligma_drawable_get_buffer (drawable), NULL,
                           ligma_drawable_get_format_without_alpha (drawable),
                           add_on, NULL,
                           index);

  ligma_item_get_offset (LIGMA_ITEM (drawable), &add_on_x, &add_on_y);

  ligma_channel_select_buffer (channel, C_("undo-type", "Select by Indexed Color"),
                              add_on, add_on_x, add_on_y,
                              op,
                              feather,
                              feather_radius_x,
                              feather_radius_y);
  g_object_unref (add_on);
}
