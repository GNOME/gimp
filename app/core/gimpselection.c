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

#include <string.h>

#include <glib-object.h>

#include "core-types.h"

#include "paint-funcs/paint-funcs.h"

#include "base/pixel-region.h"
#include "base/tile.h"
#include "base/tile-manager.h"

#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimpselection.h"

#include "gimp-intl.h"


static void       gimp_selection_class_init    (GimpSelectionClass *klass);
static void       gimp_selection_init          (GimpSelection      *selection);

static void       gimp_selection_translate     (GimpItem        *item,
                                                gint             offset_x,
                                                gint             offset_y,
                                                gboolean         push_undo);
static void       gimp_selection_scale         (GimpItem        *item,
                                                gint             new_width,
                                                gint             new_height,
                                                gint             new_offset_x,
                                                gint             new_offset_y,
                                                GimpInterpolationType interp_type);
static void       gimp_selection_resize        (GimpItem        *item,
                                                gint             new_width,
                                                gint             new_height,
                                                gint             off_x,
                                                gint             off_y);
static void       gimp_selection_flip          (GimpItem        *item,
                                                GimpOrientationType flip_type,
                                                gdouble          axis,
                                                gboolean         clip_result);
static void       gimp_selection_rotate        (GimpItem        *item,
                                                GimpRotationType rotation_type,
                                                gdouble          center_x,
                                                gdouble          center_y,
                                                gboolean         clip_result);
static gboolean   gimp_selection_stroke        (GimpItem        *item,
                                                GimpDrawable    *drawable,
                                                GimpPaintInfo   *paint_info);

static void gimp_selection_invalidate_boundary (GimpDrawable    *drawable);

static gboolean   gimp_selection_boundary      (GimpChannel     *channel,
                                                const BoundSeg **segs_in,
                                                const BoundSeg **segs_out,
                                                gint            *num_segs_in,
                                                gint            *num_segs_out,
                                                gint             x1,
                                                gint             y1,
                                                gint             x2,
                                                gint             y2);
static gboolean   gimp_selection_bounds        (GimpChannel     *channel,
                                                gint            *x1,
                                                gint            *y1,
                                                gint            *x2,
                                                gint            *y2);
static gboolean   gimp_selection_is_empty      (GimpChannel     *channel);
static gint       gimp_selection_value         (GimpChannel     *channel,
                                                gint             x,
                                                gint             y);
static void       gimp_selection_feather       (GimpChannel     *channel,
                                                gdouble          radius_x,
                                                gdouble          radius_y,
                                                gboolean         push_undo);
static void       gimp_selection_sharpen       (GimpChannel     *channel,
                                                gboolean         push_undo);
static void       gimp_selection_clear         (GimpChannel     *channel,
                                                const gchar     *undo_desc,
                                                gboolean         push_undo);
static void       gimp_selection_all           (GimpChannel     *channel,
                                                gboolean         push_undo);
static void       gimp_selection_invert        (GimpChannel     *channel,
                                                gboolean         push_undo);
static void       gimp_selection_border        (GimpChannel     *channel,
                                                gint             radius_x,
                                                gint             radius_y,
                                                gboolean         push_undo);
static void       gimp_selection_grow          (GimpChannel     *channel,
                                                gint             radius_x,
                                                gint             radius_y,
                                                gboolean         push_undo);
static void       gimp_selection_shrink        (GimpChannel     *channel,
                                                gint             radius_x,
                                                gint             radius_y,
                                                gboolean         edge_lock,
                                                gboolean         push_undo);

static void       gimp_selection_changed       (GimpChannel     *selection);
static void       gimp_selection_validate      (TileManager     *tm,
                                                Tile            *tile);


static GimpChannelClass *parent_class = NULL;


GType
gimp_selection_get_type (void)
{
  static GType selection_type = 0;

  if (! selection_type)
    {
      static const GTypeInfo selection_info =
      {
        sizeof (GimpSelectionClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_selection_class_init,
        NULL,		/* class_finalize */
        NULL,		/* class_data     */
        sizeof (GimpSelection),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_selection_init,
      };

      selection_type = g_type_register_static (GIMP_TYPE_CHANNEL,
                                               "GimpSelection",
                                               &selection_info, 0);
    }

  return selection_type;
}

static void
gimp_selection_class_init (GimpSelectionClass *klass)
{
  GimpItemClass     *item_class;
  GimpDrawableClass *drawable_class;
  GimpChannelClass  *channel_class;

  item_class     = GIMP_ITEM_CLASS (klass);
  drawable_class = GIMP_DRAWABLE_CLASS (klass);
  channel_class  = GIMP_CHANNEL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  item_class->translate               = gimp_selection_translate;
  item_class->scale                   = gimp_selection_scale;
  item_class->resize                  = gimp_selection_resize;
  item_class->flip                    = gimp_selection_flip;
  item_class->rotate                  = gimp_selection_rotate;
  item_class->stroke                  = gimp_selection_stroke;

  drawable_class->invalidate_boundary = gimp_selection_invalidate_boundary;

  channel_class->boundary             = gimp_selection_boundary;
  channel_class->bounds               = gimp_selection_bounds;
  channel_class->is_empty             = gimp_selection_is_empty;
  channel_class->value                = gimp_selection_value;
  channel_class->feather              = gimp_selection_feather;
  channel_class->sharpen              = gimp_selection_sharpen;
  channel_class->clear                = gimp_selection_clear;
  channel_class->all                  = gimp_selection_all;
  channel_class->invert               = gimp_selection_invert;
  channel_class->border               = gimp_selection_border;
  channel_class->grow                 = gimp_selection_grow;
  channel_class->shrink               = gimp_selection_shrink;

  channel_class->translate_desc       = _("Move Selection");
  channel_class->feather_desc         = _("Feather Selection");
  channel_class->sharpen_desc         = _("Sharpen Selection");
  channel_class->clear_desc           = _("Select None");
  channel_class->all_desc             = _("Select All");
  channel_class->invert_desc          = _("Invert Selection");
  channel_class->border_desc          = _("Border Selection");
  channel_class->grow_desc            = _("Grow Selection");
  channel_class->shrink_desc          = _("Shrink Selection");
}

static void
gimp_selection_init (GimpSelection *selection)
{
  selection->stroking = FALSE;
}

static void
gimp_selection_translate (GimpItem *item,
                          gint      offset_x,
                          gint      offset_y,
                          gboolean  push_undo)
{
  GIMP_ITEM_CLASS (parent_class)->translate (item, offset_x, offset_y,
                                             push_undo);

  gimp_selection_changed (GIMP_CHANNEL (item));
}

static void
gimp_selection_scale (GimpItem              *item,
                      gint                   new_width,
                      gint                   new_height,
                      gint                   new_offset_x,
                      gint                   new_offset_y,
                      GimpInterpolationType  interp_type)
{
  GIMP_ITEM_CLASS (parent_class)->scale (item, new_width, new_height,
                                         new_offset_x, new_offset_y,
                                         interp_type);

  item->offset_x = 0;
  item->offset_y = 0;

  gimp_selection_changed (GIMP_CHANNEL (item));
}

static void
gimp_selection_resize (GimpItem *item,
                       gint      new_width,
                       gint      new_height,
                       gint      off_x,
                       gint      off_y)
{
  GIMP_ITEM_CLASS (parent_class)->resize (item, new_width, new_height,
                                          off_x, off_y);

  item->offset_x = 0;
  item->offset_y = 0;

  gimp_selection_changed (GIMP_CHANNEL (item));
}

static void
gimp_selection_flip (GimpItem            *item,
                     GimpOrientationType  flip_type,
                     gdouble              axis,
                     gboolean             clip_result)
{
  GIMP_ITEM_CLASS (parent_class)->flip (item, flip_type, axis, TRUE);

  gimp_selection_changed (GIMP_CHANNEL (item));
}

static void
gimp_selection_rotate (GimpItem        *item,
                       GimpRotationType rotation_type,
                       gdouble          center_x,
                       gdouble          center_y,
                       gboolean         clip_result)
{
  GIMP_ITEM_CLASS (parent_class)->rotate (item, rotation_type,
                                          center_x, center_y,
                                          clip_result);

  gimp_selection_changed (GIMP_CHANNEL (item));
}

static gboolean
gimp_selection_stroke (GimpItem         *item,
                       GimpDrawable     *drawable,
                       GimpPaintInfo    *paint_info)
{
  GimpSelection  *selection;
  GimpImage      *gimage;
  const BoundSeg *dummy_in;
  const BoundSeg *dummy_out;
  gint            num_dummy_in;
  gint            num_dummy_out;
  gboolean        retval;

  selection = GIMP_SELECTION (item);

  if (! gimp_channel_boundary (GIMP_CHANNEL (selection),
                               &dummy_in, &dummy_out,
                               &num_dummy_in, &num_dummy_out,
                               0, 0, 0, 0))
    {
      g_message (_("No selection to stroke."));
      return FALSE;
    }

  gimage = gimp_item_get_image (item);

  selection->stroking = TRUE;

  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_PAINT,
                               _("Stroke Selection"));

  retval = GIMP_ITEM_CLASS (parent_class)->stroke (item, drawable, paint_info);

  gimp_image_undo_group_end (gimage);

  selection->stroking = FALSE;

  return retval;
}

static void
gimp_selection_invalidate_boundary (GimpDrawable *drawable)
{
  GimpChannel *selection;
  GimpLayer   *layer;
  GimpImage   *gimage;

  selection = GIMP_CHANNEL (drawable);

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  Turn the current selection off  */
  gimp_image_selection_control (gimage, GIMP_SELECTION_OFF);

  GIMP_DRAWABLE_CLASS (parent_class)->invalidate_boundary (drawable);

  /*  If there is a floating selection, update it's area...
   *  we need to do this since this selection mask can act as an additional
   *  mask in the composition of the floating selection
   */
  layer = gimp_image_get_active_layer (gimage);

  if (layer && gimp_layer_is_floating_sel (layer))
    gimp_drawable_update (GIMP_DRAWABLE (layer),
			  0, 0,
			  GIMP_ITEM (layer)->width,
			  GIMP_ITEM (layer)->height);

  /*  invalidate the preview  */
  drawable->preview_valid = FALSE;  
}

static gboolean
gimp_selection_boundary (GimpChannel     *channel,
                         const BoundSeg **segs_in,
                         const BoundSeg **segs_out,
                         gint            *num_segs_in,
                         gint            *num_segs_out,
                         gint             unused1,
                         gint             unused2,
                         gint             unused3,
                         gint             unused4)
{
  GimpImage    *gimage;
  GimpDrawable *drawable;
  GimpLayer    *layer;

  gimage = gimp_item_get_image (GIMP_ITEM (channel));

  if ((layer = gimp_image_floating_sel (gimage)))
    {
      /*  If there is a floating selection, then
       *  we need to do some slightly different boundaries.
       *  Instead of inside and outside boundaries being defined
       *  by the extents of the layer, the inside boundary (the one
       *  that actually marches and is black/white) is the boundary of
       *  the floating selection.  The outside boundary (doesn't move,
       *  is black/gray) is defined as the normal selection mask
       */

      /*  Find the selection mask boundary  */
      GIMP_CHANNEL_CLASS (parent_class)->boundary (channel,
                                                   segs_in, segs_out,
                                                   num_segs_in, num_segs_out,
                                                   0, 0, 0, 0);

      /*  Find the floating selection boundary  */
      *segs_in = floating_sel_boundary (layer, num_segs_in);

      return TRUE;
    }
  else if ((drawable = gimp_image_active_drawable (gimage)) &&
	   GIMP_IS_CHANNEL (drawable))
    {
      /*  Otherwise, return the boundary...if a channel is active  */

      return GIMP_CHANNEL_CLASS (parent_class)->boundary (channel,
                                                          segs_in, segs_out,
                                                          num_segs_in,
                                                          num_segs_out,
                                                          0, 0,
                                                          gimage->width,
                                                          gimage->height);
    }
  else if ((layer = gimp_image_get_active_layer (gimage)))
    {
      /*  If a layer is active, we return multiple boundaries based
       *  on the extents
       */

      gint x1, y1;
      gint x2, y2;
      gint off_x, off_y;

      gimp_item_offsets (GIMP_ITEM (layer), &off_x, &off_y);

      x1 = CLAMP (off_x, 0, gimage->width);
      y1 = CLAMP (off_y, 0, gimage->height);
      x2 = CLAMP (off_x + gimp_item_width (GIMP_ITEM (layer)), 0,
		  gimage->width);
      y2 = CLAMP (off_y + gimp_item_height (GIMP_ITEM (layer)), 0,
		  gimage->height);

      return GIMP_CHANNEL_CLASS (parent_class)->boundary (channel,
                                                          segs_in, segs_out,
                                                          num_segs_in,
                                                          num_segs_out,
                                                          x1, y1, x2, y2);
    }

  *segs_in      = NULL;
  *segs_out     = NULL;
  *num_segs_in  = 0;
  *num_segs_out = 0;

  return FALSE;
}

static gboolean
gimp_selection_bounds (GimpChannel *channel,
                       gint        *x1,
                       gint        *y1,
                       gint        *x2,
                       gint        *y2)
{
  return GIMP_CHANNEL_CLASS (parent_class)->bounds (channel, x1, y1, x2, y2);
}

static gboolean
gimp_selection_is_empty (GimpChannel *channel)
{
  GimpSelection *selection = GIMP_SELECTION (channel);

  /*  in order to allow stroking of selections, we need to pretend here
   *  that the selection mask is empty so that it doesn't mask the paint
   *  during the stroke operation.
   */
  if (selection->stroking)
    return TRUE;

  return GIMP_CHANNEL_CLASS (parent_class)->is_empty (channel);
}

static gint
gimp_selection_value (GimpChannel *channel,
                      gint         x,
                      gint         y)
{
  return GIMP_CHANNEL_CLASS (parent_class)->value (channel, x, y);
}

static void
gimp_selection_feather (GimpChannel *channel,
                        gdouble      radius_x,
                        gdouble      radius_y,
                        gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->feather (channel, radius_x, radius_y,
                                              push_undo);

  gimp_selection_changed (channel);
}

static void
gimp_selection_sharpen (GimpChannel *channel,
                        gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->sharpen (channel, push_undo);

  gimp_selection_changed (channel);
}

static void
gimp_selection_clear (GimpChannel *channel,
                      const gchar *undo_desc,
                      gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->clear (channel, undo_desc, push_undo);

  gimp_selection_changed (channel);
}

static void
gimp_selection_all (GimpChannel *channel,
                    gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->all (channel, push_undo);

  gimp_selection_changed (channel);
}

static void
gimp_selection_invert (GimpChannel *channel,
                       gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->invert (channel, push_undo);

  gimp_selection_changed (channel);
}

static void
gimp_selection_border (GimpChannel *channel,
                       gint         radius_x,
                       gint         radius_y,
                       gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->border (channel, radius_x, radius_y,
                                             push_undo);

  gimp_selection_changed (channel);
}

static void
gimp_selection_grow (GimpChannel *channel,
                     gint         radius_x,
                     gint         radius_y,
                     gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->grow (channel, radius_x, radius_y,
                                           push_undo);

  gimp_selection_changed (channel);
}

static void
gimp_selection_shrink (GimpChannel *channel,
                       gint         radius_x,
                       gint         radius_y,
                       gboolean     edge_lock,
                       gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->shrink (channel, radius_x, radius_y,
                                             edge_lock, push_undo);

  gimp_selection_changed (channel);
}

static void
gimp_selection_changed (GimpChannel *selection)
{
  gimp_image_mask_changed (gimp_item_get_image (GIMP_ITEM (selection)));
}

static void
gimp_selection_validate (TileManager *tm,
                         Tile        *tile)
{
  /*  Set the contents of the tile to empty  */
  memset (tile_data_pointer (tile, 0, 0),
	  TRANSPARENT_OPACITY, tile_size (tile));
}


/*  public functions  */

GimpChannel *
gimp_selection_new (GimpImage *gimage,
                    gint       width,
                    gint       height)
{
  GimpRGB      black = { 0.0, 0.0, 0.0, 0.5 };
  GimpChannel *channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  channel = g_object_new (GIMP_TYPE_SELECTION, NULL);

  gimp_drawable_configure (GIMP_DRAWABLE (channel),
                           gimage,
                           0, 0, width, height,
                           GIMP_GRAY_IMAGE,
                           _("Selection Mask"));

  channel->color       = black;
  channel->show_masked = TRUE;
  channel->x2          = width;
  channel->y2          = height;

  /*  Set the validate procedure  */
  tile_manager_set_validate_proc (GIMP_DRAWABLE (channel)->tiles,
				  gimp_selection_validate);

  return channel;
}

void
gimp_selection_load (GimpChannel *selection,
                     GimpChannel *channel)
{
  GimpItem    *src_item;
  GimpItem    *dest_item;
  PixelRegion  srcPR;
  PixelRegion  destPR;

  g_return_if_fail (GIMP_IS_SELECTION (selection));
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  src_item  = GIMP_ITEM (channel);
  dest_item = GIMP_ITEM (selection);

  g_return_if_fail (src_item->width  == dest_item->width);
  g_return_if_fail (src_item->height == dest_item->height);

  gimp_channel_push_undo (selection, _("Selection from Channel"));

  /*  copy the channel to the mask  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE (channel)->tiles,
		     0, 0, src_item->width, src_item->height,
                     FALSE);
  pixel_region_init (&destPR, GIMP_DRAWABLE (selection)->tiles,
		     0, 0, dest_item->width, dest_item->height,
                     TRUE);
  copy_region (&srcPR, &destPR);

  selection->bounds_known = FALSE;

  gimp_selection_changed (selection);
}

GimpChannel *
gimp_selection_save (GimpChannel *selection)
{
  GimpImage   *gimage;
  GimpChannel *new_channel;

  g_return_val_if_fail (GIMP_IS_SELECTION (selection), NULL);

  gimage = gimp_item_get_image (GIMP_ITEM (selection));

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  new_channel = GIMP_CHANNEL (gimp_item_duplicate (GIMP_ITEM (selection),
                                                   GIMP_TYPE_CHANNEL,
                                                   FALSE));

  /*  saved selections are not visible by default  */
  gimp_drawable_set_visible (GIMP_DRAWABLE (new_channel), FALSE, FALSE);

  gimp_image_add_channel (gimage, new_channel, -1);

  return new_channel;
}
