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
static gboolean   gimp_selection_stroke        (GimpItem        *item,
                                                GimpDrawable    *drawable,
                                                GimpPaintInfo   *paint_info);

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
static gint       gimp_selection_value         (GimpChannel     *channel,
                                                gint             x,
                                                gint             y);
static gboolean   gimp_selection_is_empty      (GimpChannel     *channel);
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

static void       gimp_selection_changed       (GimpSelection   *selection);
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
  GimpItemClass    *item_class;
  GimpChannelClass *channel_class;

  item_class    = GIMP_ITEM_CLASS (klass);
  channel_class = GIMP_CHANNEL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  item_class->translate   = gimp_selection_translate;
  item_class->stroke      = gimp_selection_stroke;

  channel_class->boundary = gimp_selection_boundary;
  channel_class->bounds   = gimp_selection_bounds;
  channel_class->value    = gimp_selection_value;
  channel_class->is_empty = gimp_selection_is_empty;
  channel_class->feather  = gimp_selection_feather;
  channel_class->sharpen  = gimp_selection_sharpen;
  channel_class->clear    = gimp_selection_clear;
  channel_class->all      = gimp_selection_all;
  channel_class->invert   = gimp_selection_invert;
  channel_class->border   = gimp_selection_border;
  channel_class->grow     = gimp_selection_grow;
  channel_class->shrink   = gimp_selection_shrink;
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
  GimpSelection *selection = GIMP_SELECTION (item);

  if (push_undo)
    gimp_selection_push_undo (selection, _("Move Selection"));
  else
    gimp_selection_invalidate (selection);

  GIMP_ITEM_CLASS (parent_class)->translate (item, offset_x, offset_y, FALSE);

  gimp_selection_changed (selection);
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

static gint
gimp_selection_value (GimpChannel *channel,
                      gint         x,
                      gint         y)
{
  return GIMP_CHANNEL_CLASS (parent_class)->value (channel, x, y);
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

static void
gimp_selection_feather (GimpChannel *channel,
                        gdouble      radius_x,
                        gdouble      radius_y,
                        gboolean     push_undo)
{
  GimpSelection *selection = GIMP_SELECTION (channel);

  if (push_undo)
    gimp_selection_push_undo (selection, _("Feather Selection"));
  else
    gimp_selection_invalidate (selection);

  GIMP_CHANNEL_CLASS (parent_class)->feather (channel, radius_x, radius_y,
                                              FALSE);

  gimp_selection_changed (selection);
}

static void
gimp_selection_sharpen (GimpChannel *channel,
                        gboolean     push_undo)
{
  GimpSelection *selection = GIMP_SELECTION (channel);

  if (push_undo)
    gimp_selection_push_undo (selection, _("Sharpen Selection"));
  else
    gimp_selection_invalidate (selection);

  GIMP_CHANNEL_CLASS (parent_class)->sharpen (channel, FALSE);

  gimp_selection_changed (selection);
}

static void
gimp_selection_clear (GimpChannel *channel,
                      const gchar *undo_desc,
                      gboolean     push_undo)
{
  GimpSelection *selection = GIMP_SELECTION (channel);

  if (push_undo)
    {
      if (! undo_desc)
        undo_desc = _("Select None");

      gimp_selection_push_undo (selection, undo_desc);
    }
  else
    gimp_selection_invalidate (selection);

  GIMP_CHANNEL_CLASS (parent_class)->clear (channel, NULL, FALSE);

  gimp_selection_changed (selection);
}

static void
gimp_selection_all (GimpChannel *channel,
                    gboolean     push_undo)
{
  GimpSelection *selection = GIMP_SELECTION (channel);

  if (push_undo)
    gimp_selection_push_undo (selection, _("Select All"));
  else
    gimp_selection_invalidate (selection);

  GIMP_CHANNEL_CLASS (parent_class)->all (channel, FALSE);

  gimp_selection_changed (selection);
}

static void
gimp_selection_invert (GimpChannel *channel,
                       gboolean     push_undo)
{
  GimpSelection *selection = GIMP_SELECTION (channel);

  if (push_undo)
    gimp_selection_push_undo (selection, _("Invert Selection"));
  else
    gimp_selection_invalidate (selection);

  GIMP_CHANNEL_CLASS (parent_class)->invert (channel, FALSE);

  gimp_selection_changed (selection);
}

static void
gimp_selection_border (GimpChannel *channel,
                       gint         radius_x,
                       gint         radius_y,
                       gboolean     push_undo)
{
  GimpSelection *selection = GIMP_SELECTION (channel);

  if (push_undo)
    gimp_selection_push_undo (selection, _("Border Selection"));
  else
    gimp_selection_invalidate (selection);

  GIMP_CHANNEL_CLASS (parent_class)->border (channel, radius_x, radius_y,
                                             FALSE);

  gimp_selection_changed (selection);
}

static void
gimp_selection_grow (GimpChannel *channel,
                     gint         radius_x,
                     gint         radius_y,
                     gboolean     push_undo)
{
  GimpSelection *selection = GIMP_SELECTION (channel);

  if (push_undo)
    gimp_selection_push_undo (selection, _("Grow Selection"));
  else
    gimp_selection_invalidate (selection);

  GIMP_CHANNEL_CLASS (parent_class)->grow (channel, radius_x, radius_y,
                                           FALSE);

  gimp_selection_changed (selection);
}

static void
gimp_selection_shrink (GimpChannel *channel,
                       gint         radius_x,
                       gint         radius_y,
                       gboolean     edge_lock,
                       gboolean     push_undo)
{
  GimpSelection *selection = GIMP_SELECTION (channel);

  if (push_undo)
    gimp_selection_push_undo (selection, _("Shrink Selection"));
  else
    gimp_selection_invalidate (selection);

  GIMP_CHANNEL_CLASS (parent_class)->shrink (channel, radius_x, radius_y,
                                             edge_lock, FALSE);

  gimp_selection_changed (selection);
}

static void
gimp_selection_changed (GimpSelection *selection)
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
gimp_selection_push_undo (GimpSelection *selection,
                          const gchar   *undo_desc)
{
  GimpImage *gimage;

  g_return_if_fail (GIMP_IS_SELECTION (selection));

  gimage = gimp_item_get_image (GIMP_ITEM (selection));

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimp_image_undo_push_mask (gimage, undo_desc, GIMP_CHANNEL (selection));

  gimp_selection_invalidate (selection);
}

void
gimp_selection_invalidate (GimpSelection *selection)
{
  GimpLayer *layer;
  GimpImage *gimage;

  g_return_if_fail (GIMP_IS_SELECTION (selection));

  gimage = gimp_item_get_image (GIMP_ITEM (selection));

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  /*  Turn the current selection off  */
  gimp_image_selection_control (gimage, GIMP_SELECTION_OFF);

  GIMP_CHANNEL (selection)->boundary_known = FALSE;

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
  GIMP_DRAWABLE (selection)->preview_valid = FALSE;
}
