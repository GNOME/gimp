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

#include <string.h>

#include <gegl.h>

#include "core-types.h"

#include "paint-funcs/paint-funcs.h"

#include "base/pixel-region.h"
#include "base/tile.h"
#include "base/tile-manager.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpdrawable-private.h"
#include "gimperror.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayermask.h"
#include "gimplayer-floating-sel.h"
#include "gimppickable.h"
#include "gimpselection.h"

#include "gimp-intl.h"


static gboolean   gimp_selection_is_attached   (const GimpItem    *item);
static GimpItemTree * gimp_selection_get_tree  (GimpItem          *item);
static void       gimp_selection_translate     (GimpItem          *item,
                                                gint               offset_x,
                                                gint               offset_y,
                                                gboolean           push_undo);
static void       gimp_selection_scale         (GimpItem          *item,
                                                gint               new_width,
                                                gint               new_height,
                                                gint               new_offset_x,
                                                gint               new_offset_y,
                                                GimpInterpolationType interp_type,
                                                GimpProgress      *progress);
static void       gimp_selection_resize        (GimpItem          *item,
                                                GimpContext       *context,
                                                gint               new_width,
                                                gint               new_height,
                                                gint               offset_x,
                                                gint               offset_y);
static void       gimp_selection_flip          (GimpItem          *item,
                                                GimpContext       *context,
                                                GimpOrientationType flip_type,
                                                gdouble            axis,
                                                gboolean           clip_result);
static void       gimp_selection_rotate        (GimpItem          *item,
                                                GimpContext       *context,
                                                GimpRotationType   rotation_type,
                                                gdouble            center_x,
                                                gdouble            center_y,
                                                gboolean           clip_result);
static gboolean   gimp_selection_stroke        (GimpItem          *item,
                                                GimpDrawable      *drawable,
                                                GimpStrokeOptions *stroke_options,
                                                gboolean           push_undo,
                                                GimpProgress      *progress,
                                                GError           **error);
static void gimp_selection_invalidate_boundary (GimpDrawable      *drawable);

static gboolean   gimp_selection_boundary      (GimpChannel       *channel,
                                                const BoundSeg   **segs_in,
                                                const BoundSeg   **segs_out,
                                                gint              *num_segs_in,
                                                gint              *num_segs_out,
                                                gint               x1,
                                                gint               y1,
                                                gint               x2,
                                                gint               y2);
static gboolean   gimp_selection_bounds        (GimpChannel       *channel,
                                                gint              *x1,
                                                gint              *y1,
                                                gint              *x2,
                                                gint              *y2);
static gboolean   gimp_selection_is_empty      (GimpChannel       *channel);
static void       gimp_selection_feather       (GimpChannel       *channel,
                                                gdouble            radius_x,
                                                gdouble            radius_y,
                                                gboolean           push_undo);
static void       gimp_selection_sharpen       (GimpChannel       *channel,
                                                gboolean           push_undo);
static void       gimp_selection_clear         (GimpChannel       *channel,
                                                const gchar       *undo_desc,
                                                gboolean           push_undo);
static void       gimp_selection_all           (GimpChannel       *channel,
                                                gboolean           push_undo);
static void       gimp_selection_invert        (GimpChannel       *channel,
                                                gboolean           push_undo);
static void       gimp_selection_border        (GimpChannel       *channel,
                                                gint               radius_x,
                                                gint               radius_y,
                                                gboolean           feather,
                                                gboolean           edge_lock,
                                                gboolean           push_undo);
static void       gimp_selection_grow          (GimpChannel       *channel,
                                                gint               radius_x,
                                                gint               radius_y,
                                                gboolean           push_undo);
static void       gimp_selection_shrink        (GimpChannel       *channel,
                                                gint               radius_x,
                                                gint               radius_y,
                                                gboolean           edge_lock,
                                                gboolean           push_undo);


G_DEFINE_TYPE (GimpSelection, gimp_selection, GIMP_TYPE_CHANNEL)

#define parent_class gimp_selection_parent_class


static void
gimp_selection_class_init (GimpSelectionClass *klass)
{
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class     = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class = GIMP_DRAWABLE_CLASS (klass);
  GimpChannelClass  *channel_class  = GIMP_CHANNEL_CLASS (klass);

  viewable_class->default_stock_id    = "gimp-selection";

  item_class->is_attached             = gimp_selection_is_attached;
  item_class->get_tree                = gimp_selection_get_tree;
  item_class->translate               = gimp_selection_translate;
  item_class->scale                   = gimp_selection_scale;
  item_class->resize                  = gimp_selection_resize;
  item_class->flip                    = gimp_selection_flip;
  item_class->rotate                  = gimp_selection_rotate;
  item_class->stroke                  = gimp_selection_stroke;
  item_class->default_name            = _("Selection Mask");
  item_class->translate_desc          = C_("undo-type", "Move Selection");
  item_class->stroke_desc             = C_("undo-type", "Stroke Selection");

  drawable_class->invalidate_boundary = gimp_selection_invalidate_boundary;

  channel_class->boundary             = gimp_selection_boundary;
  channel_class->bounds               = gimp_selection_bounds;
  channel_class->is_empty             = gimp_selection_is_empty;
  channel_class->feather              = gimp_selection_feather;
  channel_class->sharpen              = gimp_selection_sharpen;
  channel_class->clear                = gimp_selection_clear;
  channel_class->all                  = gimp_selection_all;
  channel_class->invert               = gimp_selection_invert;
  channel_class->border               = gimp_selection_border;
  channel_class->grow                 = gimp_selection_grow;
  channel_class->shrink               = gimp_selection_shrink;

  channel_class->feather_desc         = C_("undo-type", "Feather Selection");
  channel_class->sharpen_desc         = C_("undo-type", "Sharpen Selection");
  channel_class->clear_desc           = C_("undo-type", "Select None");
  channel_class->all_desc             = C_("undo-type", "Select All");
  channel_class->invert_desc          = C_("undo-type", "Invert Selection");
  channel_class->border_desc          = C_("undo-type", "Border Selection");
  channel_class->grow_desc            = C_("undo-type", "Grow Selection");
  channel_class->shrink_desc          = C_("undo-type", "Shrink Selection");
}

static void
gimp_selection_init (GimpSelection *selection)
{
  selection->stroking_count = 0;
}

static gboolean
gimp_selection_is_attached (const GimpItem *item)
{
  return (GIMP_IS_IMAGE (gimp_item_get_image (item)) &&
          gimp_image_get_mask (gimp_item_get_image (item)) ==
          GIMP_CHANNEL (item));
}

static GimpItemTree *
gimp_selection_get_tree (GimpItem *item)
{
  return NULL;
}

static void
gimp_selection_translate (GimpItem *item,
                          gint      offset_x,
                          gint      offset_y,
                          gboolean  push_undo)
{
  GIMP_ITEM_CLASS (parent_class)->translate (item, offset_x, offset_y,
                                             push_undo);
}

static void
gimp_selection_scale (GimpItem              *item,
                      gint                   new_width,
                      gint                   new_height,
                      gint                   new_offset_x,
                      gint                   new_offset_y,
                      GimpInterpolationType  interp_type,
                      GimpProgress          *progress)
{
  GIMP_ITEM_CLASS (parent_class)->scale (item, new_width, new_height,
                                         new_offset_x, new_offset_y,
                                         interp_type, progress);

  gimp_item_set_offset (item, 0, 0);
}

static void
gimp_selection_resize (GimpItem    *item,
                       GimpContext *context,
                       gint         new_width,
                       gint         new_height,
                       gint         offset_x,
                       gint         offset_y)
{
  GIMP_ITEM_CLASS (parent_class)->resize (item, context, new_width, new_height,
                                          offset_x, offset_y);

  gimp_item_set_offset (item, 0, 0);
}

static void
gimp_selection_flip (GimpItem            *item,
                     GimpContext         *context,
                     GimpOrientationType  flip_type,
                     gdouble              axis,
                     gboolean             clip_result)
{
  GIMP_ITEM_CLASS (parent_class)->flip (item, context, flip_type, axis, TRUE);
}

static void
gimp_selection_rotate (GimpItem         *item,
                       GimpContext      *context,
                       GimpRotationType  rotation_type,
                       gdouble           center_x,
                       gdouble           center_y,
                       gboolean          clip_result)
{
  GIMP_ITEM_CLASS (parent_class)->rotate (item, context, rotation_type,
                                          center_x, center_y,
                                          clip_result);
}

static gboolean
gimp_selection_stroke (GimpItem           *item,
                       GimpDrawable       *drawable,
                       GimpStrokeOptions  *stroke_options,
                       gboolean            push_undo,
                       GimpProgress       *progress,
                       GError            **error)
{
  GimpSelection  *selection = GIMP_SELECTION (item);
  const BoundSeg *dummy_in;
  const BoundSeg *dummy_out;
  gint            num_dummy_in;
  gint            num_dummy_out;
  gboolean        retval;

  if (! gimp_channel_boundary (GIMP_CHANNEL (selection),
                               &dummy_in, &dummy_out,
                               &num_dummy_in, &num_dummy_out,
                               0, 0, 0, 0))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("There is no selection to stroke."));
      return FALSE;
    }

  gimp_selection_push_stroking (selection);

  retval = GIMP_ITEM_CLASS (parent_class)->stroke (item, drawable,
                                                   stroke_options,
                                                   push_undo, progress, error);

  gimp_selection_pop_stroking (selection);

  return retval;
}

static void
gimp_selection_invalidate_boundary (GimpDrawable *drawable)
{
  GimpImage *image = gimp_item_get_image (GIMP_ITEM (drawable));
  GimpLayer *layer;

  /*  Turn the current selection off  */
  gimp_image_selection_invalidate (image);

  GIMP_DRAWABLE_CLASS (parent_class)->invalidate_boundary (drawable);

  /*  If there is a floating selection, update it's area...
   *  we need to do this since this selection mask can act as an additional
   *  mask in the composition of the floating selection
   */
  layer = gimp_image_get_active_layer (image);

  if (layer && gimp_layer_is_floating_sel (layer))
    gimp_drawable_update (GIMP_DRAWABLE (layer),
                          0, 0,
                          gimp_item_get_width  (GIMP_ITEM (layer)),
                          gimp_item_get_height (GIMP_ITEM (layer)));

  /*  invalidate the preview  */
  drawable->private->preview_valid = FALSE;
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
  GimpImage    *image = gimp_item_get_image (GIMP_ITEM (channel));
  GimpDrawable *drawable;
  GimpLayer    *layer;

  if ((layer = gimp_image_get_floating_selection (image)))
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
  else if ((drawable = gimp_image_get_active_drawable (image)) &&
           GIMP_IS_CHANNEL (drawable))
    {
      /*  Otherwise, return the boundary...if a channel is active  */

      return GIMP_CHANNEL_CLASS (parent_class)->boundary (channel,
                                                          segs_in, segs_out,
                                                          num_segs_in,
                                                          num_segs_out,
                                                          0, 0,
                                                          gimp_image_get_width  (image),
                                                          gimp_image_get_height (image));
    }
  else if ((layer = gimp_image_get_active_layer (image)))
    {
      /*  If a layer is active, we return multiple boundaries based
       *  on the extents
       */

      gint x1, y1;
      gint x2, y2;
      gint offset_x;
      gint offset_y;

      gimp_item_get_offset (GIMP_ITEM (layer), &offset_x, &offset_y);

      x1 = CLAMP (offset_x, 0, gimp_image_get_width  (image));
      y1 = CLAMP (offset_y, 0, gimp_image_get_height (image));
      x2 = CLAMP (offset_x + gimp_item_get_width (GIMP_ITEM (layer)),
                  0, gimp_image_get_width (image));
      y2 = CLAMP (offset_y + gimp_item_get_height (GIMP_ITEM (layer)),
                  0, gimp_image_get_height (image));

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
  if (selection->stroking_count > 0)
    return TRUE;

  return GIMP_CHANNEL_CLASS (parent_class)->is_empty (channel);
}

static void
gimp_selection_feather (GimpChannel *channel,
                        gdouble      radius_x,
                        gdouble      radius_y,
                        gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->feather (channel, radius_x, radius_y,
                                              push_undo);
}

static void
gimp_selection_sharpen (GimpChannel *channel,
                        gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->sharpen (channel, push_undo);
}

static void
gimp_selection_clear (GimpChannel *channel,
                      const gchar *undo_desc,
                      gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->clear (channel, undo_desc, push_undo);
}

static void
gimp_selection_all (GimpChannel *channel,
                    gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->all (channel, push_undo);
}

static void
gimp_selection_invert (GimpChannel *channel,
                       gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->invert (channel, push_undo);
}

static void
gimp_selection_border (GimpChannel *channel,
                       gint         radius_x,
                       gint         radius_y,
                       gboolean     feather,
                       gboolean     edge_lock,
                       gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->border (channel,
                                             radius_x, radius_y,
                                             feather, edge_lock,
                                             push_undo);
}

static void
gimp_selection_grow (GimpChannel *channel,
                     gint         radius_x,
                     gint         radius_y,
                     gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->grow (channel,
					   radius_x, radius_y,
                                           push_undo);
}

static void
gimp_selection_shrink (GimpChannel *channel,
                       gint         radius_x,
                       gint         radius_y,
                       gboolean     edge_lock,
                       gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->shrink (channel,
					     radius_x, radius_y, edge_lock,
					     push_undo);
}


/*  public functions  */

GimpChannel *
gimp_selection_new (GimpImage *image,
                    gint       width,
                    gint       height)
{
  GimpRGB      black = { 0.0, 0.0, 0.0, 0.5 };
  GimpChannel *channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  channel = GIMP_CHANNEL (gimp_drawable_new (GIMP_TYPE_SELECTION,
                                             image, NULL,
                                             0, 0, width, height,
                                             GIMP_GRAY_IMAGE));

  gimp_channel_set_color (channel, &black, FALSE);
  gimp_channel_set_show_masked (channel, TRUE);

  channel->x2 = width;
  channel->y2 = height;

  return channel;
}

gint
gimp_selection_push_stroking (GimpSelection *selection)
{
  g_return_val_if_fail (GIMP_IS_SELECTION (selection), 0);

  selection->stroking_count++;

  return selection->stroking_count;
}

gint
gimp_selection_pop_stroking (GimpSelection *selection)
{
  g_return_val_if_fail (GIMP_IS_SELECTION (selection), 0);
  g_return_val_if_fail (selection->stroking_count > 0, 0);

  selection->stroking_count--;

  return selection->stroking_count;
}

void
gimp_selection_load (GimpSelection *selection,
                     GimpChannel   *channel)
{
  PixelRegion srcPR;
  PixelRegion destPR;
  gint        width;
  gint        height;

  g_return_if_fail (GIMP_IS_SELECTION (selection));
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  width  = gimp_item_get_width  (GIMP_ITEM (selection));
  height = gimp_item_get_height (GIMP_ITEM (selection));

  g_return_if_fail (width  == gimp_item_get_width  (GIMP_ITEM (channel)));
  g_return_if_fail (height == gimp_item_get_height (GIMP_ITEM (channel)));

  gimp_channel_push_undo (GIMP_CHANNEL (selection),
                          C_("undo-type", "Channel to Selection"));

  /*  copy the channel to the mask  */
  pixel_region_init (&srcPR,
                     gimp_drawable_get_tiles (GIMP_DRAWABLE (channel)),
                     0, 0, width, height,
                     FALSE);
  pixel_region_init (&destPR,
                     gimp_drawable_get_tiles (GIMP_DRAWABLE (selection)),
                     0, 0, width, height,
                     TRUE);
  copy_region (&srcPR, &destPR);

  GIMP_CHANNEL (selection)->bounds_known = FALSE;

  gimp_drawable_update (GIMP_DRAWABLE (selection),
                        0, 0, width, height);
}

GimpChannel *
gimp_selection_save (GimpSelection *selection)
{
  GimpImage   *image;
  GimpChannel *new_channel;

  g_return_val_if_fail (GIMP_IS_SELECTION (selection), NULL);

  image = gimp_item_get_image (GIMP_ITEM (selection));

  new_channel = GIMP_CHANNEL (gimp_item_duplicate (GIMP_ITEM (selection),
                                                   GIMP_TYPE_CHANNEL));

  /*  saved selections are not visible by default  */
  gimp_item_set_visible (GIMP_ITEM (new_channel), FALSE, FALSE);

  gimp_image_add_channel (image, new_channel,
                          GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);

  return new_channel;
}

TileManager *
gimp_selection_extract (GimpSelection *selection,
                        GimpPickable  *pickable,
                        GimpContext   *context,
                        gboolean       cut_image,
                        gboolean       keep_indexed,
                        gboolean       add_alpha,
                        gint          *offset_x,
                        gint          *offset_y,
                        GError       **error)
{
  GimpImage         *image;
  TileManager       *tiles;
  PixelRegion        srcPR, destPR;
  guchar             bg_color[MAX_CHANNELS];
  const guchar      *colormap;
  GimpImageBaseType  base_type = GIMP_RGB;
  gint               bytes     = 0;
  gint               x1, y1, x2, y2;
  gboolean           non_empty;
  gint               off_x, off_y;

  g_return_val_if_fail (GIMP_IS_SELECTION (selection), NULL);
  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), NULL);
  if (GIMP_IS_ITEM (pickable))
    g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (pickable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  image = gimp_pickable_get_image (pickable);

  /*  If there are no bounds, then just extract the entire image
   *  This may not be the correct behavior, but after getting rid
   *  of floating selections, it's still tempting to "cut" or "copy"
   *  a small layer and expect it to work, even though there is no
   *  actual selection mask
   */
  if (GIMP_IS_DRAWABLE (pickable))
    non_empty = gimp_item_mask_bounds (GIMP_ITEM (pickable),
                                       &x1, &y1, &x2, &y2);
  else
    non_empty = gimp_channel_bounds (GIMP_CHANNEL (selection),
                                     &x1, &y1, &x2, &y2);

  if (non_empty && ((x1 == x2) || (y1 == y2)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("Unable to cut or copy because the "
			     "selected region is empty."));
      return NULL;
    }

  /*  If there is a selection, we must add alpha because the selection
   *  could have any shape.
   */
  if (non_empty)
    add_alpha = TRUE;

  /*  How many bytes in the temp buffer?  */
  switch (GIMP_IMAGE_TYPE_BASE_TYPE (gimp_pickable_get_image_type (pickable)))
    {
    case GIMP_RGB:
      bytes = add_alpha ? 4 : gimp_pickable_get_bytes (pickable);
      base_type = GIMP_RGB;
      break;

    case GIMP_GRAY:
      bytes = add_alpha ? 2 : gimp_pickable_get_bytes (pickable);
      base_type = GIMP_GRAY;
      break;

    case GIMP_INDEXED:
      if (keep_indexed)
        {
          bytes = add_alpha ? 2 : gimp_pickable_get_bytes (pickable);
          base_type = GIMP_GRAY;
        }
      else
        {
          bytes = (add_alpha ||
                   GIMP_IMAGE_TYPE_HAS_ALPHA (gimp_pickable_get_image_type (pickable))) ? 4 : 3;
          base_type = GIMP_INDEXED;
        }
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  gimp_image_get_background (image, context,
                             gimp_pickable_get_image_type (pickable),
                             bg_color);

  /*  If a cut was specified, and the selection mask is not empty,
   *  push an undo
   */
  if (GIMP_IS_DRAWABLE (pickable))
    {
      if (cut_image && non_empty)
        gimp_drawable_push_undo (GIMP_DRAWABLE (pickable), NULL,
                                 x1, y1,
                                 x2 - x1, y2 - y1,
                                 NULL, FALSE);

      gimp_item_get_offset (GIMP_ITEM (pickable), &off_x, &off_y);
      colormap = gimp_drawable_get_colormap (GIMP_DRAWABLE (pickable));
    }
  else
    {
      off_x = off_y = 0;
      colormap = NULL;
    }

  gimp_pickable_flush (pickable);

  /*  Allocate the temp buffer  */
  tiles = tile_manager_new (x2 - x1, y2 - y1, bytes);

  /* configure the pixel regions  */
  pixel_region_init (&srcPR, gimp_pickable_get_tiles (pickable),
                     x1, y1,
                     x2 - x1, y2 - y1,
                     cut_image);
  pixel_region_init (&destPR, tiles,
                     0, 0,
                     x2 - x1, y2 - y1,
                     TRUE);

  if (non_empty) /*  If there is a selection, extract from it  */
    {
      PixelRegion maskPR;

      pixel_region_init (&maskPR,
                         gimp_drawable_get_tiles (GIMP_DRAWABLE (selection)),
                         (x1 + off_x), (y1 + off_y), (x2 - x1), (y2 - y1),
                         FALSE);

      extract_from_region (&srcPR, &destPR, &maskPR,
                           colormap, bg_color, base_type, cut_image);

      if (GIMP_IS_DRAWABLE (pickable) && cut_image)
        {
          /*  Update the region  */
          gimp_drawable_update (GIMP_DRAWABLE (pickable),
                                x1, y1, (x2 - x1), (y2 - y1));
        }
    }
  else /*  Otherwise, get the entire active layer  */
    {
      if (base_type == GIMP_INDEXED && !keep_indexed)
        {
          /*  If the layer is indexed...we need to extract pixels  */
          extract_from_region (&srcPR, &destPR, NULL,
                               colormap, bg_color, base_type, FALSE);
        }
      else if (bytes > srcPR.bytes)
        {
          /*  If the layer doesn't have an alpha channel, add one  */
          add_alpha_region (&srcPR, &destPR);
        }
      else
        {
          /*  Otherwise, do a straight copy  */
          copy_region (&srcPR, &destPR);
        }

      /*  If we're cutting, remove either the layer (or floating selection),
       *  the layer mask, or the channel
       */
      if (GIMP_IS_DRAWABLE (pickable) && cut_image)
        {
          if (GIMP_IS_LAYER (pickable))
            {
              gimp_image_remove_layer (image, GIMP_LAYER (pickable),
                                       TRUE, NULL);
            }
          else if (GIMP_IS_LAYER_MASK (pickable))
            {
              gimp_layer_apply_mask (gimp_layer_mask_get_layer (GIMP_LAYER_MASK (pickable)),
                                     GIMP_MASK_DISCARD, TRUE);
            }
          else if (GIMP_IS_CHANNEL (pickable))
            {
              gimp_image_remove_channel (image, GIMP_CHANNEL (pickable),
                                         TRUE, NULL);
            }
        }
    }

  *offset_x = x1 + off_x;
  *offset_y = y1 + off_y;

  return tiles;
}

GimpLayer *
gimp_selection_float (GimpSelection *selection,
                      GimpDrawable  *drawable,
                      GimpContext   *context,
                      gboolean       cut_image,
                      gint           off_x,
                      gint           off_y,
                      GError       **error)
{
  GimpImage   *image;
  GimpLayer   *layer;
  TileManager *tiles;
  gint         x1, y1;
  gint         x2, y2;

  g_return_val_if_fail (GIMP_IS_SELECTION (selection), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  image = gimp_item_get_image (GIMP_ITEM (selection));

  /*  Make sure there is a region to float...  */
  if (! gimp_item_mask_bounds (GIMP_ITEM (drawable), &x1, &y1, &x2, &y2) ||
      (x1 == x2 || y1 == y2))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
			   _("Cannot float selection because the selected "
			     "region is empty."));
      return NULL;
    }

  /*  Start an undo group  */
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_FS_FLOAT,
                               C_("undo-type", "Float Selection"));

  /*  Cut or copy the selected region  */
  tiles = gimp_selection_extract (selection, GIMP_PICKABLE (drawable), context,
                                  cut_image, FALSE, TRUE, &x1, &y1, NULL);

  /*  Clear the selection  */
  gimp_channel_clear (GIMP_CHANNEL (selection), NULL, TRUE);

  /* Create a new layer from the buffer, using the drawable's type
   *  because it may be different from the image's type if we cut from
   *  a channel or layer mask
   */
  layer = gimp_layer_new_from_tiles (tiles,
                                     image,
                                     gimp_drawable_type_with_alpha (drawable),
                                     _("Floated Layer"),
                                     GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);

  /*  Set the offsets  */
  gimp_item_set_offset (GIMP_ITEM (layer), x1 + off_x, y1 + off_y);

  /*  Free the temp buffer  */
  tile_manager_unref (tiles);

  /*  Add the floating layer to the image  */
  floating_sel_attach (layer, drawable);

  /*  End an undo group  */
  gimp_image_undo_group_end (image);

  /*  invalidate the image's boundary variables  */
  GIMP_CHANNEL (selection)->boundary_known = FALSE;

  return layer;
}
