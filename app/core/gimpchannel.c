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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/boundary.h"
#include "base/gimplut.h"
#include "base/lut-funcs.h"
#include "base/pixel-processor.h"
#include "base/pixel-region.h"
#include "base/tile.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimpimage.h"
#include "gimpimage-projection.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpchannel.h"
#include "gimpdrawable-transform.h"
#include "gimplayer.h"
#include "gimpparasitelist.h"

#include "gimp-intl.h"


static void       gimp_channel_class_init  (GimpChannelClass *klass);
static void       gimp_channel_init        (GimpChannel      *channel);

static void       gimp_channel_finalize    (GObject          *object);

static gsize      gimp_channel_get_memsize (GimpObject       *object);

static GimpItem * gimp_channel_duplicate   (GimpItem         *item,
                                            GType             new_type,
                                            gboolean          add_alpha);
static void       gimp_channel_translate   (GimpItem         *item,
                                            gint              off_x,
                                            gint              off_y,
                                            gboolean          push_undo);
static void       gimp_channel_scale       (GimpItem         *item,
                                            gint              new_width,
                                            gint              new_height,
                                            gint              new_offset_x,
                                            gint              new_offset_y,
                                            GimpInterpolationType  interp_type);
static void       gimp_channel_resize      (GimpItem         *item,
                                            gint              new_width,
                                            gint              new_height,
                                            gint              offx,
                                            gint              offy);
static void       gimp_channel_flip        (GimpItem         *item,
                                            GimpOrientationType flip_type,
                                            gdouble           axis);
static void       gimp_channel_transform   (GimpItem         *item,
                                            GimpMatrix3       matrix,
                                            GimpTransformDirection direction,
                                            GimpInterpolationType  interpolation_type,
                                            gboolean          clip_result,
                                            GimpProgressFunc  progress_callback,
                                            gpointer          progress_data);

static void       gimp_channel_push_undo   (GimpChannel      *mask,
                                            const gchar      *undo_desc);
static void       gimp_channel_validate    (TileManager      *tm,
                                            Tile             *tile);


static GimpDrawableClass * parent_class = NULL;


GType
gimp_channel_get_type (void)
{
  static GType channel_type = 0;

  if (! channel_type)
    {
      static const GTypeInfo channel_info =
      {
        sizeof (GimpChannelClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_channel_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpChannel),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_channel_init,
      };

      channel_type = g_type_register_static (GIMP_TYPE_DRAWABLE,
					     "GimpChannel",
					     &channel_info, 0);
    }

  return channel_type;
}

static void
gimp_channel_class_init (GimpChannelClass *klass)
{
  GObjectClass      *object_class;
  GimpObjectClass   *gimp_object_class;
  GimpViewableClass *viewable_class;
  GimpItemClass     *item_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);
  viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  item_class        = GIMP_ITEM_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize           = gimp_channel_finalize;

  gimp_object_class->get_memsize   = gimp_channel_get_memsize;

  viewable_class->default_stock_id = "gimp-channel";

  item_class->duplicate            = gimp_channel_duplicate;
  item_class->translate            = gimp_channel_translate;
  item_class->scale                = gimp_channel_scale;
  item_class->resize               = gimp_channel_resize;
  item_class->flip                 = gimp_channel_flip;
  item_class->transform            = gimp_channel_transform;
  item_class->default_name         = _("Channel");
  item_class->rename_desc          = _("Rename Channel");
}

static void
gimp_channel_init (GimpChannel *channel)
{
  gimp_rgba_set (&channel->color, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);

  channel->show_masked = FALSE;

  /*  Selection mask variables  */
  channel->boundary_known = FALSE;
  channel->segs_in        = NULL;
  channel->segs_out       = NULL;
  channel->num_segs_in    = 0;
  channel->num_segs_out   = 0;
  channel->empty          = FALSE;
  channel->bounds_known   = FALSE;
  channel->x1             = 0;
  channel->y1             = 0;
  channel->x2             = 0;
  channel->y2             = 0;
}

static void
gimp_channel_finalize (GObject *object)
{
  GimpChannel *channel;

  channel = GIMP_CHANNEL (object);

  if (channel->segs_in)
    {
      g_free (channel->segs_in);
      channel->segs_in = NULL;
    }

  if (channel->segs_out)
    {
      g_free (channel->segs_out);
      channel->segs_out = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gsize
gimp_channel_get_memsize (GimpObject *object)
{
  GimpChannel *channel;
  gsize        memsize = 0;

  channel = GIMP_CHANNEL (object);

  memsize += channel->num_segs_in  * sizeof (BoundSeg);
  memsize += channel->num_segs_out * sizeof (BoundSeg);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object);
}

static GimpItem *
gimp_channel_duplicate (GimpItem *item,
                        GType     new_type,
                        gboolean  add_alpha)
{
  GimpChannel *channel;
  GimpItem    *new_item;
  GimpChannel *new_channel;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  if (g_type_is_a (new_type, GIMP_TYPE_CHANNEL))
    add_alpha = FALSE;

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type,
                                                        add_alpha);

  if (! GIMP_IS_CHANNEL (new_item))
    return new_item;

  channel     = GIMP_CHANNEL (item);
  new_channel = GIMP_CHANNEL (new_item);

  /*  set the channel color and opacity  */
  new_channel->color        = channel->color;

  new_channel->show_masked  = channel->show_masked;

  /*  selection mask variables  */
  new_channel->bounds_known = channel->bounds_known;
  new_channel->empty        = channel->empty;
  new_channel->x1           = channel->x1;
  new_channel->y1           = channel->y1;
  new_channel->x2           = channel->x2;
  new_channel->y2           = channel->y2;

  return new_item;
}

static void
gimp_channel_translate (GimpItem *item,
			gint      off_x,
			gint      off_y,
                        gboolean  push_undo)
{
  GimpChannel *mask;
  GimpChannel *tmp_mask = NULL;
  gint         width, height;
  PixelRegion  srcPR, destPR;
  guchar       empty = TRANSPARENT_OPACITY;
  gint         x1, y1, x2, y2;

  mask = GIMP_CHANNEL (item);

  gimp_channel_bounds (mask, &x1, &y1, &x2, &y2);
  x1 = CLAMP ((x1 + off_x), 0, GIMP_ITEM (mask)->width);
  y1 = CLAMP ((y1 + off_y), 0, GIMP_ITEM (mask)->height);
  x2 = CLAMP ((x2 + off_x), 0, GIMP_ITEM (mask)->width);
  y2 = CLAMP ((y2 + off_y), 0, GIMP_ITEM (mask)->height);

  width  = x2 - x1;
  height = y2 - y1;

  if (push_undo)
    {
      gimp_channel_push_undo (mask, _("Move Channel"));

      /*  update the old area  */
      gimp_drawable_update (GIMP_DRAWABLE (item),
                            x1, y1,
                            x2 - x1, y2 - y1);
    }

  /*  make sure width and height are non-zero  */
  if (width != 0 && height != 0)
    {
      /*  copy the portion of the mask we will keep to a
       *  temporary buffer
       */
      tmp_mask = gimp_channel_new_mask (gimp_item_get_image (GIMP_ITEM (mask)),
					width, height);

      pixel_region_init (&srcPR, GIMP_DRAWABLE (mask)->tiles,
			 x1 - off_x, y1 - off_y, width, height, FALSE);
      pixel_region_init (&destPR, GIMP_DRAWABLE (tmp_mask)->tiles,
			 0, 0, width, height, TRUE);
      copy_region (&srcPR, &destPR);
    }

  /*  clear the mask  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE (mask)->tiles,
		     0, 0,
		     GIMP_ITEM (mask)->width,
		     GIMP_ITEM (mask)->height, TRUE);
  color_region (&srcPR, &empty);

  if (width != 0 && height != 0)
    {
      /*  copy the temp mask back to the mask  */
      pixel_region_init (&srcPR, GIMP_DRAWABLE (tmp_mask)->tiles,
			 0, 0, width, height, FALSE);
      pixel_region_init (&destPR, GIMP_DRAWABLE (mask)->tiles,
			 x1, y1, width, height, TRUE);
      copy_region (&srcPR, &destPR);

      /*  free the temporary mask  */
      g_object_unref (tmp_mask);
    }

  /*  calculate new bounds  */
  if (width == 0 || height == 0)
    {
      mask->empty = TRUE;
      mask->x1    = 0;
      mask->y1    = 0;
      mask->x2    = GIMP_ITEM (mask)->width;
      mask->y2    = GIMP_ITEM (mask)->height;
    }
  else
    {
      mask->x1 = x1;
      mask->y1 = y1;
      mask->x2 = x2;
      mask->y2 = y2;
    }

  if (push_undo)
    {
      /*  update the new area  */
      gimp_drawable_update (GIMP_DRAWABLE (item),
                            mask->x1, mask->y1,
                            mask->x2 - mask->x1, mask->y2 - mask->y1);

      gimp_viewable_size_changed (GIMP_VIEWABLE (item));
    }
}

static void
gimp_channel_scale (GimpItem              *item,
		    gint                   new_width,
		    gint                   new_height,
                    gint                   new_offset_x,
                    gint                   new_offset_y,
                    GimpInterpolationType  interpolation_type)
{
  GimpChannel *channel;

  channel = GIMP_CHANNEL (item);

  gimp_image_undo_push_channel_mod (gimp_item_get_image (item),
                                    _("Scale Channel"),
                                    channel);

  GIMP_ITEM_CLASS (parent_class)->scale (item, new_width, new_height,
                                         new_offset_x, new_offset_y,
                                         interpolation_type);

  /*  bounds are now unknown  */
  channel->bounds_known = FALSE;
}

static void
gimp_channel_resize (GimpItem *item,
		     gint      new_width,
		     gint      new_height,
		     gint      offset_x,
		     gint      offset_y)
{
  GimpChannel *channel;

  channel = GIMP_CHANNEL (item);

  gimp_image_undo_push_channel_mod (gimp_item_get_image (item),
                                    _("Resize Channel"),
                                    channel);

  GIMP_ITEM_CLASS (parent_class)->resize (item, new_width, new_height,
                                          offset_x, offset_y);

  /*  bounds are now unknown  */
  channel->bounds_known = FALSE;
}

static void
gimp_channel_flip (GimpItem            *item,
                   GimpOrientationType  flip_type,
                   gdouble              axis)
{
#ifdef __GNUC__
#warning FIXME: implement clip_result for flipping
#endif

  g_print ("FIXME: implement channel flipping\n");

#if 0
  GimpChannel *channel;
  GimpImage   *gimage;
  TileManager *tiles;

  channel = GIMP_CHANNEL (item);
  gimage  = gimp_item_get_image (item);

  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_TRANSFORM,
                               _("Flip Channel"));

  tiles = gimp_drawable_transform_tiles_flip (GIMP_DRAWABLE (channel),
                                              GIMP_DRAWABLE (channel)->tiles,
                                              flip_type, axis,
                                              TRUE /* always clip_result */);

  if (tiles)
    gimp_drawable_transform_paste (GIMP_DRAWABLE (channel), tiles, FALSE);

  gimp_image_undo_group_end (gimage);

  /*  bounds are now unknown  */
  channel->bounds_known = FALSE;
#endif
}

static void
gimp_channel_transform (GimpItem               *item,
                        GimpMatrix3             matrix,
                        GimpTransformDirection  direction,
                        GimpInterpolationType   interpolation_type,
                        gboolean                clip_result,
                        GimpProgressFunc        progress_callback,
                        gpointer                progress_data)
{
  GimpChannel *channel;
  GimpImage   *gimage;
  TileManager *tiles;

  channel = GIMP_CHANNEL (item);
  gimage  = gimp_item_get_image (item);

  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_TRANSFORM,
                               _("Transform Channel"));

  tiles = gimp_drawable_transform_tiles_affine (GIMP_DRAWABLE (channel),
                                                GIMP_DRAWABLE (channel)->tiles,
                                                matrix, direction,
                                                interpolation_type,
                                                TRUE, /* always clip_result */
                                                progress_callback,
                                                progress_data);

  if (tiles)
    gimp_drawable_transform_paste (GIMP_DRAWABLE (channel), tiles, FALSE);

  gimp_image_undo_group_end (gimage);

  /*  bounds are now unknown  */
  channel->bounds_known = FALSE;
}

static void
gimp_channel_push_undo (GimpChannel *mask,
                        const gchar *undo_desc)
{
  GimpImage *gimage;

  gimage = gimp_item_get_image (GIMP_ITEM (mask));

  g_return_if_fail (gimage != NULL);

  gimp_image_undo_push_mask (gimage, undo_desc, mask);

  mask->boundary_known = FALSE;

  /*  invalidate the preview  */
  GIMP_DRAWABLE (mask)->preview_valid = FALSE;
}

static void
gimp_channel_validate (TileManager *tm,
		       Tile        *tile)
{
  /*  Set the contents of the tile to empty  */
  memset (tile_data_pointer (tile, 0, 0), 
	  TRANSPARENT_OPACITY, tile_size (tile));
}


/*  public functions  */

GimpChannel *
gimp_channel_new (GimpImage     *gimage,
		  gint           width,
		  gint           height,
		  const gchar   *name,
		  const GimpRGB *color)
{
  GimpChannel *channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (color != NULL, NULL);

  channel = g_object_new (GIMP_TYPE_CHANNEL, NULL);

  gimp_drawable_configure (GIMP_DRAWABLE (channel),
			   gimage,
                           0, 0, width, height,
                           GIMP_GRAY_IMAGE, name);

  /*  set the channel color and opacity  */
  channel->color       = *color;

  channel->show_masked = TRUE;

  /*  selection mask variables  */
  channel->x2          = width;
  channel->y2          = height;

  return channel;
}

GimpChannel *
gimp_channel_new_from_component (GimpImage       *gimage,
                                 GimpChannelType  type,
                                 const gchar     *name,
                                 const GimpRGB   *color)
{
  GimpChannel *channel;
  TileManager *projection;
  PixelRegion  src;
  PixelRegion  dest;
  gint         width;
  gint         height;
  gint         pixel;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (color != NULL, NULL);

  pixel = gimp_image_get_component_index (gimage, type);

  g_return_val_if_fail (pixel != -1, NULL);

  projection = gimp_image_projection (gimage);
  width  = tile_manager_width  (projection);
  height = tile_manager_height (projection);

  channel = gimp_channel_new (gimage, width, height, name, color);

  pixel_region_init (&src, projection,
                     0, 0, width, height, FALSE);
  pixel_region_init (&dest, GIMP_DRAWABLE (channel)->tiles,
                     0, 0, width, height, TRUE);

  copy_component (&src, &dest, pixel);

  return channel;
}

void 
gimp_channel_set_color (GimpChannel   *channel,
			const GimpRGB *color,
                        gboolean       push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (color != NULL);

  if (gimp_rgba_distance (&channel->color, color) > 0.0001)
    {
      if (push_undo)
        {
          GimpImage *gimage;

          gimage = gimp_item_get_image (GIMP_ITEM (channel));

          if (gimage)
            gimp_image_undo_push_channel_color (gimage, _("Set Channel Color"),
                                                channel);
        }

      channel->color = *color;

      gimp_drawable_update (GIMP_DRAWABLE (channel),
			    0, 0,
			    GIMP_ITEM (channel)->width,
			    GIMP_ITEM (channel)->height);
    }
}

void
gimp_channel_get_color (const GimpChannel *channel,
                        GimpRGB           *color)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));
  g_return_if_fail (color != NULL);

  *color = channel->color;
}

gdouble
gimp_channel_get_opacity (const GimpChannel *channel)
{
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), GIMP_OPACITY_TRANSPARENT);

  return channel->color.a;
}

void
gimp_channel_set_opacity (GimpChannel *channel,
			  gdouble      opacity,
                          gboolean     push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  opacity = CLAMP (opacity, GIMP_OPACITY_TRANSPARENT, GIMP_OPACITY_OPAQUE);

  if (channel->color.a != opacity)
    {
      if (push_undo)
        {
          GimpImage *gimage;

          gimage = gimp_item_get_image (GIMP_ITEM (channel));

          if (gimage)
            gimp_image_undo_push_channel_color (gimage, _("Set Channel Opacity"),
                                                channel);
        }

      channel->color.a = opacity;

      gimp_drawable_update (GIMP_DRAWABLE (channel),
			    0, 0,
			    GIMP_ITEM (channel)->width,
			    GIMP_ITEM (channel)->height);
    }
}

gboolean
gimp_channel_get_show_masked (GimpChannel *channel)
{
  g_return_val_if_fail (GIMP_IS_CHANNEL (channel), FALSE);

  return channel->show_masked;
}

void
gimp_channel_set_show_masked (GimpChannel *channel,
                              gboolean     show_masked)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  if (show_masked != channel->show_masked)
    {
      channel->show_masked = show_masked ? TRUE : FALSE;

      gimp_drawable_update (GIMP_DRAWABLE (channel),
			    0, 0,
			    GIMP_ITEM (channel)->width,
			    GIMP_ITEM (channel)->height);
    }
}


/******************************/
/*  selection mask functions  */
/******************************/

GimpChannel *
gimp_channel_new_mask (GimpImage *gimage,
		       gint       width,
		       gint       height)
{
  GimpRGB      black = { 0.0, 0.0, 0.0, 0.5 };
  GimpChannel *new_channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);

  /*  Create the new channel  */
  new_channel = gimp_channel_new (gimage, width, height,
				  _("Selection Mask"), &black);

  /*  Set the validate procedure  */
  tile_manager_set_validate_proc (GIMP_DRAWABLE (new_channel)->tiles,
				  gimp_channel_validate);

  return new_channel;
}

gboolean
gimp_channel_boundary (GimpChannel  *mask,
		       BoundSeg    **segs_in,
		       BoundSeg    **segs_out,
		       gint         *num_segs_in,
		       gint         *num_segs_out,
		       gint          x1,
		       gint          y1,
		       gint          x2,
		       gint          y2)
{
  gint        x3, y3, x4, y4;
  PixelRegion bPR;

  g_return_val_if_fail (GIMP_IS_CHANNEL (mask), FALSE);
  g_return_val_if_fail (segs_in != NULL, FALSE);
  g_return_val_if_fail (segs_out != NULL, FALSE);
  g_return_val_if_fail (num_segs_in != NULL, FALSE);
  g_return_val_if_fail (num_segs_out != NULL, FALSE);

  if (! mask->boundary_known)
    {
      /* free the out of date boundary segments */
      if (mask->segs_in)
	g_free (mask->segs_in);
      if (mask->segs_out)
	g_free (mask->segs_out);

      if (gimp_channel_bounds (mask, &x3, &y3, &x4, &y4))
	{
	  pixel_region_init (&bPR, GIMP_DRAWABLE (mask)->tiles,
			     x3, y3, (x4 - x3), (y4 - y3), FALSE);
	  mask->segs_out = find_mask_boundary (&bPR, &mask->num_segs_out,
					       IgnoreBounds,
					       x1, y1,
					       x2, y2);
	  x1 = MAX (x1, x3);
	  y1 = MAX (y1, y3);
	  x2 = MIN (x2, x4);
	  y2 = MIN (y2, y4);

	  if (x2 > x1 && y2 > y1)
	    {
	      pixel_region_init (&bPR, GIMP_DRAWABLE (mask)->tiles,
				 0, 0,
				 GIMP_ITEM (mask)->width,
				 GIMP_ITEM (mask)->height, FALSE);
	      mask->segs_in =  find_mask_boundary (&bPR, &mask->num_segs_in,
						   WithinBounds,
						   x1, y1,
						   x2, y2);
	    }
	  else
	    {
	      mask->segs_in     = NULL;
	      mask->num_segs_in = 0;
	    }
	}
      else
	{
	  mask->segs_in      = NULL;
	  mask->segs_out     = NULL;
	  mask->num_segs_in  = 0;
	  mask->num_segs_out = 0;
	}

      mask->boundary_known = TRUE;
    }

  *segs_in      = mask->segs_in;
  *segs_out     = mask->segs_out;
  *num_segs_in  = mask->num_segs_in;
  *num_segs_out = mask->num_segs_out;

  return TRUE;
}

gint
gimp_channel_value (GimpChannel *mask,
		    gint         x,
		    gint         y)
{
  Tile *tile;
  gint  val;

  g_return_val_if_fail (GIMP_IS_CHANNEL (mask), 0);

  /*  Some checks to cut back on unnecessary work  */
  if (mask->bounds_known)
    {
      if (mask->empty)
	return 0;
      else if (x < mask->x1 || x >= mask->x2 || y < mask->y1 || y >= mask->y2)
	return 0;
    }
  else
    {
      if (x < 0 || x >= GIMP_ITEM (mask)->width ||
	  y < 0 || y >= GIMP_ITEM (mask)->height)
	return 0;
    }

  tile = tile_manager_get_tile (GIMP_DRAWABLE (mask)->tiles, x, y, TRUE, FALSE);
  val = *(guchar *) (tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT));
  tile_release (tile, FALSE);

  return val;
}

gboolean
gimp_channel_bounds (GimpChannel *mask,
		     gint        *x1,
		     gint        *y1,
		     gint        *x2,
		     gint        *y2)
{
  PixelRegion  maskPR;
  guchar      *data, *data1;
  gint         x, y;
  gint         ex, ey;
  gint         tx1, tx2, ty1, ty2;
  gint         minx, maxx;
  gpointer     pr;

  g_return_val_if_fail (GIMP_IS_CHANNEL (mask), FALSE);
  g_return_val_if_fail (x1 != NULL, FALSE);
  g_return_val_if_fail (y1 != NULL, FALSE);
  g_return_val_if_fail (x2 != NULL, FALSE);
  g_return_val_if_fail (y2 != NULL, FALSE);

  /*  if the mask's bounds have already been reliably calculated...  */
  if (mask->bounds_known)
    {
      *x1 = mask->x1;
      *y1 = mask->y1;
      *x2 = mask->x2;
      *y2 = mask->y2;

      return ! mask->empty;
    }

  /*  go through and calculate the bounds  */
  tx1 = GIMP_ITEM (mask)->width;
  ty1 = GIMP_ITEM (mask)->height;
  tx2 = 0;
  ty2 = 0;

  pixel_region_init (&maskPR, GIMP_DRAWABLE (mask)->tiles,
		     0, 0,
		     GIMP_ITEM (mask)->width,
		     GIMP_ITEM (mask)->height, FALSE);

  for (pr = pixel_regions_register (1, &maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      data1 = data = maskPR.data;
      ex = maskPR.x + maskPR.w;
      ey = maskPR.y + maskPR.h;
      /* only check the pixels if this tile is not fully within the currently
	 computed bounds */
      if (maskPR.x < tx1 || ex > tx2 ||
	  maskPR.y < ty1 || ey > ty2)
        {
	  /* Check upper left and lower right corners to see if we can
	     avoid checking the rest of the pixels in this tile */
	  if (data[0] && data[maskPR.rowstride*(maskPR.h - 1) + maskPR.w - 1])
            {
              if (maskPR.x < tx1)
                tx1 = maskPR.x;
              if (ex > tx2)
                tx2 = ex;
              if (maskPR.y < ty1)
                ty1 = maskPR.y;
              if (ey > ty2)
                ty2 = ey;
            }
	  else
            {
              for (y = maskPR.y; y < ey; y++, data1 += maskPR.rowstride)
                {
                  for (x = maskPR.x, data = data1; x < ex; x++, data++)
                    {
                      if (*data)
                        {
                          minx = x;
                          maxx = x;

                          for (; x < ex; x++, data++)
                            if (*data)
                              maxx = x;

                          if (minx < tx1)
                            tx1 = minx;
                          if (maxx > tx2)
                            tx2 = maxx;
                          if (y < ty1)
                            ty1 = y;
                          if (y > ty2)
                            ty2 = y;
                        }
                    }
                }
            }
        }
    }

  tx2 = CLAMP (tx2 + 1, 0, GIMP_ITEM (mask)->width);
  ty2 = CLAMP (ty2 + 1, 0, GIMP_ITEM (mask)->height);

  if (tx1 == GIMP_ITEM (mask)->width && ty1 == GIMP_ITEM (mask)->height)
    {
      mask->empty = TRUE;
      mask->x1    = 0;
      mask->y1    = 0;
      mask->x2    = GIMP_ITEM (mask)->width;
      mask->y2    = GIMP_ITEM (mask)->height;
    }
  else
    {
      mask->empty = FALSE;
      mask->x1    = tx1;
      mask->y1    = ty1;
      mask->x2    = tx2;
      mask->y2    = ty2;
    }

  mask->bounds_known = TRUE;

  *x1 = tx1;
  *x2 = tx2;
  *y1 = ty1;
  *y2 = ty2;

  return !mask->empty;
}

gboolean
gimp_channel_is_empty (GimpChannel *mask)
{
  PixelRegion  maskPR;
  guchar      *data;
  gint         x, y;
  gpointer     pr;

  g_return_val_if_fail (GIMP_IS_CHANNEL (mask), FALSE);

  if (mask->bounds_known)
    return mask->empty;

  pixel_region_init (&maskPR, GIMP_DRAWABLE (mask)->tiles,
		     0, 0,
		     GIMP_ITEM (mask)->width,
		     GIMP_ITEM (mask)->height, FALSE);

  for (pr = pixel_regions_register (1, &maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      /*  check if any pixel in the mask is non-zero  */
      data = maskPR.data;

      for (y = 0; y < maskPR.h; y++)
	for (x = 0; x < maskPR.w; x++)
	  if (*data++)
	    {
	      pixel_regions_process_stop (pr);
	      return FALSE;
	    }
    }

  /*  The mask is empty, meaning we can set the bounds as known  */
  if (mask->segs_in)
    g_free (mask->segs_in);
  if (mask->segs_out)
    g_free (mask->segs_out);

  mask->empty          = TRUE;
  mask->segs_in        = NULL;
  mask->segs_out       = NULL;
  mask->num_segs_in    = 0;
  mask->num_segs_out   = 0;
  mask->bounds_known   = TRUE;
  mask->boundary_known = TRUE;
  mask->x1             = 0;
  mask->y1             = 0;
  mask->x2             = GIMP_ITEM (mask)->width;
  mask->y2             = GIMP_ITEM (mask)->height;

  return TRUE;
}

void
gimp_channel_add_segment (GimpChannel *mask,
			  gint         x,
			  gint         y,
			  gint         width,
			  gint         value)
{
  PixelRegion  maskPR;
  guchar      *data;
  gint         val;
  gint         x2;
  gpointer     pr;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  /*  check horizontal extents...  */
  x2 = x + width;
  x2 = CLAMP (x2, 0, GIMP_ITEM (mask)->width);
  x  = CLAMP (x,  0, GIMP_ITEM (mask)->width);
  width = x2 - x;
  if (!width)
    return;

  if (y < 0 || y > GIMP_ITEM (mask)->height)
    return;

  pixel_region_init (&maskPR, GIMP_DRAWABLE (mask)->tiles,
		     x, y, width, 1, TRUE);

  for (pr = pixel_regions_register (1, &maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      data = maskPR.data;
      width = maskPR.w;
      while (width--)
	{
	  val = *data + value;
	  if (val > 255)
	    val = 255;
	  *data++ = val;
	}
    }
}

void
gimp_channel_sub_segment (GimpChannel *mask,
			  gint         x,
			  gint         y,
			  gint         width,
			  gint         value)
{
  PixelRegion  maskPR;
  guchar      *data;
  gint         val;
  gint         x2;
  gpointer     pr;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  /*  check horizontal extents...  */
  x2 = x + width;
  x2 = CLAMP (x2, 0, GIMP_ITEM (mask)->width);
  x =  CLAMP (x,  0, GIMP_ITEM (mask)->width);
  width = x2 - x;

  if (! width)
    return;

  if (y < 0 || y > GIMP_ITEM (mask)->height)
    return;

  pixel_region_init (&maskPR, GIMP_DRAWABLE (mask)->tiles,
                     x, y, width, 1, TRUE);

  for (pr = pixel_regions_register (1, &maskPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      data = maskPR.data;
      width = maskPR.w;
      while (width--)
	{
	  val = *data - value;
	  if (val < 0)
	    val = 0;
	  *data++ = val;
	}
    }
}

void
gimp_channel_combine_rect (GimpChannel    *mask,
			   GimpChannelOps  op,
			   gint            x,
			   gint            y,
			   gint            w,
			   gint            h)
{
  gint        x2, y2;
  PixelRegion maskPR;
  guchar      color;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  y2 = y + h;
  x2 = x + w;

  x  = CLAMP (x,  0, GIMP_ITEM (mask)->width);
  y  = CLAMP (y,  0, GIMP_ITEM (mask)->height);
  x2 = CLAMP (x2, 0, GIMP_ITEM (mask)->width);
  y2 = CLAMP (y2, 0, GIMP_ITEM (mask)->height);

  if (x2 - x <= 0 || y2 - y <= 0)
    return;

  pixel_region_init (&maskPR, GIMP_DRAWABLE (mask)->tiles,
		     x, y, x2 - x, y2 - y, TRUE);

  if (op == GIMP_CHANNEL_OP_ADD || op == GIMP_CHANNEL_OP_REPLACE)
    color = OPAQUE_OPACITY;
  else
    color = TRANSPARENT_OPACITY;

  color_region (&maskPR, &color);

  /*  Determine new boundary  */
  if (mask->bounds_known && (op == GIMP_CHANNEL_OP_ADD) && !mask->empty)
    {
      if (x < mask->x1)
	mask->x1 = x;
      if (y < mask->y1)
	mask->y1 = y;
      if ((x + w) > mask->x2)
	mask->x2 = (x + w);
      if ((y + h) > mask->y2)
	mask->y2 = (y + h);
    }
  else if (op == GIMP_CHANNEL_OP_REPLACE || mask->empty)
    {
      mask->empty = FALSE;
      mask->x1    = x;
      mask->y1    = y;
      mask->x2    = x + w;
      mask->y2    = y + h;
    }
  else
    mask->bounds_known = FALSE;

  mask->x1 = CLAMP (mask->x1, 0, GIMP_ITEM (mask)->width);
  mask->y1 = CLAMP (mask->y1, 0, GIMP_ITEM (mask)->height);
  mask->x2 = CLAMP (mask->x2, 0, GIMP_ITEM (mask)->width);
  mask->y2 = CLAMP (mask->y2, 0, GIMP_ITEM (mask)->height);
}

void
gimp_channel_combine_ellipse (GimpChannel    *mask,
			      GimpChannelOps  op,
			      gint            x,
			      gint            y,
			      gint            w,
			      gint            h,
			      gboolean        antialias)
{
  gint   i, j;
  gint   x0, x1, x2;
  gint   val, last;
  gfloat a_sqr, b_sqr, aob_sqr;
  gfloat w_sqr, h_sqr;
  gfloat y_sqr;
  gfloat t0, t1;
  gfloat r;
  gfloat cx, cy;
  gfloat rad;
  gfloat dist;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  if (!w || !h)
    return;

  a_sqr = (w * w / 4.0);
  b_sqr = (h * h / 4.0);
  aob_sqr = a_sqr / b_sqr;

  cx = x + w / 2.0;
  cy = y + h / 2.0;

  for (i = y; i < (y + h); i++)
    {
      if (i >= 0 && i < GIMP_ITEM (mask)->height)
	{
	  /*  Non-antialiased code  */
	  if (!antialias)
	    {
	      y_sqr = (i + 0.5 - cy) * (i + 0.5 - cy);
	      rad = sqrt (a_sqr - a_sqr * y_sqr / (double) b_sqr);
	      x1 = ROUND (cx - rad);
	      x2 = ROUND (cx + rad);

	      switch (op)
		{
		case GIMP_CHANNEL_OP_ADD:
		case GIMP_CHANNEL_OP_REPLACE:
		  gimp_channel_add_segment (mask, x1, i, (x2 - x1), 255);
		  break;

		case GIMP_CHANNEL_OP_SUBTRACT:
		  gimp_channel_sub_segment (mask, x1, i, (x2 - x1), 255);
		  break;

		default:
		  g_warning ("Only ADD, REPLACE, and SUBTRACT are valid for channel_combine!");
		  break;
		}
	    }
	  /*  antialiasing  */
	  else
	    {
	      x0 = x;
	      last = 0;
	      h_sqr = (i + 0.5 - cy) * (i + 0.5 - cy);
	      for (j = x; j < (x + w); j++)
		{
		  w_sqr = (j + 0.5 - cx) * (j + 0.5 - cx);

		  if (h_sqr != 0)
		    {
		      t0 = w_sqr / h_sqr;
		      t1 = a_sqr / (t0 + aob_sqr);
		      r = sqrt (t1 + t0 * t1);
		      rad = sqrt (w_sqr + h_sqr);
		      dist = rad - r;
		    }
		  else
		    dist = -1.0;

		  if (dist < -0.5)
		    val = 255;
		  else if (dist < 0.5)
		    val = (int) (255 * (1 - (dist + 0.5)));
		  else
		    val = 0;
		  
		  if (last != val && last)
		    {
		      switch (op)
			{
			case GIMP_CHANNEL_OP_ADD:
			case GIMP_CHANNEL_OP_REPLACE:
			  gimp_channel_add_segment (mask, x0, i, j - x0, last);
			  break;

			case GIMP_CHANNEL_OP_SUBTRACT:
			  gimp_channel_sub_segment (mask, x0, i, j - x0, last);
			  break;

			default:
			  g_warning ("Only ADD, REPLACE, and SUBTRACT are valid for channel_combine!");
			  break;
			}
		    }

		  if (last != val)
		    {
		      x0 = j;
		      last = val;
		      /* because we are symetric accross the y axis we can
			 skip ahead a bit if we are inside the ellipse*/
		      if (val == 255 && j < cx)
			j = cx + (cx - j) - 1;
		    }
		}

	      if (last)
		{
                  switch (op)
                    {
                    case GIMP_CHANNEL_OP_ADD:
                    case GIMP_CHANNEL_OP_REPLACE:
                      gimp_channel_add_segment (mask, x0, i, j - x0, last);
                      break;

                    case GIMP_CHANNEL_OP_SUBTRACT:
                      gimp_channel_sub_segment (mask, x0, i, j - x0, last);
                      break;

                    default:
                      g_warning ("Only ADD, REPLACE, and SUBTRACT are valid for channel_combine!");
                      break;
                    }
		}
	    }

	}
    }

  /*  Determine new boundary  */
  if (mask->bounds_known && (op == GIMP_CHANNEL_OP_ADD) && !mask->empty)
    {
      if (x < mask->x1)
	mask->x1 = x;
      if (y < mask->y1)
	mask->y1 = y;
      if ((x + w) > mask->x2)
	mask->x2 = (x + w);
      if ((y + h) > mask->y2)
	mask->y2 = (y + h);
    }
  else if (op == GIMP_CHANNEL_OP_REPLACE || mask->empty)
    {
      mask->empty = FALSE;
      mask->x1    = x;
      mask->y1    = y;
      mask->x2    = x + w;
      mask->y2    = y + h;
    }
  else
    mask->bounds_known = FALSE;

  mask->x1 = CLAMP (mask->x1, 0, GIMP_ITEM (mask)->width);
  mask->y1 = CLAMP (mask->y1, 0, GIMP_ITEM (mask)->height);
  mask->x2 = CLAMP (mask->x2, 0, GIMP_ITEM (mask)->width);
  mask->y2 = CLAMP (mask->y2, 0, GIMP_ITEM (mask)->height);
}

static void
gimp_channel_combine_sub_region_add (gpointer     unused,
				     PixelRegion *srcPR,
				     PixelRegion *destPR)
{
  guchar *src, *dest;
  gint    x, y, val;

  src  = srcPR->data;
  dest = destPR->data;

  for (y = 0; y < srcPR->h; y++)
    {
      for (x = 0; x < srcPR->w; x++)
	{
	  val = dest[x] + src[x];
	  if (val > 255)
	    dest[x] = 255;
	  else
	    dest[x] = val;
	}
      src  += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}

static void
gimp_channel_combine_sub_region_sub (gpointer     unused,
				     PixelRegion *srcPR,
				     PixelRegion *destPR)
{
  guchar *src, *dest;
  gint    x, y;

  src  = srcPR->data;
  dest = destPR->data;

  for (y = 0; y < srcPR->h; y++)
    {
      for (x = 0; x < srcPR->w; x++)
	{
	  if (src[x] > dest[x])
	    dest[x] = 0;
	  else
	    dest[x]-= src[x];
	}
      src  += srcPR->rowstride;
      dest += destPR->rowstride;
    }
}

static void
gimp_channel_combine_sub_region_intersect (gpointer     unused,
					   PixelRegion *srcPR,
					   PixelRegion *destPR)
{
  guchar *src, *dest;
  gint    x, y;

  src  = srcPR->data;
  dest = destPR->data;

  for (y = 0; y < srcPR->h; y++)
    {
      for (x = 0; x < srcPR->w; x++)
	{
	  dest[x] = MIN (dest[x], src[x]);
	}
      src  += srcPR->rowstride;
      dest += destPR->rowstride;
  }
}

void
gimp_channel_combine_mask (GimpChannel    *mask,
			   GimpChannel    *add_on,
			   GimpChannelOps  op,
			   gint            off_x,
			   gint            off_y)
{
  PixelRegion srcPR, destPR;
  gint        x1, y1, x2, y2;
  gint        w, h;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));
  g_return_if_fail (GIMP_IS_CHANNEL (add_on));

  x1 = CLAMP (off_x, 0, GIMP_ITEM (mask)->width);
  y1 = CLAMP (off_y, 0, GIMP_ITEM (mask)->height);
  x2 = CLAMP (off_x + GIMP_ITEM (add_on)->width, 0,
	      GIMP_ITEM (mask)->width);
  y2 = CLAMP (off_y + GIMP_ITEM (add_on)->height, 0,
	      GIMP_ITEM (mask)->height);

  w = (x2 - x1);
  h = (y2 - y1);

  pixel_region_init (&srcPR, GIMP_DRAWABLE (add_on)->tiles,
		     (x1 - off_x), (y1 - off_y), w, h, FALSE);
  pixel_region_init (&destPR, GIMP_DRAWABLE (mask)->tiles,
                     x1, y1, w, h, TRUE);

  switch (op)
    {
    case GIMP_CHANNEL_OP_ADD:
    case GIMP_CHANNEL_OP_REPLACE:
      pixel_regions_process_parallel ((p_func)
				      gimp_channel_combine_sub_region_add,
				      NULL, 2, &srcPR, &destPR);
      break;

    case GIMP_CHANNEL_OP_SUBTRACT:
      pixel_regions_process_parallel ((p_func)
				      gimp_channel_combine_sub_region_sub,
				      NULL, 2, &srcPR, &destPR);
      break;

    case GIMP_CHANNEL_OP_INTERSECT:
      pixel_regions_process_parallel ((p_func)
				      gimp_channel_combine_sub_region_intersect,
				      NULL, 2, &srcPR, &destPR);
      break;

    default:
      g_warning ("%s: unknown operation type\n", G_GNUC_PRETTY_FUNCTION);
      break;
    }

  mask->bounds_known = FALSE;
}

void
gimp_channel_feather (GimpChannel *mask,
		      gdouble      radius_x,
		      gdouble      radius_y,
                      gboolean     push_undo)
{
  PixelRegion srcPR;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  if (push_undo)
    gimp_channel_push_undo (mask, _("Feather Channel"));

  pixel_region_init (&srcPR, GIMP_DRAWABLE (mask)->tiles,
		     0, 0,
                     gimp_item_width  (GIMP_ITEM (mask)),
                     gimp_item_height (GIMP_ITEM (mask)),
                     FALSE);
  gaussian_blur_region (&srcPR, radius_x, radius_y);

  mask->bounds_known = FALSE;
}

void
gimp_channel_sharpen (GimpChannel *mask,
                      gboolean     push_undo)
{
  PixelRegion  maskPR;
  GimpLut     *lut;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  if (push_undo)
    gimp_channel_push_undo (mask, _("Sharpen Channel"));

  pixel_region_init (&maskPR, GIMP_DRAWABLE (mask)->tiles,
		     0, 0,
                     gimp_item_width  (GIMP_ITEM (mask)),
                     gimp_item_height (GIMP_ITEM (mask)),
		     TRUE);
  lut = threshold_lut_new (0.5, 1);

  pixel_regions_process_parallel ((p_func) gimp_lut_process_inline,
				  lut, 1, &maskPR);
  gimp_lut_free (lut);

  mask->bounds_known = FALSE;
}

void
gimp_channel_clear (GimpChannel *mask,
                    gboolean     push_undo)
{
  PixelRegion maskPR;
  guchar      bg = TRANSPARENT_OPACITY;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  if (push_undo)
    gimp_channel_push_undo (mask, _("Clear Channel"));

  if (mask->bounds_known && !mask->empty)
    {
      pixel_region_init (&maskPR, GIMP_DRAWABLE (mask)->tiles,
			 mask->x1, mask->y1,
			 (mask->x2 - mask->x1), (mask->y2 - mask->y1), TRUE);
      color_region (&maskPR, &bg);
    }
  else
    {
      /*  clear the mask  */
      pixel_region_init (&maskPR, GIMP_DRAWABLE (mask)->tiles,
			 0, 0,
			 GIMP_ITEM (mask)->width,
			 GIMP_ITEM (mask)->height, TRUE);
      color_region (&maskPR, &bg);
    }

  /*  we know the bounds  */
  mask->bounds_known = TRUE;
  mask->empty        = TRUE;
  mask->x1           = 0;
  mask->y1           = 0;
  mask->x2           = GIMP_ITEM (mask)->width;
  mask->y2           = GIMP_ITEM (mask)->height;
}

void
gimp_channel_all (GimpChannel *mask,
                  gboolean     push_undo)
{
  PixelRegion maskPR;
  guchar      bg = OPAQUE_OPACITY;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  if (push_undo)
    gimp_channel_push_undo (mask, _("Fill Channel"));

  /*  clear the mask  */
  pixel_region_init (&maskPR, GIMP_DRAWABLE (mask)->tiles,
		     0, 0,
		     GIMP_ITEM (mask)->width,
		     GIMP_ITEM (mask)->height, TRUE);
  color_region (&maskPR, &bg);

  /*  we know the bounds  */
  mask->bounds_known = TRUE;
  mask->empty        = FALSE;
  mask->x1           = 0;
  mask->y1           = 0;
  mask->x2           = GIMP_ITEM (mask)->width;
  mask->y2           = GIMP_ITEM (mask)->height;
}

void
gimp_channel_invert (GimpChannel *mask,
                     gboolean     push_undo)
{
  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  if (push_undo)
    gimp_channel_push_undo (mask, _("Invert Channel"));

  if (mask->bounds_known && mask->empty)
    {
      gimp_channel_all (mask, FALSE);
    }
  else
    {
      PixelRegion  maskPR;
      GimpLut     *lut;

      pixel_region_init (&maskPR, GIMP_DRAWABLE (mask)->tiles,
                         0, 0,
                         GIMP_ITEM (mask)->width,
                         GIMP_ITEM (mask)->height, TRUE);
  
      lut = invert_lut_new (1);

      pixel_regions_process_parallel ((p_func) gimp_lut_process_inline, lut,
                                      1, &maskPR);

      gimp_lut_free (lut);

      mask->bounds_known = FALSE;
    }
}

void
gimp_channel_border (GimpChannel *mask,
		     gint         radius_x,
		     gint         radius_y,
                     gboolean     push_undo)
{
  PixelRegion bPR;
  gint        x1, y1, x2, y2;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  if (radius_x < 0 || radius_y < 0)
    return;

  if (! gimp_channel_bounds (mask, &x1, &y1, &x2, &y2))
    return;

  if (gimp_channel_is_empty (mask))
    return;

  if (x1 - radius_x < 0)
    x1 = 0;
  else
    x1 -= radius_x;
  if (x2 + radius_x > GIMP_ITEM (mask)->width)
    x2 = GIMP_ITEM (mask)->width;
  else
    x2 += radius_x;

  if (y1 - radius_y < 0)
    y1 = 0;
  else
    y1 -= radius_y;
  if (y2 + radius_y > GIMP_ITEM (mask)->height)
    y2 = GIMP_ITEM (mask)->height;
  else
    y2 += radius_y;

  if (push_undo)
    gimp_channel_push_undo (mask, _("Border Channel"));

  pixel_region_init (&bPR, GIMP_DRAWABLE (mask)->tiles, x1, y1,
		     (x2-x1), (y2-y1), TRUE);

  border_region (&bPR, radius_x, radius_y);

  mask->bounds_known = FALSE;
}

void
gimp_channel_grow (GimpChannel *mask,
		   gint         radius_x,
		   gint         radius_y,
                   gboolean     push_undo)
{
  PixelRegion bPR;
  gint        x1, y1, x2, y2;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  if (radius_x == 0 && radius_y == 0)
    return;

  if (radius_x <= 0 && radius_y <= 0)
    {
      gimp_channel_shrink (mask, -radius_x, -radius_y, FALSE, push_undo);
      return;
    }

  if (radius_x < 0 || radius_y < 0)
    return;
  
  if (! gimp_channel_bounds (mask, &x1, &y1, &x2, &y2))
    return;

  if (gimp_channel_is_empty (mask))
    return;

  if (x1 - radius_x > 0)
    x1 = x1 - radius_x;
  else
    x1 = 0;
  if (y1 - radius_y > 0)
    y1 = y1 - radius_y;
  else
    y1 = 0;
  if (x2 + radius_x < GIMP_ITEM (mask)->width)
    x2 = x2 + radius_x;
  else
    x2 = GIMP_ITEM (mask)->width;
  if (y2 + radius_y < GIMP_ITEM (mask)->height)
    y2 = y2 + radius_y;
  else
    y2 = GIMP_ITEM (mask)->height;

  if (push_undo)
    gimp_channel_push_undo (mask, _("Grow Channel"));

  /*  need full extents for grow, not! */
  pixel_region_init (&bPR, GIMP_DRAWABLE (mask)->tiles, x1, y1, (x2 - x1),
		     (y2 - y1), TRUE);

  fatten_region (&bPR, radius_x, radius_y);

  mask->bounds_known = FALSE;
}

void
gimp_channel_shrink (GimpChannel  *mask,
		     gint          radius_x,
		     gint          radius_y,
		     gboolean      edge_lock,
                     gboolean      push_undo)
{
  PixelRegion bPR;
  gint        x1, y1, x2, y2;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));

  if (radius_x == 0 && radius_y == 0)
    return;

  if (radius_x <= 0 && radius_y <= 0)
    {
      gimp_channel_grow (mask, -radius_x, -radius_y, push_undo);
      return;
    }

  if (radius_x < 0 || radius_y < 0)
    return;
  
  if (! gimp_channel_bounds (mask, &x1, &y1, &x2, &y2))
    return;

  if (gimp_channel_is_empty (mask))
    return;

  if (x1 > 0)
    x1--;
  if (y1 > 0)
    y1--;
  if (x2 < GIMP_ITEM (mask)->width)
    x2++;
  if (y2 < GIMP_ITEM (mask)->height)
    y2++;

  if (push_undo)
    gimp_channel_push_undo (mask, _("Shrink Channel"));

  pixel_region_init (&bPR, GIMP_DRAWABLE (mask)->tiles, x1, y1, (x2 - x1),
		     (y2 - y1), TRUE);

  thin_region (&bPR, radius_x, radius_y, edge_lock);

  mask->bounds_known = FALSE;
}

void
gimp_channel_load (GimpChannel *mask,
		   GimpChannel *channel,
                   gboolean     push_undo)
{
  PixelRegion srcPR, destPR;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  if (push_undo)
    gimp_channel_push_undo (mask, _("Channel Load"));

  /*  copy the channel to the mask  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE (channel)->tiles,
		     0, 0,
		     GIMP_ITEM (channel)->width,
		     GIMP_ITEM (channel)->height, FALSE);
  pixel_region_init (&destPR, GIMP_DRAWABLE (mask)->tiles,
		     0, 0,
		     GIMP_ITEM (channel)->width,
		     GIMP_ITEM (channel)->height, TRUE);
  copy_region (&srcPR, &destPR);

  mask->bounds_known = FALSE;
}

void
gimp_channel_layer_alpha (GimpChannel *mask,
			  GimpLayer   *layer,
                          gboolean     push_undo)
{
  PixelRegion srcPR, destPR;
  gint        x1, y1, x2, y2;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));
  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)));

  if (push_undo)
    gimp_channel_push_undo (mask, _("Channel from Alpha"));

  /*  clear the mask if it is not already known to be empty  */
  if (! (mask->bounds_known && mask->empty))
    gimp_channel_clear (mask, FALSE);

  x1 = CLAMP (GIMP_ITEM (layer)->offset_x, 0, GIMP_ITEM (mask)->width);
  y1 = CLAMP (GIMP_ITEM (layer)->offset_y, 0, GIMP_ITEM (mask)->height);
  x2 = CLAMP (GIMP_ITEM (layer)->offset_x + GIMP_ITEM (layer)->width,
	      0, GIMP_ITEM (mask)->width);
  y2 = CLAMP (GIMP_ITEM (layer)->offset_y + GIMP_ITEM (layer)->height,
	      0, GIMP_ITEM (mask)->height);

  pixel_region_init (&srcPR, GIMP_DRAWABLE (layer)->tiles,
		     (x1 - GIMP_ITEM (layer)->offset_x),
		     (y1 - GIMP_ITEM (layer)->offset_y),
		     (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, GIMP_DRAWABLE (mask)->tiles,
		     x1, y1, (x2 - x1), (y2 - y1), TRUE);
  extract_alpha_region (&srcPR, NULL, &destPR);

  mask->bounds_known = FALSE;
}

void
gimp_channel_layer_mask (GimpChannel *mask,
			 GimpLayer   *layer,
                         gboolean     push_undo)
{
  PixelRegion srcPR, destPR;
  gint        x1, y1, x2, y2;

  g_return_if_fail (GIMP_IS_CHANNEL (mask));
  g_return_if_fail (GIMP_IS_LAYER (layer));
  g_return_if_fail (gimp_layer_get_mask (layer));

  if (push_undo)
    gimp_channel_push_undo (mask, _("Channel from Mask"));

  /*  clear the mask if it is not already known to be empty  */
  if (! (mask->bounds_known && mask->empty))
    gimp_channel_clear (mask, FALSE);

  x1 = CLAMP (GIMP_ITEM (layer)->offset_x, 0, GIMP_ITEM (mask)->width);
  y1 = CLAMP (GIMP_ITEM (layer)->offset_y, 0, GIMP_ITEM (mask)->height);
  x2 = CLAMP (GIMP_ITEM (layer)->offset_x + GIMP_ITEM (layer)->width, 
	      0, GIMP_ITEM (mask)->width);
  y2 = CLAMP (GIMP_ITEM (layer)->offset_y + GIMP_ITEM (layer)->height, 
	      0, GIMP_ITEM (mask)->height);

  pixel_region_init (&srcPR, GIMP_DRAWABLE (layer->mask)->tiles,
		     (x1 - GIMP_ITEM (layer)->offset_x), 
		     (y1 - GIMP_ITEM (layer)->offset_y),
		     (x2 - x1), (y2 - y1), 
		     FALSE);
  pixel_region_init (&destPR, GIMP_DRAWABLE (mask)->tiles, 
		     x1, y1, 
		     (x2 - x1), (y2 - y1), 
		     TRUE);
  copy_region (&srcPR, &destPR);

  mask->bounds_known = FALSE;
}

void
gimp_channel_invalidate_bounds (GimpChannel *channel)
{
  g_return_if_fail (GIMP_IS_CHANNEL (channel));

  channel->bounds_known = FALSE;
}
