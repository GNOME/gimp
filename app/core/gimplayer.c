/* TODO: make sure has_alpha gets set */

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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/boundary.h"
#include "base/pixel-region.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gimpdrawable-invert.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplayermask.h"
#include "parasitelist.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


enum
{
  OPACITY_CHANGED,
  MODE_CHANGED,
  PRESERVE_TRANS_CHANGED,
  LINKED_CHANGED,
  MASK_CHANGED,
  LAST_SIGNAL
};


static void      gimp_layer_class_init           (GimpLayerClass     *klass);
static void      gimp_layer_init                 (GimpLayer          *layer);
static void      gimp_layer_destroy              (GtkObject          *object);
static void      gimp_layer_invalidate_preview   (GimpViewable       *viewable);

static void      gimp_layer_transform_color      (GimpImage          *gimage,
						  PixelRegion        *layerPR,
						  PixelRegion        *bufPR,
						  GimpDrawable       *drawable,
						  GimpImageBaseType   type);


static guint  layer_signals[LAST_SIGNAL] = { 0 };

static GimpDrawableClass *parent_class   = NULL;


GtkType
gimp_layer_get_type (void)
{
  static GtkType layer_type = 0;

  if (! layer_type)
    {
      GtkTypeInfo layer_info =
      {
	"GimpLayer",
	sizeof (GimpLayer),
	sizeof (GimpLayerClass),
	(GtkClassInitFunc) gimp_layer_class_init,
	(GtkObjectInitFunc) gimp_layer_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      layer_type = gtk_type_unique (GIMP_TYPE_DRAWABLE, &layer_info);
    }

  return layer_type;
}

static void
gimp_layer_class_init (GimpLayerClass *klass)
{
  GtkObjectClass    *object_class;
  GimpDrawableClass *drawable_class;
  GimpViewableClass *viewable_class;

  object_class   = (GtkObjectClass *) klass;
  drawable_class = (GimpDrawableClass *) klass;
  viewable_class = (GimpViewableClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_DRAWABLE);

  layer_signals[OPACITY_CHANGED] =
    gtk_signal_new ("opacity_changed",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpLayerClass,
				       opacity_changed),
                    gtk_signal_default_marshaller,
                    GTK_TYPE_NONE, 0);

  layer_signals[MODE_CHANGED] =
    gtk_signal_new ("mode_changed",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpLayerClass,
				       mode_changed),
                    gtk_signal_default_marshaller,
                    GTK_TYPE_NONE, 0);

  layer_signals[PRESERVE_TRANS_CHANGED] =
    gtk_signal_new ("preserve_trans_changed",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpLayerClass,
				       preserve_trans_changed),
                    gtk_signal_default_marshaller,
                    GTK_TYPE_NONE, 0);

  layer_signals[LINKED_CHANGED] =
    gtk_signal_new ("linked_changed",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpLayerClass,
				       linked_changed),
                    gtk_signal_default_marshaller,
                    GTK_TYPE_NONE, 0);

  layer_signals[MASK_CHANGED] =
    gtk_signal_new ("mask_changed",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpLayerClass,
				       mask_changed),
                    gtk_signal_default_marshaller,
                    GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, layer_signals, LAST_SIGNAL);

  object_class->destroy = gimp_layer_destroy;

  viewable_class->invalidate_preview = gimp_layer_invalidate_preview;

  klass->mask_changed = NULL;
}

static void
gimp_layer_init (GimpLayer *layer)
{
  layer->linked         = FALSE;
  layer->preserve_trans = FALSE;

  layer->mask           = NULL;

  layer->opacity        = 100;
  layer->mode           = NORMAL_MODE;

  /*  floating selection  */
  layer->fs.backing_store  = NULL;
  layer->fs.drawable       = NULL;
  layer->fs.initial        = TRUE;
  layer->fs.boundary_known = FALSE;
  layer->fs.segs           = NULL;
  layer->fs.num_segs       = 0;
}

static void
gimp_layer_invalidate_preview (GimpViewable *viewable)
{
  GimpLayer *layer;

  if (GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview)
    GIMP_VIEWABLE_CLASS (parent_class)->invalidate_preview (viewable);

  layer = GIMP_LAYER (viewable);

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_invalidate (layer);
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
  guchar   *s;
  guchar   *d;
  gpointer  pr;

  for (pr = pixel_regions_register (2, layerPR, bufPR); 
       pr != NULL; 
       pr = pixel_regions_process (pr))
    {
      h = layerPR->h;
      s = bufPR->data;
      d = layerPR->data;

      while (h--)
	{
	  for (i = 0; i < layerPR->w; i++)
	    {
	      gimp_image_transform_color (gimage, drawable,
					  s + (i * bufPR->bytes),
					  d + (i * layerPR->bytes), type);
	      /*  copy alpha channel  */
	      d[(i + 1) * layerPR->bytes - 1] = s[(i + 1) * bufPR->bytes - 1];
	    }

	  s += bufPR->rowstride;
	  d += layerPR->rowstride;
	}
    }
}

/**************************/
/*  Function definitions  */
/**************************/

GimpLayer *
gimp_layer_new (GimpImage        *gimage,
		gint              width,
		gint              height,
		GimpImageType     type,
		const gchar      *name,
		gint              opacity,
		LayerModeEffects  mode)
{
  GimpLayer *layer;

  if (width < 1 || height < 1)
    {
      g_message (_("Zero width or height layers not allowed."));
      return NULL;
    }

  layer = gtk_type_new (GIMP_TYPE_LAYER);

  gimp_drawable_configure (GIMP_DRAWABLE (layer),
			   gimage, width, height, type, name);

  /*  mode and opacity  */
  layer->mode    = mode;
  layer->opacity = opacity;

  return layer;
}

GimpLayer *
gimp_layer_copy (GimpLayer *layer,
		 gboolean   add_alpha)
{
  gchar         *layer_name;
  GimpLayer     *new_layer;
  GimpImageType  new_type;
  gchar         *ext;
  gint           number;
  const gchar   *name;
  gint           len;
  PixelRegion    srcPR;
  PixelRegion    destPR;

  /*  formulate the new layer name  */
  name = gimp_object_get_name (GIMP_OBJECT (layer));

  ext = strrchr (name, '#');
  len = strlen (_("copy"));

  if ((strlen(name) >= len &&
       strcmp (&name[strlen (name) - len], _("copy")) == 0) ||
      (ext && (number = atoi (ext + 1)) > 0 && 
       ((gint) (log10 (number) + 1)) == strlen (ext + 1)))
    /* don't have redundant "copy"s */
    layer_name = g_strdup (name);
  else
    layer_name = g_strdup_printf (_("%s copy"), name);

  /*  when copying a layer, the copy ALWAYS has an alpha channel  */
  if (add_alpha)
    {
      switch (GIMP_DRAWABLE (layer)->type)
	{
	case RGB_GIMAGE:
	  new_type = RGBA_GIMAGE;
	  break;
	case GRAY_GIMAGE:
	  new_type = GRAYA_GIMAGE;
	  break;
	case INDEXED_GIMAGE:
	  new_type = INDEXEDA_GIMAGE;
	  break;
	default:
	  new_type = GIMP_DRAWABLE (layer)->type;
	  break;
	}
    }
  else
    {
      new_type = GIMP_DRAWABLE (layer)->type;
    }

  /*  allocate a new layer object  */
  new_layer = gimp_layer_new (GIMP_DRAWABLE (layer)->gimage,
			      GIMP_DRAWABLE (layer)->width,
			      GIMP_DRAWABLE (layer)->height,
			      new_type,
			      layer_name,
			      layer->opacity,
			      layer->mode);
  if (! new_layer)
    {
      g_message ("gimp_layer_copy: could not allocate new layer");
      goto cleanup;
    }

  GIMP_DRAWABLE (new_layer)->offset_x = GIMP_DRAWABLE (layer)->offset_x;
  GIMP_DRAWABLE (new_layer)->offset_y = GIMP_DRAWABLE (layer)->offset_y;
  GIMP_DRAWABLE (new_layer)->visible  = GIMP_DRAWABLE (layer)->visible;

  new_layer->linked         = layer->linked;
  new_layer->preserve_trans = layer->preserve_trans;

  /*  copy the contents across layers  */
  if (new_type == GIMP_DRAWABLE (layer)->type)
    {
      pixel_region_init (&srcPR, GIMP_DRAWABLE (layer)->tiles, 
			 0, 0, 
			 GIMP_DRAWABLE (layer)->width, 
			 GIMP_DRAWABLE (layer)->height, 
			 FALSE);
      pixel_region_init (&destPR, GIMP_DRAWABLE(new_layer)->tiles, 
			 0, 0, 
			 GIMP_DRAWABLE (layer)->width, 
			 GIMP_DRAWABLE (layer)->height, 
			 TRUE);
      copy_region (&srcPR, &destPR);
    }
  else
    {
      pixel_region_init (&srcPR, GIMP_DRAWABLE (layer)->tiles, 
			 0, 0, 
			 GIMP_DRAWABLE (layer)->width, 
			 GIMP_DRAWABLE (layer)->height, 
			 FALSE);
      pixel_region_init (&destPR, GIMP_DRAWABLE (new_layer)->tiles, 
			 0, 0, 
			 GIMP_DRAWABLE (layer)->width, 
			 GIMP_DRAWABLE(layer)->height, 
			 TRUE);
      add_alpha_region (&srcPR, &destPR);
    }

  /*  duplicate the layer mask if necessary  */
  if (layer->mask)
    {
      new_layer->mask = gimp_layer_mask_copy (layer->mask);

      gtk_object_ref (GTK_OBJECT (new_layer->mask));
      gtk_object_sink (GTK_OBJECT (new_layer->mask));

      gimp_layer_mask_set_layer (new_layer->mask, new_layer);
    }

  /* copy the parasites */
  gtk_object_unref (GTK_OBJECT (GIMP_DRAWABLE (new_layer)->parasites));
  GIMP_DRAWABLE (new_layer)->parasites =
    gimp_parasite_list_copy (GIMP_DRAWABLE (layer)->parasites);

 cleanup:
  /*  free up the layer_name memory  */
  g_free (layer_name);

  return new_layer;
}

GimpLayer *
gimp_layer_new_from_tiles (GimpImage        *gimage,
			   GimpImageType     layer_type,
			   TileManager      *tiles,
			   gchar            *name,
			   gint              opacity,
			   LayerModeEffects  mode)
{
  GimpLayer   *new_layer;
  PixelRegion  layerPR;
  PixelRegion  bufPR;

  /*  Function copies buffer to a layer
   *  taking into consideration the possibility of transforming
   *  the contents to meet the requirements of the target image type
   */

  /*  If no image or no tile manager, return NULL  */
  if (!gimage || !tiles )
    return NULL;
  
  /*  the layer_type needs to have alpha */
  g_return_val_if_fail (GIMP_IMAGE_TYPE_HAS_ALPHA (layer_type), NULL);

   /*  Create the new layer  */
  new_layer = gimp_layer_new (0,
			      tile_manager_width (tiles),
			      tile_manager_height (tiles),
			      layer_type,
			      name,
			      opacity,
			      mode);

  if (!new_layer)
    {
      g_message ("gimp_layer_new_from_tiles: could not allocate new layer");
      return NULL;
    }

  /*  Configure the pixel regions  */
  pixel_region_init (&layerPR, GIMP_DRAWABLE (new_layer)->tiles, 
		     0, 0, 
		     GIMP_DRAWABLE (new_layer)->width, 
		     GIMP_DRAWABLE (new_layer)->height, 
		     TRUE);
  pixel_region_init (&bufPR, tiles, 
		     0, 0, 
		     GIMP_DRAWABLE (new_layer)->width, 
		     GIMP_DRAWABLE (new_layer)->height,
		     FALSE);

  if ((tile_manager_bpp (tiles) == 4 &&
       GIMP_DRAWABLE (new_layer)->type == RGBA_GIMAGE) ||
      (tile_manager_bpp (tiles) == 2 &&
       GIMP_DRAWABLE (new_layer)->type == GRAYA_GIMAGE))
    /*  If we want a layer the same type as the buffer  */
    copy_region (&bufPR, &layerPR);
  else
    /*  Transform the contents of the buf to the new_layer  */
    gimp_layer_transform_color (gimage, &layerPR, &bufPR,
				GIMP_DRAWABLE (new_layer),
				(tile_manager_bpp (tiles) == 4) ? RGB : GRAY);
  
  return new_layer;
}

GimpLayerMask *
gimp_layer_add_mask (GimpLayer     *layer,
		     GimpLayerMask *mask,
                     gboolean       push_undo)
{
  GimpImage     *gimage;
  LayerMaskUndo *lmu;

  g_return_val_if_fail (layer != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NULL);

  g_return_val_if_fail (mask != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_LAYER_MASK (mask), NULL);

  gimage = gimp_drawable_gimage (GIMP_DRAWABLE (layer));

  if (! gimage)
    {
      g_message (_("Cannot add layer mask to layer\n"
                   "which is not part of an image."));
      return NULL;
    }

  if (layer->mask)
    {
      g_message(_("Unable to add a layer mask since\n"
                  "the layer already has one."));
      return NULL;
    }

  if (gimp_drawable_is_indexed (GIMP_DRAWABLE (layer)))
    {
      g_message(_("Unable to add a layer mask to a\n"
                  "layer in an indexed image."));
      return NULL;
    }

  if (! gimp_layer_has_alpha (layer))
    {
      g_message (_("Cannot add layer mask to a layer\n"
                   "with no alpha channel."));
      return NULL;
    }

  if ((gimp_drawable_width (GIMP_DRAWABLE (layer)) !=
       gimp_drawable_width (GIMP_DRAWABLE (mask))) ||
      (gimp_drawable_height (GIMP_DRAWABLE (layer)) !=
       gimp_drawable_height (GIMP_DRAWABLE (mask))))
    {
      g_message(_("Cannot add layer mask of different\n"
                  "dimensions than specified layer."));
      return NULL;
    }

  layer->mask = mask;

  gtk_object_ref (GTK_OBJECT (layer->mask));
  gtk_object_sink (GTK_OBJECT (layer->mask));

  gimp_layer_mask_set_layer (mask, layer);

  drawable_update (GIMP_DRAWABLE (layer),
		   0, 0,
		   GIMP_DRAWABLE (layer)->width, 
		   GIMP_DRAWABLE (layer)->height);

  if (push_undo)
    {
      /*  Prepare a layer undo and push it  */
      lmu        = g_new0 (LayerMaskUndo, 1);
      lmu->layer = layer;
      lmu->mask  = mask;
      undo_push_layer_mask (gimage, LAYER_MASK_ADD_UNDO, lmu);
    }

  gtk_signal_emit (GTK_OBJECT (layer), layer_signals[MASK_CHANGED]);

  return layer->mask;
}

GimpLayerMask *
gimp_layer_create_mask (GimpLayer   *layer,
			AddMaskType  add_mask_type)
{
  PixelRegion    maskPR;
  PixelRegion    layerPR;
  GimpLayerMask *mask;
  GimpImage     *gimage;
  GimpDrawable  *selection;
  gchar         *mask_name;
  GimpRGB        black = { 0.0, 0.0, 0.0, 1.0 };
  guchar         white_mask = OPAQUE_OPACITY;
  guchar         black_mask = TRANSPARENT_OPACITY;

  gimage = GIMP_DRAWABLE (layer)->gimage;

  selection = GIMP_DRAWABLE(gimage->selection_mask);

  mask_name = g_strdup_printf (_("%s mask"),
			       gimp_object_get_name (GIMP_OBJECT (layer)));

  /*  Create the layer mask  */
  mask = gimp_layer_mask_new (GIMP_DRAWABLE (layer)->gimage,
			      GIMP_DRAWABLE (layer)->width,
			      GIMP_DRAWABLE (layer)->height,
			      mask_name, &black);
  GIMP_DRAWABLE (mask)->offset_x = GIMP_DRAWABLE (layer)->offset_x;
  GIMP_DRAWABLE (mask)->offset_y = GIMP_DRAWABLE (layer)->offset_y;

  pixel_region_init (&maskPR, GIMP_DRAWABLE (mask)->tiles, 
		     0, 0, 
		     GIMP_DRAWABLE (mask)->width, GIMP_DRAWABLE (mask)->height, 
		     TRUE);

  switch (add_mask_type)
    {
    case ADD_WHITE_MASK:
      color_region (&maskPR, &white_mask);
      break;
    case ADD_BLACK_MASK:
      color_region (&maskPR, &black_mask);
      break;
    case ADD_ALPHA_MASK:
      /*  Extract the layer's alpha channel  */
      if (gimp_layer_has_alpha (layer))
	{
	  pixel_region_init (&layerPR, GIMP_DRAWABLE (layer)->tiles, 
			     0, 0, 
			     GIMP_DRAWABLE (layer)->width, 
			     GIMP_DRAWABLE (layer)->height, 
			     FALSE);
	  extract_alpha_region (&layerPR, NULL, &maskPR);
	}
      break;
     case ADD_SELECTION_MASK:
       pixel_region_init (&layerPR, GIMP_DRAWABLE (selection)->tiles, 
			  GIMP_DRAWABLE (layer)->offset_x,
			  GIMP_DRAWABLE (layer)->offset_y, 
			  GIMP_DRAWABLE (layer)->width, 
			  GIMP_DRAWABLE (layer)->height, 
			  FALSE);
       copy_region (&layerPR, &maskPR);
       break;
     case ADD_INV_SELECTION_MASK:
       pixel_region_init (&layerPR, GIMP_DRAWABLE (selection)->tiles, 
			  GIMP_DRAWABLE (layer)->offset_x,
			  GIMP_DRAWABLE (layer)->offset_y, 
			  GIMP_DRAWABLE (layer)->width, 
			  GIMP_DRAWABLE (layer)->height, 
			  FALSE);
       copy_region (&layerPR, &maskPR);

       gimp_drawable_invert (GIMP_DRAWABLE (mask));
       break;
    }

  g_free (mask_name);

  return mask;
}

static void
gimp_layer_destroy (GtkObject *object)
{
  GimpLayer *layer;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_LAYER (object));

  layer = GIMP_LAYER (object);

  /*  if a layer mask exists, free it  */
  if (layer->mask)
    gtk_object_unref (GTK_OBJECT (layer->mask));

  /*  free the layer boundary if it exists  */
  if (layer->fs.segs)
    g_free (layer->fs.segs);

  /*  free the floating selection if it exists  */
  if (gimp_layer_is_floating_sel (layer))
    {
      tile_manager_destroy (layer->fs.backing_store);
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

void
gimp_layer_apply_mask (GimpLayer     *layer,
		       MaskApplyMode  mode,
                       gboolean       push_undo)
{
  GimpImage     *gimage;
  LayerMaskUndo *lmu = NULL;
  gint           off_x;
  gint           off_y;
  PixelRegion    srcPR, maskPR;
  gboolean       view_changed = FALSE;

  g_return_if_fail (layer != NULL);
  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (! layer->mask)
    return;

  /*  this operation can only be done to layers with an alpha channel  */
  if (! gimp_layer_has_alpha (layer))
    return;

  gimage = gimp_drawable_gimage (GIMP_DRAWABLE (layer));

  if (! gimage)
    return;

  if (push_undo)
    {
      /*  Start an undo group  */
      undo_push_group_start (gimage, LAYER_APPLY_MASK_UNDO);

      /*  Prepare a layer mask undo--push it below  */
      lmu = g_new (LayerMaskUndo, 1);
      lmu->layer      = layer;
      lmu->mask       = layer->mask;
      lmu->mode       = mode;
    }

  /*  check if applying the mask changes the projection  */
  if ((mode == APPLY   && (!layer->mask->apply_mask || layer->mask->show_mask)) ||
      (mode == DISCARD && ( layer->mask->apply_mask || layer->mask->show_mask)))
    {
      view_changed = TRUE;
    }

  if (mode == APPLY)
    {
      if (push_undo)
        {
          /*  Put this apply mask operation on the undo stack  */
          drawable_apply_image (GIMP_DRAWABLE (layer),
                                0, 0,
                                GIMP_DRAWABLE (layer)->width,
                                GIMP_DRAWABLE (layer)->height,
                                NULL, FALSE);
        }

      /*  Combine the current layer's alpha channel and the mask  */
      pixel_region_init (&srcPR, GIMP_DRAWABLE (layer)->tiles, 
			 0, 0, 
			 GIMP_DRAWABLE (layer)->width, 
			 GIMP_DRAWABLE (layer)->height, 
			 TRUE);
      pixel_region_init (&maskPR, GIMP_DRAWABLE (layer->mask)->tiles, 
			 0, 0, 
			 GIMP_DRAWABLE (layer)->width, 
			 GIMP_DRAWABLE (layer)->height, 
			 FALSE);

      apply_mask_to_region (&srcPR, &maskPR, OPAQUE_OPACITY);
      GIMP_DRAWABLE (layer)->preview_valid = FALSE;
    }

  layer->mask = NULL;

  if (push_undo)
    {
      /*  Push the undo--Important to do it here, AFTER applying
       *   the mask, in case the undo push fails and the
       *   mask is deleted
       */
      undo_push_layer_mask (gimage, LAYER_MASK_REMOVE_UNDO, lmu);

      /*  end the undo group  */
      undo_push_group_end (gimage);
    }

  /*  If applying actually changed the view  */
  if (view_changed)
    {
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gimage));

      gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

      drawable_update (GIMP_DRAWABLE (layer),
                       0, 0,
		       gimp_drawable_width  (GIMP_DRAWABLE (layer)),
		       gimp_drawable_height (GIMP_DRAWABLE (layer)));
    }

  gtk_signal_emit (GTK_OBJECT (layer), layer_signals[MASK_CHANGED]);
}

void
gimp_layer_translate (GimpLayer *layer,
		      gint       off_x,
		      gint       off_y)
{
  /*  the undo call goes here  */
  undo_push_layer_displace (GIMP_DRAWABLE (layer)->gimage, layer);

  /*  update the affected region  */
  drawable_update (GIMP_DRAWABLE (layer),
		   0, 0, 
		   GIMP_DRAWABLE (layer)->width, 
		   GIMP_DRAWABLE (layer)->height);

  /*  invalidate the selection boundary because of a layer modification  */
  gimp_layer_invalidate_boundary (layer);
 
  /*  update the layer offsets  */
  GIMP_DRAWABLE (layer)->offset_x += off_x;
  GIMP_DRAWABLE (layer)->offset_y += off_y;

  /*  update the affected region  */
  drawable_update (GIMP_DRAWABLE (layer), 
		   0, 0, 
		   GIMP_DRAWABLE (layer)->width, 
		   GIMP_DRAWABLE (layer)->height);

  if (layer->mask) 
    {
      GIMP_DRAWABLE (layer->mask)->offset_x += off_x;
      GIMP_DRAWABLE (layer->mask)->offset_y += off_y;

      /*  invalidate the mask preview  */
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (layer->mask));
    }
}

void
gimp_layer_add_alpha (GimpLayer *layer)
{
  PixelRegion    srcPR, destPR;
  TileManager   *new_tiles;
  GimpImageType  type;

  /*  Don't bother if the layer already has alpha  */
  switch (GIMP_DRAWABLE (layer)->type)
    {
    case RGB_GIMAGE:
      type = RGBA_GIMAGE;
      break;
    case GRAY_GIMAGE:
      type = GRAYA_GIMAGE;
      break;
    case INDEXED_GIMAGE:
      type = INDEXEDA_GIMAGE;
      break;
    case RGBA_GIMAGE:
    case GRAYA_GIMAGE:
    case INDEXEDA_GIMAGE:
    default:
      return;
      break;
    }

  /*  Configure the pixel regions  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE (layer)->tiles, 
		     0, 0, 
		     GIMP_DRAWABLE (layer)->width, 
		     GIMP_DRAWABLE (layer)->height, 
		     FALSE);

  /*  Allocate the new layer, configure dest region  */
  new_tiles = tile_manager_new (GIMP_DRAWABLE (layer)->width, 
				GIMP_DRAWABLE (layer)->height, 
				GIMP_DRAWABLE (layer)->bytes + 1);
  pixel_region_init (&destPR, new_tiles, 
		     0, 0, 
		     GIMP_DRAWABLE (layer)->width, 
		     GIMP_DRAWABLE (layer)->height, 
		     TRUE);

  /*  Add an alpha channel  */
  add_alpha_region (&srcPR, &destPR);

  /*  Push the layer on the undo stack  */
  undo_push_layer_mod (GIMP_DRAWABLE (layer)->gimage, layer);

  /*  Configure the new layer  */
  GIMP_DRAWABLE (layer)->tiles         = new_tiles;
  GIMP_DRAWABLE (layer)->type          = type;
  GIMP_DRAWABLE (layer)->bytes         = GIMP_DRAWABLE(layer)->bytes + 1;
  GIMP_DRAWABLE (layer)->has_alpha     = GIMP_IMAGE_TYPE_HAS_ALPHA (type);
  GIMP_DRAWABLE (layer)->preview_valid = FALSE;

  gimp_image_alpha_changed (gimp_drawable_gimage (GIMP_DRAWABLE (layer)));
}

static void
gimp_layer_scale_lowlevel (GimpLayer *layer,
			   gint       new_width,
			   gint       new_height,
			   gint       new_offset_x,
			   gint       new_offset_y)
{
  PixelRegion  srcPR, destPR;
  TileManager *new_tiles;

  /*  Update the old layer position  */
  drawable_update (GIMP_DRAWABLE (layer),
		   0, 0,
		   GIMP_DRAWABLE (layer)->width, 
		   GIMP_DRAWABLE (layer)->height);

  /*  Configure the pixel regions  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE(layer)->tiles, 
		     0, 0, 
		     GIMP_DRAWABLE (layer)->width, 
		     GIMP_DRAWABLE (layer)->height, 
		     FALSE);

  /*  Allocate the new layer, configure dest region  */
  new_tiles = tile_manager_new (new_width, new_height, 
				GIMP_DRAWABLE (layer)->bytes);
  pixel_region_init (&destPR, new_tiles, 
		     0, 0, 
		     new_width, new_height, 
		     TRUE);

  /*  Scale the layer -
   *   If the layer is of type INDEXED, then we don't use pixel-value
   *   resampling because that doesn't necessarily make sense for INDEXED
   *   images.
   */
  if ((GIMP_DRAWABLE (layer)->type == INDEXED_GIMAGE) || 
      (GIMP_DRAWABLE (layer)->type == INDEXEDA_GIMAGE))
    scale_region_no_resample (&srcPR, &destPR);
  else
    scale_region (&srcPR, &destPR);

  /*  Push the layer on the undo stack  */
  undo_push_layer_mod (GIMP_DRAWABLE (layer)->gimage, layer);

  /*  Configure the new layer  */

  GIMP_DRAWABLE (layer)->offset_x = new_offset_x;
  GIMP_DRAWABLE (layer)->offset_y = new_offset_y;
  GIMP_DRAWABLE (layer)->tiles    = new_tiles;
  GIMP_DRAWABLE (layer)->width    = new_width;
  GIMP_DRAWABLE (layer)->height   = new_height;

  /*  If there is a layer mask, make sure it gets scaled also  */
  if (layer->mask) 
    {
      GIMP_DRAWABLE (layer->mask)->offset_x = GIMP_DRAWABLE (layer)->offset_x;
      GIMP_DRAWABLE (layer->mask)->offset_y = GIMP_DRAWABLE (layer)->offset_y;
      gimp_channel_scale (GIMP_CHANNEL (layer->mask), new_width, new_height);
    }

  /*  Make sure we're not caching any old selection info  */
  gimp_layer_invalidate_boundary (layer);

  /*  Update the new layer position  */

  drawable_update (GIMP_DRAWABLE (layer),
		   0, 0,
		   GIMP_DRAWABLE (layer)->width, 
		   GIMP_DRAWABLE (layer)->height);
}

/**
 * gimp_layer_check_scaling:
 * @layer:      Layer to check
 * @new_width:  proposed width of layer's image, in pixels
 * @new_height: proposed height of layer's image, in pixels
 *
 * Scales layer dimensions, then snaps them to pixel centers
 *
 * Returns: #FALSE if any dimension reduces to zero as a result 
 *          of this; otherwise, returns #TRUE.
 **/
gboolean
gimp_layer_check_scaling (GimpLayer *layer,
			  gint       new_width,
			  gint       new_height)
{
  GimpImage *gimage;
  gdouble    img_scale_w;
  gdouble    img_scale_h;
  gint       new_layer_width;
  gint       new_layer_height;

  gimage           = GIMP_DRAWABLE (layer)->gimage;
  img_scale_w      = (gdouble) new_width  / (gdouble) gimage->width;
  img_scale_h      = (gdouble) new_height / (gdouble) gimage->height;
  new_layer_width  = ROUND (img_scale_w *
			    (gdouble) GIMP_DRAWABLE (layer)->width);
  new_layer_height = ROUND (img_scale_h *
			    (gdouble) GIMP_DRAWABLE (layer)->height);

  return (new_layer_width != 0 && new_layer_height != 0);  
}

/**
 * gimp_layer_scale_by_factors:
 * @layer:    Layer to be transformed by explicit width and height factors.
 * @w_factor: scale factor to apply to width and horizontal offset
 * @h_factor: scale factor to apply to height and vertical offset
 * 
 * Scales layer dimensions and offsets by uniform width and
 * height factors.
 *
 * Use gimp_layer_scale_by_factors() in circumstances when the
 * same width and height scaling factors are to be uniformly
 * applied to a set of layers. In this context, the layer's
 * dimensions and offsets from the sides of the containing
 * image all change by these predetermined factors. By fiat,
 * the fixed point of the transform is the upper left hand
 * corner of the image. Returns gboolean FALSE if a requested
 * scale factor is zero or if a scaling zero's out a layer
 * dimension; returns #TRUE otherwise.
 *
 * Use gimp_layer_scale() in circumstances where new layer width
 * and height dimensions are predetermined instead.
 *
 * Side effects: Undo set created for layer. Old layer imagery 
 *               scaled & painted to new layer tiles. 
 *
 * Returns: #TRUE, if the scaled layer has positive dimensions
 *          #FALSE if the scaled layer has at least one zero dimension
 **/
gboolean
gimp_layer_scale_by_factors (GimpLayer *layer,
			     gdouble    w_factor,
			     gdouble    h_factor)
{
  gint new_width, new_height;
  gint new_offset_x, new_offset_y;

  if (w_factor == 0.0 || h_factor == 0.0)
    {
      g_message ("gimp_layer_scale_by_factors: Error. Requested width or height scale equals zero.");
      return FALSE;
    }

  new_offset_x = ROUND (w_factor * (gdouble) GIMP_DRAWABLE (layer)->offset_x);
  new_offset_y = ROUND (h_factor * (gdouble) GIMP_DRAWABLE (layer)->offset_y);
  new_width    = ROUND (w_factor * (gdouble) GIMP_DRAWABLE (layer)->width);
  new_height   = ROUND (h_factor * (gdouble) GIMP_DRAWABLE (layer)->height);

  if (new_width != 0 && new_height != 0)
    {
      gimp_layer_scale_lowlevel (layer,
				 new_width, new_height,
				 new_offset_x, new_offset_y);
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_layer_scale:
 * @layer:        The layer to be transformed by width & height scale factors
 * @new_width:    The width that layer will acquire
 * @new_height:   The height that the layer will acquire
 * @local_origin: sets fixed point of the scaling transform. See below.
 *
 * Sets layer dimensions to new_width and
 * new_height. Derives vertical and horizontal scaling
 * transforms from new width and height. If local_origin is
 * TRUE, the fixed point of the scaling transform coincides
 * with the layer's center point.  Otherwise, the fixed
 * point is taken to be [-GIMP_DRAWABLE(layer)->offset_x,
 * -GIMP_DRAWABLE(layer)->offset_y].
 *
 * Since this function derives scale factors from new and
 * current layer dimensions, these factors will vary from
 * layer to layer because of aliasing artifacts; factor
 * variations among layers can be quite large where layer
 * dimensions approach pixel dimensions. Use 
 * gimp_layer_scale_by_factors() where constant scales are to
 * be uniformly applied to a number of layers.
 *
 * Side effects: undo set created for layer.
 *               Old layer imagery scaled 
 *               & painted to new layer tiles 
 **/
void
gimp_layer_scale (GimpLayer *layer,
		  gint       new_width,
		  gint       new_height,
		  gboolean   local_origin)
{
  gint new_offset_x, new_offset_y;

  if (new_width == 0 || new_height == 0)
    {
      g_message ("gimp_layer_scale: Error. Requested width or height equals zero.");
      return;
    }

  if (local_origin)
    {
      new_offset_x = GIMP_DRAWABLE (layer)->offset_x + 
	((GIMP_DRAWABLE (layer)->width  - new_width) / 2.0);

      new_offset_y = GIMP_DRAWABLE (layer)->offset_y + 
	((GIMP_DRAWABLE (layer)->height - new_height) / 2.0);
    }
  else
    {
      new_offset_x = (gint) (((gdouble) new_width * 
			      GIMP_DRAWABLE (layer)->offset_x / 
			      (gdouble) GIMP_DRAWABLE (layer)->width));

      new_offset_y = (gint) (((gdouble) new_height * 
			      GIMP_DRAWABLE (layer)->offset_y / 
			      (gdouble) GIMP_DRAWABLE (layer)->height));
    }

  gimp_layer_scale_lowlevel (layer, 
			     new_width, new_height, 
			     new_offset_x, new_offset_y);
}

void
gimp_layer_resize (GimpLayer *layer,
		   gint       new_width,
		   gint       new_height,
		   gint       offx,
		   gint       offy)
{
  PixelRegion  srcPR, destPR;
  TileManager *new_tiles;
  gint         w, h;
  gint         x1, y1, x2, y2;

  if (new_width < 1 || new_height < 1)
    return;

  x1 = CLAMP (offx, 0, new_width);
  y1 = CLAMP (offy, 0, new_height);
  x2 = CLAMP ((offx + GIMP_DRAWABLE(layer)->width),  0, new_width);
  y2 = CLAMP ((offy + GIMP_DRAWABLE(layer)->height), 0, new_height);
  w = x2 - x1;
  h = y2 - y1;

  if (offx > 0)
    {
      x1 = 0;
      x2 = offx;
    }
  else
    {
      x1 = -offx;
      x2 = 0;
    }

  if (offy > 0)
    {
      y1 = 0;
      y2 = offy;
    }
  else
    {
      y1 = -offy;
      y2 = 0;
    }

  /*  Update the old layer position  */
  drawable_update (GIMP_DRAWABLE( layer),
		   0, 0,
		   GIMP_DRAWABLE (layer)->width,
		   GIMP_DRAWABLE (layer)->height);

  /*  Configure the pixel regions  */
  pixel_region_init (&srcPR, GIMP_DRAWABLE (layer)->tiles, 
		     x1, y1, 
		     w, h, 
		     FALSE);

  /*  Allocate the new layer, configure dest region  */
  new_tiles = tile_manager_new (new_width, new_height, 
				GIMP_DRAWABLE (layer)->bytes);
  pixel_region_init (&destPR, new_tiles, 
		     0, 0, 
		     new_width, new_height, 
		     TRUE);

  /*  fill with the fill color  */
  if (gimp_layer_has_alpha (layer))
    {
      /*  Set to transparent and black  */
      guchar bg[4] = {0, 0, 0, 0};

      color_region (&destPR, bg);
    }
  else
    {
      guchar bg[3];

      gimp_image_get_background (GIMP_DRAWABLE (layer)->gimage, 
				 GIMP_DRAWABLE (layer), bg);
      color_region (&destPR, bg);
    }

  pixel_region_init (&destPR, new_tiles, 
		     x2, y2, 
		     w, h, 
		     TRUE);

  /*  copy from the old to the new  */
  if (w && h)
    copy_region (&srcPR, &destPR);

  /*  Push the layer on the undo stack  */
  undo_push_layer_mod (GIMP_DRAWABLE(layer)->gimage, layer);

  /*  Configure the new layer  */
  GIMP_DRAWABLE (layer)->tiles = new_tiles;
  GIMP_DRAWABLE (layer)->offset_x = x1 + GIMP_DRAWABLE (layer)->offset_x - x2;
  GIMP_DRAWABLE (layer)->offset_y = y1 + GIMP_DRAWABLE (layer)->offset_y - y2;
  GIMP_DRAWABLE (layer)->width = new_width;
  GIMP_DRAWABLE (layer)->height = new_height;

  /*  If there is a layer mask, make sure it gets resized also  */
  if (layer->mask)
    {
      GIMP_DRAWABLE (layer->mask)->offset_x = GIMP_DRAWABLE (layer)->offset_x;
      GIMP_DRAWABLE (layer->mask)->offset_y = GIMP_DRAWABLE (layer)->offset_y;
      gimp_channel_resize (GIMP_CHANNEL (layer->mask),
			   new_width, new_height, offx, offy);
    }

  /*  Make sure we're not caching any old selection info  */
  gimp_layer_invalidate_boundary (layer);

  /*  update the new layer area  */
  drawable_update (GIMP_DRAWABLE (layer),
		   0, 0,
		   GIMP_DRAWABLE (layer)->width,
		   GIMP_DRAWABLE (layer)->height);
}

void
gimp_layer_resize_to_image (GimpLayer *layer)
{
  GimpImage *gimage;
  gint       offset_x;
  gint       offset_y;

  if (!(gimage = GIMP_DRAWABLE (layer)->gimage))
    return;

  undo_push_group_start (gimage, LAYER_RESIZE_UNDO);

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_relax (layer, TRUE);

   gimp_drawable_offsets (GIMP_DRAWABLE (layer), &offset_x, &offset_y);
   gimp_layer_resize (layer, gimage->width, gimage->height, offset_x, offset_y);

  if (gimp_layer_is_floating_sel (layer))
    floating_sel_rigor (layer, TRUE);

  undo_push_group_end (gimage);
}

BoundSeg *
gimp_layer_boundary (GimpLayer *layer,
		     gint      *num_segs)
{
  BoundSeg *new_segs;

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

  new_segs[0].x1 = GIMP_DRAWABLE (layer)->offset_x;
  new_segs[0].y1 = GIMP_DRAWABLE (layer)->offset_y;
  new_segs[0].x2 = GIMP_DRAWABLE (layer)->offset_x;
  new_segs[0].y2 = GIMP_DRAWABLE (layer)->offset_y + GIMP_DRAWABLE (layer)->height;
  new_segs[0].open = 1;

  new_segs[1].x1 = GIMP_DRAWABLE (layer)->offset_x;
  new_segs[1].y1 = GIMP_DRAWABLE (layer)->offset_y;
  new_segs[1].x2 = GIMP_DRAWABLE (layer)->offset_x + GIMP_DRAWABLE (layer)->width;
  new_segs[1].y2 = GIMP_DRAWABLE (layer)->offset_y;
  new_segs[1].open = 1;

  new_segs[2].x1 = GIMP_DRAWABLE (layer)->offset_x + GIMP_DRAWABLE (layer)->width;
  new_segs[2].y1 = GIMP_DRAWABLE (layer)->offset_y;
  new_segs[2].x2 = GIMP_DRAWABLE (layer)->offset_x + GIMP_DRAWABLE (layer)->width;
  new_segs[2].y2 = GIMP_DRAWABLE (layer)->offset_y + GIMP_DRAWABLE (layer)->height;
  new_segs[2].open = 0;

  new_segs[3].x1 = GIMP_DRAWABLE (layer)->offset_x;
  new_segs[3].y1 = GIMP_DRAWABLE (layer)->offset_y + GIMP_DRAWABLE (layer)->height;
  new_segs[3].x2 = GIMP_DRAWABLE (layer)->offset_x + GIMP_DRAWABLE (layer)->width;
  new_segs[3].y2 = GIMP_DRAWABLE (layer)->offset_y + GIMP_DRAWABLE (layer)->height;
  new_segs[3].open = 0;

  return new_segs;
}

void
gimp_layer_invalidate_boundary (GimpLayer *layer)
{
  GimpImage   *gimage;
  GimpChannel *mask;

  if (! (gimage = gimp_drawable_gimage (GIMP_DRAWABLE (layer))))
    return;

  /*  Turn the current selection off  */
  gdisplays_selection_visibility (gimage, SELECTION_OFF);

  /*  clear the affected region surrounding the layer  */
  gdisplays_selection_visibility (gimage, SELECTION_LAYER_OFF);

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

gint
gimp_layer_pick_correlate (GimpLayer *layer,
			   gint       x,
			   gint       y)
{
  Tile *tile;
  Tile *mask_tile;
  gint  val;

  /*  Is the point inside the layer?
   *  First transform the point to layer coordinates...
   */
  x -= GIMP_DRAWABLE (layer)->offset_x;
  y -= GIMP_DRAWABLE (layer)->offset_y;

  if (x >= 0 && x < GIMP_DRAWABLE (layer)->width &&
      y >= 0 && y < GIMP_DRAWABLE (layer)->height &&
      gimp_drawable_get_visible (GIMP_DRAWABLE (layer)))
    {
      /*  If the point is inside, and the layer has no
       *  alpha channel, success!
       */
      if (! gimp_layer_has_alpha (layer))
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
gimp_layer_get_mask (GimpLayer *layer)
{
  return layer->mask;
}

gboolean
gimp_layer_has_alpha (GimpLayer *layer)
{
  g_return_val_if_fail (layer != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  return GIMP_IMAGE_TYPE_HAS_ALPHA (GIMP_DRAWABLE (layer)->type);
}

gboolean
gimp_layer_is_floating_sel (GimpLayer *layer)
{
  g_return_val_if_fail (layer != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  return (layer->fs.drawable != NULL);
}

void
gimp_layer_set_opacity (GimpLayer *layer,
                        gdouble    opacity)
{
  gint layer_opacity;

  g_return_if_fail (layer != NULL);
  g_return_if_fail (GIMP_IS_LAYER (layer));

  layer_opacity = (gint) (opacity * 255.999);

  if (layer->opacity != layer_opacity)
    {
      layer->opacity = layer_opacity;

      gtk_signal_emit (GTK_OBJECT (layer), layer_signals[OPACITY_CHANGED]);
    }
}

gdouble
gimp_layer_get_opacity (GimpLayer *layer)
{
  g_return_val_if_fail (layer != NULL, 1.0);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), 1.0);

  return (gdouble) layer->opacity / 255.0;
}

void
gimp_layer_set_mode (GimpLayer        *layer,
                     LayerModeEffects  mode)
{
  g_return_if_fail (layer != NULL);
  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (layer->mode != mode)
    {
      layer->mode = mode;

      gtk_signal_emit (GTK_OBJECT (layer), layer_signals[MODE_CHANGED]);
    }
}

LayerModeEffects
gimp_layer_get_mode (GimpLayer *layer)
{
  g_return_val_if_fail (layer != NULL, NORMAL_MODE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), NORMAL_MODE);

  return layer->mode;
}

void
gimp_layer_set_preserve_trans (GimpLayer *layer,
                               gboolean   preserve)
{
  g_return_if_fail (layer != NULL);
  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (layer->preserve_trans != preserve)
    {
      layer->preserve_trans = preserve ? TRUE : FALSE;

      gtk_signal_emit (GTK_OBJECT (layer),
                       layer_signals[PRESERVE_TRANS_CHANGED]);
    }
}

gboolean
gimp_layer_get_preserve_trans (GimpLayer *layer)
{
  g_return_val_if_fail (layer != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  return layer->preserve_trans;
}

void
gimp_layer_set_linked (GimpLayer *layer,
                       gboolean   linked)
{
  g_return_if_fail (layer != NULL);
  g_return_if_fail (GIMP_IS_LAYER (layer));

  if (layer->linked != linked)
    {
      layer->linked = linked ? TRUE : FALSE;

      gtk_signal_emit (GTK_OBJECT (layer), layer_signals[LINKED_CHANGED]);
    }
}

gboolean
gimp_layer_get_linked (GimpLayer *layer)
{
  g_return_val_if_fail (layer != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_LAYER (layer), FALSE);

  return layer->linked;
}
