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
#include <stdio.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable.h"
#include "gimpdrawable-preview.h"
#include "gimpimage.h"
#include "gimpimage-mask.h"
#include "gimplayer.h"
#include "gimplist.h"
#include "gimpmarshal.h"
#include "gimpparasite.h"
#include "gimpparasitelist.h"
#include "gimppreviewcache.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


enum
{
  VISIBILITY_CHANGED,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void    gimp_drawable_class_init         (GimpDrawableClass *klass);
static void    gimp_drawable_init               (GimpDrawable      *drawable);

static void    gimp_drawable_finalize           (GObject           *object);

static gsize   gimp_drawable_get_memsize        (GimpObject        *object);

static void    gimp_drawable_invalidate_preview (GimpViewable      *viewable);


/*  private variables  */

static guint gimp_drawable_signals[LAST_SIGNAL] = { 0 };

static GimpItemClass *parent_class = NULL;


GType
gimp_drawable_get_type (void)
{
  static GType drawable_type = 0;

  if (! drawable_type)
    {
      static const GTypeInfo drawable_info =
      {
        sizeof (GimpDrawableClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_drawable_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpDrawable),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_drawable_init,
      };

      drawable_type = g_type_register_static (GIMP_TYPE_ITEM,
					      "GimpDrawable", 
					      &drawable_info, 0);
    }

  return drawable_type;
}

static void
gimp_drawable_class_init (GimpDrawableClass *klass)
{
  GObjectClass      *object_class;
  GimpObjectClass   *gimp_object_class;
  GimpViewableClass *viewable_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);
  viewable_class    = GIMP_VIEWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gimp_drawable_signals[VISIBILITY_CHANGED] =
    g_signal_new ("visibility_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpDrawableClass, visibility_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->finalize             = gimp_drawable_finalize;

  gimp_object_class->get_memsize     = gimp_drawable_get_memsize;

  viewable_class->invalidate_preview = gimp_drawable_invalidate_preview;
  viewable_class->get_preview        = gimp_drawable_get_preview;

  klass->visibility_changed          = NULL;
}

static void
gimp_drawable_init (GimpDrawable *drawable)
{
  drawable->tiles         = NULL;
  drawable->visible       = FALSE;
  drawable->width         = 0;
  drawable->height        = 0;
  drawable->offset_x      = 0;
  drawable->offset_y      = 0;
  drawable->bytes         = 0;
  drawable->type          = -1;
  drawable->has_alpha     = FALSE;
  drawable->preview_cache = NULL;
  drawable->preview_valid = FALSE;
  drawable->preview_cache = NULL;
  drawable->preview_valid = FALSE;
}

static void
gimp_drawable_finalize (GObject *object)
{
  GimpDrawable *drawable;

  g_return_if_fail (GIMP_IS_DRAWABLE (object));

  drawable = GIMP_DRAWABLE (object);

  if (drawable->tiles)
    {
      tile_manager_destroy (drawable->tiles);
      drawable->tiles = NULL;
    }

  if (drawable->preview_cache)
    gimp_preview_cache_invalidate (&drawable->preview_cache);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gsize
gimp_drawable_get_memsize (GimpObject *object)
{
  GimpDrawable *drawable;
  gsize         memsize = 0;

  drawable = GIMP_DRAWABLE (object);

  if (drawable->tiles)
    memsize += tile_manager_get_memsize (drawable->tiles);

  if (drawable->preview_cache)
    memsize += gimp_preview_cache_get_memsize (drawable->preview_cache);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object);
}

static void
gimp_drawable_invalidate_preview (GimpViewable *viewable)
{
  GimpDrawable *drawable;
  GimpImage    *gimage;

  if (GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview)
    GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview (viewable);

  drawable = GIMP_DRAWABLE (viewable);

  drawable->preview_valid = FALSE;

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  if (gimage)
    gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));
}

void
gimp_drawable_configure (GimpDrawable  *drawable,
			 GimpImage     *gimage,
			 gint           width,
			 gint           height, 
			 GimpImageType  type,
			 const gchar   *name)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (name != NULL);

  GIMP_ITEM (drawable)->ID = gimage->gimp->next_item_ID++;

  g_hash_table_insert (gimage->gimp->item_table,
		       GINT_TO_POINTER (GIMP_ITEM (drawable)->ID),
		       drawable);

  drawable->width     = width;
  drawable->height    = height;
  drawable->type      = type;
  drawable->bytes     = GIMP_IMAGE_TYPE_BYTES (type);
  drawable->has_alpha = GIMP_IMAGE_TYPE_HAS_ALPHA (type);
  drawable->offset_x  = 0;
  drawable->offset_y  = 0;

  if (drawable->tiles)
    tile_manager_destroy (drawable->tiles);

  drawable->tiles = tile_manager_new (width, height,
                                      drawable->bytes);

  drawable->visible = TRUE;

  gimp_item_set_image (GIMP_ITEM (drawable), gimage);

  gimp_object_set_name (GIMP_OBJECT (drawable), name);

  /*  preview variables  */
  drawable->preview_cache = NULL;
  drawable->preview_valid = FALSE;
}

GimpDrawable *
gimp_drawable_copy (GimpDrawable *drawable,
                    GType         new_type,
                    gboolean      add_alpha)
{
  GimpDrawable  *new_drawable;
  GimpImageType  new_image_type;
  PixelRegion    srcPR;
  PixelRegion    destPR;
  gchar         *new_name;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  /*  formulate the new name  */
  {
    const gchar *name;
    gchar       *ext;
    gint         number;
    gint         len;

    name = gimp_object_get_name (GIMP_OBJECT (drawable));

    ext = strrchr (name, '#');
    len = strlen (_("copy"));

    if ((strlen (name) >= len &&
         strcmp (&name[strlen (name) - len], _("copy")) == 0) ||
        (ext && (number = atoi (ext + 1)) > 0 && 
         ((int)(log10 (number) + 1)) == strlen (ext + 1)))
      {
        /* don't have redundant "copy"s */
        new_name = g_strdup (name);
      }
    else
      {
        new_name = g_strdup_printf (_("%s copy"), name);
      }
  }

  if (add_alpha)
    {
      new_image_type = gimp_drawable_type_with_alpha (drawable);
    }
  else
    {
      new_image_type = drawable->type;
    }

  new_drawable = g_object_new (new_type, NULL);

  gimp_drawable_configure (new_drawable,
                           gimp_item_get_image (GIMP_ITEM (drawable)),
                           gimp_drawable_width (drawable),
                           gimp_drawable_height (drawable),
                           new_image_type,
                           new_name);
  g_free (new_name);

  new_drawable->offset_x = drawable->offset_x;
  new_drawable->offset_y = drawable->offset_y;
  new_drawable->visible  = drawable->visible;

  pixel_region_init (&srcPR, drawable->tiles, 
                     0, 0, 
                     drawable->width, 
                     drawable->height, 
                     FALSE);
  pixel_region_init (&destPR, new_drawable->tiles,
                     0, 0, 
                     drawable->width, 
                     drawable->height, 
                     TRUE);

  if (new_image_type == drawable->type)
    {
      copy_region (&srcPR, &destPR);
    }
  else
    {
      add_alpha_region (&srcPR, &destPR);
    }

  g_object_unref (G_OBJECT (GIMP_ITEM (new_drawable)->parasites));
  GIMP_ITEM (new_drawable)->parasites =
    gimp_parasite_list_copy (GIMP_ITEM (drawable)->parasites);

  return new_drawable;
}

void
gimp_drawable_update (GimpDrawable *drawable,
		      gint          x,
		      gint          y,
		      gint          w,
		      gint          h)
{
  GimpImage *gimage;
  gint       offset_x;
  gint       offset_y;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  g_return_if_fail (gimage != NULL);

  gimp_drawable_offsets (drawable, &offset_x, &offset_y);
  x += offset_x;
  y += offset_y;

  gimp_image_update (gimage, x, y, w, h);

  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (drawable));
}

void
gimp_drawable_apply_image (GimpDrawable *drawable, 
			   gint          x1,
			   gint          y1,
			   gint          x2,
			   gint          y2, 
			   TileManager  *tiles,
			   gint          sparse)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  if (! tiles)
    undo_push_image (gimp_item_get_image (GIMP_ITEM (drawable)),
                     drawable, 
		     x1, y1, x2, y2);
  else
    undo_push_image_mod (gimp_item_get_image (GIMP_ITEM (drawable)),
                         drawable, 
			 x1, y1, x2, y2,
                         tiles, sparse);
}

void
gimp_drawable_merge_shadow (GimpDrawable *drawable,
			    gboolean      undo)
{
  GimpImage   *gimage;
  PixelRegion  shadowPR;
  gint         x1, y1, x2, y2;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (gimage->shadow != NULL);

  /*  A useful optimization here is to limit the update to the
   *  extents of the selection mask, as it cannot extend beyond
   *  them.
   */
  gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
  pixel_region_init (&shadowPR, gimage->shadow, x1, y1,
		     (x2 - x1), (y2 - y1), FALSE);
  gimp_image_apply_image (gimage, drawable, &shadowPR, undo,
                          GIMP_OPACITY_OPAQUE, GIMP_REPLACE_MODE,
                          NULL, x1, y1);
}

void
gimp_drawable_fill (GimpDrawable  *drawable,
		    const GimpRGB *color)
{
  GimpImage   *gimage;
  PixelRegion  destPR;
  guchar       c[MAX_CHANNELS];
  guchar       i;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  g_return_if_fail (gimage != NULL);

  switch (GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (drawable)))
    {
    case GIMP_RGB:
      gimp_rgba_get_uchar (color,
			   &c[RED_PIX],
			   &c[GREEN_PIX],
			   &c[BLUE_PIX],
			   &c[ALPHA_PIX]);
      if (gimp_drawable_type (drawable) != GIMP_RGBA_IMAGE)
	c[ALPHA_PIX] = 255;
      break;

    case GIMP_GRAY:
      gimp_rgba_get_uchar (color,
			   &c[GRAY_PIX],
			   NULL,
			   NULL,
			   &c[ALPHA_G_PIX]);
      if (gimp_drawable_type (drawable) != GIMP_GRAYA_IMAGE)
	c[ALPHA_G_PIX] = 255;
      break;

    case GIMP_INDEXED:
      gimp_rgb_get_uchar (color,
			  &c[RED_PIX],
			  &c[GREEN_PIX],
			  &c[BLUE_PIX]);
      gimp_image_transform_color (gimage, drawable, c, &i, GIMP_RGB);
      c[INDEXED_PIX] = i;
      if (gimp_drawable_type (drawable) == GIMP_INDEXEDA_IMAGE)
	gimp_rgba_get_uchar (color,
			     NULL,
			     NULL,
			     NULL,
			     &c[ALPHA_I_PIX]);
      break;

    default:
      g_warning ("%s: Cannot fill unknown image type.", 
                 G_GNUC_PRETTY_FUNCTION);
      break;
    }

  pixel_region_init (&destPR,
		     gimp_drawable_data (drawable),
		     0, 0,
		     gimp_drawable_width  (drawable),
		     gimp_drawable_height (drawable),
		     TRUE);
  color_region (&destPR, c);

  gimp_drawable_update (drawable,
			0, 0,
			gimp_drawable_width  (drawable),
			gimp_drawable_height (drawable));
}

void
gimp_drawable_fill_by_type (GimpDrawable *drawable,
			    GimpContext  *context,
			    GimpFillType  fill_type)
{
  GimpRGB color;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  color.a = GIMP_OPACITY_OPAQUE;

  switch (fill_type)
    {
    case GIMP_FOREGROUND_FILL:
      gimp_context_get_foreground (context, &color);
      break;

    case  GIMP_BACKGROUND_FILL:
      gimp_context_get_background (context, &color);
      break;

    case GIMP_WHITE_FILL:
      gimp_rgb_set (&color, 1.0, 1.0, 1.0);
      break;

    case GIMP_TRANSPARENT_FILL:
      gimp_rgba_set (&color, 0.0, 0.0, 0.0, GIMP_OPACITY_TRANSPARENT);
      break;

    case  GIMP_NO_FILL:
      return;

    default:
      g_warning ("%s: unknown fill type %d", G_GNUC_PRETTY_FUNCTION, fill_type);
      return;
    }

  gimp_drawable_fill (drawable, &color);
}

gboolean
gimp_drawable_mask_bounds (GimpDrawable *drawable, 
			   gint         *x1,
			   gint         *y1,
			   gint         *x2,
			   gint         *y2)
{
  GimpImage *gimage;
  gint       off_x, off_y;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  g_return_val_if_fail (gimage != NULL, FALSE);

  if (gimp_image_mask_bounds (gimage, x1, y1, x2, y2))
    {
      gimp_drawable_offsets (drawable, &off_x, &off_y);
      *x1 = CLAMP (*x1 - off_x, 0, gimp_drawable_width  (drawable));
      *y1 = CLAMP (*y1 - off_y, 0, gimp_drawable_height (drawable));
      *x2 = CLAMP (*x2 - off_x, 0, gimp_drawable_width  (drawable));
      *y2 = CLAMP (*y2 - off_y, 0, gimp_drawable_height (drawable));
      return TRUE;
    }
  else
    {
      *x2 = gimp_drawable_width  (drawable);
      *y2 = gimp_drawable_height (drawable);
      return FALSE;
    }
}

gboolean
gimp_drawable_has_alpha (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return drawable->has_alpha;
}

GimpImageType
gimp_drawable_type (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->type;
}

GimpImageType
gimp_drawable_type_with_alpha (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return GIMP_IMAGE_TYPE_WITH_ALPHA (gimp_drawable_type (drawable));
}

gboolean
gimp_drawable_is_rgb (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return GIMP_IMAGE_TYPE_IS_RGB (gimp_drawable_type (drawable));
}

gboolean
gimp_drawable_is_gray (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return GIMP_IMAGE_TYPE_IS_GRAY (gimp_drawable_type (drawable));
}

gboolean
gimp_drawable_is_indexed (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return GIMP_IMAGE_TYPE_IS_INDEXED (gimp_drawable_type (drawable));
}

TileManager *
gimp_drawable_data (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return drawable->tiles;
}

TileManager *
gimp_drawable_shadow (GimpDrawable *drawable)
{
  GimpImage *gimage;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  if (! (gimage = gimp_item_get_image (GIMP_ITEM (drawable))))
    return NULL;

  return gimp_image_shadow (gimage, drawable->width, drawable->height, 
			    drawable->bytes);
}

gint
gimp_drawable_bytes (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->bytes;
}

gint
gimp_drawable_bytes_with_alpha (const GimpDrawable *drawable)
{
  GimpImageType type;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  type = GIMP_IMAGE_TYPE_WITH_ALPHA (gimp_drawable_type (drawable));

  return GIMP_IMAGE_TYPE_BYTES (type);
}

gint
gimp_drawable_width (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->width;
}

gint
gimp_drawable_height (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->height;
}

gboolean
gimp_drawable_get_visible (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return drawable->visible;
}

void
gimp_drawable_set_visible (GimpDrawable *drawable,
                           gboolean      visible)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  visible = visible ? TRUE : FALSE;

  if (drawable->visible != visible)
    {
      drawable->visible = visible;

      g_signal_emit (G_OBJECT (drawable),
		     gimp_drawable_signals[VISIBILITY_CHANGED], 0);

      gimp_drawable_update (drawable,
			    0, 0,
			    drawable->width,
			    drawable->height);
    }
}

void
gimp_drawable_offsets (const GimpDrawable *drawable,
		       gint               *off_x,
		       gint               *off_y)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  *off_x = drawable->offset_x;
  *off_y = drawable->offset_y;
}

guchar *
gimp_drawable_cmap (const GimpDrawable *drawable)
{
  GimpImage *gimage;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  if (! (gimage = gimp_item_get_image (GIMP_ITEM (drawable))))
    return NULL;

  return gimage->cmap;
}

guchar *
gimp_drawable_get_color_at (GimpDrawable *drawable,
			    gint          x,
			    gint          y)
{
  Tile   *tile;
  guchar *src;
  guchar *dest;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_get_image (GIMP_ITEM (drawable)) ||
			! gimp_drawable_is_indexed (drawable), NULL);

  /* do not make this a g_return_if_fail() */
  if ( !(x >= 0 && x < drawable->width && y >= 0 && y < drawable->height))
    return NULL;

  dest = g_new (guchar, 5);

  tile = tile_manager_get_tile (gimp_drawable_data (drawable), x, y,
				TRUE, FALSE);
  src = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);

  gimp_image_get_color (gimp_item_get_image (GIMP_ITEM (drawable)),
			gimp_drawable_type (drawable), dest, src);

  if (GIMP_IMAGE_TYPE_HAS_ALPHA (gimp_drawable_type (drawable)))
    dest[3] = src[gimp_drawable_bytes (drawable) - 1];
  else
    dest[3] = 255;

  if (gimp_drawable_is_indexed (drawable))
    dest[4] = src[0];
  else
    dest[4] = 0;

  tile_release (tile, FALSE);

  return dest;
}
