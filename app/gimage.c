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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include "appenv.h"
#include "channels_dialog.h"
#include "drawable.h"
#include "errors.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "general.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "indexed_palette.h"
#include "interface.h"
#include "layers_dialog.h"
#include "paint_funcs.h"
#include "palette.h"
#include "plug_in.h"
#include "undo.h"

/*  Local function declarations  */
static void     gimage_free_projection       (GImage *);
static void     gimage_allocate_shadow       (GImage *, int, int, int);
static GImage * gimage_create                (void);
static void     gimage_allocate_projection   (GImage *);
static void     gimage_free_layers           (GImage *);
static void     gimage_free_channels         (GImage *);
static void     gimage_construct_layers      (GImage *, int, int, int, int);
static void     gimage_construct_channels    (GImage *, int, int, int, int);
static void     gimage_initialize_projection (GImage *, int, int, int, int);
static void     gimage_get_active_channels   (GImage *, int, int *);

/*  projection functions  */
static void     project_intensity            (GImage *, Layer *, PixelRegion *,
					      PixelRegion *, PixelRegion *);
static void     project_intensity_alpha      (GImage *, Layer *, PixelRegion *,
					      PixelRegion *, PixelRegion *);
static void     project_indexed              (GImage *, Layer *, PixelRegion *,
					      PixelRegion *);
static void     project_channel              (GImage *, Channel *, PixelRegion *,
					      PixelRegion *);

/*
 *  Global variables
 */
int valid_combinations[][MAX_CHANNELS + 1] =
{
  /* RGB GIMAGE */
  { -1, -1, -1, COMBINE_INTEN_INTEN, COMBINE_INTEN_INTEN_A },
  /* RGBA GIMAGE */
  { -1, -1, -1, COMBINE_INTEN_A_INTEN, COMBINE_INTEN_A_INTEN_A },
  /* GRAY GIMAGE */
  { -1, COMBINE_INTEN_INTEN, COMBINE_INTEN_INTEN_A, -1, -1 },
  /* GRAYA GIMAGE */
  { -1, COMBINE_INTEN_A_INTEN, COMBINE_INTEN_A_INTEN_A, -1, -1 },
  /* INDEXED GIMAGE */
  { -1, COMBINE_INDEXED_INDEXED, COMBINE_INDEXED_INDEXED_A, -1, -1 },
  /* INDEXEDA GIMAGE */
  { -1, -1, COMBINE_INDEXED_A_INDEXED_A, -1, -1 },
};


/*
 *  Static variables
 */
static int global_gimage_ID = 1;
link_ptr image_list = NULL;


/* static functions */

static GImage *
gimage_create (void)
{
  GImage *gimage;

  gimage = (GImage *) g_malloc (sizeof (GImage));

  gimage->has_filename = 0;
  gimage->num_cols = 0;
  gimage->cmap = NULL;
  gimage->ID = global_gimage_ID++;
  gimage->ref_count = 0;
  gimage->instance_count = 0;
  gimage->shadow = NULL;
  gimage->dirty = 1;
  gimage->undo_on = TRUE;
  gimage->flat = TRUE;
  gimage->construct_flag = -1;
  gimage->projection = NULL;
  gimage->guides = NULL;
  gimage->layers = NULL;
  gimage->channels = NULL;
  gimage->layer_stack = NULL;
  gimage->undo_stack = NULL;
  gimage->redo_stack = NULL;
  gimage->undo_bytes = 0;
  gimage->undo_levels = 0;
  gimage->pushing_undo_group = 0;
  gimage->comp_preview_valid[0] = FALSE;
  gimage->comp_preview_valid[1] = FALSE;
  gimage->comp_preview_valid[2] = FALSE;
  gimage->comp_preview = NULL;

  image_list = append_to_list (image_list, (void *) gimage);

  return gimage;
}

static void
gimage_allocate_projection (GImage *gimage)
{
  if (gimage->projection)
    gimage_free_projection (gimage);

  /*  Find the number of bytes required for the projection.
   *  This includes the intensity channels and an alpha channel
   *  if one doesn't exist.
   */
  switch (gimage_base_type (gimage))
    {
    case RGB:
    case INDEXED:
      gimage->proj_bytes = 4;
      gimage->proj_type = RGBA_GIMAGE;
      break;
    case GRAY:
      gimage->proj_bytes = 2;
      gimage->proj_type = GRAYA_GIMAGE;
      break;
    default:
      warning ("gimage type unsupported.\n");
      break;
    }

  /*  allocate the new projection  */
  gimage->projection = tile_manager_new (gimage->width, gimage->height, gimage->proj_bytes);
  gimage->projection->user_data = (void *) gimage;
  tile_manager_set_validate_proc (gimage->projection, gimage_validate);
}

static void
gimage_free_projection (GImage *gimage)
{
  if (gimage->projection)
    tile_manager_destroy (gimage->projection);

  gimage->projection = NULL;
}

static void
gimage_allocate_shadow (GImage *gimage, int width, int height, int bpp)
{
  /*  allocate the new projection  */
  gimage->shadow = tile_manager_new (width, height, bpp);
}


/* function definitions */

GImage *
gimage_new (int width, int height, int base_type)
{
  GImage *gimage;
  int i;

  gimage = gimage_create ();

  gimage->filename = NULL;
  gimage->width = width;
  gimage->height = height;

  gimage->base_type = base_type;

  switch (base_type)
    {
    case RGB:
    case GRAY:
      break;
    case INDEXED:
      /* always allocate 256 colors for the colormap */
      gimage->num_cols = 0;
      gimage->cmap = (unsigned char *) g_malloc (COLORMAP_SIZE);
      memset (gimage->cmap, 0, COLORMAP_SIZE);
      break;
    default:
      break;
    }

  /*  configure the active pointers  */
  gimage->active_layer = -1;
  gimage->active_channel = -1;  /* no default active channel */
  gimage->floating_sel = -1;

  /*  set all color channels visible and active  */
  for (i = 0; i < MAX_CHANNELS; i++)
    {
      gimage->visible[i] = 1;
      gimage->active[i] = 1;
    }

  /* create the selection mask */
  gimage->selection_mask = channel_new_mask (gimage->ID, gimage->width, gimage->height);

  lc_dialog_update_image_list ();
  indexed_palette_update_image_list ();

  return gimage;
}


void
gimage_set_filename (GImage *gimage, char *filename)
{
  char *new_filename;

  new_filename = g_strdup (filename);
  if (gimage->has_filename)
    g_free (gimage->filename);

  if (filename && filename[0])
    {
      gimage->filename = new_filename;
      gimage->has_filename = TRUE;
    }
  else
    {
      gimage->filename = NULL;
      gimage->has_filename = FALSE;
    }

  gdisplays_update_title (gimage->ID);
  lc_dialog_update_image_list ();
  indexed_palette_update_image_list ();
}


void
gimage_resize (GImage *gimage, int new_width, int new_height,
	       int offset_x, int offset_y)
{
  Channel *channel;
  Layer *layer;
  Layer *floating_layer;
  link_ptr list;

  if (new_width <= 0 || new_height <= 0) 
    {
      warning("gimage_resize: width and height must be positive");
      return;
    }

  /*  Get the floating layer if one exists  */
  floating_layer = gimage_floating_sel (gimage);

  undo_push_group_start (gimage, GIMAGE_MOD_UNDO);

  /*  Relax the floating selection  */
  if (floating_layer)
    floating_sel_relax (floating_layer, TRUE);

  /*  Push the image size to the stack  */
  undo_push_gimage_mod (gimage);

  /*  Set the new width and height  */
  gimage->width = new_width;
  gimage->height = new_height;

  /*  Resize all channels  */
  list = gimage->channels;
  while (list)
    {
      channel = (Channel *) list->data;
      channel_resize (channel, new_width, new_height, offset_x, offset_y);
      list = next_item (list);
    }

  /*  Don't forget the selection mask!  */
  channel_resize (gimage->selection_mask, new_width, new_height, offset_x, offset_y);
  gimage_mask_invalidate (gimage);

  /*  Reposition all layers  */
  list = gimage->layers;
  while (list)
    {
      layer = (Layer *) list->data;
      layer_translate (layer, offset_x, offset_y);
      list = next_item (list);
    }

  /*  Make sure the projection matches the gimage size  */
  gimage_projection_realloc (gimage);

  /*  Rigor the floating selection  */
  if (floating_layer)
    floating_sel_rigor (floating_layer, TRUE);

  undo_push_group_end (gimage);

  /*  shrink wrap and update all views  */
  channel_invalidate_previews (gimage->ID);
  layer_invalidate_previews (gimage->ID);
  gimage_invalidate_preview (gimage);
  gdisplays_update_full (gimage->ID);
  gdisplays_shrink_wrap (gimage->ID);
}


void
gimage_scale (GImage *gimage, int new_width, int new_height)
{
  Channel *channel;
  Layer *layer;
  Layer *floating_layer;
  link_ptr list;
  int old_width, old_height;
  int layer_width, layer_height;

  /*  Get the floating layer if one exists  */
  floating_layer = gimage_floating_sel (gimage);

  undo_push_group_start (gimage, GIMAGE_MOD_UNDO);

  /*  Relax the floating selection  */
  if (floating_layer)
    floating_sel_relax (floating_layer, TRUE);

  /*  Push the image size to the stack  */
  undo_push_gimage_mod (gimage);

  /*  Set the new width and height  */
  old_width = gimage->width;
  old_height = gimage->height;
  gimage->width = new_width;
  gimage->height = new_height;

  /*  Scale all channels  */
  list = gimage->channels;
  while (list)
    {
      channel = (Channel *) list->data;
      channel_scale (channel, new_width, new_height);
      list = next_item (list);
    }

  /*  Don't forget the selection mask!  */
  channel_scale (gimage->selection_mask, new_width, new_height);
  gimage_mask_invalidate (gimage);

  /*  Scale all layers  */
  list = gimage->layers;
  while (list)
    {
      layer = (Layer *) list->data;

      layer_width = (new_width * layer->width) / old_width;
      layer_height = (new_height * layer->height) / old_height;
      layer_scale (layer, layer_width, layer_height, FALSE);
      list = next_item (list);
    }

  /*  Make sure the projection matches the gimage size  */
  gimage_projection_realloc (gimage);

  /*  Rigor the floating selection  */
  if (floating_layer)
    floating_sel_rigor (floating_layer, TRUE);

  undo_push_group_end (gimage);

  /*  shrink wrap and update all views  */
  channel_invalidate_previews (gimage->ID);
  layer_invalidate_previews (gimage->ID);
  gimage_invalidate_preview (gimage);
  gdisplays_update_full (gimage->ID);
  gdisplays_shrink_wrap (gimage->ID);
}


GImage *
gimage_get_named (char *name)
{
  link_ptr tmp = image_list;
  GImage *gimage;
  char *str;

  while (tmp)
    {
      gimage = tmp->data;
      str = prune_filename (gimage_filename (gimage));
      if (strcmp (str, name) == 0)
	return gimage;

      tmp = next_item (tmp);
    }

  return NULL;
}


GImage *
gimage_get_ID (int ID)
{
  link_ptr tmp = image_list;
  GImage *gimage;

  while (tmp)
    {
      gimage = (GImage *) tmp->data;
      if (gimage->ID == ID)
	return gimage;

      tmp = next_item (tmp);
    }

  return NULL;
}


TileManager *
gimage_shadow (GImage *gimage, int width, int height, int bpp)
{
  if (gimage->shadow &&
      ((width != gimage->shadow->levels[0].width) ||
       (height != gimage->shadow->levels[0].height) ||
       (bpp != gimage->shadow->levels[0].bpp)))
    gimage_free_shadow (gimage);
  else if (gimage->shadow)
    return gimage->shadow;

  gimage_allocate_shadow (gimage, width, height, bpp);

  return gimage->shadow;
}


void
gimage_free_shadow (GImage *gimage)
{
  /*  Free the shadow buffer from the specified gimage if it exists  */
  if (gimage->shadow)
    tile_manager_destroy (gimage->shadow);

  gimage->shadow = NULL;
}


void
gimage_delete (GImage *gimage)
{
  gimage->ref_count--;

  if (gimage->ref_count <= 0)
    {
      /*  free the undo list  */
      undo_free (gimage);

      /*  remove this image from the global list  */
      image_list = remove_from_list (image_list, (void *) gimage);

      gimage_free_projection (gimage);
      gimage_free_shadow (gimage);

      if (gimage->cmap)
	g_free (gimage->cmap);

      if (gimage->has_filename)
	g_free (gimage->filename);

      gimage_free_layers (gimage);
      gimage_free_channels (gimage);
      channel_delete (gimage->selection_mask);

      g_free (gimage);

      lc_dialog_update_image_list ();

      indexed_palette_update_image_list ();
    }
}

void
gimage_apply_image (GImage *gimage, int drawable_id, PixelRegion *src2PR,
		    int undo, int opacity, int mode,
		    /*  alternative to using drawable tiles as src1: */
		    TileManager *src1_tiles,
		    int x, int y)
{
  Channel * mask;
  int x1, y1, x2, y2;
  int offset_x, offset_y;
  PixelRegion src1PR, destPR, maskPR;
  int operation;
  int active [MAX_CHANNELS];

  /*  get the selection mask if one exists  */
  mask = (gimage_mask_is_empty (gimage)) ?
    NULL : gimage_get_mask (gimage);

  /*  configure the active channel array  */
  gimage_get_active_channels (gimage, drawable_id, active);

  /*  determine what sort of operation is being attempted and
   *  if it's actually legal...
   */
  operation = valid_combinations [drawable_type (drawable_id)][src2PR->bytes];
  if (operation == -1)
    {
      warning ("gimage_apply_image sent illegal parameters\n");
      return;
    }

  /*  get the layer offsets  */
  drawable_offsets (drawable_id, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within gimage bounds  */
  x1 = BOUNDS (x, 0, drawable_width (drawable_id));
  y1 = BOUNDS (y, 0, drawable_height (drawable_id));
  x2 = BOUNDS (x + src2PR->w, 0, drawable_width (drawable_id));
  y2 = BOUNDS (y + src2PR->h, 0, drawable_height (drawable_id));

  if (mask)
    {
      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      x1 = BOUNDS (x1, -offset_x, mask->width - offset_x);
      y1 = BOUNDS (y1, -offset_y, mask->height - offset_y);
      x2 = BOUNDS (x2, -offset_x, mask->width - offset_x);
      y2 = BOUNDS (y2, -offset_y, mask->height - offset_y);
    }

  /*  If the calling procedure specified an undo step...  */
  if (undo)
    drawable_apply_image (drawable_id, x1, y1, x2, y2, NULL, FALSE);

  /* configure the pixel regions
   *  If an alternative to using the drawable's data as src1 was provided...
   */
  if (src1_tiles)
    pixel_region_init (&src1PR, src1_tiles, x1, y1, (x2 - x1), (y2 - y1), FALSE);
  else
    pixel_region_init (&src1PR, drawable_data (drawable_id), x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, drawable_data (drawable_id), x1, y1, (x2 - x1), (y2 - y1), TRUE);
  pixel_region_resize (src2PR, src2PR->x + (x1 - x), src2PR->y + (y1 - y), (x2 - x1), (y2 - y1));

  if (mask)
    {
      int mx, my;

      /*  configure the mask pixel region
       *  don't use x1 and y1 because they are in layer
       *  coordinate system.  Need mask coordinate system
       */
      mx = x1 + offset_x;
      my = y1 + offset_y;

      pixel_region_init (&maskPR, mask->tiles, mx, my, (x2 - x1), (y2 - y1), FALSE);
      combine_regions (&src1PR, src2PR, &destPR, &maskPR, NULL,
		       opacity, mode, active, operation);
    }
  else
    combine_regions (&src1PR, src2PR, &destPR, NULL, NULL,
		     opacity, mode, active, operation);
}


void
gimage_get_foreground (GImage *gimage, int drawable_id, unsigned char *fg)
{
  unsigned char pfg[3];

  /*  Get the palette color  */
  palette_get_foreground (&pfg[0], &pfg[1], &pfg[2]);

  gimage_transform_color (gimage, drawable_id, pfg, fg, RGB);
}


void
gimage_get_background (GImage *gimage, int drawable_id, unsigned char *bg)
{
  unsigned char pbg[3];

  /*  Get the palette color  */
  palette_get_background (&pbg[0], &pbg[1], &pbg[2]);

  gimage_transform_color (gimage, drawable_id, pbg, bg, RGB);
}


void
gimage_get_color (GImage *gimage, int d_type,
		  unsigned char *rgb, unsigned char *src)
{
  switch (d_type)
    {
    case RGB_GIMAGE: case RGBA_GIMAGE:
      map_to_color (0, NULL, src, rgb);
      break;
    case GRAY_GIMAGE: case GRAYA_GIMAGE:
      map_to_color (1, NULL, src, rgb);
      break;
    case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
      map_to_color (2, gimage->cmap, src, rgb);
      break;
    }
}


void
gimage_transform_color (GImage *gimage, int drawable_id,
			unsigned char *src, unsigned char *dest, int type)
{
#define INTENSITY(r,g,b) (r * 0.30 + g * 0.59 + b * 0.11 + 0.001)
  int d_type;

  d_type = (drawable_id != -1) ? drawable_type (drawable_id) :
    gimage_base_type_with_alpha (gimage);

  switch (type)
    {
    case RGB:
      switch (d_type)
	{
	case RGB_GIMAGE: case RGBA_GIMAGE:
	  /*  Straight copy  */
	  *dest++ = *src++;
	  *dest++ = *src++;
	  *dest++ = *src++;
	  break;
	case GRAY_GIMAGE: case GRAYA_GIMAGE:
	  /*  NTSC conversion  */
	  *dest = INTENSITY (src[RED_PIX],
			     src[GREEN_PIX],
			     src[BLUE_PIX]);
	  break;
	case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
	  /*  Least squares method  */
	  *dest = map_rgb_to_indexed (gimage->cmap,
				      gimage->num_cols,
				      gimage->ID,
				      src[RED_PIX],
				      src[GREEN_PIX],
				      src[BLUE_PIX]);
	  break;
	}
      break;
    case GRAY:
      switch (d_type)
	{
	case RGB_GIMAGE: case RGBA_GIMAGE:
	  /*  Gray to RG&B */
	  *dest++ = *src;
	  *dest++ = *src;
	  *dest++ = *src;
	  break;
	case GRAY_GIMAGE: case GRAYA_GIMAGE:
	  /*  Straight copy  */
	  *dest = *src;
	  break;
	case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
	  /*  Least squares method  */
	  *dest = map_rgb_to_indexed (gimage->cmap,
				      gimage->num_cols,
				      gimage->ID,
				      src[GRAY_PIX],
				      src[GRAY_PIX],
				      src[GRAY_PIX]);
	  break;
	}
      break;
    default:
      break;
    }
}

Guide*
gimage_add_hguide (GImage *gimage)
{
  Guide *guide;

  guide = g_new (Guide, 1);
  guide->position = -1;
  guide->orientation = HORIZONTAL_GUIDE;

  gimage->guides = g_list_prepend (gimage->guides, guide);

  return guide;
}

Guide*
gimage_add_vguide (GImage *gimage)
{
  Guide *guide;

  guide = g_new (Guide, 1);
  guide->position = -1;
  guide->orientation = VERTICAL_GUIDE;

  gimage->guides = g_list_prepend (gimage->guides, guide);

  return guide;
}

void
gimage_add_guide (GImage *gimage,
		  Guide  *guide)
{
  gimage->guides = g_list_prepend (gimage->guides, guide);
}

void
gimage_remove_guide (GImage *gimage,
		     Guide  *guide)
{
  gimage->guides = g_list_remove (gimage->guides, guide);
}

void
gimage_delete_guide (GImage *gimage,
		     Guide  *guide)
{
  gimage->guides = g_list_remove (gimage->guides, guide);
  g_free (guide);
}


/************************************************************/
/*  Projection functions                                    */
/************************************************************/

static void
project_intensity (GImage *gimage, Layer *layer,
		   PixelRegion *src, PixelRegion *dest, PixelRegion *mask)
{
  if (! gimage->construct_flag)
    initial_region (src, dest, mask, NULL, layer->opacity,
		    layer->mode, gimage->visible, INITIAL_INTENSITY);
  else
    combine_regions (dest, src, dest, mask, NULL, layer->opacity,
		     layer->mode, gimage->visible, COMBINE_INTEN_A_INTEN);
}


static void
project_intensity_alpha (GImage *gimage, Layer *layer,
			 PixelRegion *src, PixelRegion *dest,
			 PixelRegion *mask)
{
  if (! gimage->construct_flag)
    initial_region (src, dest, mask, NULL, layer->opacity,
		    layer->mode, gimage->visible, INITIAL_INTENSITY_ALPHA);
  else
    combine_regions (dest, src, dest, mask, NULL, layer->opacity,
		     layer->mode, gimage->visible, COMBINE_INTEN_A_INTEN_A);
}


static void
project_indexed (GImage *gimage, Layer *layer,
		 PixelRegion *src, PixelRegion *dest)
{
  if (! gimage->construct_flag)
    initial_region (src, dest, NULL, gimage->cmap, layer->opacity,
		    layer->mode, gimage->visible, INITIAL_INDEXED);
  else
    warning ("Unable to project indexed image.");
}


static void
project_indexed_alpha (GImage *gimage, Layer *layer,
		       PixelRegion *src, PixelRegion *dest,
		       PixelRegion *mask)
{
  if (! gimage->construct_flag)
    initial_region (src, dest, mask, gimage->cmap, layer->opacity,
		    layer->mode, gimage->visible, INITIAL_INDEXED_ALPHA);
  else
    combine_regions (dest, src, dest, mask, gimage->cmap, layer->opacity,
		     layer->mode, gimage->visible, COMBINE_INTEN_A_INDEXED_A);
}


static void
project_channel (GImage *gimage, Channel *channel,
		 PixelRegion *src, PixelRegion *src2)
{
  int type;

  if (! gimage->construct_flag)
    {
      type = (channel->show_masked) ?
	INITIAL_CHANNEL_MASK : INITIAL_CHANNEL_SELECTION;
      initial_region (src2, src, NULL, channel->col, channel->opacity,
		      NORMAL, NULL, type);
    }
  else
    {
      type = (channel->show_masked) ?
	COMBINE_INTEN_A_CHANNEL_MASK : COMBINE_INTEN_A_CHANNEL_SELECTION;
      combine_regions (src, src2, src, NULL, channel->col, channel->opacity,
		       NORMAL, NULL, type);
    }
}


/************************************************************/
/*  Layer/Channel functions                                 */
/************************************************************/


static void
gimage_free_layers (GImage *gimage)
{
  link_ptr list = gimage->layers;
  Layer * layer;

  while (list)
    {
      layer = (Layer *) list->data;
      layer_delete (layer);
      list = next_item (list);
    }
  free_list (gimage->layers);
  free_list (gimage->layer_stack);
}


static void
gimage_free_channels (GImage *gimage)
{
  link_ptr list = gimage->channels;
  Channel * channel;

  while (list)
    {
      channel = (Channel *) list->data;
      channel_delete (channel);
      list = next_item (list);
    }
  free_list (gimage->channels);
}


static void
gimage_construct_layers (GImage *gimage, int x, int y, int w, int h)
{
  Layer * layer;
  int x1, y1, x2, y2;
  PixelRegion src1PR, src2PR, maskPR;
  PixelRegion * mask;
  link_ptr list = gimage->layers;
  link_ptr reverse_list = NULL;

  /*  composite the floating selection if it exists  */
  if ((layer = gimage_floating_sel (gimage)))
    floating_sel_composite (layer, x, y, w, h, FALSE);

  /*  If all channels are not visible, simply return  */
  switch (gimage_base_type (gimage))
    {
    case RGB:
      if (! gimage_get_component_visible (gimage, Red) &&
	  ! gimage_get_component_visible (gimage, Green) &&
	  ! gimage_get_component_visible (gimage, Blue))
	return;
      break;
    case GRAY:
      if (! gimage_get_component_visible (gimage, Gray))
	return;
      break;
    case INDEXED:
      if (! gimage_get_component_visible (gimage, Indexed))
	return;
      break;
    }

  while (list)
    {
      layer = (Layer *) list->data;

      /*  only add layers that are visible and not floating selections to the list  */
      if (!layer_is_floating_sel (layer) && layer->visible)
	reverse_list = add_to_list (reverse_list, layer);

      list = next_item (list);
    }

  while (reverse_list)
    {
      layer = (Layer *) reverse_list->data;

      x1 = BOUNDS (layer->offset_x, x, x + w);
      y1 = BOUNDS (layer->offset_y, y, y + h);
      x2 = BOUNDS (layer->offset_x + layer->width, x, x + w);
      y2 = BOUNDS (layer->offset_y + layer->height, y, y + h);

      /* configure the pixel regions  */
      pixel_region_init (&src1PR, gimage_projection (gimage), x1, y1, (x2 - x1), (y2 - y1), TRUE);

      /*  If we're showing the layer mask instead of the layer...  */
      if (layer->mask && layer->show_mask)
	{
	  pixel_region_init (&src2PR, layer->mask->tiles,
			     (x1 - layer->offset_x), (y1 - layer->offset_y),
			     (x2 - x1), (y2 - y1), FALSE);

	  copy_gray_to_region (&src2PR, &src1PR);
	}
      /*  Otherwise, normal  */
      else
	{
	  pixel_region_init (&src2PR, layer->tiles,
			     (x1 - layer->offset_x), (y1 - layer->offset_y),
			     (x2 - x1), (y2 - y1), FALSE);

	  if (layer->mask && layer->apply_mask)
	    {
	      pixel_region_init (&maskPR, layer->mask->tiles,
				 (x1 - layer->offset_x), (y1 - layer->offset_y),
				 (x2 - x1), (y2 - y1), FALSE);
	      mask = &maskPR;
	    }
	  else
	    mask = NULL;

	  /*  Based on the type of the layer, project the layer onto the
	   *  projection image...
	   */
	  switch (layer->type)
	    {
	    case RGB_GIMAGE: case GRAY_GIMAGE:
	      /* no mask possible */
	      project_intensity (gimage, layer, &src2PR, &src1PR, mask);
	      break;
	    case RGBA_GIMAGE: case GRAYA_GIMAGE:
	      project_intensity_alpha (gimage, layer, &src2PR, &src1PR, mask);
	      break;
	    case INDEXED_GIMAGE:
	      /* no mask possible */
	      project_indexed (gimage, layer, &src2PR, &src1PR);
	      break;
	    case INDEXEDA_GIMAGE:
	      project_indexed_alpha (gimage, layer, &src2PR, &src1PR, mask);
	      break;
	    default:
	      break;
	    }
	}
      gimage->construct_flag = 1;  /*  something was projected  */

      reverse_list = next_item (reverse_list);
    }

  free_list (reverse_list);
}


static void
gimage_construct_channels (GImage *gimage, int x, int y, int w, int h)
{
  Channel * channel;
  PixelRegion src1PR, src2PR;
  link_ptr list = gimage->channels;
  link_ptr reverse_list = NULL;

  /*  reverse the channel list  */
  while (list)
    {
      reverse_list = add_to_list (reverse_list, list->data);
      list = next_item (list);
    }

  while (reverse_list)
    {
      channel = (Channel *) reverse_list->data;

      if (channel->visible)
	{
	  /* configure the pixel regions  */
	  pixel_region_init (&src1PR, gimage_projection (gimage), x, y, w, h, TRUE);
	  pixel_region_init (&src2PR, channel->tiles, x, y, w, h, FALSE);

	  project_channel (gimage, channel, &src1PR, &src2PR);

	  gimage->construct_flag = 1;
	}

      reverse_list = next_item (reverse_list);
    }

  free_list (reverse_list);
}


static void
gimage_initialize_projection (GImage *gimage, int x, int y, int w, int h)
{
  link_ptr list;
  Layer *layer;
  int coverage = 0;
  PixelRegion PR;
  unsigned char clear[4] = { 0, 0, 0, 0 };

  /*  this function determines whether a visible layer
   *  provides complete coverage over the image.  If not,
   *  the projection is initialized to transparent
   */
  list = gimage->layers;
  while (list)
    {
      layer = (Layer *) list->data;
      if (layer->visible &&
	  ! layer_has_alpha (layer) &&
	  (layer->offset_x <= x) &&
	  (layer->offset_y <= y) &&
	  (layer->offset_x + layer->width >= x + w) &&
	  (layer->offset_y + layer->height >= y + h))
	coverage = 1;

      list = next_item (list);
    }

  if (!coverage)
    {
      pixel_region_init (&PR, gimage_projection (gimage), x, y, w, h, TRUE);
      color_region (&PR, clear);
    }
}


static void
gimage_get_active_channels (GImage *gimage, int drawable_id, int *active)
{
  Layer * layer;
  int i;

  /*  first, blindly copy the gimage active channels  */
  for (i = 0; i < MAX_CHANNELS; i++)
    active[i] = gimage->active[i];

  /*  If the drawable is a channel (a saved selection, etc.)
   *  make sure that the alpha channel is not valid
   */
  if (drawable_channel (drawable_id) != NULL)
    active[ALPHA_G_PIX] = 0;  /*  no alpha values in channels  */
  else
    {
      /*  otherwise, check whether preserve transparency is
       *  enabled in the layer and if the layer has alpha
       */
      if ((layer = drawable_layer (drawable_id)))
	if (layer_has_alpha (layer) && layer->preserve_trans)
	  active[layer->bytes - 1] = 0;
    }
}


void
gimage_inflate (GImage *gimage)
{
  /*  Make sure the projection image is allocated  */
  gimage_allocate_projection (gimage);

  gimage->flat = FALSE;

  /*  update the gdisplay titles  */
  gdisplays_update_title (gimage->ID);
}


void
gimage_deflate (GImage *gimage)
{
  /*  Make sure the projection image is deallocated  */
  gimage_free_projection (gimage);

  gimage->flat = TRUE;

  /*  update the gdisplay titles  */
  gdisplays_update_title (gimage->ID);
}


void
gimage_construct (GImage *gimage, int x, int y, int w, int h)
{
  /*  if the gimage is not flat, construction is necessary.  */
  if (! gimage_is_flat (gimage))
    {
      /*  set the construct flag, used to determine if anything
       *  has been written to the gimage raw image yet.
       */
      gimage->construct_flag = 0;

      /*  First, determine if the projection image needs to be
       *  initialized--this is the case when there are no visible
       *  layers that cover the entire canvas--either because layers
       *  are offset or only a floating selection is visible
       */
      gimage_initialize_projection (gimage, x, y, w, h);

      /*  call functions which process the list of layers and
       *  the list of channels
       */
      gimage_construct_layers (gimage, x, y, w, h);
      gimage_construct_channels (gimage, x, y, w, h);
    }
}

void
gimage_invalidate (GImage *gimage, int x, int y, int w, int h, int x1, int y1,
		   int x2, int y2)
{
  Tile *tile;
  TileManager *tm;
  int i, j;
  int startx, starty;
  int endx, endy;
  int tilex, tiley;
  int flat;

  flat = gimage_is_flat (gimage);
  tm = gimage_projection (gimage);

  startx = x;
  starty = y;
  endx = x + w;
  endy = y + h;

  /*  invalidate all tiles which are located outside of the displayed area
   *   all tiles inside the displayed area are constructed.
   */
  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
      {
	tile = tile_manager_get_tile (tm, j, i, 0);

	/*  invalidate all lower level tiles  */
	/*tile_manager_invalidate_tiles (gimage_projection (gimage), tile);*/

	if (! flat)
	  {
	    /*  check if the tile is outside the bounds  */
	    if ((MIN ((j + tile->ewidth), x2) - MAX (j, x1)) <= 0)
	      {
		tile->valid = FALSE;
		if (j < x1)
		  startx = MAX (startx, (j + tile->ewidth));
		else
		  endx = MIN (endx, j);
	      }
	    else if (MIN ((i + tile->eheight), y2) - MAX (i, y1) <= 0)
	      {
		tile->valid = FALSE;
		if (i < y1)
		  starty = MAX (starty, (i + tile->eheight));
		else
		  endy = MIN (endy, i);
	      }
	    else
	      {
		/*  If the tile is not valid, make sure we get the entire tile
		 *   in the construction extents
		 */
		if (tile->valid == FALSE)
		  {
		    tilex = j - (j % TILE_WIDTH);
		    tiley = i - (i % TILE_HEIGHT);

		    startx = MIN (startx, tilex);
		    endx = MAX (endx, tilex + tile->ewidth);
		    starty = MIN (starty, tiley);
		    endy = MAX (endy, tiley + tile->eheight);

		    tile->valid = TRUE;
		  }
	      }
	  }
      }

  if (! flat && (endx - startx) > 0 && (endy - starty) > 0)
    gimage_construct (gimage, startx, starty, (endx - startx), (endy - starty));
}

void
gimage_validate (TileManager *tm, Tile *tile, int level)
{
  GImage *gimage;
  int x, y;
  int w, h;

  /*  Get the gimage from the tilemanager  */
  gimage = (GImage *) tm->user_data;

  /*  Find the coordinates of this tile  */
  x = TILE_WIDTH * (tile->tile_num % tm->levels[0].ntile_cols);
  y = TILE_HEIGHT * (tile->tile_num / tm->levels[0].ntile_cols);
  w = tile->ewidth;
  h = tile->eheight;

  gimage_construct (gimage, x, y, w, h);
}

int
gimage_get_layer_index (GImage *gimage, int layer_ID)
{
  Layer *layer;
  link_ptr layers = gimage->layers;
  int index = 0;

  while (layers)
    {
      layer = (Layer *) layers->data;
      if (layer->ID == layer_ID)
	return index;

      index++;
      layers = next_item (layers);
    }

  return -1;
}

int
gimage_get_channel_index (GImage *gimage, int channel_ID)
{
  Channel *channel;
  link_ptr channels = gimage->channels;
  int index = 0;

  while (channels)
    {
      channel = (Channel *) channels->data;
      if (channel->ID == channel_ID)
	return index;

      index++;
      channels = next_item (channels);
    }

  return -1;
}


Layer *
gimage_get_active_layer (GImage *gimage)
{
  return layer_get_ID (gimage->active_layer);
}


Channel *
gimage_get_active_channel (GImage *gimage)
{
  return channel_get_ID (gimage->active_channel);
}


int
gimage_get_component_active (GImage *gimage, ChannelType type)
{
  /*  No sanity checking here...  */
  switch (type)
    {
    case Red:     return gimage->active[RED_PIX]; break;
    case Green:   return gimage->active[GREEN_PIX]; break;
    case Blue:    return gimage->active[BLUE_PIX]; break;
    case Gray:    return gimage->active[GRAY_PIX]; break;
    case Indexed: return gimage->active[INDEXED_PIX]; break;
    default:      return 0; break;
    }
}


int
gimage_get_component_visible (GImage *gimage, ChannelType type)
{
  /*  No sanity checking here...  */
  switch (type)
    {
    case Red:     return gimage->visible[RED_PIX]; break;
    case Green:   return gimage->visible[GREEN_PIX]; break;
    case Blue:    return gimage->visible[BLUE_PIX]; break;
    case Gray:    return gimage->visible[GRAY_PIX]; break;
    case Indexed: return gimage->visible[INDEXED_PIX]; break;
    default:      return 0; break;
    }
}


Channel *
gimage_get_mask (GImage *gimage)
{
  return gimage->selection_mask;
}


int
gimage_layer_boundary (GImage *gimage, BoundSeg **segs, int *num_segs)
{
  Layer *layer;

  /*  The second boundary corresponds to the active layer's
   *  perimeter...
   */
  if ((layer = layer_get_ID (gimage->active_layer)))
    {
      *segs = layer_boundary (layer, num_segs);
      return 1;
    }
  else
    {
      *segs = NULL;
      *num_segs = 0;
      return 0;
    }
}


Layer *
gimage_set_active_layer (GImage *gimage, int layer_id)
{
  Layer *layer;

  /*  First, find the layer in the gimage
   *  If it isn't valid, find the first layer that is
   */
  if (gimage_get_layer_index (gimage, layer_id) == -1)
    {
      if (! gimage->layers)
	return NULL;
      layer = (Layer *) gimage->layers->data;
    }
  else
    layer = layer_get_ID (layer_id);

  if (! layer)
    return NULL;

  /*  Configure the layer stack to reflect this change  */
  gimage->layer_stack = remove_from_list (gimage->layer_stack, (void *) layer);
  gimage->layer_stack = add_to_list (gimage->layer_stack, (void *) layer);

  /*  invalidate the selection boundary because of a layer modification  */
  layer_invalidate_boundary (layer);

  /*  Set the active layer  */
  gimage->active_layer = layer->ID;
  gimage->active_channel = -1;

  /*  return the layer  */
  return layer;
}


Channel *
gimage_set_active_channel (GImage *gimage, int channel_id)
{
  Channel *channel;

  /*  Not if there is a floating selection  */
  if (gimage_floating_sel (gimage))
    return NULL;

  /*  First, find the channel
   *  If it doesn't exist, find the first channel that does
   */
  if (! (channel = channel_get_ID (channel_id)))
    {
      if (! gimage->channels)
	{
	  gimage->active_channel = -1;
	  return NULL;
	}
      channel = (Channel *) gimage->channels->data;
    }

  /*  Set the active channel  */
  gimage->active_channel = channel->ID;

  /*  return the channel  */
  return channel;
}


Channel *
gimage_unset_active_channel (GImage *gimage)
{
  Channel *channel;

  /*  make sure there is an active channel  */
  if (! (channel = channel_get_ID (gimage->active_channel)))
    return NULL;

  /*  Set the active channel  */
  gimage->active_channel = -1;

  return channel;
}


void
gimage_set_component_active (GImage *gimage, ChannelType type, int value)
{
  /*  No sanity checking here...  */
  switch (type)
    {
    case Red:     gimage->active[RED_PIX] = value; break;
    case Green:   gimage->active[GREEN_PIX] = value; break;
    case Blue:    gimage->active[BLUE_PIX] = value; break;
    case Gray:    gimage->active[GRAY_PIX] = value; break;
    case Indexed: gimage->active[INDEXED_PIX] = value; break;
    case Auxillary: break;
    }

  /*  If there is an active channel and we mess with the components,
   *  the active channel gets unset...
   */
  if (type != Auxillary)
    gimage_unset_active_channel (gimage);
}


void
gimage_set_component_visible (GImage *gimage, ChannelType type, int value)
{
  /*  No sanity checking here...  */
  switch (type)
    {
    case Red:     gimage->visible[RED_PIX] = value; break;
    case Green:   gimage->visible[GREEN_PIX] = value; break;
    case Blue:    gimage->visible[BLUE_PIX] = value; break;
    case Gray:    gimage->visible[GRAY_PIX] = value; break;
    case Indexed: gimage->visible[INDEXED_PIX] = value; break;
    default:      break;
    }
}


Layer *
gimage_pick_correlate_layer (GImage *gimage, int x, int y)
{
  Layer *layer;
  link_ptr list;

  list = gimage->layers;
  while (list)
    {
      layer = (Layer *) list->data;
      if (layer_pick_correlate (layer, x, y))
	return layer;

      list = next_item (list);
    }

  return NULL;
}


void
gimage_set_layer_mask_apply (GImage *gimage, int layer_id)
{
  Layer *layer;

  /*  find the layer  */
  if (! (layer = layer_get_ID (layer_id)))
    return;
  if (! layer->mask)
    return;

  layer->apply_mask = ! layer->apply_mask;
  gdisplays_update_area (gimage->ID, layer->offset_x, layer->offset_y,
			 layer->width, layer->height);
}


void
gimage_set_layer_mask_edit (GImage *gimage, int layer_id, int edit)
{
  Layer *layer;

  /*  find the layer  */
  if (! (layer = layer_get_ID (layer_id)))
    return;

  if (layer->mask)
    layer->edit_mask = edit;
}


void
gimage_set_layer_mask_show (GImage *gimage, int layer_id)
{
  Layer *layer;

  /*  find the layer  */
  if (! (layer = layer_get_ID (layer_id)))
    return;
  if (! layer->mask)
    return;

  layer->show_mask = ! layer->show_mask;
  gdisplays_update_area (gimage->ID, layer->offset_x, layer->offset_y,
			 layer->width, layer->height);
}


Layer *
gimage_raise_layer (GImage *gimage, int layer_ID)
{
  Layer *layer;
  Layer *prev_layer;
  link_ptr list;
  link_ptr prev;
  int x1, y1, x2, y2;
  int index = -1;

  list = gimage->layers;
  prev = NULL; prev_layer = NULL;

  while (list)
    {
      layer = (Layer *) list->data;
      if (prev)
	prev_layer = (Layer *) prev->data;

      if (layer->ID == layer_ID)
	{
	  /*  We can only raise a layer if it has an alpha channel &&
	   *  If it's not already the top layer
	   */
	  if (prev && layer_has_alpha (layer) && layer_has_alpha (prev_layer))
	    {
	      list->data = prev_layer;
	      prev->data = layer;

	      /*  calculate minimum area to update  */
	      x1 = MAXIMUM (layer->offset_x, prev_layer->offset_x);
	      y1 = MAXIMUM (layer->offset_y, prev_layer->offset_y);
	      x2 = MINIMUM (layer->offset_x + layer->width,
			    prev_layer->offset_x + prev_layer->width);
	      y2 = MINIMUM (layer->offset_y + layer->height,
			    prev_layer->offset_y + prev_layer->height);
	      if ((x2 - x1) > 0 && (y2 - y1) > 0)
		gdisplays_update_area (gimage->ID, x1, y1, (x2 - x1), (y2 - y1));

	      /*  invalidate the composite preview  */
	      gimage_invalidate_preview (gimage);

	      return prev_layer;
	    }
	  else
	    {
	      message_box ("Layer cannot be raised any further", NULL, NULL);
	      return NULL;
	    }
	}

      prev = list;
      index++;
      list = next_item (list);
    }

  return NULL;
}


Layer *
gimage_lower_layer (GImage *gimage, int layer_ID)
{
  Layer *layer;
  Layer *next_layer;
  link_ptr list;
  link_ptr next;
  int x1, y1, x2, y2;
  int index = 0;

  next_layer = NULL;

  list = gimage->layers;

  while (list)
    {
      layer = (Layer *) list->data;
      next = next_item (list);

      if (next)
	next_layer = (Layer *) next->data;
      index++;

      if (layer->ID == layer_ID)
	{
	  /*  We can only lower a layer if it has an alpha channel &&
	   *  The layer beneath it has an alpha channel &&
	   *  If it's not already the bottom layer
	   */
	  if (next && layer_has_alpha (layer) && layer_has_alpha (next_layer))
	    {
	      list->data = next_layer;
	      next->data = layer;

	      /*  calculate minimum area to update  */
	      x1 = MAXIMUM (layer->offset_x, next_layer->offset_x);
	      y1 = MAXIMUM (layer->offset_y, next_layer->offset_y);
	      x2 = MINIMUM (layer->offset_x + layer->width,
			    next_layer->offset_x + next_layer->width);
	      y2 = MINIMUM (layer->offset_y + layer->height,
			    next_layer->offset_y + next_layer->height);
	      if ((x2 - x1) > 0 && (y2 - y1) > 0)
		gdisplays_update_area (gimage->ID, x1, y1, (x2 - x1), (y2 - y1));

	      /*  invalidate the composite preview  */
	      gimage_invalidate_preview (gimage);

	      return next_layer;
	    }
	  else
	    {
	      message_box ("Layer cannot be lowered any further", NULL, NULL);
	      return NULL;
	    }
	}

      list = next;
    }

  return NULL;
}


Layer *
gimage_merge_visible_layers (GImage *gimage, MergeType merge_type)
{
  link_ptr layer_list;
  link_ptr merge_list = NULL;
  Layer *layer;

  layer_list = gimage->layers;
  while (layer_list)
    {
      layer = (Layer *) layer_list->data;
      if (layer->visible)
	merge_list = append_to_list (merge_list, layer);

      layer_list = next_item (layer_list);
    }

  if (merge_list && merge_list->next)
    {
      layer = gimage_merge_layers (gimage, merge_list, merge_type);
      free_list (merge_list);
      return layer;
    }
  else
    {
      message_box ("There are not enough visible layers for a merge.\nThere must be at least two.",
		   NULL, NULL);
      free_list (merge_list);
      return NULL;
    }
}


Layer *
gimage_flatten (GImage *gimage)
{
  link_ptr layer_list;
  link_ptr merge_list = NULL;
  Layer *layer;

  layer_list = gimage->layers;
  while (layer_list)
    {
      layer = (Layer *) layer_list->data;
      if (layer->visible)
	merge_list = append_to_list (merge_list, layer);

      layer_list = next_item (layer_list);
    }

  layer = gimage_merge_layers (gimage, merge_list, FlattenImage);
  free_list (merge_list);
  return layer;
}


Layer *
gimage_merge_layers (GImage *gimage, link_ptr merge_list, MergeType merge_type)
{
  link_ptr reverse_list = NULL;
  PixelRegion src1PR, src2PR, maskPR;
  PixelRegion * mask;
  Layer *merge_layer;
  Layer *layer;
  unsigned char bg[4] = {0, 0, 0, 0};
  int type;
  int count;
  int x1, y1, x2, y2;
  int x3, y3, x4, y4;
  int operation;
  int position;
  int active[MAX_CHANNELS] = {1, 1, 1, 1};

  layer = NULL;
  type  = RGBA_GIMAGE;
  x1 = y1 = x2 = y2 = 0;

  /*  Get the layer extents  */
  count = 0;
  while (merge_list)
    {
      layer = (Layer *) merge_list->data;

      switch (merge_type)
	{
	case ExpandAsNecessary:
	case ClipToImage:
	  if (!count)
	    {
	      x1 = layer->offset_x;
	      y1 = layer->offset_y;
	      x2 = layer->offset_x + layer->width;
	      y2 = layer->offset_y + layer->height;
	    }
	  else
	    {
	      if (layer->offset_x < x1)
		x1 = layer->offset_x;
	      if (layer->offset_y < y1)
		y1 = layer->offset_y;
	      if ((layer->offset_x + layer->width) > x2)
		x2 = (layer->offset_x + layer->width);
	      if ((layer->offset_y + layer->height) > y2)
		y2 = (layer->offset_y + layer->height);
	    }
	  if (merge_type == ClipToImage)
	    {
	      x1 = BOUNDS (x1, 0, gimage->width);
	      y1 = BOUNDS (y1, 0, gimage->height);
	      x2 = BOUNDS (x2, 0, gimage->width);
	      y2 = BOUNDS (y2, 0, gimage->height);
	    }
	  break;
	case ClipToBottomLayer:
	  if (merge_list->next == NULL)
	    {
	      x1 = layer->offset_x;
	      y1 = layer->offset_y;
	      x2 = layer->offset_x + layer->width;
	      y2 = layer->offset_y + layer->height;
	    }
	  break;
	case FlattenImage:
	  if (merge_list->next == NULL)
	    {
	      x1 = 0;
	      y1 = 0;
	      x2 = gimage->width;
	      y2 = gimage->height;
	    }
	  break;
	}

      count ++;
      reverse_list = add_to_list (reverse_list, layer);
      merge_list = next_item (merge_list);
    }

  if ((x2 - x1) == 0 || (y2 - y1) == 0)
    return NULL;

  /*  Start a merge undo group  */
  undo_push_group_start (gimage, LAYER_MERGE_UNDO);

  if (merge_type == FlattenImage)
    {
      switch (gimage_base_type (gimage))
	{
	case RGB: type = RGB_GIMAGE; break;
	case GRAY: type = GRAY_GIMAGE; break;
	case INDEXED: type = INDEXED_GIMAGE; break;
	}
      merge_layer = layer_new (gimage->ID, gimage->width, gimage->height,
			       type, layer->name, OPAQUE, NORMAL_MODE);

      if (!merge_layer) {
	warning("gimage_merge_layers: could not allocate merge layer");
	return NULL;
      }

      if (!merge_layer) {
	warning("gimage_merge_layers: could not allocate merge layer");
	return NULL;
      }

      /*  get the background for compositing  */
      gimage_get_background (gimage, merge_layer->ID, bg);

      /*  init the pixel region  */
      pixel_region_init (&src1PR, drawable_data (merge_layer->ID), 0, 0, gimage->width, gimage->height, TRUE);

      /*  set the region to the background color  */
      color_region (&src1PR, bg);

      position = 0;
    }
  else
    {
      /*  The final merged layer inherits the attributes of the bottomost layer,
       *  with a notable exception:  The resulting layer has an alpha channel
       *  whether or not the original did
       */
      merge_layer = layer_new (gimage->ID, (x2 - x1), (y2 - y1),
			       drawable_type_with_alpha (layer->ID), layer->name,
			       OPAQUE, NORMAL_MODE);

      if (!merge_layer) {
	warning("gimage_merge_layers: could not allocate merge layer");
	return NULL;
      }

      if (!merge_layer) {
	warning("gimage_merge_layers: could not allocate merge layer");
	return NULL;
      }

      merge_layer->offset_x = x1;
      merge_layer->offset_y = y1;

      /*  Set the layer to transparent  */
      pixel_region_init (&src1PR, drawable_data (merge_layer->ID), 0, 0, (x2 - x1), (y2 - y1), TRUE);

      /*  set the region to 0's  */
      color_region (&src1PR, bg);

      /*  Find the index in the layer list of the bottom layer--we need this
       *  in order to add the final, merged layer to the layer list correctly
       */
      layer = (Layer *) reverse_list->data;
      position = list_length (gimage->layers) - gimage_get_layer_index (gimage, layer->ID);
    }

  while (reverse_list)
    {
      layer = (Layer *) reverse_list->data;

      /*  determine what sort of operation is being attempted and
       *  if it's actually legal...
       */
      operation = valid_combinations [merge_layer->type][layer->bytes];
      if (operation == -1)
	{
	  warning ("gimage_merge_layers attempting to merge incompatible layers\n");
	  return NULL;
	}

      x3 = BOUNDS (layer->offset_x, x1, x2);
      y3 = BOUNDS (layer->offset_y, y1, y2);
      x4 = BOUNDS (layer->offset_x + layer->width, x1, x2);
      y4 = BOUNDS (layer->offset_y + layer->height, y1, y2);

      /* configure the pixel regions  */
      pixel_region_init (&src1PR, merge_layer->tiles, (x3 - x1), (y3 - y1), (x4 - x3), (y4 - y3), TRUE);
      pixel_region_init (&src2PR, layer->tiles, (x3 - layer->offset_x), (y3 - layer->offset_y),
			 (x4 - x3), (y4 - y3), FALSE);

      if (layer->mask)
	{
	  pixel_region_init (&maskPR, layer->mask->tiles, (x3 - layer->offset_x), (y3 - layer->offset_y),
			     (x4 - x3), (y4 - y3), FALSE);
	  mask = &maskPR;
	}
      else
	mask = NULL;

      combine_regions (&src1PR, &src2PR, &src1PR, mask, NULL,
		       layer->opacity, layer->mode, active, operation);

      gimage_remove_layer (gimage, layer->ID);
      reverse_list = next_item (reverse_list);
    }

  free_list (reverse_list);

  /*  if the type is flatten, remove all the remaining layers  */
  if (merge_type == FlattenImage)
    {
      merge_list = gimage->layers;
      while (merge_list)
	{
	  layer = (Layer *) merge_list->data;
	  merge_list = next_item (merge_list);
	  gimage_remove_layer (gimage, layer->ID);
	}

      gimage_add_layer (gimage, merge_layer, position);
    }
  else
    {
      /*  Add the layer to the gimage  */
      gimage_add_layer (gimage, merge_layer, (list_length (gimage->layers) - position + 1));
    }

  /*  End the merge undo group  */
  undo_push_group_end (gimage);

  /*  Update the gimage  */
  merge_layer->visible = TRUE;

  /*  update gdisplay titles to reflect the possibility of
   *  this layer being the only layer in the gimage
   */
  gdisplays_update_title (gimage->ID);

  drawable_update (merge_layer->ID, 0, 0, merge_layer->width, merge_layer->height);

  return merge_layer;
}


Layer *
gimage_add_layer (GImage *gimage, Layer *float_layer, int position)
{
  LayerUndo * lu;

  if (float_layer->gimage_ID != 0 && 
      float_layer->gimage_ID != gimage->ID) 
    {
      warning("gimage_add_layer: attempt to add layer to wrong image");
      return NULL;
    }

  {
    link_ptr ll = gimage->layers;
    while (ll) 
      {
	if (ll->data == float_layer) 
	  {
	    warning("gimage_add_layer: trying to add layer to image twice");
	    return NULL;
	  }
	ll = next_item(ll);
      }
  }  

  /*  Prepare a layer undo and push it  */
  lu = (LayerUndo *) g_malloc (sizeof (LayerUndo));
  lu->layer = float_layer;
  lu->prev_position = 0;
  lu->prev_layer = gimage->active_layer;
  lu->undo_type = 0;
  undo_push_layer (gimage, lu);

  /*  If the layer is a floating selection, set the ID  */
  if (layer_is_floating_sel (float_layer))
    gimage->floating_sel = float_layer->ID;

  /*  let the layer know about the gimage  */
  float_layer->gimage_ID = gimage->ID;

  /*  add the layer to the list at the specified position  */
  if (position == -1)
    position = gimage_get_layer_index (gimage, gimage->active_layer);
  if (position != -1)
    {
      /*  If there is a floating selection (and this isn't it!),
       *  make sure the insert position is greater than 0
       */
      if (gimage_floating_sel (gimage) && (gimage->floating_sel != float_layer->ID) && position == 0)
	position = 1;
      gimage->layers = insert_in_list (gimage->layers, float_layer, position);
    }
  else
    gimage->layers = add_to_list (gimage->layers, float_layer);
  gimage->layer_stack = add_to_list (gimage->layer_stack, float_layer);

  /*  notify the layers dialog of the currently active layer  */
  gimage_set_active_layer (gimage, float_layer->ID);

  /*  update the new layer's area  */
  drawable_update (float_layer->ID, 0, 0, float_layer->width, float_layer->height);

  /*  invalidate the composite preview  */
  gimage_invalidate_preview (gimage);

  return float_layer;
}


Layer *
gimage_remove_layer (GImage *gimage, int layer_id)
{
  LayerUndo *lu;
  Layer *layer;

  if ((layer = layer_get_ID (layer_id)))
    {
      /*  Prepare a layer undo--push it at the end  */
      lu = (LayerUndo *) g_malloc (sizeof (LayerUndo));
      lu->layer = layer;
      lu->prev_position = gimage_get_layer_index (gimage, layer_id);
      lu->prev_layer = layer_id;
      lu->undo_type = 1;

      gimage->layers = remove_from_list (gimage->layers, layer);
      gimage->layer_stack = remove_from_list (gimage->layer_stack, layer);

      /*  If this was the floating selection, reset the fs pointer  */
      if (gimage->floating_sel == layer_id)
	{
	  gimage->floating_sel = -1;

	  floating_sel_reset (layer);
	}
      else if (gimage->active_layer == layer_id)
	{
	  if (gimage->layers)
	    gimage->active_layer = ((Layer *) gimage->layer_stack->data)->ID;
	  else
	    gimage->active_layer = -1;
	}

      gdisplays_update_area (gimage->ID, layer->offset_x, layer->offset_y,
			     layer->width, layer->height);

      /*  Push the layer undo--It is important it goes here since layer might
       *   be immediately destroyed if the undo push fails
       */
      undo_push_layer (gimage, lu);

      /*  invalidate the composite preview  */
      gimage_invalidate_preview (gimage);

      return NULL;
    }
  else
    return NULL;
}


Channel *
gimage_add_layer_mask (GImage *gimage, int layer_id, int mask_id)
{
  Layer *layer;
  Channel *mask;
  LayerMaskUndo *lmu;
  char *error = NULL;;

  if (! (layer = layer_get_ID (layer_id)))
    return NULL;
  if (! (mask = channel_get_ID (mask_id)))
    return NULL;

  if (layer->mask != NULL)
    error = "Unable to add a layer mask since\nthe layer already has one.";
  if (drawable_indexed (layer_id))
    error = "Unable to add a layer mask to a\nlayer in an indexed image.";
  if (! layer_has_alpha (layer))
    error = "Cannot add layer mask to a layer\nwith no alpha channel.";
  if (layer->width != mask->width || layer->height != mask->height)
    error = "Cannot add layer mask of different dimensions than specified layer.";

  if (error)
    {
      message_box (error, NULL, NULL);
      return NULL;
    }

  layer_add_mask (layer, mask_id);

  /*  Prepare a layer undo and push it  */
  lmu = (LayerMaskUndo *) g_malloc (sizeof (LayerMaskUndo));
  lmu->layer = layer;
  lmu->mask = mask;
  lmu->undo_type = 0;
  lmu->apply_mask = layer->apply_mask;
  lmu->edit_mask = layer->edit_mask;
  lmu->show_mask = layer->show_mask;
  undo_push_layer_mask (gimage, lmu);

  gimage_set_layer_mask_edit (gimage, layer_id, layer->edit_mask);

  return mask;
}


Channel *
gimage_remove_layer_mask (GImage *gimage, int layer_id, int mode)
{
  Layer *layer;
  LayerMaskUndo *lmu;

  if (! (layer = layer_get_ID (layer_id)))
    return NULL;
  if (! layer->mask)
    return NULL;

  /*  Start an undo group  */
  undo_push_group_start (gimage, LAYER_APPLY_MASK_UNDO);

  /*  Prepare a layer mask undo--push it below  */
  lmu = (LayerMaskUndo *) g_malloc (sizeof (LayerMaskUndo));
  lmu->layer = layer;
  lmu->mask = layer->mask;
  lmu->undo_type = 1;
  lmu->mode = mode;
  lmu->apply_mask = layer->apply_mask;
  lmu->edit_mask = layer->edit_mask;
  lmu->show_mask = layer->show_mask;

  layer_apply_mask (layer, mode);

  /*  Push the undo--Important to do it here, AFTER the call
   *   to layer_apply_mask, in case the undo push fails and the
   *   mask is deleted
   */
  undo_push_layer_mask (gimage, lmu);

  /*  end the undo group  */
  undo_push_group_end (gimage);

  /*  If the layer mode is discard, update the layer--invalidate gimage also  */
  if (mode == DISCARD)
    {
      gimage_invalidate_preview (gimage);

      gdisplays_update_area (gimage->ID, layer->offset_x, layer->offset_y,
			     layer->width, layer->height);
    }
  gdisplays_flush ();

  return layer->mask;
}


Channel *
gimage_raise_channel (GImage *gimage, int channel_ID)
{
  Channel *channel;
  Channel *prev_channel;
  link_ptr list;
  link_ptr prev;
  int index = -1;

  list = gimage->channels;
  prev = NULL;
  prev_channel = NULL;

  while (list)
    {
      channel = (Channel *) list->data;
      if (prev)
	prev_channel = (Channel *) prev->data;

      if (channel->ID == channel_ID)
	{
	  if (prev)
	    {
	      list->data = prev_channel;
	      prev->data = channel;
	      drawable_update (channel->ID, 0, 0, channel->width, channel->height);
	      return prev_channel;
	    }
	  else
	    {
	      message_box ("Channel cannot be raised any further", NULL, NULL);
	      return NULL;
	    }
	}

      prev = list;
      index++;
      list = next_item (list);
    }

  return NULL;
}


Channel *
gimage_lower_channel (GImage *gimage, int channel_ID)
{
  Channel *channel;
  Channel *next_channel;
  link_ptr list;
  link_ptr next;
  int index = 0;

  list = gimage->channels;
  next_channel = NULL;

  while (list)
    {
      channel = (Channel *) list->data;
      next = next_item (list);

      if (next)
	next_channel = (Channel *) next->data;
      index++;

      if (channel->ID == channel_ID)
	{
	  if (next)
	    {
	      list->data = next_channel;
	      next->data = channel;
	      drawable_update (channel->ID, 0, 0, channel->width, channel->height);

	      return next_channel;
	    }
	  else
	    {
	      message_box ("Channel cannot be lowered any further", NULL, NULL);
	      return NULL;
	    }
	}

      list = next;
    }

  return NULL;
}


Channel *
gimage_add_channel (GImage *gimage, Channel *channel, int position)
{
  ChannelUndo * cu;

  /*  Prepare a channel undo and push it  */
  cu = (ChannelUndo *) g_malloc (sizeof (ChannelUndo));
  cu->channel = channel;
  cu->prev_position = 0;
  cu->prev_channel = gimage->active_channel;
  cu->undo_type = 0;
  undo_push_channel (gimage, cu);

  /*  add the channel to the list  */
  gimage->channels = add_to_list (gimage->channels, channel);

  /*  notify this gimage of the currently active channel  */
  gimage_set_active_channel (gimage, channel->ID);

  /*  if channel is visible, update the image  */
  if (channel->visible)
    drawable_update (channel->ID, 0, 0, channel->width, channel->height);

  return channel;
}


Channel *
gimage_remove_channel (GImage *gimage, int channel_id)
{
  Channel * channel;
  ChannelUndo * cu;

  if ((channel = channel_get_ID (channel_id)))
    {
      /*  Prepare a channel undo--push it below  */
      cu = (ChannelUndo *) g_malloc (sizeof (ChannelUndo));
      cu->channel = channel;
      cu->prev_position = gimage_get_channel_index (gimage, channel_id);
      cu->prev_channel = gimage->active_channel;
      cu->undo_type = 1;

      gimage->channels = remove_from_list (gimage->channels, channel);

      if (gimage->active_channel == channel_id)
	{
	  if (gimage->channels)
	    gimage->active_channel = ((Channel *) gimage->channels->data)->ID;
	  else
	    gimage->active_channel = -1;
	}

      if (channel->visible)
	drawable_update (channel->ID, 0, 0, channel->width, channel->height);

      /*  Important to push the undo here in case the push fails  */
      undo_push_channel (gimage, cu);

      return channel;
    }
  else
    return NULL;
}


/************************************************************/
/*  Access functions                                        */
/************************************************************/

int
gimage_is_flat (GImage *gimage)
{
  Layer *layer;
  int ac_visible = TRUE;
  int flat = TRUE;

  /*  Are there no layers?  */
  if (gimage_is_empty (gimage))
    flat = FALSE;
  /*  Is there more than one layer?  */
  else if (gimage->layers->next)
    flat = FALSE;
  else
    {
      /*  determine if all channels are visible  */
      int a, b;

      layer = gimage->layers->data;
      a = layer_has_alpha (layer) ? layer->bytes - 1 : layer->bytes;
      for (b = 0; b < a; b++)
	if (gimage->visible[b] == FALSE)
	  ac_visible = FALSE;

      /*  What makes a flat image?
       *  1) the solitary layer is exactly gimage-sized and placed
       *  2) no layer mask
       *  3) opacity == OPAQUE
       *  4) all channels must be visible
       */
      if ((layer->width != gimage->width) ||
	  (layer->height != gimage->height) ||
	  (layer->offset_x != 0) ||
	  (layer->offset_y != 0) ||
	  (layer->mask != NULL) ||
	  (layer->opacity != OPAQUE) ||
	  (ac_visible == FALSE))
	flat = FALSE;
    }

  /*  Are there any channels?  */
  if (gimage->channels)
    flat = FALSE;

  if (gimage->flat != flat)
    {
      if (flat)
	gimage_deflate (gimage);
      else
	gimage_inflate (gimage);
    }

  return gimage->flat;
}

int
gimage_is_empty (GImage *gimage)
{
  return (! gimage->layers);
}

int
gimage_active_drawable (GImage *gimage)
{
  Layer *layer;

  /*  If there is an active channel (a saved selection, etc.),
   *  we ignore the active layer
   */
  if (gimage->active_channel >= 0)
    return gimage->active_channel;
  else if (gimage->active_layer >= 0)
    {
      if ((layer = layer_get_ID (gimage->active_layer)))
	{
	  if (layer->mask && layer->edit_mask)
	    return layer->mask->ID;
	  else
	    return layer->ID;
	}
      else
	return -1;
    }
  else
    return -1;
}

int
gimage_base_type (GImage *gimage)
{
  return gimage->base_type;
}

int
gimage_base_type_with_alpha (GImage *gimage)
{
  switch (gimage->base_type)
    {
    case RGB:
      return RGBA_GIMAGE;
    case GRAY:
      return GRAYA_GIMAGE;
    case INDEXED:
      return INDEXEDA_GIMAGE;
    }
  return RGB_GIMAGE;
}

char *
gimage_filename (GImage *gimage)
{
  if (gimage->has_filename)
    return gimage->filename;
  else
    return "Untitled";
}

int
gimage_enable_undo (GImage *gimage)
{
  /*  Free all undo steps as they are now invalidated  */
  undo_free (gimage);

  gimage->undo_on = TRUE;

  return TRUE;
}

int
gimage_disable_undo (GImage *gimage)
{
  gimage->undo_on = FALSE;
  return TRUE;
}

int
gimage_dirty (GImage *gimage)
{
  if (gimage->dirty < 0)
    gimage->dirty = 2;
  else
    gimage->dirty ++;
  return gimage->dirty;
}

int
gimage_clean (GImage *gimage)
{
  if (gimage->dirty <= 0)
    gimage->dirty = 0;
  else
    gimage->dirty --;
  return gimage->dirty;
}

void
gimage_clean_all (GImage *gimage)
{
  gimage->dirty = 0;
}

Layer *
gimage_floating_sel (GImage *gimage)
{
  if (gimage->floating_sel == -1)
    return NULL;
  else return layer_get_ID (gimage->floating_sel);
}

unsigned char *
gimage_cmap (GImage *gimage)
{
  return drawable_cmap (gimage_active_drawable (gimage));
}


/************************************************************/
/*  Projection access functions                             */
/************************************************************/

TileManager *
gimage_projection (GImage *gimage)
{
  Layer * layer;

  /*  If the gimage is flat, we simply want the data of the
   *  first layer...Otherwise, we'll pass back the projection
   */
  if (gimage_is_flat (gimage))
    {
      if ((layer = layer_get_ID (gimage->active_layer)))
	return layer->tiles;
      else
	return NULL;
    }
  else
    {
      if ((gimage->projection->levels[0].width != gimage->width) ||
	  (gimage->projection->levels[0].height != gimage->height))
	gimage_allocate_projection (gimage);

      return gimage->projection;
    }
}

int
gimage_projection_type (GImage *gimage)
{
  Layer * layer;

  /*  If the gimage is flat, we simply want the type of the
   *  first layer...Otherwise, we'll pass back the proj_type
   */
  if (gimage_is_flat (gimage))
    {
      if ((layer = layer_get_ID (gimage->active_layer)))
	return layer->type;
      else
	return -1;
    }
  else
    return gimage->proj_type;
}

int
gimage_projection_bytes (GImage *gimage)
{
  Layer * layer;

  /*  If the gimage is flat, we simply want the bytes in the
   *  first layer...Otherwise, we'll pass back the proj_bytes
   */
  if (gimage_is_flat (gimage))
    {
      if ((layer = layer_get_ID (gimage->active_layer)))
	return layer->bytes;
      else
	return -1;
    }
  else
    return gimage->proj_bytes;
}

int
gimage_projection_opacity (GImage *gimage)
{
  Layer * layer;

  /*  If the gimage is flat, return the opacity of the active layer
   *  Otherwise, we'll pass back OPAQUE
   */
  if (gimage_is_flat (gimage))
    {
      if ((layer = layer_get_ID (gimage->active_layer)))
	return layer->opacity;
      else
	return OPAQUE;
    }
  else
    return OPAQUE;
}

void
gimage_projection_realloc (GImage *gimage)
{
  if (! gimage_is_flat (gimage))
    gimage_allocate_projection (gimage);
}

/************************************************************/
/*  Composition access functions                            */
/************************************************************/

TileManager *
gimage_composite (GImage *gimage)
{
  return gimage_projection (gimage);
}

int
gimage_composite_type (GImage *gimage)
{
  return gimage_projection_type (gimage);
}

int
gimage_composite_bytes (GImage *gimage)
{
  return gimage_projection_bytes (gimage);
}

static TempBuf *
gimage_construct_composite_preview (GImage *gimage, int width, int height)
{
  Layer * layer;
  PixelRegion src1PR, src2PR, maskPR;
  PixelRegion * mask;
  TempBuf *comp;
  TempBuf *layer_buf;
  TempBuf *mask_buf;
  link_ptr list = gimage->layers;
  link_ptr reverse_list = NULL;
  double ratio;
  int x, y, w, h;
  int x1, y1, x2, y2;
  int bytes;
  int construct_flag;
  int visible[MAX_CHANNELS] = {1, 1, 1, 1};

  ratio = (double) width / (double) gimage->width;

  switch (gimage_base_type (gimage))
    {
    case RGB:
    case INDEXED:
      bytes = 4;
      break;
    case GRAY:
      bytes = 2;
      break;
    default:
      bytes = 0;
      break;
    }

  /*  The construction buffer  */
  comp = temp_buf_new (width, height, bytes, 0, 0, NULL);
  memset (temp_buf_data (comp), 0, comp->width * comp->height * comp->bytes);

  while (list)
    {
      layer = (Layer *) list->data;

      /*  only add layers that are visible and not floating selections to the list  */
      if (!layer_is_floating_sel (layer) && layer->visible)
	reverse_list = add_to_list (reverse_list, layer);

      list = next_item (list);
    }

  construct_flag = 0;

  while (reverse_list)
    {
      layer = (Layer *) reverse_list->data;

      x = (int) (ratio * layer->offset_x + 0.5);
      y = (int) (ratio * layer->offset_y + 0.5);
      w = (int) (ratio * layer->width + 0.5);
      h = (int) (ratio * layer->height + 0.5);

      x1 = BOUNDS (x, 0, width);
      y1 = BOUNDS (y, 0, height);
      x2 = BOUNDS (x + w, 0, width);
      y2 = BOUNDS (y + h, 0, height);

      src1PR.bytes = comp->bytes;
      src1PR.x = x1;  src1PR.y = y1;
      src1PR.w = (x2 - x1);
      src1PR.h = (y2 - y1);
      src1PR.rowstride = comp->width * src1PR.bytes;
      src1PR.data = temp_buf_data (comp) + y1 * src1PR.rowstride + x1 * src1PR.bytes;

      layer_buf = layer_preview (layer, w, h);
      src2PR.bytes = layer_buf->bytes;
      src2PR.w = src1PR.w;  src2PR.h = src1PR.h;
      src2PR.rowstride = layer_buf->width * src2PR.bytes;
      src2PR.data = temp_buf_data (layer_buf) +
	(y1 - y) * src2PR.rowstride + (x1 - x) * src2PR.bytes;

      if (layer->mask && layer->apply_mask)
	{
	  mask_buf = layer_mask_preview (layer, w, h);
	  maskPR.bytes = mask_buf->bytes;
	  maskPR.rowstride = mask_buf->width;
	  maskPR.data = mask_buf_data (mask_buf) +
	    (y1 - y) * maskPR.rowstride + (x1 - x) * maskPR.bytes;
	  mask = &maskPR;
	}
      else
	mask = NULL;

      /*  Based on the type of the layer, project the layer onto the
       *   composite preview...
       *  Indexed images are actually already converted to RGB and RGBA,
       *   so just project them as if they were type "intensity"
       *  Send in all TRUE for visible since that info doesn't matter for previews
       */
      switch (layer->type)
	{
	case RGB_GIMAGE: case GRAY_GIMAGE: case INDEXED_GIMAGE:
	  if (! construct_flag)
	    initial_region (&src2PR, &src1PR, mask, NULL, layer->opacity,
			    layer->mode, visible, INITIAL_INTENSITY);
	  else
	    combine_regions (&src1PR, &src2PR, &src1PR, mask, NULL, layer->opacity,
			     layer->mode, visible, COMBINE_INTEN_A_INTEN);
	  break;

	case RGBA_GIMAGE: case GRAYA_GIMAGE: case INDEXEDA_GIMAGE:
	  if (! construct_flag)
	    initial_region (&src2PR, &src1PR, mask, NULL, layer->opacity,
			    layer->mode, visible, INITIAL_INTENSITY_ALPHA);
	  else
	    combine_regions (&src1PR, &src2PR, &src1PR, mask, NULL, layer->opacity,
			     layer->mode, visible, COMBINE_INTEN_A_INTEN_A);
	  break;

	default:
	  break;
	}

      construct_flag = 1;

      reverse_list = next_item (reverse_list);
    }

  free_list (reverse_list);

  return comp;
}

TempBuf *
gimage_composite_preview (GImage *gimage, ChannelType type,
			  int width, int height)
{
  int channel;

  switch (type)
    {
    case Red:     channel = RED_PIX; break;
    case Green:   channel = GREEN_PIX; break;
    case Blue:    channel = BLUE_PIX; break;
    case Gray:    channel = GRAY_PIX; break;
    case Indexed: channel = INDEXED_PIX; break;
    default: return NULL;
    }

  /*  The easy way  */
  if (gimage->comp_preview_valid[channel] &&
      gimage->comp_preview->width == width &&
      gimage->comp_preview->height == height)
    return gimage->comp_preview;
  /*  The hard way  */
  else
    {
      if (gimage->comp_preview)
	temp_buf_free (gimage->comp_preview);

      /*  Actually construct the composite preview from the layer previews!
       *  This might seem ridiculous, but it's actually the best way, given
       *  a number of unsavory alternatives.
       */
      gimage->comp_preview = gimage_construct_composite_preview (gimage, width, height);
      gimage->comp_preview_valid[channel] = TRUE;

      return gimage->comp_preview;
    }
}

int
gimage_preview_valid (gimage, type)
     GImage *gimage;
     ChannelType type;
{
  switch (type)
    {
    case Red:     return gimage->comp_preview_valid [RED_PIX]; break;
    case Green:   return gimage->comp_preview_valid [GREEN_PIX]; break;
    case Blue:    return gimage->comp_preview_valid [BLUE_PIX]; break;
    case Gray:    return gimage->comp_preview_valid [GRAY_PIX]; break;
    case Indexed: return gimage->comp_preview_valid [INDEXED_PIX]; break;
    default: return TRUE;
    }
}

void
gimage_invalidate_preview (GImage *gimage)
{
  Layer *layer;
  /*  Invalidate the floating sel if it exists  */
  if ((layer = gimage_floating_sel (gimage)))
    floating_sel_invalidate (layer);

  gimage->comp_preview_valid[0] = FALSE;
  gimage->comp_preview_valid[1] = FALSE;
  gimage->comp_preview_valid[2] = FALSE;
}

void
gimage_invalidate_previews (void)
{
  link_ptr tmp = image_list;
  GImage *gimage;

  while (tmp)
    {
      gimage = (GImage *) tmp->data;
      gimage_invalidate_preview (gimage);
      tmp = next_item (tmp);
    }
}
