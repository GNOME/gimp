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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include "appenv.h"
#include "canvas.h"
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
#include "paint_funcs_area.h"
#include "palette.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "plug_in.h"
#include "tools.h"
#include "undo.h"

#include "layer_pvt.h"
#include "drawable_pvt.h"		/* ick ick. */

/*  Local function declarations  */
static void     gimage_free_projection       (GImage *);
static GImage * gimage_create                (void);
static void     gimage_allocate_projection   (GImage *);
static void     gimage_free_layers           (GImage *);
static void     gimage_free_channels         (GImage *);
static void     gimage_construct_layers      (GImage *, int, int, int, int);
static void     gimage_construct_channels    (GImage *, int, int, int, int);
static void     gimage_initialize_projection (GImage *, int, int, int, int);
static void     gimage_get_active_channels   (GImage *, GimpDrawable *, int *);

/*  projection functions  */
static void     project_intensity            (GImage *, Layer *, PixelArea *,
					      PixelArea *, PixelArea *);
static void     project_intensity_alpha      (GImage *, Layer *, PixelArea *,
					      PixelArea *, PixelArea *);
static void     project_indexed              (GImage *, Layer *, PixelArea *,
					      PixelArea *);
static void     project_channel              (GImage *, Channel *, PixelArea *,
					      PixelArea *);

static guint    gimage_validate              (Canvas * c, int x, int y, void * data);


static int
get_operation (
               Tag t1,
               Tag t2
               )
{
  int y = 0;
  
  
  if (tag_precision (t1) != tag_precision (t2))
    return -1;

  if (tag_format (t1) != tag_format (t2))
    return -1;
  
  if (tag_alpha (t1) == ALPHA_YES)
    y += 2;
  
  if (tag_alpha (t2) == ALPHA_YES)
    y += 1;

  switch (tag_format (t1))
    {
    case FORMAT_RGB:
    case FORMAT_GRAY:
      {
        static int x[] = {
          COMBINE_INTEN_INTEN,
          COMBINE_INTEN_INTEN_A,
          COMBINE_INTEN_A_INTEN,
          COMBINE_INTEN_A_INTEN_A
        };
        return x[y];
      }
    case FORMAT_INDEXED:
      {
        static int x[] = {
          COMBINE_INDEXED_INDEXED,
          COMBINE_INDEXED_INDEXED_A,
          -1,
          COMBINE_INDEXED_A_INDEXED_A
        };
        return x[y];
      }
    case FORMAT_NONE:
      return -1;
    }
  
  return -1;
}


/*
 *  Static variables
 */
static int global_gimage_ID = 1;
GSList *image_list = NULL;


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

  image_list = g_slist_append (image_list, (void *) gimage);

  return gimage;
}


Tag 
gimage_tag  (
             GImage * gimage
             )
{
  return (gimage ? gimage->tag : tag_null ());
}

Format 
gimage_format  (
                GImage * gimage
                )
{
  return tag_format (gimage_tag (gimage));
}

static void
gimage_allocate_projection (GImage *gimage)
{
  Tag t;
  
  t = gimage_tag (gimage);

  t = tag_set_alpha (t, ALPHA_YES);

  if (tag_format (t) == FORMAT_INDEXED)
    t = tag_set_format (t, FORMAT_RGB);
  
  if (gimage->projection)
    gimage_free_projection (gimage);

  gimage->projection = canvas_new (t, 
                                   gimage->width, gimage->height,
                                   STORAGE_TILED);

  canvas_portion_init_setup (gimage->projection,
                             gimage_validate,
                             (void*) gimage);
}

static void
gimage_free_projection (GImage *gimage)
{
  if (gimage->projection)
    canvas_delete (gimage->projection);

  gimage->projection = NULL;
}


/* function definitions */

GImage * 
gimage_new  (
             int width,
             int height,
             Tag tag
             )
{
  GImage *gimage;
  int i;

  gimage = gimage_create ();

  gimage->filename = NULL;
  gimage->width = width;
  gimage->height = height;
  
  gimage->tag = tag;

  switch (tag_format (tag))
    {
    case FORMAT_INDEXED:
      /* always allocate 256 colors for the colormap */
      gimage->num_cols = 0;
      gimage->cmap = (unsigned char *) g_malloc (COLORMAP_SIZE);
      memset (gimage->cmap, 0, COLORMAP_SIZE);
      break;
    case FORMAT_RGB:
    case FORMAT_GRAY:
    case FORMAT_NONE:
    default:
      break;
    }

  /*  configure the active pointers  */
  gimage->active_layer = NULL;
  gimage->active_channel = NULL;  /* no default active channel */
  gimage->floating_sel = NULL;

  /*  set all color channels visible and active  */
  for (i = 0; i < MAX_CHANNELS; i++)
    {
      gimage->visible[i] = 1;
      gimage->active[i] = 1;
    }

  /* create the selection mask */
  gimage->selection_mask = channel_new_mask (gimage->ID, 
                                             gimage->width, gimage->height,
                                             tag_precision (tag));

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
  GSList *list;

  if (new_width <= 0 || new_height <= 0) 
    {
      g_message ("gimage_resize: width and height must be positive");
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
      list = g_slist_next (list);
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
      list = g_slist_next (list);
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
  GSList *list;
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
      list = g_slist_next (list);
    }

  /*  Don't forget the selection mask!  */
  channel_scale (gimage->selection_mask, new_width, new_height);
  gimage_mask_invalidate (gimage);

  /*  Scale all layers  */
  list = gimage->layers;
  while (list)
    {
      layer = (Layer *) list->data;

      layer_width = (new_width * drawable_width (GIMP_DRAWABLE(layer))) / old_width;
      layer_height = (new_height * drawable_height (GIMP_DRAWABLE(layer))) / old_height;
      layer_scale (layer, layer_width, layer_height, FALSE);
      list = g_slist_next (list);
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
gimage_get_ID (int ID)
{
  GSList *tmp = image_list;
  GImage *gimage;

  while (tmp)
    {
      gimage = (GImage *) tmp->data;
      if (gimage->ID == ID)
	return gimage;

      tmp = g_slist_next (tmp);
    }

  return NULL;
}


Canvas *
gimage_shadow (GImage *gimage, int width, int height, Tag tag)
{
  if (gimage->shadow &&
      ((width != canvas_width (gimage->shadow)) ||
       (height != canvas_height (gimage->shadow)) ||
       (!tag_equal (tag ,canvas_tag (gimage->shadow)))))
    gimage_free_shadow (gimage);
  else if (gimage->shadow)
    return gimage->shadow;

  /*  allocate the new projection  */
  gimage->shadow = canvas_new (tag, width, height, STORAGE_TILED);

  return gimage->shadow;
}


void
gimage_free_shadow (GImage *gimage)
{
  /*  Free the shadow buffer from the specified gimage if it exists  */
  if (gimage->shadow)
    canvas_delete (gimage->shadow);

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
      image_list = g_slist_remove (image_list, (void *) gimage);

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
gimage_apply_painthit  (
                        GImage * gimage,
                        GimpDrawable * drawable,
                        Canvas * src1,
                        PixelArea * src2_area,
                        int undo,
                        gfloat opacity,
                        int mode,
                        int x,
                        int y
                        )
{
  int operation;
  
  /* make sure we're doing something legal */
  operation = get_operation (drawable_tag (drawable),
                             pixelarea_tag (src2_area));

  if (operation == -1)
    {
      g_warning ("invalid op in gimage_apply_painthit()");
      return;
    }
  
  {
    PixelArea src1PR, destPR, maskPR;
    Channel * mask;

    /* get the mask (if any) */
    mask = (gimage_mask_is_empty (gimage))
      ? NULL : gimage_get_mask (gimage);

    /* init src1 if necessary */
    if (src1 == NULL)
      src1 = drawable_data (drawable);
    
    {
      int offset_x, offset_y;
      int x1, y1, x2, y2;
      
      /*  get the layer offsets  */
      drawable_offsets (drawable, &offset_x, &offset_y);
      
      /*  make sure the image application coordinates are within
          gimage and mask bounds */
      x1 = CLAMP (x, 0, drawable_width (drawable));
      y1 = CLAMP (y, 0, drawable_height (drawable));
      x2 = CLAMP (x + pixelarea_width (src2_area), 0, drawable_width (drawable));
      y2 = CLAMP (y + pixelarea_height (src2_area), 0, drawable_height (drawable));
      
      if (mask)
        {
          x1 = CLAMP (x1, -offset_x, drawable_width (GIMP_DRAWABLE(mask)) - offset_x);
          y1 = CLAMP (y1, -offset_y, drawable_height (GIMP_DRAWABLE(mask)) - offset_y);
          x2 = CLAMP (x2, -offset_x, drawable_width (GIMP_DRAWABLE(mask)) - offset_x);
          y2 = CLAMP (y2, -offset_y, drawable_height (GIMP_DRAWABLE(mask)) - offset_y);
        }


      /* apply the undo if the caller wants one */
      if (undo)
        drawable_apply_image (drawable, x1, y1, x2, y2, NULL);

      /* the underlying image */
      pixelarea_init (&src1PR, src1,
                      x1, y1,
                      (x2 - x1), (y2 - y1),
                      FALSE);
      
      /* the painthit */

      pixelarea_resize (src2_area,
                        pixelarea_x (src2_area) + (x1-x),
                        pixelarea_y (src2_area) + (y1-y),
                        x2 - x1, y2 - y1,
                        FALSE);
      
      pixelarea_init (&destPR, drawable_data (drawable),
                      x1, y1,
                      (x2 - x1), (y2 - y1),
                      TRUE);

      if (mask)
        {
          pixelarea_init (&maskPR, drawable_data (GIMP_DRAWABLE (mask)),
                          x1 + offset_x, y1 + offset_y,
                          (x2 - x1), (y2 - y1),
                          FALSE);
        }
    }
    
    
    {
      int active [MAX_CHANNELS];
      
      gimage_get_active_channels (gimage, drawable, active);

      {
        if (mask)
          combine_areas (&src1PR, src2_area, &destPR, &maskPR, NULL,
                         opacity, mode, active, operation);
        else
          combine_areas (&src1PR, src2_area, &destPR, NULL, NULL,
                         opacity, mode, active, operation);
      }
    }
  }
}


void 
gimage_replace_painthit  (
                          GImage * gimage,
                          GimpDrawable * drawable,
                          Canvas * src2,       /*painthit_canvas*/
                          int undo,
                          gfloat opacity,
                          Canvas * mask_canvas,  /*brush_mask*/
                          int x,
                          int y
                          )
{
  Channel * mask;
  int x1, y1, x2, y2;
  int offset_x, offset_y;
  PixelArea src1_area, src2_area, dest_area, temp_area;
  int operation;
  int active [MAX_CHANNELS];
  Precision prec = tag_precision (drawable_tag (drawable));

  /*  configure the active channel array  */
  gimage_get_active_channels (gimage, drawable, active);
  
  /*  get the selection mask if one exists  */
  mask = (gimage_mask_is_empty (gimage)) ?
    NULL : gimage_get_mask (gimage);

  /*  determine what sort of operation is being attempted and
   *  if it's actually legal...
   */
  operation = get_operation (drawable_tag (drawable),
                             canvas_tag (src2));
  
  if (operation == -1)
    {
      g_message ("gimage_replace_painthit: not a valid combination\n");
      return;
    }

  /*  get the layer offsets  */
  drawable_offsets (drawable, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within gimage bounds  */
  x1 = CLAMP (x, 0, drawable_width (drawable));
  y1 = CLAMP (y, 0, drawable_height (drawable));
  x2 = CLAMP (x + canvas_width (src2), 0, drawable_width (drawable));
  y2 = CLAMP (y + canvas_height (src2), 0, drawable_height (drawable));

  if (mask)
    {
      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      x1 = CLAMP (x1, -offset_x, drawable_width (GIMP_DRAWABLE(mask)) - offset_x);
      y1 = CLAMP (y1, -offset_y, drawable_height (GIMP_DRAWABLE(mask)) - offset_y);
      x2 = CLAMP (x2, -offset_x, drawable_width (GIMP_DRAWABLE(mask)) - offset_x);
      y2 = CLAMP (y2, -offset_y, drawable_height (GIMP_DRAWABLE(mask)) - offset_y);
    }
  
  /*  If the calling procedure specified an undo step...  */
  if (undo)
    drawable_apply_image (drawable, x1, y1, x2, y2, NULL);

  if (mask) /* combine selection and brush masks */  
    {
      Canvas *temp_canvas;
      PixelArea mask_area;
      Tag temp_tag;
      int mx, my;
      gfloat opac = OPAQUE_OPACITY;
      
      /* create a temp canvas to hold the combined selection and brush masks */
      temp_tag = tag_new (prec, FORMAT_GRAY, ALPHA_NO);
      temp_canvas = canvas_new ( temp_tag, x2 - x1, y2 - y1, STORAGE_FLAT);
      
      /*  first initialize mask_area with the selection mask 
       *  don't use x1 and y1 because they are in layer
       *  coordinate system.  Need selection mask coordinate system
       */
      
      mx = x1 + offset_x;
      my = y1 + offset_y;
      pixelarea_init (&mask_area, drawable_data(GIMP_DRAWABLE (mask)),
		    mx, my, 
		    (x2 - x1), (y2 - y1),
		    FALSE);

      pixelarea_init (&temp_area, temp_canvas, 0 , 0 , 0, 0, TRUE);
     
      /* copy selection mask to temp canvas */ 
      copy_area (&mask_area, &temp_area);

      /* next initialize mask_area with the brush mask */
      pixelarea_init (&mask_area, mask_canvas, 0, 0, 0, 0, FALSE);
      pixelarea_init (&temp_area, temp_canvas, 0, 0, 0, 0, TRUE);
      
      /* apply brush mask to temp canvas */
      apply_mask_to_area (&temp_area, &mask_area, opac);
       
      /* ready the temp_area for combine_areas_replace below */ 
      pixelarea_init (&temp_area, temp_canvas, 0, 0, 0, 0, TRUE);
    }
  else /* just use the brush mask */
    pixelarea_init (&temp_area, mask_canvas, 0 , 0 , 0, 0, TRUE);
   
  /* the underlying image */
  pixelarea_init (&src1_area, drawable_data (drawable),
		  x1, y1,
		  (x2 - x1), (y2 - y1),
		  FALSE);
  
  /* the painthit */
  pixelarea_init (&src2_area, src2,
                      x1-x, y1-y,
                      (x2 - (x1-x)), (y2 - (y1-y)),
                      FALSE);
 
  /* the destination image */ 
  pixelarea_init (&dest_area, drawable_data (drawable),
		  x1, y1,
		  (x2 - x1), (y2 - y1),
		  TRUE);
   
  /* combine the painthit(src2_area) with the 
     source(src1_area) using temp_area as a mask */ 
  combine_areas_replace (&src1_area, &src2_area, &dest_area, &temp_area, NULL,
                         opacity, active, operation);
}


Guide*
gimage_add_hguide (GImage *gimage)
{
  Guide *guide;

  guide = g_new (Guide, 1);
  guide->ref_count = 0;
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
  guide->ref_count = 0;
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
		   PixelArea *src, PixelArea *dest, PixelArea *mask)
{
  if (! gimage->construct_flag)
    initial_area (src, dest, mask, NULL, layer->opacity,
		    layer->mode, gimage->visible, INITIAL_INTENSITY);
  else
    combine_areas (dest, src, dest, mask, NULL, layer->opacity,
		     layer->mode, gimage->visible, COMBINE_INTEN_A_INTEN);
}


static void
project_intensity_alpha (GImage *gimage, Layer *layer,
			 PixelArea *src, PixelArea *dest,
			 PixelArea *mask)
{
  if (! gimage->construct_flag)
    initial_area (src, dest, mask, NULL, layer->opacity,
		    layer->mode, gimage->visible, INITIAL_INTENSITY_ALPHA);
  else
    combine_areas (dest, src, dest, mask, NULL, layer->opacity,
		     layer->mode, gimage->visible, COMBINE_INTEN_A_INTEN_A);
}


static void
project_indexed (GImage *gimage, Layer *layer,
		 PixelArea *src, PixelArea *dest)
{
  if (! gimage->construct_flag)
    initial_area (src, dest, NULL, gimage->cmap, layer->opacity,
		    layer->mode, gimage->visible, INITIAL_INDEXED);
  else
    g_message ("Unable to project indexed image.");
}


static void
project_indexed_alpha (GImage *gimage, Layer *layer,
		       PixelArea *src, PixelArea *dest,
		       PixelArea *mask)
{
  if (! gimage->construct_flag)
    initial_area (src, dest, mask, gimage->cmap, layer->opacity,
		    layer->mode, gimage->visible, INITIAL_INDEXED_ALPHA);
  else
    combine_areas (dest, src, dest, mask, gimage->cmap, layer->opacity,
		     layer->mode, gimage->visible, COMBINE_INTEN_A_INDEXED_A);
}


static void
project_channel (GImage *gimage, Channel *channel,
		 PixelArea *src, PixelArea *src2)
{
  int type;
  if (! gimage->construct_flag)
    {
      type = (channel->show_masked) ?
	INITIAL_CHANNEL_MASK : INITIAL_CHANNEL_SELECTION;
      initial_area (src2, src, NULL, (unsigned char *)&channel->col, channel->opacity,
		      NORMAL, NULL, type);
    }
  else
    {
      type = (channel->show_masked) ?
	COMBINE_INTEN_A_CHANNEL_MASK : COMBINE_INTEN_A_CHANNEL_SELECTION;
      combine_areas (src, src2, src, NULL, (unsigned char*)&channel->col, channel->opacity,
		       NORMAL, NULL, type);
    }
}


/************************************************************/
/*  Layer/Channel functions                                 */
/************************************************************/


static void
gimage_free_layers (GImage *gimage)
{
  GSList *list = gimage->layers;
  Layer * layer;

  while (list)
    {
      layer = (Layer *) list->data;
      layer_delete (layer);
      list = g_slist_next (list);
    }
  g_slist_free (gimage->layers);
  g_slist_free (gimage->layer_stack);
}


static void
gimage_free_channels (GImage *gimage)
{
  GSList *list = gimage->channels;
  Channel * channel;

  while (list)
    {
      channel = (Channel *) list->data;
      channel_delete (channel);
      list = g_slist_next (list);
    }
  g_slist_free (gimage->channels);
}


static void
gimage_construct_layers (GImage *gimage, int x, int y, int w, int h)
{
  Layer * layer;
  int x1, y1, x2, y2;
  PixelArea src1PR, src2PR, maskPR;
  PixelArea * mask;
  GSList *list = gimage->layers;
  GSList *reverse_list = NULL;
  int off_x, off_y;

  /*  composite the floating selection if it exists  */
  if ((layer = gimage_floating_sel (gimage)))
    floating_sel_composite (layer, x, y, w, h, FALSE);

  while (list)
    {
      layer = (Layer *) list->data;

      /*  only add layers that are visible and not floating selections to the list  */
      if (!layer_is_floating_sel (layer) && drawable_visible (GIMP_DRAWABLE(layer)))
	reverse_list = g_slist_prepend (reverse_list, layer);

      list = g_slist_next (list);
    }

  while (reverse_list)
    {
      layer = (Layer *) reverse_list->data;
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);

      x1 = BOUNDS (off_x, x, x + w);
      y1 = BOUNDS (off_y, y, y + h);
      x2 = BOUNDS (off_x + drawable_width (GIMP_DRAWABLE(layer)), x, x + w);
      y2 = BOUNDS (off_y + drawable_height (GIMP_DRAWABLE(layer)), y, y + h);

      /* configure the pixel regions  */
      pixelarea_init (&src1PR, gimage_projection (gimage), 
                      x1, y1,
                      (x2 - x1), (y2 - y1),
                      TRUE);

      /*  If we're showing the layer mask instead of the layer...  */
      if (layer->mask && layer->show_mask)
	{
	  pixelarea_init (&src2PR, drawable_data (GIMP_DRAWABLE(layer->mask)),
                          (x1 - off_x), (y1 - off_y),
                          (x2 - x1), (y2 - y1),
                          FALSE);

	  copy_gray_to_area (&src2PR, &src1PR);
	}
      /*  Otherwise, normal  */
      else
	{
	  pixelarea_init (&src2PR, drawable_data (GIMP_DRAWABLE(layer)),
                          (x1 - off_x), (y1 - off_y),
                          (x2 - x1), (y2 - y1),
                          FALSE);

	  if (layer->mask && layer->apply_mask)
	    {
	      pixelarea_init (&maskPR, drawable_data (GIMP_DRAWABLE(layer->mask)),
                              (x1 - off_x), (y1 - off_y),
                              (x2 - x1), (y2 - y1),
                              FALSE);
	      mask = &maskPR;
	    }
	  else
	    mask = NULL;

	  /*  Based on the type of the layer, project the layer onto the
	   *  projection image...
	   */
          {
            Tag t = drawable_tag (GIMP_DRAWABLE(layer));
            
            switch (tag_format (t))
              {
              case FORMAT_RGB:
              case FORMAT_GRAY:
                if (tag_alpha (t) == ALPHA_NO)
                  project_intensity (gimage, layer, &src2PR, &src1PR, mask);
                else
                  project_intensity_alpha (gimage, layer, &src2PR, &src1PR, mask);
                break;
                
              case FORMAT_INDEXED:
                if (tag_alpha (t) == ALPHA_NO)
                  project_indexed (gimage, layer, &src2PR, &src1PR);
                else
                  project_indexed_alpha (gimage, layer, &src2PR, &src1PR, mask);
                break;
                
              case FORMAT_NONE:
              default:
                break;
              }
          }
        }

      gimage->construct_flag = 1;  /*  something was projected  */

      reverse_list = g_slist_next (reverse_list);
    }

  g_slist_free (reverse_list);
}


static void
gimage_construct_channels (GImage *gimage, int x, int y, int w, int h)
{
  Channel * channel;
  PixelArea src1PR, src2PR;
  GSList *list = gimage->channels;
  GSList *reverse_list = NULL;

  /*  reverse the channel list  */
  while (list)
    {
      reverse_list = g_slist_prepend (reverse_list, list->data);
      list = g_slist_next (list);
    }

  while (reverse_list)
    {
      channel = (Channel *) reverse_list->data;

      if (drawable_visible (GIMP_DRAWABLE(channel)))
	{
	  /* configure the pixel regions  */
	  pixelarea_init (&src1PR, gimage_projection (gimage), 
		x, y, w, h, TRUE);
	  pixelarea_init (&src2PR, drawable_data (GIMP_DRAWABLE(channel)), 
		x, y, w, h, FALSE);
	  project_channel (gimage, channel, &src1PR, &src2PR);

	  gimage->construct_flag = 1;
	}

      reverse_list = g_slist_next (reverse_list);
    }

  g_slist_free (reverse_list);
}


static void
gimage_initialize_projection (GImage *gimage, int x, int y, int w, int h)
{
  /*  this function determines whether a visible layer
   *  provides complete coverage over the image.  If not,
   *  the projection is initialized to transparent
   */
  GSList * list = gimage->layers;

  while (list)
    {
      Layer * l = (Layer *) list->data;
      GimpDrawable * d = GIMP_DRAWABLE(l);
      int off_x, off_y;
      
      drawable_offsets (d, &off_x, &off_y);
      if (drawable_visible (d) &&
	  ! layer_has_alpha (l) &&
	  (off_x <= x) &&
	  (off_y <= y) &&
	  (off_x + drawable_width (d) >= x + w) &&
	  (off_y + drawable_height (d) >= y + h))
	return;

      list = g_slist_next (list);
    }

  {
    PixelArea PR;
    COLOR16_NEW (color, canvas_tag (gimage_projection (gimage)));
    
    COLOR16_INIT (color);
    palette_get_transparent (&color);
    pixelarea_init (&PR, gimage_projection (gimage), 
                    x, y,
                    w, h,
                    TRUE);
    color_area (&PR, &color);
  }
}


static void
gimage_get_active_channels (GImage *gimage, GimpDrawable *drawable, int *active)
{
  Layer * layer;
  int i;

  /*  first, blindly copy the gimage active channels  */
  for (i = 0; i < MAX_CHANNELS; i++)
    active[i] = gimage->active[i];

  /*  If the drawable is a channel (a saved selection, etc.)
   *  make sure that the alpha channel is not valid
   */
  if (drawable_channel (drawable) != NULL)
    active[ALPHA_G_PIX] = 0;  /*  no alpha values in channels  */
  else
    {
      /*  otherwise, check whether preserve transparency is
       *  enabled in the layer and if the layer has alpha
       */
      if ((layer = drawable_layer (drawable)))
	if (layer_has_alpha (layer) && layer->preserve_trans)
	  active[ tag_num_channels (drawable_tag (GIMP_DRAWABLE(layer))) - 1] = 0;
    }
}


void
gimage_construct (GImage *gimage, int x, int y, int w, int h)
{
  gimage->construct_flag = 0;
  gimage_initialize_projection (gimage, x, y, w, h);
  gimage_construct_layers (gimage, x, y, w, h);
  gimage_construct_channels (gimage, x, y, w, h);
}

void
gimage_invalidate (GImage *gimage, int x, int y, int w, int h, int x1, int y1,
		   int x2, int y2)
{
  Canvas * c = gimage_projection (gimage);
  PixelArea a;
  void * pag;

  pixelarea_init (&a, c,
                  x, y,
                  w, h,
                  TRUE);

  for (pag = pixelarea_register_noref (1, &a);
       pag;
       pag = pixelarea_process (pag))
    {
      canvas_portion_unalloc (c,
                              pixelarea_x (&a),
                              pixelarea_y (&a));
    }
}

static guint 
gimage_validate  (
                  Canvas * c,
                  int x,
                  int y,
                  void * data
                  )
{
  int w = canvas_portion_width (c, x, y);
  int h = canvas_portion_height (c, x, y);
  
  gimage_construct ((GImage *) data, x, y, w, h);

  return TRUE;
}

int
gimage_get_layer_index (GImage *gimage, Layer *layer_arg)
{
  Layer *layer;
  GSList *layers = gimage->layers;
  int index = 0;

  while (layers)
    {
      layer = (Layer *) layers->data;
      if (layer == layer_arg)
	return index;

      index++;
      layers = g_slist_next (layers);
    }

  return -1;
}

int
gimage_get_channel_index (GImage *gimage, Channel *channel_ID)
{
  Channel *channel;
  GSList *channels = gimage->channels;
  int index = 0;

  while (channels)
    {
      channel = (Channel *) channels->data;
      if (channel == channel_ID)
	return index;

      index++;
      channels = g_slist_next (channels);
    }

  return -1;
}


Layer *
gimage_get_active_layer (GImage *gimage)
{
  return gimage->active_layer;
}


Channel *
gimage_get_active_channel (GImage *gimage)
{
  return gimage->active_channel;
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
  if ((layer = gimage->active_layer))
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
gimage_set_active_layer (GImage *gimage, Layer * layer)
{

  /*  First, find the layer in the gimage
   *  If it isn't valid, find the first layer that is
   */
  if (gimage_get_layer_index (gimage, layer) == -1)
    {
      if (! gimage->layers)
	return NULL;
      layer = (Layer *) gimage->layers->data;
    }

  if (! layer)
    return NULL;

  /*  Configure the layer stack to reflect this change  */
  gimage->layer_stack = g_slist_remove (gimage->layer_stack, (void *) layer);
  gimage->layer_stack = g_slist_prepend (gimage->layer_stack, (void *) layer);

  /*  invalidate the selection boundary because of a layer modification  */
  layer_invalidate_boundary (layer);

  /*  Set the active layer  */
  gimage->active_layer = layer;
  gimage->active_channel = NULL;

  /*  return the layer  */
  return layer;
}


Channel *
gimage_set_active_channel (GImage *gimage, Channel * channel)
{

  /*  Not if there is a floating selection  */
  if (gimage_floating_sel (gimage))
    return NULL;

  /*  First, find the channel
   *  If it doesn't exist, find the first channel that does
   */
  if (! channel) 
    {
      if (! gimage->channels)
	{
	  gimage->active_channel = NULL;
	  return NULL;
	}
      channel = (Channel *) gimage->channels->data;
    }

  /*  Set the active channel  */
  gimage->active_channel = channel;

  /*  return the channel  */
  return channel;
}


Channel *
gimage_unset_active_channel (GImage *gimage)
{
  Channel *channel;

  /*  make sure there is an active channel  */
  if (! (channel = gimage->active_channel))
    return NULL;

  /*  Set the active channel  */
  gimage->active_channel = NULL;

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
  GSList *list;

  list = gimage->layers;
  while (list)
    {
      layer = (Layer *) list->data;
      if (layer_pick_correlate (layer, x, y))
	return layer;

      list = g_slist_next (list);
    }

  return NULL;
}


void
gimage_set_layer_mask_apply (GImage *gimage, int layer_id)
{
  Layer *layer;
  int off_x, off_y;

  /*  find the layer  */
  if (! (layer = layer_get_ID (layer_id)))
    return;
  if (! layer->mask)
    return;

  layer->apply_mask = ! layer->apply_mask;
  drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
  gdisplays_update_area (gimage->ID, off_x, off_y,
			 drawable_width (GIMP_DRAWABLE(layer)), 
			 drawable_height (GIMP_DRAWABLE(layer)));
}


void
gimage_set_layer_mask_edit (GImage *gimage, Layer * layer, int edit)
{
  /*  find the layer  */
  if (!layer)
    return;

  if (layer->mask)
    layer->edit_mask = edit;
}


void
gimage_set_layer_mask_show (GImage *gimage, int layer_id)
{
  Layer *layer;
  int off_x, off_y;

  /*  find the layer  */
  if (! (layer = layer_get_ID (layer_id)))
    return;
  if (! layer->mask)
    return;

  layer->show_mask = ! layer->show_mask;
  drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
  gdisplays_update_area (gimage->ID, off_x, off_y,
			 drawable_width (GIMP_DRAWABLE(layer)), drawable_height (GIMP_DRAWABLE(layer)));
}


Layer *
gimage_raise_layer (GImage *gimage, Layer *layer_arg)
{
  Layer *layer;
  Layer *prev_layer;
  GSList *list;
  GSList *prev;
  int x1, y1, x2, y2;
  int index = -1;
  int off_x, off_y;
  int off2_x, off2_y;

  list = gimage->layers;
  prev = NULL; prev_layer = NULL;

  while (list)
    {
      layer = (Layer *) list->data;
      if (prev)
	prev_layer = (Layer *) prev->data;

      if (layer == layer_arg)
	{
	  /*  We can only raise a layer if it has an alpha channel &&
	   *  If it's not already the top layer
	   */
	  if (prev && layer_has_alpha (layer) && layer_has_alpha (prev_layer))
	    {
	      list->data = prev_layer;
	      prev->data = layer;
	      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
	      drawable_offsets (GIMP_DRAWABLE(prev_layer), &off2_x, &off2_y);

	      /*  calculate minimum area to update  */
	      x1 = MAXIMUM (off_x, off2_x);
	      y1 = MAXIMUM (off_y, off2_y);
	      x2 = MINIMUM (off_x + drawable_width (GIMP_DRAWABLE(layer)),
			    off2_x + drawable_width (GIMP_DRAWABLE(prev_layer)));
	      y2 = MINIMUM (off_y + drawable_height (GIMP_DRAWABLE(layer)),
			    off2_y + drawable_height (GIMP_DRAWABLE(prev_layer)));
	      if ((x2 - x1) > 0 && (y2 - y1) > 0)
		gdisplays_update_area (gimage->ID, x1, y1, (x2 - x1), (y2 - y1));

	      /*  invalidate the composite preview  */
	      gimage_invalidate_preview (gimage);

	      return prev_layer;
	    }
	  else
	    {
	      g_message ("Layer cannot be raised any further");
	      return NULL;
	    }
	}

      prev = list;
      index++;
      list = g_slist_next (list);
    }

  return NULL;
}


Layer *
gimage_lower_layer (GImage *gimage, Layer *layer_arg)
{
  Layer *layer;
  Layer *next_layer;
  GSList *list;
  GSList *next;
  int x1, y1, x2, y2;
  int index = 0;
  int off_x, off_y;
  int off2_x, off2_y;

  next_layer = NULL;

  list = gimage->layers;

  while (list)
    {
      layer = (Layer *) list->data;
      next = g_slist_next (list);

      if (next)
	next_layer = (Layer *) next->data;
      index++;

      if (layer == layer_arg)
	{
	  /*  We can only lower a layer if it has an alpha channel &&
	   *  The layer beneath it has an alpha channel &&
	   *  If it's not already the bottom layer
	   */
	  if (next && layer_has_alpha (layer) && layer_has_alpha (next_layer))
	    {
	      list->data = next_layer;
	      next->data = layer;
	      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
	      drawable_offsets (GIMP_DRAWABLE(next_layer), &off2_x, &off2_y);

	      /*  calculate minimum area to update  */
	      x1 = MAXIMUM (off_x, off2_x);
	      y1 = MAXIMUM (off_y, off2_y);
	      x2 = MINIMUM (off_x + drawable_width (GIMP_DRAWABLE(layer)),
			    off2_x + drawable_width (GIMP_DRAWABLE(next_layer)));
	      y2 = MINIMUM (off_y + drawable_height (GIMP_DRAWABLE(layer)),
			    off2_y + drawable_height (GIMP_DRAWABLE(next_layer)));
	      if ((x2 - x1) > 0 && (y2 - y1) > 0)
		gdisplays_update_area (gimage->ID, x1, y1, (x2 - x1), (y2 - y1));

	      /*  invalidate the composite preview  */
	      gimage_invalidate_preview (gimage);

	      return next_layer;
	    }
	  else
	    {
	      g_message ("Layer cannot be lowered any further");
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
  GSList *layer_list;
  GSList *merge_list = NULL;
  Layer *layer;

  layer_list = gimage->layers;
  while (layer_list)
    {
      layer = (Layer *) layer_list->data;
      if (drawable_visible (GIMP_DRAWABLE(layer)))
	merge_list = g_slist_append (merge_list, layer);

      layer_list = g_slist_next (layer_list);
    }

  if (merge_list && merge_list->next)
    {
      layer = gimage_merge_layers (gimage, merge_list, merge_type);
      g_slist_free (merge_list);
      return layer;
    }
  else
    {
      g_message ("There are not enough visible layers for a merge.\nThere must be at least two.");
      g_slist_free (merge_list);
      return NULL;
    }
}


Layer *
gimage_flatten (GImage *gimage)
{
  GSList *layer_list;
  GSList *merge_list = NULL;
  Layer *layer;

  layer_list = gimage->layers;
  while (layer_list)
    {
      layer = (Layer *) layer_list->data;
      if (drawable_visible (GIMP_DRAWABLE(layer)))
	merge_list = g_slist_append (merge_list, layer);

      layer_list = g_slist_next (layer_list);
    }

  layer = gimage_merge_layers (gimage, merge_list, FlattenImage);
  g_slist_free (merge_list);
  return layer;
}


Layer *
gimage_merge_layers (GImage *gimage, GSList *merge_list, MergeType merge_type)
{
  GSList *reverse_list = NULL;
  PixelArea src1PR, src2PR, maskPR;
  PixelArea * mask;
  Layer *merge_layer;
  Tag layer_tag, merge_layer_tag;
  Layer *layer = NULL;
  Layer *bottom;
  int count;
  int x1, y1, x2, y2;
  int x3, y3, x4, y4;
  int operation;
  int position;
  int active[MAX_CHANNELS] = {1, 1, 1, 1};
  int off_x, off_y;

  layer = NULL;
  x1 = y1 = x2 = y2 = 0;
  bottom = NULL;

  /*  Get the layer extents  */
  count = 0;
  while (merge_list)
    {
      layer = (Layer *) merge_list->data;
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);

      switch (merge_type)
	{
	case ExpandAsNecessary:
	case ClipToImage:
	  if (!count)
	    {
	      x1 = off_x;
	      y1 = off_y;
	      x2 = off_x + drawable_width (GIMP_DRAWABLE(layer));
	      y2 = off_y + drawable_height (GIMP_DRAWABLE(layer));
	    }
	  else
	    {
	      if (off_x < x1)
		x1 = off_x;
	      if (off_y < y1)
		y1 = off_y;
	      if ((off_x + drawable_width (GIMP_DRAWABLE(layer))) > x2)
		x2 = (off_x + drawable_width (GIMP_DRAWABLE(layer)));
	      if ((off_y + drawable_height (GIMP_DRAWABLE(layer))) > y2)
		y2 = (off_y + drawable_height (GIMP_DRAWABLE(layer)));
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
	      x1 = off_x;
	      y1 = off_y;
	      x2 = off_x + drawable_width (GIMP_DRAWABLE(layer));
	      y2 = off_y + drawable_height (GIMP_DRAWABLE(layer));
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
      reverse_list = g_slist_prepend (reverse_list, layer);
      merge_list = g_slist_next (merge_list);
    }

  if ((x2 - x1) == 0 || (y2 - y1) == 0)
    return NULL;

  /*  Start a merge undo group  */
  undo_push_group_start (gimage, LAYER_MERGE_UNDO);

  if (merge_type == FlattenImage ||
      tag_format (drawable_tag (GIMP_DRAWABLE (layer))) == FORMAT_INDEXED)
    {
      merge_layer_tag = gimage_tag (gimage);
      merge_layer = layer_new (gimage->ID, gimage->width, gimage->height,
                               merge_layer_tag, STORAGE_TILED,
                               drawable_name (GIMP_DRAWABLE(layer)), 
                               OPAQUE_OPACITY, NORMAL_MODE);

      if (!merge_layer) {
	g_message ("gimage_merge_layers: could not allocate merge layer");
	return NULL;
      }

      /*  init the pixel region  */
      pixelarea_init (&src1PR, drawable_data (GIMP_DRAWABLE(merge_layer)),
                      0, 0, gimage->width, gimage->height, TRUE);

      /*  set the region to the background color  */
      {
        COLOR16_NEW (bg_color, drawable_tag(GIMP_DRAWABLE(merge_layer)) );
        COLOR16_INIT (bg_color);
        palette_get_background (&bg_color);
        color_area (&src1PR, &bg_color);
      }
      
      position = 0;
    }
  else
    {
      /*  The final merged layer inherits the attributes of the bottomost layer,
       *  with a notable exception:  The resulting layer has an alpha channel
       *  whether or not the original did
       */

      layer_tag = drawable_tag (GIMP_DRAWABLE(layer));
      merge_layer_tag = tag_set_alpha (layer_tag, ALPHA_YES);

      merge_layer = layer_new (gimage->ID, (x2 - x1), (y2 - y1),
                               merge_layer_tag, STORAGE_TILED,
                               drawable_name (GIMP_DRAWABLE(layer)),
                               layer->opacity, layer->mode);
      
      if (!merge_layer) {
	g_message ("gimage_merge_layers: could not allocate merge layer");
	return NULL;
      }

      GIMP_DRAWABLE(merge_layer)->offset_x = x1;
      GIMP_DRAWABLE(merge_layer)->offset_y = y1;

      /*  Set the layer to transparent  */
      pixelarea_init (&src1PR, drawable_data (GIMP_DRAWABLE(merge_layer)), 
                      0, 0, (x2 - x1), (y2 - y1), TRUE);

      /*  set the region to 0's  */
      {
        COLOR16_NEW (bg_color, drawable_tag(GIMP_DRAWABLE(merge_layer)) );
        COLOR16_INIT (bg_color);
        palette_get_transparent (&bg_color);
        color_area (&src1PR, &bg_color);
      }
      
      /*  Find the index in the layer list of the bottom layer--we need this
       *  in order to add the final, merged layer to the layer list correctly
       */
      layer = (Layer *) reverse_list->data;
      position = g_slist_length (gimage->layers) - gimage_get_layer_index (gimage, layer);
      
      /* set the mode of the bottom layer to normal so that the contents
       *  aren't lost when merging with the all-alpha merge_layer
       *  Keep a pointer to it so that we can set the mode right after it's been
       *  merged so that undo works correctly.
       */
      layer -> mode =NORMAL;
      bottom = layer;

    }

  while (reverse_list)
    {
      layer = (Layer *) reverse_list->data;

      /*  determine what sort of operation is being attempted and
       *  if it's actually legal...
       */
      operation = get_operation (drawable_tag (GIMP_DRAWABLE(merge_layer)),
                                 drawable_tag (GIMP_DRAWABLE(layer)));
      
      if (operation == -1)
	{
	  g_message ("gimage_merge_layers attempting to merge incompatible layers\n");
	  return NULL;
	}

      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      x3 = BOUNDS (off_x, x1, x2);
      y3 = BOUNDS (off_y, y1, y2);
      x4 = BOUNDS (off_x + drawable_width (GIMP_DRAWABLE(layer)), x1, x2);
      y4 = BOUNDS (off_y + drawable_height (GIMP_DRAWABLE(layer)), y1, y2);

      /* configure the pixel regions  */
      pixelarea_init (&src1PR, drawable_data (GIMP_DRAWABLE(merge_layer)), 
		(x3 - x1), (y3 - y1), (x4 - x3), (y4 - y3), TRUE);
      pixelarea_init (&src2PR, drawable_data (GIMP_DRAWABLE(layer)),
		(x3 - off_x), (y3 - off_y), (x4 - x3), (y4 - y3), FALSE);

      if (layer->mask)
	{
	  pixelarea_init (&maskPR, drawable_data (GIMP_DRAWABLE(layer->mask)), 
			(x3 - off_x), (y3 - off_y), (x4 - x3), (y4 - y3), FALSE);
	  mask = &maskPR;
	}
      else
	mask = NULL;

      combine_areas (&src1PR, &src2PR, &src1PR, mask, NULL,
		       layer->opacity, layer->mode, active, operation);

      gimage_remove_layer (gimage, layer);
      reverse_list = g_slist_next (reverse_list);
    }

  /* Save old mode in undo */
  if (bottom)
    bottom -> mode = merge_layer -> mode;

  g_slist_free (reverse_list);

  /*  if the type is flatten, remove all the remaining layers  */
  if (merge_type == FlattenImage)
    {
      merge_list = gimage->layers;
      while (merge_list)
	{
	  layer = (Layer *) merge_list->data;
	  merge_list = g_slist_next (merge_list);
	  gimage_remove_layer (gimage, layer);
	}

      gimage_add_layer (gimage, merge_layer, position);
    }
  else
    {
      /*  Add the layer to the gimage  */
      gimage_add_layer (gimage, merge_layer, (g_slist_length (gimage->layers) - position + 1));
    }

  /*  End the merge undo group  */
  undo_push_group_end (gimage);

  /*  Update the gimage  */
  GIMP_DRAWABLE(merge_layer)->visible = TRUE;

  /*  update gdisplay titles to reflect the possibility of
   *  this layer being the only layer in the gimage
   */
  gdisplays_update_title (gimage->ID);

  drawable_update (GIMP_DRAWABLE(merge_layer),
                   0, 0,
                   0, 0);

  return merge_layer;
}


Layer *
gimage_add_layer (GImage *gimage, Layer *float_layer, int position)
{
  LayerUndo * lu;

  if (GIMP_DRAWABLE(float_layer)->gimage_ID != 0 && 
      GIMP_DRAWABLE(float_layer)->gimage_ID != gimage->ID) 
    {
      g_message ("gimage_add_layer: attempt to add layer to wrong image");
      return NULL;
    }

  {
    GSList *ll = gimage->layers;
    while (ll) 
      {
	if (ll->data == float_layer) 
	  {
	    g_message ("gimage_add_layer: trying to add layer to image twice");
	    return NULL;
	  }
	ll = g_slist_next(ll);
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
    gimage->floating_sel = float_layer;

  /*  let the layer know about the gimage  */
  GIMP_DRAWABLE(float_layer)->gimage_ID = gimage->ID;

  /*  add the layer to the list at the specified position  */
  if (position == -1)
    position = gimage_get_layer_index (gimage, gimage->active_layer);
  if (position != -1)
    {
      /*  If there is a floating selection (and this isn't it!),
       *  make sure the insert position is greater than 0
       */
      if (gimage_floating_sel (gimage) && (gimage->floating_sel != float_layer) && position == 0)
	position = 1;
      gimage->layers = g_slist_insert (gimage->layers, layer_ref (float_layer), position);
    }
  else
    gimage->layers = g_slist_prepend (gimage->layers, layer_ref (float_layer));
  gimage->layer_stack = g_slist_prepend (gimage->layer_stack, float_layer);

  /*  notify the layers dialog of the currently active layer  */
  gimage_set_active_layer (gimage, float_layer);

  /*  update the new layer's area  */
  drawable_update (GIMP_DRAWABLE(float_layer), 
                   0, 0,
                   0, 0);

  /*  invalidate the composite preview  */
  gimage_invalidate_preview (gimage);

  return float_layer;
}


Layer *
gimage_remove_layer (GImage *gimage, Layer * layer)
{
  LayerUndo *lu;
  int off_x, off_y;

  if (layer)
    {
      /*  Prepare a layer undo--push it at the end  */
      lu = (LayerUndo *) g_malloc (sizeof (LayerUndo));
      lu->layer = layer;
      lu->prev_position = gimage_get_layer_index (gimage, layer);
      lu->prev_layer = layer;
      lu->undo_type = 1;

      gimage->layers = g_slist_remove (gimage->layers, layer);
      gimage->layer_stack = g_slist_remove (gimage->layer_stack, layer);

      /*  If this was the floating selection, reset the fs pointer  */
      if (gimage->floating_sel == layer)
	{
	  gimage->floating_sel = NULL;

	  floating_sel_reset (layer);
	}
      if (gimage->active_layer == layer)
	{
	  if (gimage->layers)
	    gimage->active_layer = (Layer *) gimage->layer_stack->data;
	  else
	    gimage->active_layer = NULL;
	}

      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      gdisplays_update_area (gimage->ID, off_x, off_y,
			     drawable_width (GIMP_DRAWABLE(layer)), drawable_height (GIMP_DRAWABLE(layer)));

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


LayerMask *
gimage_add_layer_mask (GImage *gimage, Layer *layer, LayerMask *mask)
{
  LayerMaskUndo *lmu;
  char *error = NULL;;

  if (layer->mask != NULL)
    error = "Unable to add a layer mask since\nthe layer already has one.";
  if (drawable_indexed (GIMP_DRAWABLE(layer)))
    error = "Unable to add a layer mask to a\nlayer in an indexed image.";
  if (! layer_has_alpha (layer))
    error = "Cannot add layer mask to a layer\nwith no alpha channel.";
  if (drawable_width (GIMP_DRAWABLE(layer)) != drawable_width (GIMP_DRAWABLE(mask)) || drawable_height (GIMP_DRAWABLE(layer)) != drawable_height (GIMP_DRAWABLE(mask)))
    error = "Cannot add layer mask of different dimensions than specified layer.";

  if (error)
    {
      g_message (error);
      return NULL;
    }

  layer_add_mask (layer, mask);

  /*  Prepare a layer undo and push it  */
  lmu = (LayerMaskUndo *) g_malloc (sizeof (LayerMaskUndo));
  lmu->layer = layer;
  lmu->mask = mask;
  lmu->undo_type = 0;
  lmu->apply_mask = layer->apply_mask;
  lmu->edit_mask = layer->edit_mask;
  lmu->show_mask = layer->show_mask;
  undo_push_layer_mask (gimage, lmu);

  gimage_set_layer_mask_edit (gimage, layer, layer->edit_mask);

  return mask;
}


Channel *
gimage_remove_layer_mask (GImage *gimage, Layer *layer, int mode)
{
  LayerMaskUndo *lmu;
  int off_x, off_y;

  if (! (layer) )
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
   *   mask is delete : NULL)d
   */
  undo_push_layer_mask (gimage, lmu);

  /*  end the undo group  */
  undo_push_group_end (gimage);

  /*  If the layer mode is discard, update the layer--invalidate gimage also  */
  if (mode == DISCARD)
    {
      gimage_invalidate_preview (gimage);

      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      gdisplays_update_area (gimage->ID, off_x, off_y,
			     drawable_width (GIMP_DRAWABLE(layer)), drawable_height (GIMP_DRAWABLE(layer)));
    }

  drawable_update (GIMP_DRAWABLE(layer),
                   0, 0,
                   0, 0);

  gdisplays_flush ();

  return NULL;
}


Channel *
gimage_raise_channel (GImage *gimage, Channel * channel_arg)
{
  Channel *channel;
  Channel *prev_channel;
  GSList *list;
  GSList *prev;
  int index = -1;

  list = gimage->channels;
  prev = NULL;
  prev_channel = NULL;

  while (list)
    {
      channel = (Channel *) list->data;
      if (prev)
	prev_channel = (Channel *) prev->data;

      if (channel == channel_arg)
	{
	  if (prev)
	    {
	      list->data = prev_channel;
	      prev->data = channel;
	      drawable_update (GIMP_DRAWABLE(channel),
                               0, 0,
                               0, 0);
	      return prev_channel;
	    }
	  else
	    {
	      g_message ("Channel cannot be raised any further");
	      return NULL;
	    }
	}

      prev = list;
      index++;
      list = g_slist_next (list);
    }

  return NULL;
}


Channel *
gimage_lower_channel (GImage *gimage, Channel *channel_arg)
{
  Channel *channel;
  Channel *next_channel;
  GSList *list;
  GSList *next;
  int index = 0;

  list = gimage->channels;
  next_channel = NULL;

  while (list)
    {
      channel = (Channel *) list->data;
      next = g_slist_next (list);

      if (next)
	next_channel = (Channel *) next->data;
      index++;

      if (channel == channel_arg)
	{
	  if (next)
	    {
	      list->data = next_channel;
	      next->data = channel;
	      drawable_update (GIMP_DRAWABLE(channel),
                               0, 0,
                               0, 0);

	      return next_channel;
	    }
	  else
	    {
	      g_message ("Channel cannot be lowered any further");
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

  if (GIMP_DRAWABLE(channel)->gimage_ID != 0 &&
      GIMP_DRAWABLE(channel)->gimage_ID != gimage->ID)
    {
      g_message ("gimage_add_channel: attempt to add channel to wrong image");
      return NULL;
    }

  {
    GSList *cc = gimage->channels;
    while (cc) 
      {
	if (cc->data == channel) 
	  {
	    g_message ("gimage_add_channel: trying to add channel to image twice");
	    return NULL;
	  }
	cc = g_slist_next (cc);
      }
  }  


  /*  Prepare a channel undo and push it  */
  cu = (ChannelUndo *) g_malloc (sizeof (ChannelUndo));
  cu->channel = channel;
  cu->prev_position = 0;
  cu->prev_channel = gimage->active_channel;
  cu->undo_type = 0;
  undo_push_channel (gimage, cu);

  /*  add the channel to the list  */
  gimage->channels = g_slist_prepend (gimage->channels, channel_ref (channel));

  /*  notify this gimage of the currently active channel  */
  gimage_set_active_channel (gimage, channel);

  /*  if channel is visible, update the image  */
  if (drawable_visible (GIMP_DRAWABLE(channel)))
    drawable_update (GIMP_DRAWABLE(channel),
                     0, 0,
                     0, 0);

  return channel;
}


Channel *
gimage_remove_channel (GImage *gimage, Channel *channel)
{
  ChannelUndo * cu;

  if (channel)
    {
      /*  Prepare a channel undo--push it below  */
      cu = (ChannelUndo *) g_malloc (sizeof (ChannelUndo));
      cu->channel = channel;
      cu->prev_position = gimage_get_channel_index (gimage, channel);
      cu->prev_channel = gimage->active_channel;
      cu->undo_type = 1;

      gimage->channels = g_slist_remove (gimage->channels, channel);

      if (gimage->active_channel == channel)
	{
	  if (gimage->channels)
	    gimage->active_channel = (((Channel *) gimage->channels->data));
	  else
	    gimage->active_channel = NULL;
	}

      if (drawable_visible (GIMP_DRAWABLE(channel)))
	drawable_update (GIMP_DRAWABLE(channel),
                         0, 0,
                         0, 0);

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
gimage_is_empty (GImage *gimage)
{
  return (! gimage->layers);
}

GimpDrawable *
gimage_active_drawable (GImage *gimage)
{
  Layer *layer;

  /*  If there is an active channel (a saved selection, etc.),
   *  we ignore the active layer
   */
  if (gimage->active_channel != NULL)
    return GIMP_DRAWABLE (gimage->active_channel);
  else if (gimage->active_layer != NULL)
    {
      layer = gimage->active_layer;
      if (layer->mask && layer->edit_mask)
	return GIMP_DRAWABLE(layer->mask);
      else
	return GIMP_DRAWABLE(layer);
    }
  else
    return NULL;
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
  GDisplay *gdisp;
  
  if (gimage->dirty < 0)
    gimage->dirty = 2;
  else
    gimage->dirty ++;
  if (active_tool && !active_tool->preserve) {
    gdisp = active_tool->gdisp_ptr;
    if (gdisp) {
      if (gdisp->gimage->ID == gimage->ID)
        tools_initialize (active_tool->type, gdisp);
      else
        tools_initialize (active_tool->type, NULL);
    }
  }
  
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
  if (gimage->floating_sel == NULL)
    return NULL;
  else return gimage->floating_sel;
}

unsigned char *
gimage_cmap (GImage *gimage)
{
  return drawable_cmap (gimage_active_drawable (gimage));
}


/************************************************************/
/*  Projection access functions                             */
/************************************************************/

Canvas *
gimage_projection (GImage *gimage)
{
  if (( canvas_width (gimage->projection) != gimage->width) ||
      ( canvas_height (gimage->projection)!= gimage->height))
    gimage_allocate_projection (gimage);
  
  return gimage->projection;
}

int
gimage_projection_opacity (GImage *gimage)
{
  return OPAQUE_OPACITY;
}

void
gimage_projection_realloc (GImage *gimage)
{
  gimage_allocate_projection (gimage);
}

/************************************************************/
/*  Composition access functions                            */
/************************************************************/

static Canvas *
gimage_construct_composite_preview (GImage *gimage, int width, int height)
{
  Layer * layer;
  PixelArea src1PR, src2PR, maskPR;
  PixelArea * mask;
  Canvas *comp;
  Canvas *layer_buf;
  Canvas *mask_buf;
  GSList *reverse_list = NULL;
  double ratio;
  int x, y, w, h;
  int x1, y1, x2, y2;
  int construct_flag;
  int visible[MAX_CHANNELS] = {1, 1, 1, 1};
  int off_x, off_y;
  Precision p;
  Tag t;
  Format f;
  
  ratio = (double) width / (double) gimage->width;

  /*  The construction buffer  */
  p = tag_precision (gimage_tag (gimage));
  f = tag_format (gimage_tag (gimage));

  comp = canvas_new (tag_new (p, f, ALPHA_YES),
                     width, height,
                     STORAGE_FLAT);
  /* the list of layers to preview */
  {
    GSList *list = gimage->layers;

    while (list)
      {
        layer = (Layer *) list->data;
        
        /*  only add layers that are visible and not floating selections to the list  */
        if (!layer_is_floating_sel (layer) && drawable_visible (GIMP_DRAWABLE(layer)))
          reverse_list = g_slist_prepend (reverse_list, layer);
        
        list = g_slist_next (list);
      }
  }
  
  construct_flag = 0;

  while (reverse_list)
    {
      layer = (Layer *) reverse_list->data;

      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);

      x = (int) (ratio * off_x + 0.5);
      y = (int) (ratio * off_y + 0.5);
      w = (int) (ratio * drawable_width (GIMP_DRAWABLE(layer)) + 0.5);
      h = (int) (ratio * drawable_height (GIMP_DRAWABLE(layer)) + 0.5);

      x1 = BOUNDS (x, 0, width);
      y1 = BOUNDS (y, 0, height);
      x2 = BOUNDS (x + w, 0, width);
      y2 = BOUNDS (y + h, 0, height);

      pixelarea_init (&src1PR, comp,
                      x1, y1,
                      (x2 - x1), (y2 - y1),
                      TRUE);
      
      layer_buf = layer_preview (layer, w, h);

      pixelarea_init (&src2PR, layer_buf,
                      x1, y1,
                      (x2 - x1), (y2 - y1),
                      FALSE);
      
      mask = NULL;
      if (layer->mask && layer->apply_mask)
	{
	  mask = &maskPR;

	  mask_buf = layer_mask_preview (layer, w, h);
          
          pixelarea_init (&maskPR, mask_buf,
                          x1, y1,
                          (x2 - x1), (y2 - y1),
                          FALSE);
	}

      /*  Based on the type of the layer, project the layer onto the
       *   composite preview...
       *  Indexed images are actually already converted to RGB and RGBA,
       *   so just project them as if they were type "intensity"
       *  Send in all TRUE for visible since that info doesn't matter for previews
       */
      
        switch (tag_alpha (drawable_tag (GIMP_DRAWABLE(layer))))
        {
        case ALPHA_NO:
	  if (! construct_flag)
	    initial_area (&src2PR, &src1PR, mask, NULL, layer->opacity,
                          layer->mode, visible, INITIAL_INTENSITY);
	  else
	    combine_areas (&src1PR, &src2PR, &src1PR, mask, NULL, layer->opacity,
                           layer->mode, visible, COMBINE_INTEN_A_INTEN);
	  break;

	case ALPHA_YES:
	  if (! construct_flag)
	    initial_area (&src2PR, &src1PR, mask, NULL, layer->opacity,
                          layer->mode, visible, INITIAL_INTENSITY_ALPHA);
	  else
	    combine_areas (&src1PR, &src2PR, &src1PR, mask, NULL, layer->opacity,
                           layer->mode, visible, COMBINE_INTEN_A_INTEN_A);
	  break;

        case ALPHA_NONE:
	default:
	  break;
	}

      construct_flag = 1;

      reverse_list = g_slist_next (reverse_list);
    }

  g_slist_free (reverse_list);

  return comp;
}

Canvas *
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
      canvas_width (gimage->comp_preview) == width &&
      canvas_height (gimage->comp_preview) == height)
    return gimage->comp_preview;
  /*  The hard way  */
  else
    {
      if (gimage->comp_preview)
	canvas_delete (gimage->comp_preview);

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
  GSList *tmp = image_list;
  GImage *gimage;

  while (tmp)
    {
      gimage = (GImage *) tmp->data;
      gimage_invalidate_preview (gimage);
      tmp = g_slist_next (tmp);
    }
}

