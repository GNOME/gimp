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

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/boundary.h"
#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "gimpdrawable-invert.h"
#include "gimpcontainer.h"
#include "gimpimage.h"
#include "gimpimage-convert.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-floating-sel.h"
#include "gimplayermask.h"
#include "gimpmarshal.h"
#include "gimpparasitelist.h"

#include "gimp-intl.h"


enum
{
  OPACITY_CHANGED,
  MODE_CHANGED,
  PRESERVE_TRANS_CHANGED,
  LINKED_CHANGED,
  MASK_CHANGED,
  LAST_SIGNAL
};


static void       gimp_layer_class_init         (GimpLayerClass     *klass);
static void       gimp_layer_init               (GimpLayer          *layer);

static void       gimp_layer_finalize           (GObject            *object);

static gsize      gimp_layer_get_memsize        (GimpObject         *object);

static void       gimp_layer_invalidate_preview (GimpViewable       *viewable);

static GimpItem * gimp_layer_duplicate          (GimpItem           *item,
                                                 GType               new_type,
                                                 gboolean            add_alpha);
static void       gimp_layer_rename             (GimpItem           *item,
                                                 const gchar        *new_name,
                                                 const gchar        *undo_desc);
static void       gimp_layer_translate          (GimpItem           *item,
                                                 gint                offset_x,
                                                 gint                offset_y);
static void       gimp_layer_scale              (GimpItem           *item,
                                                 gint                new_width,
                                                 gint                new_height,
                                                 gint                new_offset_x,
                                                 gint                new_offset_y,
                                                 GimpInterpolationType  interp_type);
static void       gimp_layer_resize             (GimpItem           *item,
                                                 gint                new_width,
                                                 gint                new_height,
                                                 gint                offset_x,
                                                 gint                offset_y);

static void       gimp_layer_transform_color    (GimpImage          *gimage,
                                                 PixelRegion        *layerPR,
                                                 PixelRegion        *bufPR,
                                                 GimpDrawable       *drawable,
                                                 GimpImageBaseType   type);


static guint  layer_signals[LAST_SIGNAL] = { 0 };

static GimpDrawableClass *parent_class   = NULL;


GType
gimp_layer_get_type (void)
{
  static GType layer_type = 0;

  if (! layer_type)
    {
      static const GTypeInfo layer_info =
      {
        sizeof (GimpLayerClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_layer_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpLayer),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_layer_init,
      };

      layer_type = g_type_register_static (GIMP_TYPE_DRAWABLE,
					   "GimpLayer",
					   &layer_info, 0);
    }

  return layer_type;
}

static void
gimp_layer_class_init (GimpLayerClass *klass)
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

  layer_signals[OPACITY_CHANGED] =
    g_signal_new ("opacity_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpLayerClass, opacity_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  layer_signals[MODE_CHANGED] =
    g_signal_new ("mode_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpLayerClass, mode_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  layer_signals[PRESERVE_TRANS_CHANGED] =
    g_signal_new ("preserve_trans_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpLayerClass, preserve_trans_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  layer_signals[LINKED_CHANGED] =
    g_signal_new ("linked_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpLayerClass, linked_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  layer_signals[MASK_CHANGED] =
    g_signal_new ("mask_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpLayerClass, mask_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->finalize             = gimp_layer_finalize;

  gimp_object_class->get_memsize     = gimp_layer_get_memsize;

  viewable_class->default_stock_id   = "gimp-layer";
  viewable_class->invalidate_preview = gimp_layer_invalidate_preview;

  item_class->duplicate              = gimp_layer_duplicate;
  item_class->rename                 = gimp_layer_rename;
  item_class->translate              = gimp_layer_translate;
  item_class->scale                  = gimp_layer_scale;
  item_class->resize                 = gimp_layer_resize;
  item_class->default_name           = _("Layer");
  item_class->rename_desc            = _("Rename Layer");
  item_class->translate_desc         = _("Move Layer");

  klass->opacity_changed             = NULL;
  klass->mode_changed                = NULL;
  klass->preserve_trans_changed      = NULL;
  klass->linked_changed              = NULL;
  klass->mask_changed                = NULL;
}

static void
gimp_layer_init (GimpLayer *layer)
{
  layer->opacity        = GIMP_OPACITY_OPAQUE;
  layer->mode           = GIMP_NORMAL_MODE;
  layer->preserve_trans = FALSE;
  layer->linked         = FALSE;

  layer->mask           = NULL;

  /*  floating selection  */
  layer->fs.backing_store  = NULL;
  layer->fs.drawable       = NULL;
  layer->fs.initial        = TRUE;
  layer->fs.boundary_known = FALSE;
  layer->fs.segs           = NULL;
  layer->fs.num_segs       = 0;
}

static void
gimp_layer_finalize (GObject *object)
{
  GimpLayer *layer;

  g_return_if_fail (GIMP_IS_LAYER (object));

  layer = GIMP_LAYER (object);

  if (layer->mask)
    {
      g_object_unref (layer->mask);
      layer->mask = NULL;
    }

  if (layer->fs.segs)
    {
      g_free (layer->fs.segs);
      layer->fs.segs = NULL;
    }

  /*  free the floating selection if it exists  */
  if (layer->fs.backing_store)
    {
      tile_manager_destroy (layer->fs.backing_store);
      layer->fs.backing_store = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gsize
gimp_layer_get_memsize (GimpObject *object)
{
  GimpLayer *layer;
  gsize      memsize = 0;

  layer = GIMP_LAYER (object);

  if (layer->mask)
    memsize += gimp_object_get_memsize (GIMP_OBJECT (layer->mask));

  if (layer->fs.backing_store)
    memsize += tile_manager_get_memsize (layer->fs.backing_store);

  memsize += layer->fs.num_segs * sizeof (BoundSeg);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object);
}

static void
gimp_layer_invalidate_preview (GimpViewable *viewable)
{
  GimpLayer *layer;

  layer = GIMP_LAYER (viewable);

  if (GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview)
    GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview (viewable);

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_invalidate (layer);
}

static GimpItem *
gimp_layer_duplicate (GimpItem *item,
                      GType     new_type,
                      gboolean  add_alpha)
{
  GimpLayer *layer;
  GimpItem  *new_item;
  GimpLayer *new_layer;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type,
                                                        add_alpha);

  if (! GIMP_IS_LAYER (new_item))
    return new_item;

  layer     = GIMP_LAYER (item);
  new_layer = GIMP_LAYER (new_item);

  new_layer->linked         = layer->linked;
  new_layer->preserve_trans = layer->preserve_trans;

  new_layer->mode           = layer->mode;
  new_layer->opacity        = layer->opacity;

  /*  duplicate the layer mask if necessary  */
  if (layer->mask)
    {
      new_layer->mask =
        GIMP_LAYER_MASK (gimp_item_duplicate (GIMP_ITEM (layer->mask),
                                              G_TYPE_FROM_INSTANCE (layer->mask),
                                              FALSE));

      gimp_layer_mask_set_layer (new_layer->mask, new_layer);
    }

  return new_item;
}

static void
gimp_layer_rename (GimpItem    *item,
                   const gchar *new_name,
                   const gchar *undo_desc)
{
  GimpImage *gimage;
  gboolean   floating_sel = FALSE;

  gimage = gimp_item_get_image (item);
  floating_sel = gimp_layer_is_floating_sel (GIMP_LAYER (item));

  if (gimage && floating_sel)
    {
      gimp_image_undo_group_start (gimage,
                                   GIMP_UNDO_GROUP_ITEM_PROPERTIES,
                                   undo_desc);

      floating_sel_to_layer (GIMP_LAYER (item));
    }

  GIMP_ITEM_CLASS (parent_class)->rename (item, new_name, undo_desc);

  if (gimage && floating_sel)
    gimp_image_undo_group_end (gimage);
}

static void
gimp_layer_translate (GimpItem *item,
		      gint      off_x,
		      gint      off_y)
{
  GimpLayer *layer;

  layer = GIMP_LAYER (item);

  /*  update the old region  */
  gimp_drawable_update (GIMP_DRAWABLE (layer), 0, 0, item->width, item->height);

  /*  invalidate the selection boundary because of a layer modification  */
  gimp_layer_invalidate_boundary (layer);

  GIMP_ITEM_CLASS (parent_class)->translate (item, off_x, off_y);

  /*  update the new region  */
  gimp_drawable_update (GIMP_DRAWABLE (layer), 0, 0, item->width, item->height);

  if (layer->mask) 
    {
      GIMP_ITEM (layer->mask)->offset_x = item->offset_x;
      GIMP_ITEM (layer->mask)->offset_y = item->offset_y;

      /*  invalidate the mask preview  */
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer->mask));
    }
}

static void
gimp_layer_scale (GimpItem              *item,
                  gint                   new_width,
                  gint                   new_height,
                  gint                   new_offset_x,
                  gint                   new_offset_y,
                  GimpInterpolationType  interpolation_type)
{
  GimpLayer *layer;
  GimpImage *gimage;

  layer  = GIMP_LAYER (item);
  gimage = gimp_item_get_image (item);

  if (layer->mask)
    gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_LAYER_SCALE,
                                 _("Scale Layer"));

  gimp_image_undo_push_layer_mod (gimage, _("Scale Layer"), layer);

  GIMP_ITEM_CLASS (parent_class)->scale (item, new_width, new_height,
                                         new_offset_x, new_offset_y,
                                         interpolation_type);

  /*  If there is a layer mask, make sure it gets scaled also  */
  if (layer->mask)
    {
      gimp_item_scale (GIMP_ITEM (layer->mask),
                       new_width, new_height,
                       new_offset_x, new_offset_y,
                       interpolation_type);

      gimp_image_undo_group_end (gimage);
    }

  /*  Make sure we're not caching any old selection info  */
  gimp_layer_invalidate_boundary (layer);
}

static void
gimp_layer_resize (GimpItem *item,
		   gint      new_width,
		   gint      new_height,
		   gint      offset_x,
		   gint      offset_y)
{
  GimpLayer *layer;
  GimpImage *gimage;

  layer  = GIMP_LAYER (item);
  gimage = gimp_item_get_image (item);

  if (layer->mask)
    gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_LAYER_RESIZE,
                                 _("Resize Layer"));

  gimp_image_undo_push_layer_mod (gimage, _("Resize Layer"), layer);

  GIMP_ITEM_CLASS (parent_class)->resize (item, new_width, new_height,
                                          offset_x, offset_y);

  if (layer->mask)
    {
      gimp_item_resize (GIMP_ITEM (layer->mask),
                        new_width, new_height, offset_x, offset_y);

      gimp_image_undo_group_end (gimage);
    }

  /*  Make sure we're not caching any old selection info  */
  gimp_layer_invalidate_boundary (layer);
}

static void
gimp_layer_transform_color (GimpImage         *gimage,
			    PixelRegion       *layerPR,
			    PixelRegion       *bufPR,
			    GimpDrawable      *drawable,
			    GimpImageBaseType  type)
{
  gint      i;
  gint      h;
  guchar   *src;
  guchar   *dest;
  gpointer  pr;

  for (pr = pixel_regions_register (2, layerPR, bufPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      h    = layerPR->h;
      src  = bufPR->data;
      dest = layerPR->data;

      while (h--)
	{
	  for (i = 0; i < layerPR->w; i++)
	    {
	      gimp_image_transform_color (gimage, drawable,
					  src  + (i * bufPR->bytes),
					  dest + (i * layerPR->bytes), type);
	      /*  copy alpha channel  */
	      dest[(i + 1) * layerPR->bytes - 1] = src[(i + 1) * bufPR->bytes - 1];
	    }

	  src  += bufPR->rowstride;
	  dest += layerPR->rowstride;
	}
    }
}

/**************************/
/*  Function definitions  */
/**************************/

GimpLayer *
gimp_layer_new (GimpImage            *gimage,
		gint                  width,
		gint                  height,
		GimpImageType         type,
		const gchar          *name,
		gdouble               opacity,
		GimpLayerModeEffects  mode)
{
  GimpLayer *layer;

  g_return_val_if_fail (GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  layer = g_object_new (GIMP_TYPE_LAYER, NULL);

  gimp_drawable_configure (GIMP_DRAWABLE (layer),
			   gimage,
                           0, 0, width, height,
                           type,
                           name);

  opacity = CLAMP (opacity, GIMP_OPACITY_TRANSPARENT, GIMP_OPACITY_OPAQUE);

  layer->opacity = opacity;
  layer->mode    = mode;

  return layer;
}

/**
 * gimp_layer_new_from_tiles:
 * @tiles:       The buffer to make the new layer from.
 * @dest_gimage: The image the new layer will be added to.
 * @type:        The #GimpImageType of the new layer.
 * @name:        The new layer's name.
 * @opacity:     The new layer's opacity.
 * @mode:        The new layer's mode.
 * 
 * Copies %tiles to a layer taking into consideration the
 * possibility of transforming the contents to meet the requirements
 * of the target image type
 * 
 * Return value: The new layer.
 **/
GimpLayer *
gimp_layer_new_from_tiles (TileManager          *tiles,
                           GimpImage            *dest_gimage,
                           GimpImageType         type,
			   const gchar          *name,
			   gdouble               opacity,
			   GimpLayerModeEffects  mode)
{
  GimpLayer   *new_layer;
  PixelRegion  layerPR;
  PixelRegion  bufPR;
  gint         width, height;

  g_return_val_if_fail (tiles != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_gimage), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  width  = tile_manager_width (tiles);
  height = tile_manager_height (tiles);

  new_layer = gimp_layer_new (dest_gimage,
                              width, height,
			      type,
			      name,
			      opacity,
			      mode);

  if (! new_layer)
    {
      g_message ("gimp_layer_new_from_tiles: could not allocate new layer");
      return NULL;
    }

  /*  Configure the pixel regions  */
  pixel_region_init (&bufPR, tiles,
		     0, 0,
		     width, height,
		     FALSE);
  pixel_region_init (&layerPR, GIMP_DRAWABLE (new_layer)->tiles,
		     0, 0,
		     width, height,
		     TRUE);

  if ((tile_manager_bpp (tiles) == 4 &&
       GIMP_DRAWABLE (new_layer)->type == GIMP_RGBA_IMAGE) ||
      (tile_manager_bpp (tiles) == 2 &&
       GIMP_DRAWABLE (new_layer)->type == GIMP_GRAYA_IMAGE))
    {
      /*  If we want a layer the same type as the buffer  */
      copy_region (&bufPR, &layerPR);
    }
  else
    {
      /*  Transform the contents of the buf to the new_layer  */
      gimp_layer_transform_color (dest_gimage,
                                  &layerPR, &bufPR,
                                  GIMP_DRAWABLE (new_layer),
                                  ((tile_manager_bpp (tiles) == 4) ? 
                                   GIMP_RGB : GIMP_GRAY));
    }

  return new_layer;
}

GimpLayer *
gimp_layer_new_from_drawable (GimpDrawable *drawable,
                              GimpImage    *dest_image)
{
  GimpImageBaseType  src_base_type;
  GimpDrawable      *new_drawable;
  GimpItem          *new_item;
  GimpImageBaseType  new_base_type;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);

  src_base_type = GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (drawable));
  new_base_type = gimp_image_base_type (dest_image);

  new_item = gimp_item_duplicate (GIMP_ITEM (drawable),
                                  GIMP_TYPE_LAYER,
                                  TRUE);

  new_drawable = GIMP_DRAWABLE (new_item);

  if (src_base_type != new_base_type)
    {
      TileManager   *new_tiles;
      GimpImageType  new_type;

      new_type = GIMP_IMAGE_TYPE_FROM_BASE_TYPE (new_base_type);

      if (gimp_drawable_has_alpha (new_drawable))
        new_type = GIMP_IMAGE_TYPE_WITH_ALPHA (new_type);

      new_tiles = tile_manager_new (gimp_item_width (new_item),
                                    gimp_item_height (new_item),
                                    GIMP_IMAGE_TYPE_BYTES (new_type));

      switch (new_base_type)
        {
        case GIMP_RGB:
          gimp_drawable_convert_rgb (new_drawable,
                                     new_tiles,
                                     src_base_type);
          break;

        case GIMP_GRAY:
          gimp_drawable_convert_grayscale (new_drawable,
                                           new_tiles,
                                           src_base_type);
          break;

        case GIMP_INDEXED:
          {
            PixelRegion layerPR;
            PixelRegion newPR;

            pixel_region_init (&layerPR, new_drawable->tiles,
                               0, 0,
                               gimp_item_width (new_item),
                               gimp_item_height (new_item),
                               FALSE);
            pixel_region_init (&newPR, new_tiles,
                               0, 0,
                               gimp_item_width (new_item),
                               gimp_item_height (new_item),
                               TRUE);

            gimp_layer_transform_color (dest_image,
                                        &newPR, &layerPR,
                                        NULL,
                                        src_base_type);
          }
          break;
        }

      tile_manager_destroy (new_drawable->tiles);

      new_drawable->tiles     = new_tiles;
      new_drawable->type      = new_type;
      new_drawable->bytes     = GIMP_IMAGE_TYPE_BYTES (new_type);
      new_drawable->has_alpha = GIMP_IMAGE_TYPE_HAS_ALPHA (new_type);
    }

  gimp_item_set_image (new_item, dest_image);

  return GIMP_LAYER (new_drawable);
}

GimpLayerMask *
gimp_layer_add_mask (GimpLayer     *layer,
		     GimpLayerMask *mask,
                     gboolean       push_undo)
{
  GimpImage *gimage;

  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (GIMP_IS_LAYER_MASK (mask), NULL);

  gimage = gimp_item_get_image (GIMP_ITEM (layer));

  if (! gimage)
    {
      g_message (_("Cannot add layer mask to layer "
                   "which is not part of an image."));
      return NULL;
    }

  if (layer->mask)
    {
      g_message (_("Unable to add a layer mask since "
		   "the layer already has one."));
      return NULL;
    }

  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    {
      g_message (_("Cannot add layer mask to a layer "
                   "with no alpha channel."));
      return NULL;
    }

  if ((gimp_item_width (GIMP_ITEM (layer)) !=
       gimp_item_width (GIMP_ITEM (mask))) ||
      (gimp_item_height (GIMP_ITEM (layer)) !=
       gimp_item_height (GIMP_ITEM (mask))))
    {
      g_message(_("Cannot add layer mask of different "
                  "dimensions than specified layer."));
      return NULL;
    }

  layer->mask = mask;
  g_object_ref (layer->mask);

  gimp_layer_mask_set_layer (mask, layer);

  gimp_drawable_update (GIMP_DRAWABLE (layer),
			0, 0,
			GIMP_ITEM (layer)->width, 
			GIMP_ITEM (layer)->height);

  if (push_undo)
    gimp_image_undo_push_layer_mask_add (gimage, _("Add Layer Mask"),
                                         layer, mask);

  g_signal_emit (layer, layer_signals[MASK_CHANGED], 0);

  return layer->mask;
}

GimpLayerMask *
gimp_layer_create_mask (const GimpLayer *layer,
			GimpAddMaskType  add_mask_type)
{
  GimpDrawable  *drawable;
  GimpItem      *item;
  PixelRegion    srcPR;
  PixelRegion    destPR;
  GimpLayerMask *mask;
  GimpImage     *gimage;
  gchar         *mask_name;
  GimpRGB        black = { 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE };

  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);

  drawable = GIMP_DRAWABLE (layer);
  item     = GIMP_ITEM (layer);
  gimage   = gimp_item_get_image (item);

  mask_name = g_strdup_printf (_("%s mask"),
			       gimp_object_get_name (GIMP_OBJECT (layer)));

  mask = gimp_layer_mask_new (gimage,
			      item->width,
			      item->height,
			      mask_name, &black);

  g_free (mask_name);

  GIMP_ITEM (mask)->offset_x = item->offset_x;
  GIMP_ITEM (mask)->offset_y = item->offset_y;

  switch (add_mask_type)
    {
    case GIMP_ADD_WHITE_MASK:
      gimp_channel_all (GIMP_CHANNEL (mask), FALSE);
      return mask;

    case GIMP_ADD_BLACK_MASK:
      gimp_channel_clear (GIMP_CHANNEL (mask), FALSE);
      return mask;

    default:
      break;
    }

  pixel_region_init (&destPR, GIMP_DRAWABLE (mask)->tiles, 
		     0, 0, 
		     GIMP_ITEM (mask)->width,
                     GIMP_ITEM (mask)->height, 
		     TRUE);

  switch (add_mask_type)
    {
    case GIMP_ADD_WHITE_MASK:
    case GIMP_ADD_BLACK_MASK:
      break;

    case GIMP_ADD_ALPHA_MASK:
      if (gimp_drawable_has_alpha (drawable))
	{
	  pixel_region_init (&srcPR, drawable->tiles, 
			     0, 0, 
			     item->width, 
			     item->height, 
			     FALSE);

	  extract_alpha_region (&srcPR, NULL, &destPR);
	}
      break;

    case GIMP_ADD_SELECTION_MASK:
      {
        GimpChannel *selection;

        selection = gimp_image_get_mask (gimage);

        if (item->offset_x < 0 ||
            item->offset_y < 0 ||
            item->offset_x + item->width  > gimage->width ||
            item->offset_y + item->height > gimage->height)
          {
            gint width, height;

            width  = item->width;
            height = item->height;

            if (item->offset_x < 0)
              width += item->offset_x;

            if (item->offset_y < 0)
              height += item->offset_y;

            if (item->offset_x + item->width > gimage->width)
              width -= item->offset_x + item->width - gimage->width;

            if (item->offset_y + item->height > gimage->height)
              height -= item->offset_y + item->height - gimage->height;

            if (width < item->width || height < item->height)
              gimp_channel_clear (GIMP_CHANNEL (mask), FALSE);

            if (width > 0 && height > 0)
              {
                gint x, y;

                x = MAX (0, item->offset_x);
                y = MAX (0, item->offset_y);

                pixel_region_init (&srcPR, GIMP_DRAWABLE (selection)->tiles,
                                   x, y, width, height,
                                   FALSE);

                x = MAX (0, -item->offset_x);
                y = MAX (0, -item->offset_y);

                pixel_region_init (&destPR, GIMP_DRAWABLE (mask)->tiles,
                                   x, y, width, height,
                                   TRUE);

                copy_region (&srcPR, &destPR);
              }
          }
        else
          {
            pixel_region_init (&srcPR, GIMP_DRAWABLE (selection)->tiles, 
                               item->offset_x,
                               item->offset_y, 
                               item->width, 
                               item->height, 
                               FALSE);

            copy_region (&srcPR, &destPR);
          }

        if (! (selection->bounds_known && selection->empty))
          GIMP_CHANNEL (mask)->bounds_known = FALSE;
      }
      break;

    case GIMP_ADD_COPY_MASK:
      {
        TileManager   *copy_tiles = NULL;
        GimpImageType  layer_type;

        layer_type = drawable->type;

        if (GIMP_IMAGE_TYPE_BASE_TYPE (layer_type) != GIMP_GRAY)
          {
            GimpImageType copy_type;

            copy_type = (GIMP_IMAGE_TYPE_HAS_ALPHA (layer_type) ?
                         GIMP_GRAYA_IMAGE : GIMP_GRAY_IMAGE);

            copy_tiles = tile_manager_new (item->width,
                                           item->height,
                                           GIMP_IMAGE_TYPE_BYTES (copy_type));

            gimp_drawable_convert_grayscale (drawable,
                                             copy_tiles,
                                             GIMP_IMAGE_TYPE_BASE_TYPE (layer_type));

            pixel_region_init (&srcPR, copy_tiles,
                               0, 0,
                               item->width, 
                               item->height, 
                               FALSE);
          }
        else
          {
            pixel_region_init (&srcPR, drawable->tiles,
                               0, 0,
                               item->width, 
                               item->height, 
                               FALSE);
          }

        if (gimp_drawable_has_alpha (drawable))
          {
            guchar black_uchar[] = { 0, 0, 0, 0 };

            flatten_region (&srcPR, &destPR, black_uchar);
          }
        else
          {
            copy_region (&srcPR, &destPR);
          }

        if (copy_tiles)
          tile_manager_destroy (copy_tiles);
      }

      GIMP_CHANNEL (mask)->bounds_known = FALSE;
      break;
    }

  return mask;
}

void
gimp_layer_apply_mask (GimpLayer         *layer,
		       GimpMaskApplyMode  mode,
                       gboolean           push_undo)
{
  GimpItem    *item;
  GimpImage   *gimage;
  PixelRegion  srcPR, maskPR;
  gboolean     view_changed = FALSE;

  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (! layer->mask)
    return;

  /*  this operation can only be done to layers with an alpha channel  */
  if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    return;

  item = GIMP_ITEM (layer);

  gimage = gimp_item_get_image (item);

  if (! gimage)
    return;

  if (push_undo)
    {
      gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_LAYER_APPLY_MASK,
                                   _("Apply Layer Mask"));

      gimp_image_undo_push_layer_mask_remove (gimage, NULL, layer, layer->mask);
    }

  /*  check if applying the mask changes the projection  */
  if ((mode == GIMP_MASK_APPLY && (! layer->mask->apply_mask ||
                                   layer->mask->show_mask)) ||
      (mode == GIMP_MASK_DISCARD && (layer->mask->apply_mask ||
                                     layer->mask->show_mask)))
    {
      view_changed = TRUE;
    }

  if (mode == GIMP_MASK_APPLY)
    {
      if (push_undo)
        gimp_drawable_push_undo (GIMP_DRAWABLE (layer), NULL,
                                 0, 0,
                                 item->width,
                                 item->height,
                                 NULL, FALSE);

      /*  Combine the current layer's alpha channel and the mask  */
      pixel_region_init (&srcPR, GIMP_DRAWABLE (layer)->tiles, 
			 0, 0, 
			 item->width, 
			 item->height, 
			 TRUE);
      pixel_region_init (&maskPR, GIMP_DRAWABLE (layer->mask)->tiles, 
			 0, 0, 
			 item->width, 
			 item->height, 
			 FALSE);

      apply_mask_to_region (&srcPR, &maskPR, OPAQUE_OPACITY);
    }

  g_object_unref (layer->mask);
  layer->mask = NULL;

  if (push_undo)
    gimp_image_undo_group_end (gimage);

  /*  If applying actually changed the view  */
  if (view_changed)
    {
      gimp_drawable_update (GIMP_DRAWABLE (layer),
			    0, 0,
			    item->width,
			    item->height);
    }
  else
    {
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer));
    }

  g_signal_emit (layer, layer_signals[MASK_CHANGED], 0);
}

void
gimp_layer_add_alpha (GimpLayer *layer)
{
  PixelRegion    srcPR, destPR;
  TileManager   *new_tiles;
  GimpImageType  type;
  GimpImage     *gimage;

  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
    return;

  type = gimp_drawable_type_with_alpha (GIMP_DRAWABLE (layer));

  /*  Configure the pixel regions  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE (layer)->tiles, 
		     0, 0, 
		     GIMP_ITEM (layer)->width, 
		     GIMP_ITEM (layer)->height, 
		     FALSE);

  /*  Allocate the new layer, configure dest region  */
  new_tiles = tile_manager_new (GIMP_ITEM (layer)->width, 
				GIMP_ITEM (layer)->height, 
				gimp_drawable_bytes_with_alpha (GIMP_DRAWABLE (layer)));
  pixel_region_init (&destPR, new_tiles, 
		     0, 0,
		     GIMP_ITEM (layer)->width, 
		     GIMP_ITEM (layer)->height, 
		     TRUE);

  /*  Add an alpha channel  */
  add_alpha_region (&srcPR, &destPR);

  /*  Push the layer on the undo stack  */
  gimp_image_undo_push_layer_mod (gimp_item_get_image (GIMP_ITEM (layer)),
                                  _("Add Alpha Channel"), layer);

  /*  Configure the new layer  */
  GIMP_DRAWABLE (layer)->tiles         = new_tiles;
  GIMP_DRAWABLE (layer)->type          = type;
  GIMP_DRAWABLE (layer)->bytes         = GIMP_DRAWABLE (layer)->bytes + 1;
  GIMP_DRAWABLE (layer)->has_alpha     = GIMP_IMAGE_TYPE_HAS_ALPHA (type);
  GIMP_DRAWABLE (layer)->preview_valid = FALSE;

  gimage = gimp_item_get_image (GIMP_ITEM (layer));

  if (gimp_container_num_children (gimage->layers) == 1)
    gimp_image_alpha_changed (gimage);
}

void
gimp_layer_resize_to_image (GimpLayer *layer)
{
  GimpImage *gimage;
  gint       offset_x;
  gint       offset_y;

  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (! (gimage = gimp_item_get_image (GIMP_ITEM (layer))))
    return;

  gimp_image_undo_group_start (gimage, GIMP_UNDO_GROUP_LAYER_RESIZE,
                               _("Layer to Image Size"));

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_relax (layer, TRUE);

   gimp_item_offsets (GIMP_ITEM (layer), &offset_x, &offset_y);
   gimp_item_resize (GIMP_ITEM (layer), gimage->width, gimage->height,
                     offset_x, offset_y);

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_rigor (layer, TRUE);

  gimp_image_undo_group_end (gimage);
}

BoundSeg *
gimp_layer_boundary (GimpLayer *layer,
		     gint      *num_segs)
{
  GimpItem *item;
  BoundSeg *new_segs;

  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);

  item = GIMP_ITEM (layer);

  /*  Create the four boundary segments that encompass this
   *  layer's boundary.
   */
  new_segs  = g_new (BoundSeg, 4);
  *num_segs = 4;

  /*  if the layer is a floating selection  */
  if (gimp_layer_is_floating_sel (layer))
    {
      if (GIMP_IS_CHANNEL (layer->fs.drawable))
	{
	  /*  if the owner drawable is a channel, just return nothing  */

	  g_free (new_segs);
	  *num_segs = 0;
	  return NULL;
	}
      else
	{
	  /*  otherwise, set the layer to the owner drawable  */

	  layer = GIMP_LAYER (layer->fs.drawable);
	}
    }

  new_segs[0].x1   = item->offset_x;
  new_segs[0].y1   = item->offset_y;
  new_segs[0].x2   = item->offset_x;
  new_segs[0].y2   = item->offset_y + item->height;
  new_segs[0].open = 1;

  new_segs[1].x1  = item->offset_x;
  new_segs[1].y1   = item->offset_y;
  new_segs[1].x2   = item->offset_x + item->width;
  new_segs[1].y2   = item->offset_y;
  new_segs[1].open = 1;

  new_segs[2].x1   = item->offset_x + item->width;
  new_segs[2].y1   = item->offset_y;
  new_segs[2].x2   = item->offset_x + item->width;
  new_segs[2].y2   = item->offset_y + item->height;
  new_segs[2].open = 0;

  new_segs[3].x1   = item->offset_x;
  new_segs[3].y1   = item->offset_y + item->height;
  new_segs[3].x2   = item->offset_x + item->width;
  new_segs[3].y2   = item->offset_y + item->height;
  new_segs[3].open = 0;

  return new_segs;
}

void
gimp_layer_invalidate_boundary (GimpLayer *layer)
{
  GimpImage   *gimage;
  GimpChannel *mask;

  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (! (gimage = gimp_item_get_image (GIMP_ITEM (layer))))
    return;

  /*  Turn the current selection off  */
  gimp_image_selection_control (gimage, GIMP_SELECTION_OFF);

  /*  clear the affected region surrounding the layer  */
  gimp_image_selection_control (gimage, GIMP_SELECTION_LAYER_OFF);

  /*  get the selection mask channel  */
  mask = gimp_image_get_mask (gimage);

  /*  Only bother with the bounds if there is a selection  */
  if (! gimp_channel_is_empty (mask))
    {
      mask->bounds_known   = FALSE;
      mask->boundary_known = FALSE;
    }

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_invalidate (layer);
}

gboolean
gimp_layer_pick_correlate (GimpLayer *layer,
			   gint       x,
			   gint       y)
{
  Tile *tile;
  Tile *mask_tile;
  gint  val;

  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  /*  Is the point inside the layer?
   *  First transform the point to layer coordinates...
   */
  x -= GIMP_ITEM (layer)->offset_x;
  y -= GIMP_ITEM (layer)->offset_y;

  if (x >= 0 && x < GIMP_ITEM (layer)->width &&
      y >= 0 && y < GIMP_ITEM (layer)->height &&
      gimp_drawable_get_visible (GIMP_DRAWABLE (layer)))
    {
      /*  If the point is inside, and the layer has no
       *  alpha channel, success!
       */
      if (! gimp_drawable_has_alpha (GIMP_DRAWABLE (layer)))
	return TRUE;

      /*  Otherwise, determine if the alpha value at
       *  the given point is non-zero
       */
      tile = tile_manager_get_tile (GIMP_DRAWABLE(layer)->tiles, 
				    x, y, TRUE, FALSE);

      val = * ((guchar *) tile_data_pointer (tile,
					     x % TILE_WIDTH,
					     y % TILE_HEIGHT) +
	       tile_bpp (tile) - 1);

      if (layer->mask)
	{
	  guchar *ptr;

	  mask_tile = tile_manager_get_tile (GIMP_DRAWABLE(layer->mask)->tiles,
					     x, y, TRUE, FALSE);
	  ptr = tile_data_pointer (mask_tile, x % TILE_WIDTH, y % TILE_HEIGHT);
	  val = val * (*ptr) / 255;
	  tile_release (mask_tile, FALSE);
	}

      tile_release (tile, FALSE);

      if (val > 63)
	return TRUE;
    }

  return FALSE;
}

/**********************/
/*  access functions  */
/**********************/

GimpLayerMask *
gimp_layer_get_mask (const GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);

  return layer->mask;
}

gboolean
gimp_layer_is_floating_sel (const GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  return (layer->fs.drawable != NULL);
}

void
gimp_layer_set_opacity (GimpLayer *layer,
                        gdouble    opacity,
                        gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));

  opacity = CLAMP (opacity, GIMP_OPACITY_TRANSPARENT, GIMP_OPACITY_OPAQUE);

  if (layer->opacity != opacity)
    {
      if (push_undo)
        {
          GimpImage *gimage = gimp_item_get_image (GIMP_ITEM (layer));

          if (gimage)
            gimp_image_undo_push_layer_opacity (gimage, NULL, layer);
        }
                                                   
      layer->opacity = opacity;

      g_signal_emit (layer, layer_signals[OPACITY_CHANGED], 0);

      gimp_drawable_update (GIMP_DRAWABLE (layer),
			    0, 0,
			    GIMP_ITEM (layer)->width,
			    GIMP_ITEM (layer)->height);
    }
}

gdouble
gimp_layer_get_opacity (const GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), GIMP_OPACITY_OPAQUE);

  return layer->opacity;
}

void
gimp_layer_set_mode (GimpLayer            *layer,
                     GimpLayerModeEffects  mode,
                     gboolean              push_undo)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (layer->mode != mode)
    {
      if (push_undo)
        {
          GimpImage *gimage = gimp_item_get_image (GIMP_ITEM (layer));

          if (gimage)
            gimp_image_undo_push_layer_mode (gimage, NULL, layer);
        }
                                                   
      layer->mode = mode;

      g_signal_emit (layer, layer_signals[MODE_CHANGED], 0);

      gimp_drawable_update (GIMP_DRAWABLE (layer),
			    0, 0,
			    GIMP_ITEM (layer)->width,
			    GIMP_ITEM (layer)->height);
    }
}

GimpLayerModeEffects
gimp_layer_get_mode (const GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), GIMP_NORMAL_MODE);

  return layer->mode;
}

void
gimp_layer_set_preserve_trans (GimpLayer *layer,
                               gboolean   preserve,
                               gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (layer->preserve_trans != preserve)
    {
      if (push_undo)
        {
          GimpImage *gimage = gimp_item_get_image (GIMP_ITEM (layer));

          if (gimage)
            gimp_image_undo_push_layer_preserve_trans (gimage, NULL, layer);
        }
                                                   
      layer->preserve_trans = preserve ? TRUE : FALSE;

      g_signal_emit (layer, layer_signals[PRESERVE_TRANS_CHANGED], 0);
    }
}

gboolean
gimp_layer_get_preserve_trans (const GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  return layer->preserve_trans;
}

void
gimp_layer_set_linked (GimpLayer *layer,
                       gboolean   linked,
                       gboolean   push_undo)
{
  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (layer->linked != linked)
    {
      if (push_undo)
        {
          GimpImage *gimage = gimp_item_get_image (GIMP_ITEM (layer));

          if (gimage)
            gimp_image_undo_push_layer_linked (gimage, NULL, layer);
        }

      layer->linked = linked ? TRUE : FALSE;

      g_signal_emit (layer, layer_signals[LINKED_CHANGED], 0);
    }
}

gboolean
gimp_layer_get_linked (const GimpLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  return layer->linked;
}
