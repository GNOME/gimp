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
#include <stdlib.h>
#include <string.h>
#include "appenv.h"
#include "by_color_select.h"
#include "channel.h"
#include "channels_dialog.h"
#include "drawable.h"
#include "errors.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gdisplay_ops.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "indexed_palette.h"
#include "layer.h"
#include "paint_core.h"
#include "paint_funcs.h"
#include "tools.h"
#include "transform_core.h"
#include "undo.h"

#include "drawable_pvt.h"
#include "layer_pvt.h"
#include "channel_pvt.h"
#include "tile_manager_pvt.h"
#include "tile_pvt.h"			/* ick. */



typedef int   (* UndoPopFunc)  (GImage *, int, int, void *);
typedef void  (* UndoFreeFunc) (int, void *);

typedef struct _undo Undo;

struct _undo
{
  int           type;           /*  undo type                           */
  void *        data;           /*  data to implement the undo          */
  long          bytes;          /*  size of undo item                   */

  UndoPopFunc   pop_func;       /*  function pointer to undo pop proc   */
  UndoFreeFunc  free_func;      /*  function with specifics for freeing */
};

/*  Pop functions  */

int      undo_pop_image            (GImage *, int, int, void *);
int      undo_pop_mask             (GImage *, int, int, void *);
int      undo_pop_layer_displace   (GImage *, int, int, void *);
int      undo_pop_transform        (GImage *, int, int, void *);
int      undo_pop_paint            (GImage *, int, int, void *);
int      undo_pop_layer            (GImage *, int, int, void *);
int      undo_pop_layer_mod        (GImage *, int, int, void *);
int      undo_pop_layer_mask       (GImage *, int, int, void *);
int      undo_pop_channel          (GImage *, int, int, void *);
int      undo_pop_channel_mod      (GImage *, int, int, void *);
int      undo_pop_fs_to_layer      (GImage *, int, int, void *);
int      undo_pop_fs_rigor         (GImage *, int, int, void *);
int      undo_pop_fs_relax         (GImage *, int, int, void *);
int      undo_pop_gimage_mod       (GImage *, int, int, void *);
int      undo_pop_guide            (GImage *, int, int, void *);

/*  Free functions  */

void     undo_free_image           (int, void *);
void     undo_free_mask            (int, void *);
void     undo_free_layer_displace  (int, void *);
void     undo_free_transform       (int, void *);
void     undo_free_paint           (int, void *);
void     undo_free_layer           (int, void *);
void     undo_free_layer_mod       (int, void *);
void     undo_free_layer_mask      (int, void *);
void     undo_free_channel         (int, void *);
void     undo_free_channel_mod     (int, void *);
void     undo_free_fs_to_layer     (int, void *);
void     undo_free_fs_rigor        (int, void *);
void     undo_free_fs_relax        (int, void *);
void     undo_free_gimage_mod      (int, void *);
void     undo_free_guide           (int, void *);


/*  Sizing functions  */
static int   layer_size                (Layer *);
static int   channel_size              (Channel *);

static int   group_count = 0;
static int   shrink_wrap = FALSE;

#define UNDO          0
#define REDO          1

static int
layer_size (Layer *layer)
{
  int size;

  size = sizeof (Layer) + GIMP_DRAWABLE(layer)->width * GIMP_DRAWABLE(layer)->height * GIMP_DRAWABLE(layer)->bytes +
    strlen (GIMP_DRAWABLE(layer)->name);

  if (layer_mask (layer))
    size += channel_size (GIMP_CHANNEL (layer_mask (layer)));

  return size;
}


static int
channel_size (Channel *channel)
{
  int size;

  size = sizeof (Channel) + GIMP_DRAWABLE(channel)->width * GIMP_DRAWABLE(channel)->height +
    strlen (GIMP_DRAWABLE(channel)->name);

  return size;
}


static void
undo_free_list (GImage   *gimage,
		int       state,
		GSList   *list)
{
  GSList * orig;
  Undo * undo;

  orig = list;

  while (list)
    {
      undo = (Undo *) list->data;
      if (undo)
	{
	  (* undo->free_func) (state, undo->data);
	  gimage->undo_bytes -= undo->bytes;
	  g_free (undo);
	}
      list = g_slist_next (list);
    }

  g_slist_free (orig);
}


static GSList *
remove_stack_bottom (GImage *gimage)
{
  GSList *list;
  GSList *last;
  int in_group = 0;

  list = gimage->undo_stack;

  last = NULL;
  while (list)
    {
      if (list->next == NULL)
	{
	  if (last)
	    undo_free_list (gimage, UNDO, last->next);
	  else
	    undo_free_list (gimage, UNDO, list);

	  gimage->undo_levels --;
	  if (last)
	    last->next = NULL;
	  list = NULL;
	}
      else
	{
	  if (list->data == NULL)
	    in_group = (in_group) ? 0 : 1;

	  /*  Make sure last points to only a single item, or the
	   *  tail of an aggregate undo item
	   */
	  if ((list->data && !in_group) ||
	      (!list->data && !in_group))
	  last = list;

	  list = g_slist_next (list);
	}
    }

  if (last)
    return gimage->undo_stack;
  else
    return NULL;

}


static int
undo_free_up_space (GImage *gimage)
{
  /*  If there are 0 levels of undo return FALSE.
   */
  if (levels_of_undo == 0)
    return FALSE;

  /*  Delete the item on the bottom of the stack if we have the maximum
   *  levels of undo already
   */
  while (gimage->undo_levels >= levels_of_undo)
    gimage->undo_stack = remove_stack_bottom (gimage);

  return TRUE;
}



static Undo *
undo_push (GImage *gimage,
	   long    size,
	   int     type)
{
  Undo * new;

  if (! gimage->undo_on)
    return NULL;

  size += sizeof (Undo);

  if (gimage->redo_stack)
    {
      undo_free_list (gimage, REDO, gimage->redo_stack);
      gimage->redo_stack = NULL;
    }

  if (!gimage->pushing_undo_group)
    if (! undo_free_up_space (gimage))
      return NULL;

  new = (Undo *) g_malloc (sizeof (Undo));

  new->bytes = size;
  gimage->undo_bytes += size;
  if (!gimage->pushing_undo_group)  /*  only increment levels if not in a group  */
    {
      new->type = type;
      gimage->undo_levels ++;
    }
  else
    new->type = gimage->pushing_undo_group;

  gimage->undo_stack = g_slist_prepend (gimage->undo_stack, (void *) new);

  return new;
}


static int
pop_stack (GImage  *gimage,
	   GSList **stack_ptr,
	   GSList **unstack_ptr,
	   int       state)
{
  Undo * object;
  GSList *stack;
  GSList *tmp;
  int status = 0;
  int in_group = 0;
  int x, y;
  GDisplay *gdisp;

  /*  Keep popping until we pop a valid object
   *  or get to the end of a group if we're in one
   */
  while (*stack_ptr)
    {
      stack = *stack_ptr;

      object = (Undo *) stack->data;
      if (object == NULL)
	{
	  in_group = (in_group) ? 0 : 1;
	  if (in_group)
	    gimage->undo_levels += (state == UNDO) ? -1 : 1;

	  if (status && !in_group)
	    status = 1;
	  else
	    status = 0;
	}
      else
	{
	  status = (* object->pop_func) (gimage, state, object->type, object->data);
	  if (!in_group)
	    gimage->undo_levels += (state == UNDO) ? -1 : 1;
	}

      *unstack_ptr = g_slist_prepend (*unstack_ptr, (void *) object);

      tmp = stack;
      *stack_ptr = g_slist_next (*stack_ptr);
      tmp->next = NULL;
      g_slist_free (tmp);

      if (status && !in_group)
	{
	  /*  Flush any image updates and displays  */
	  gdisp = gdisplay_active();
	  if (gdisp != NULL) {
	    if (gdisp->disp_xoffset || gdisp->disp_yoffset)
	      {
		gdk_window_get_size (gdisp->canvas->window, &x, &y);
		if (gdisp->disp_yoffset)
		  {
		    gdisplay_expose_area (gdisp, 0, 0, gdisp->disp_width,
					  gdisp->disp_yoffset);
		    gdisplay_expose_area (gdisp, 0, gdisp->disp_yoffset + y,
					  gdisp->disp_width, gdisp->disp_height);
		  }
		if (gdisp->disp_xoffset)
		  {
		    gdisplay_expose_area (gdisp, 0, 0, gdisp->disp_xoffset,
					  gdisp->disp_height);
		    gdisplay_expose_area (gdisp, gdisp->disp_xoffset + x, 0,
					  gdisp->disp_width, gdisp->disp_height);
		  }
	      }
	  }
	  gdisplays_flush ();

	  /*  If the shrink_wrap flag was set  */
	  if (shrink_wrap)
	    {
	      gdisplays_shrink_wrap (gimage);
	      shrink_wrap = FALSE;
	    }

	  return TRUE;
	}
    }

  return FALSE;
}


int
undo_pop (GImage *gimage)
{
  return pop_stack (gimage, &gimage->undo_stack, &gimage->redo_stack, UNDO);
}


int
undo_redo (GImage *gimage)
{
  return pop_stack (gimage, &gimage->redo_stack, &gimage->undo_stack, REDO);
}


void
undo_free (GImage *gimage)
{
  undo_free_list (gimage, UNDO, gimage->undo_stack);
  undo_free_list (gimage, REDO, gimage->redo_stack);
  gimage->undo_stack = NULL;
  gimage->redo_stack = NULL;
  gimage->undo_bytes = 0;
  gimage->undo_levels = 0;
}


/**********************/
/*  group Undo funcs  */

int
undo_push_group_start (GImage *gimage,
		       int     type)
{
  if (! gimage->undo_on)
    return FALSE;

  group_count ++;

  /*  If we're already in a group...ignore  */
  if (group_count > 1)
    return TRUE;

  if (gimage->redo_stack)
    {
      undo_free_list (gimage, REDO, gimage->redo_stack);
      gimage->redo_stack = NULL;
    }

  if (! undo_free_up_space (gimage))
    return FALSE;

  gimage->pushing_undo_group = type;
  gimage->undo_stack = g_slist_prepend (gimage->undo_stack, NULL);
  gimage->undo_levels++;

  return TRUE;
}


int
undo_push_group_end (GImage *gimage)
{
  if (! gimage->undo_on)
    return FALSE;

  group_count --;

  if (group_count == 0)
    {
      gimage->pushing_undo_group = 0;
      gimage->undo_stack = g_slist_prepend (gimage->undo_stack, NULL);
    }

  return TRUE;
}


/*********************************/
/*  Image Undo functions         */

typedef struct _image_undo ImageUndo;

struct _image_undo
{
  TileManager *tiles;
  GimpDrawable *drawable;
  int x1, y1, x2, y2;
  int sparse;
};


int
undo_push_image (GImage *gimage,
		 GimpDrawable *drawable,
		 int     x1,
		 int     y1,
		 int     x2,
		 int     y2)
{
  long size;
  Undo *new;
  ImageUndo *image_undo;
  TileManager *tiles;
  PixelRegion srcPR, destPR;

  /*  increment the dirty flag for this gimage  */
  gimage_dirty (gimage);
  drawable_dirty (drawable);

  x1 = BOUNDS (x1, 0, drawable_width (drawable));
  y1 = BOUNDS (y1, 0, drawable_height (drawable));
  x2 = BOUNDS (x2, 0, drawable_width (drawable));
  y2 = BOUNDS (y2, 0, drawable_height (drawable));

  size = (x2 - x1) * (y2 - y1) * drawable_bytes (drawable) + sizeof (void *) * 2;

  if ((new = undo_push (gimage, size, IMAGE_UNDO)))
    {
      image_undo = (ImageUndo *) g_malloc (sizeof (ImageUndo));

      /*  If we cannot create a new temp buf--either because our parameters are
       *  degenerate or something else failed, simply return an unsuccessful push.
       */
      tiles = tile_manager_new ((x2 - x1), (y2 - y1), drawable_bytes (drawable));
      pixel_region_init (&srcPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);
      copy_region (&srcPR, &destPR);

      /*  set the image undo structure  */
      image_undo->tiles = tiles;
      image_undo->drawable = drawable;
      image_undo->x1 = x1;
      image_undo->y1 = y1;
      image_undo->x2 = x2;
      image_undo->y2 = y2;
      image_undo->sparse = FALSE;

      new->data      = image_undo;
      new->pop_func  = undo_pop_image;
      new->free_func = undo_free_image;

      return TRUE;
    }
  else
    return FALSE;
}


int
undo_push_image_mod (GImage *gimage,
		     GimpDrawable *drawable,
		     int     x1,
		     int     y1,
		     int     x2,
		     int     y2,
		     void   *tiles_ptr,
		     int     sparse)
{
  long size;
  Undo * new;
  ImageUndo *image_undo;
  TileManager *tiles;

  /*  increment the dirty flag for this gimage  */
  gimage_dirty (gimage);
  drawable_dirty (drawable);

  if (! tiles_ptr)
    return FALSE;

  x1 = BOUNDS (x1, 0, drawable_width (drawable));
  y1 = BOUNDS (y1, 0, drawable_height (drawable));
  x2 = BOUNDS (x2, 0, drawable_width (drawable));
  y2 = BOUNDS (y2, 0, drawable_height (drawable));

  tiles = (TileManager *) tiles_ptr;
  size = tiles->levels[0].width * tiles->levels[0].height *
    tiles->levels[0].bpp + sizeof (void *) * 2;

  if ((new = undo_push (gimage, size, IMAGE_MOD_UNDO)))
    {
      image_undo = (ImageUndo *) g_malloc (sizeof (ImageUndo));
      image_undo->tiles = tiles;
      image_undo->drawable = drawable;
      image_undo->x1 = x1;
      image_undo->y1 = y1;
      image_undo->x2 = x2;
      image_undo->y2 = y2;
      image_undo->sparse = sparse;

      new->data      = image_undo;
      new->pop_func  = undo_pop_image;
      new->free_func = undo_free_image;

      return TRUE;
    }
  else
    {
      tile_manager_destroy (tiles);
      return FALSE;
    }
}


int
undo_pop_image (GImage *gimage,
		int     state,
		int     type,
		void   *image_undo_ptr)
{
  ImageUndo *image_undo;
  TileManager *tiles;
  PixelRegion PR1, PR2;
  int x, y;
  int w, h;

  image_undo = (ImageUndo *) image_undo_ptr;
  tiles = image_undo->tiles;

  switch (state)
    {
    case UNDO:
      gimage_clean (gimage);
      drawable_clean (image_undo->drawable);
      break;
    case REDO:
      gimage_dirty (gimage);
      drawable_dirty (image_undo->drawable);
      break;
    }

  /*  some useful values  */
  x = image_undo->x1;
  y = image_undo->y1;

  if (image_undo->sparse == FALSE)
    {
      w = tiles->levels[0].width;
      h = tiles->levels[0].height;

      pixel_region_init (&PR1, tiles, 0, 0, w, h, TRUE);
      pixel_region_init (&PR2, drawable_data (image_undo->drawable), x, y, w, h, TRUE);

      /*  swap the regions  */
      swap_region (&PR1, &PR2);
    }
  else
    {
      int i, j;
      Tile *src_tile;
      Tile *dest_tile;

      w = image_undo->x2 - image_undo->x1;
      h = image_undo->y2 - image_undo->y1;

      for (i = y; i < image_undo->y2; i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
	{
	  for (j = x; j < image_undo->x2; j += (TILE_WIDTH - (j % TILE_WIDTH)))
	    {
	      src_tile = tile_manager_get_tile (tiles, j, i, 0, FALSE, FALSE);
	      if (src_tile->valid == TRUE)
		{
		  src_tile = tile_manager_get_tile (tiles, j, i, 0, TRUE, TRUE);
		  dest_tile = tile_manager_get_tile (drawable_data (image_undo->drawable), j, i, 0, TRUE, TRUE);
		  swap_pixels (src_tile->data, dest_tile->data,
			       (src_tile->ewidth * src_tile->eheight * src_tile->bpp));
		  tile_release (dest_tile, TRUE);
		  tile_release (src_tile, TRUE);
		}
	    }
	}
    }

  drawable_update (image_undo->drawable, x, y, w, h);

  return TRUE;
}


void
undo_free_image (int   state,
		 void *image_undo_ptr)
{
  ImageUndo *image_undo;

  image_undo = (ImageUndo *) image_undo_ptr;

  tile_manager_destroy (image_undo->tiles);
  g_free (image_undo);
}


/******************************/
/*  Mask Undo functions  */

int
undo_push_mask (GImage *gimage,
		void   *mask_ptr)
{
  Undo *new;
  MaskUndo *mask_undo;
  int size;

  mask_undo = (MaskUndo *) mask_ptr;
  if (mask_undo->tiles)
    size = mask_undo->tiles->levels[0].width * mask_undo->tiles->levels[0].height;
  else
    size = 0;

  if ((new = undo_push (gimage, size, MASK_UNDO)))
    {
      new->data          = mask_undo;
      new->pop_func      = undo_pop_mask;
      new->free_func     = undo_free_mask;

      return TRUE;
    }
  else
    {
      if (mask_undo->tiles)
	tile_manager_destroy (mask_undo->tiles);
      g_free (mask_undo);
      return FALSE;
    }
}


int
undo_pop_mask (GImage *gimage,
	       int     state,
	       int     type,
	       void   *mask_ptr)
{
  MaskUndo *mask_undo;
  TileManager *new_tiles;
  Channel *sel_mask;
  PixelRegion srcPR, destPR;
  int selection;
  int x1, y1, x2, y2;
  int width, height;
  unsigned char empty = 0;

  width = height = 0;

  mask_undo = (MaskUndo *) mask_ptr;

  /*  save current selection mask  */
  sel_mask = gimage_get_mask (gimage);
  selection = channel_bounds (sel_mask, &x1, &y1, &x2, &y2);
  pixel_region_init (&srcPR, GIMP_DRAWABLE(sel_mask)->tiles, x1, y1, (x2 - x1), (y2 - y1), FALSE);

  if (selection)
    {
      new_tiles = tile_manager_new (srcPR.w, srcPR.h, 1);
      pixel_region_init (&destPR, new_tiles, 0, 0, srcPR.w, srcPR.h, TRUE);

      copy_region (&srcPR, &destPR);

      pixel_region_init (&srcPR, GIMP_DRAWABLE(sel_mask)->tiles, x1, y1, (x2 - x1), (y2 - y1), TRUE);
      color_region (&srcPR, &empty);
    }
  else
    new_tiles = NULL;

  if (mask_undo->tiles)
    {
      width = mask_undo->tiles->levels[0].width;
      height = mask_undo->tiles->levels[0].height;
      pixel_region_init (&srcPR, mask_undo->tiles, 0, 0, width, height, FALSE);
      pixel_region_init (&destPR, GIMP_DRAWABLE(sel_mask)->tiles, mask_undo->x, mask_undo->y, width, height, TRUE);
      copy_region (&srcPR, &destPR);

      tile_manager_destroy (mask_undo->tiles);
    }

  /* invalidate the current bounds and boundary of the mask */
  gimage_mask_invalidate (gimage);

  if (mask_undo->tiles)
    {
      sel_mask->empty = FALSE;
      sel_mask->x1 = mask_undo->x;
      sel_mask->y1 = mask_undo->y;
      sel_mask->x2 = mask_undo->x + width;
      sel_mask->y2 = mask_undo->y + height;
    }
  else
    {
      sel_mask->empty = TRUE;
      sel_mask->x1 = 0;
      sel_mask->y1 = 0;
      sel_mask->x2 = GIMP_DRAWABLE(sel_mask)->width;
      sel_mask->y2 = GIMP_DRAWABLE(sel_mask)->height;
    }

  /*  set the new mask undo parameters  */
  mask_undo->tiles = new_tiles;
  mask_undo->x = x1;
  mask_undo->y = y1;

  /* we know the bounds */
  sel_mask->bounds_known = TRUE;

  /*  if there is a "by color" selection dialog active
   *  for this gimage's mask, send it an update notice
   */
  if (gimage->by_color_select)
    by_color_select_initialize_by_image (gimage);

  return TRUE;
}


void
undo_free_mask (int   state,
		void *mask_ptr)
{
  MaskUndo *mask_undo;

  mask_undo = (MaskUndo *) mask_ptr;
  if (mask_undo->tiles)
    tile_manager_destroy (mask_undo->tiles);
  g_free (mask_undo);
}


/***************************************/
/*  Layer displacement Undo functions  */

int
undo_push_layer_displace (GImage *gimage,
			  GimpLayer *layer)
{
  Undo * new;
  int * info;

  if ((new = undo_push (gimage, 12, LAYER_DISPLACE_UNDO)))
    {
      new->data          = (void *) g_malloc (sizeof (int) * 3);
      new->pop_func      = undo_pop_layer_displace;
      new->free_func     = undo_free_layer_displace;

      info = (int *) new->data;
      info[0] = drawable_ID(GIMP_DRAWABLE(layer));
      info[1] = GIMP_DRAWABLE(layer)->offset_x;
      info[2] = GIMP_DRAWABLE(layer)->offset_y;

      return TRUE;
    }
  else
    return FALSE;
}


int
undo_pop_layer_displace (GImage *gimage,
			 int     state,
			 int     type,
			 void   *info_ptr)
{
  Layer * layer;
  int old_offsets[2];
  int *info;

  info = (int *) info_ptr;
  layer = layer_get_ID (info[0]);
  if (layer)
    {
      old_offsets[0] = GIMP_DRAWABLE(layer)->offset_x;
      old_offsets[1] = GIMP_DRAWABLE(layer)->offset_y;
      drawable_update (GIMP_DRAWABLE(layer), 0, 0,
		       GIMP_DRAWABLE(layer)->width,
		       GIMP_DRAWABLE(layer)->height);

      GIMP_DRAWABLE(layer)->offset_x = info[1];
      GIMP_DRAWABLE(layer)->offset_y = info[2];
      drawable_update (GIMP_DRAWABLE(layer), 0, 0,
		       GIMP_DRAWABLE(layer)->width,
		       GIMP_DRAWABLE(layer)->height);
      
      if (layer->mask) 
	{
	  GIMP_DRAWABLE(layer->mask)->offset_x = info[1];
	  GIMP_DRAWABLE(layer->mask)->offset_y = info[2];
	  drawable_update (GIMP_DRAWABLE(layer->mask), 0, 0,
			   GIMP_DRAWABLE(layer->mask)->width,
			   GIMP_DRAWABLE(layer->mask)->height);
	}


      /*  invalidate the selection boundary because of a layer modification  */
      layer_invalidate_boundary (layer);

      info[1] = old_offsets[0];
      info[2] = old_offsets[1];

      return TRUE;
    }
  else
    return FALSE;
}


void
undo_free_layer_displace (int   state,
			  void *info_ptr)
{
  g_free (info_ptr);
}


/*********************************/
/*  Transform Undo               */

int
undo_push_transform (GImage *gimage,
		     void   *tu_ptr)
{
  Undo * new;
  int size;

  size = sizeof (TransformUndo);

  if ((new = undo_push (gimage, size, TRANSFORM_UNDO)))
    {
      new->data          = tu_ptr;
      new->pop_func      = undo_pop_transform;
      new->free_func     = undo_free_transform;

      return TRUE;
    }
  else
    {
      g_free (tu_ptr);
      return FALSE;
    }
}


int
undo_pop_transform (GImage *gimage,
		    int     state,
		    int     type,
		    void   *tu_ptr)
{
  TransformCore * tc;
  TransformUndo * tu;
  TileManager * temp;
  double d;
  int i;

  /* Can't have ANY tool selected - maybe a plugin running */
  if (active_tool == NULL)
    return TRUE;

  tc = (TransformCore *) active_tool->private;
  tu = (TransformUndo *) tu_ptr;

  /*  only pop if the active tool is the tool that pushed this undo  */
  if (tu->tool_ID != active_tool->ID)
    return TRUE;

  /*  swap the transformation information arrays  */
  for (i = 0; i < TRAN_INFO_SIZE; i++)
    {
      d = tu->trans_info[i];
      tu->trans_info[i] = tc->trans_info[i];
      tc->trans_info[i] = d;
    }

  /*  if this is the first transform in a string, swap the
   *  original buffer--the source buffer for repeated transforms
   */
  if (tu->first)
    {
      temp = tu->original;
      tu->original = tc->original;
      tc->original = temp;

      /*  If we're re-implementing the first transform, reactivate tool  */
      if (state == REDO && tc->original)
	{
	  active_tool->state = ACTIVE;
	  draw_core_resume (tc->core, active_tool);
	}
    }

  return TRUE;
}


void
undo_free_transform (int   state,
		     void *tu_ptr)
{
  TransformUndo * tu;

  tu = (TransformUndo *) tu_ptr;
  if (tu->original)
    tile_manager_destroy (tu->original);
  g_free (tu);
}


/*********************************/
/*  Paint Undo                   */

int
undo_push_paint (GImage *gimage,
		 void   *pu_ptr)
{
  Undo * new;
  int size;

  size = sizeof (PaintUndo);

  if ((new = undo_push (gimage, size, PAINT_UNDO)))
    {
      new->data          = pu_ptr;
      new->pop_func      = undo_pop_paint;
      new->free_func     = undo_free_paint;

      return TRUE;
    }
  else
    {
      g_free (pu_ptr);
      return FALSE;
    }
}


int
undo_pop_paint (GImage *gimage,
		int     state,
		int     type,
		void   *pu_ptr)
{
  PaintCore * pc;
  PaintUndo * pu;
  double tmp;

  /* Can't have ANY tool selected - maybe a plugin running */
  if (active_tool == NULL)
    return TRUE;

  pc = (PaintCore *) active_tool->private;
  pu = (PaintUndo *) pu_ptr;

  /*  only pop if the active tool is the tool that pushed this undo  */
  if (pu->tool_ID != active_tool->ID)
    return TRUE;

  /*  swap the paint core information  */
  tmp = pc->lastx;
  pc->lastx = pu->lastx;
  pu->lastx = tmp;

  tmp = pc->lasty;
  pc->lasty = pu->lasty;
  pu->lasty = tmp;

  tmp = pc->lastpressure;
  pc->lastpressure = pu->lastpressure;
  pu->lastpressure = tmp;

  tmp = pc->lastxtilt;
  pc->lastxtilt = pu->lastxtilt;
  pu->lastxtilt = tmp;

  tmp = pc->lastytilt;
  pc->lastytilt = pu->lastytilt;
  pu->lastytilt = tmp;

  return TRUE;
}


void
undo_free_paint (int   state,
		 void *pu_ptr)
{
  PaintUndo * pu;

  pu = (PaintUndo *) pu_ptr;
  g_free (pu);
}


/*********************************/
/*  New Layer Undo               */

int
undo_push_layer (GImage *gimage,
		 void   *lu_ptr)
{
  LayerUndo *lu;
  Undo *new;
  int size;

  lu = (LayerUndo *) lu_ptr;

  /*  increment the dirty flag for this gimage  */
  gimage_dirty (gimage);
  drawable_dirty (GIMP_DRAWABLE(lu->layer));

  size = layer_size (lu->layer) + sizeof (LayerUndo);

  if ((new = undo_push (gimage, size, LAYER_UNDO)))
    {
      new->data          = lu_ptr;
      new->pop_func      = undo_pop_layer;
      new->free_func     = undo_free_layer;

      return TRUE;
    }
  else
    {
      /*  if this is a remove layer, delete the layer  */
      if (lu->undo_type == 1)
	layer_unref (lu->layer);
      g_free (lu);
      return FALSE;
    }
}


int
undo_pop_layer (GImage *gimage,
		int     state,
		int     type,
		void   *lu_ptr)
{
  LayerUndo *lu;

  lu = (LayerUndo *) lu_ptr;

  switch (state)
    {
    case UNDO:
      gimage_clean (gimage);
      drawable_clean (GIMP_DRAWABLE(lu->layer));
      break;
    case REDO:
      gimage_dirty (gimage);
      drawable_dirty (GIMP_DRAWABLE(lu->layer));
      break;
    }

  /*  remove layer  */
  if ((state == UNDO && lu->undo_type == 0) ||
      (state == REDO && lu->undo_type == 1))
    {
      /*  record the current position  */
      lu->prev_position = gimage_get_layer_index (gimage, lu->layer);
      /*  set the previous layer  */
      gimage_set_active_layer (gimage, lu->prev_layer);

      /*  remove the layer  */
      gimage->layers = g_slist_remove (gimage->layers, lu->layer);
      gimage->layer_stack = g_slist_remove (gimage->layer_stack, lu->layer);

      /*  reset the gimage values  */
      if (layer_is_floating_sel (lu->layer))
	{
	  gimage->floating_sel = NULL;
	  /*  reset the old drawable  */
	  floating_sel_reset (lu->layer);
	}

      drawable_update (GIMP_DRAWABLE(lu->layer), 0, 0, GIMP_DRAWABLE(lu->layer)->width, GIMP_DRAWABLE(lu->layer)->height);
    }
  /*  restore layer  */
  else
    {
      /*  record the active layer  */
      lu->prev_layer = gimage->active_layer;

      /*  hide the current selection--for all views  */
      if (gimage->active_layer != NULL)
	layer_invalidate_boundary ((gimage->active_layer));

      /*  if this is a floating selection, set the fs pointer  */
      if (layer_is_floating_sel (lu->layer))
	gimage->floating_sel = lu->layer;

      /*  add the new layer  */
      gimage->layers = g_slist_insert (gimage->layers, lu->layer, lu->prev_position);
      gimage->layer_stack = g_slist_prepend (gimage->layer_stack, lu->layer);
      gimage->active_layer = lu->layer;
      drawable_update (GIMP_DRAWABLE(lu->layer), 0, 0, GIMP_DRAWABLE(lu->layer)->width, GIMP_DRAWABLE(lu->layer)->height);
    }

  return TRUE;
}


void
undo_free_layer (int   state,
		 void *lu_ptr)
{
  LayerUndo *lu;

  lu = (LayerUndo *) lu_ptr;

  /*  Only free the layer if we're freeing from the redo
   *  stack and it's a layer add, or if we're freeing from
   *  the undo stack and it's a layer remove
   */
  if ((state == REDO && lu->undo_type == 0) ||
      (state == UNDO && lu->undo_type == 1))
    layer_unref (lu->layer);

  g_free (lu);
}



/*********************************/
/*  Layer Mod Undo               */

int
undo_push_layer_mod (GImage *gimage,
		     void   *layer_ptr)
{
  Layer *layer;
  Undo *new;
  TileManager *tiles;
  void **data;
  int size;

  layer = (Layer *) layer_ptr;

  /*  increment the dirty flag for this gimage  */
  gimage_dirty (gimage);
  drawable_dirty (GIMP_DRAWABLE(layer));

  tiles = GIMP_DRAWABLE(layer)->tiles;
  tiles->x = GIMP_DRAWABLE(layer)->offset_x;
  tiles->y = GIMP_DRAWABLE(layer)->offset_y;
  size = GIMP_DRAWABLE(layer)->width * GIMP_DRAWABLE(layer)->height * GIMP_DRAWABLE(layer)->bytes + sizeof (void *) * 3;

  if ((new = undo_push (gimage, size, LAYER_MOD)))
    {
      data               = (void **) g_malloc (sizeof (void *) * 3);
      new->data          = data;
      new->pop_func      = undo_pop_layer_mod;
      new->free_func     = undo_free_layer_mod;

      data[0] = layer_ptr;
      data[1] = (void *) tiles;
      data[2] = (void *) ((long) GIMP_DRAWABLE(layer)->type);

      return TRUE;
    }
  else
    {
      tile_manager_destroy (tiles);
      return FALSE;
    }
}


int
undo_pop_layer_mod (GImage *gimage,
		    int     state,
		    int     type,
		    void   *data_ptr)
{
  void **data;
  int layer_type;
  TileManager *tiles;
  TileManager *temp;
  Layer *layer;

  data = (void **) data_ptr;
  layer = (Layer *) data[0];

  switch (state)
    {
    case UNDO:
      gimage_clean (gimage);
      drawable_clean (GIMP_DRAWABLE(layer));
      break;
    case REDO:
      gimage_dirty (gimage);
      drawable_dirty (GIMP_DRAWABLE(layer));
      break;
    }

  tiles = (TileManager *) data[1];

  /*  Issue the first update  */
  gdisplays_update_area (gimage,
			 GIMP_DRAWABLE(layer)->offset_x, GIMP_DRAWABLE(layer)->offset_y,
			 GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height);

  /*  Create a tile manager to store the current layer contents  */
  temp = GIMP_DRAWABLE(layer)->tiles;
  temp->x = GIMP_DRAWABLE(layer)->offset_x;
  temp->y = GIMP_DRAWABLE(layer)->offset_y;
  layer_type = (long) data[2];
  data[2] = (void *) ((long) GIMP_DRAWABLE(layer)->type);

  /*  restore the layer's data  */
  GIMP_DRAWABLE(layer)->tiles = tiles;
  GIMP_DRAWABLE(layer)->offset_x = tiles->x;
  GIMP_DRAWABLE(layer)->offset_y = tiles->y;
  GIMP_DRAWABLE(layer)->width = tiles->levels[0].width;
  GIMP_DRAWABLE(layer)->height = tiles->levels[0].height;
  GIMP_DRAWABLE(layer)->bytes = tiles->levels[0].bpp;
  GIMP_DRAWABLE(layer)->type = layer_type;
  GIMP_DRAWABLE(layer)->has_alpha = TYPE_HAS_ALPHA (layer_type);

  if (layer->mask) 
    {
      GIMP_DRAWABLE(layer->mask)->offset_x = tiles->x;
      GIMP_DRAWABLE(layer->mask)->offset_y = tiles->y;
    }

  /*  If the layer type changed, update the gdisplay titles  */
  if (GIMP_DRAWABLE(layer)->type != (long) data[2])
    gdisplays_update_title (GIMP_DRAWABLE(layer)->gimage);

  /*  Set the new tile manager  */
  data[1] = temp;

  /*  Issue the second update  */
  drawable_update (GIMP_DRAWABLE(layer), 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height);

  return TRUE;
}


void
undo_free_layer_mod (int   state,
		     void *data_ptr)
{
  void ** data;

  data = (void **) data_ptr;
  tile_manager_destroy ((TileManager *) data[1]);
  g_free (data);
}


/*********************************/
/*  Layer Mask Undo               */

int
undo_push_layer_mask (GImage *gimage,
		      void   *lmu_ptr)
{
  LayerMaskUndo *lmu;
  Undo *new;
  int size;

  lmu = (LayerMaskUndo *) lmu_ptr;

  /*  increment the dirty flag for this gimage  */
  gimage_dirty (gimage);
  drawable_dirty (GIMP_DRAWABLE(lmu->layer));

  size = channel_size (GIMP_CHANNEL (lmu->mask)) + sizeof (LayerMaskUndo);

  if ((new = undo_push (gimage, size, LAYER_MASK_UNDO)))
    {
      new->data          = lmu_ptr;
      new->pop_func      = undo_pop_layer_mask;
      new->free_func     = undo_free_layer_mask;

      return TRUE;
    }
  else
    {
      if (lmu->undo_type == 1)
	layer_mask_delete (lmu->mask);
      g_free (lmu);
      return FALSE;
    }
}


int
undo_pop_layer_mask (GImage *gimage,
		     int     state,
		     int     type,
		     void   *lmu_ptr)
{
  LayerMaskUndo *lmu;

  lmu = (LayerMaskUndo *) lmu_ptr;

  switch (state)
    {
    case UNDO:
      gimage_clean (gimage);
      drawable_clean (GIMP_DRAWABLE(lmu->layer));
      break;
    case REDO:
      gimage_dirty (gimage);
      drawable_dirty (GIMP_DRAWABLE(lmu->layer));
      break;
    }

  /*  remove layer mask  */
  if ((state == UNDO && lmu->undo_type == 0) ||
      (state == REDO && lmu->undo_type == 1))
    {
      /*  remove the layer mask  */
      lmu->layer->mask = NULL;
      lmu->layer->apply_mask = 0;
      lmu->layer->edit_mask = 0;
      lmu->layer->show_mask = 0;

      /*  if this is redoing a remove operation &
       *  the mode of application was DISCARD or
       *  this is undoing an add...
       */
      if ((state == REDO && lmu->mode == DISCARD) || state == UNDO)
	drawable_update (GIMP_DRAWABLE(lmu->layer), 0, 0, GIMP_DRAWABLE(lmu->layer)->width, GIMP_DRAWABLE(lmu->layer)->height);
    }
  /*  restore layer  */
  else
    {
      lmu->layer->mask = lmu->mask;
      lmu->layer->apply_mask = lmu->apply_mask;
      lmu->layer->edit_mask = lmu->edit_mask;
      lmu->layer->show_mask = lmu->show_mask;

      gimage_set_layer_mask_edit (gimage, lmu->layer, lmu->edit_mask);

      /*  if this is undoing a remove operation &
       *  the mode of application was DISCARD or
       *  this is redoing an add
       */
      if ((state == UNDO && lmu->mode == DISCARD) || state == REDO)
	drawable_update (GIMP_DRAWABLE(lmu->layer), 0, 0, GIMP_DRAWABLE(lmu->layer)->width, GIMP_DRAWABLE(lmu->layer)->height);
    }

  return TRUE;
}


void
undo_free_layer_mask (int   state,
		      void *lmu_ptr)
{
  LayerMaskUndo *lmu;

  lmu = (LayerMaskUndo *) lmu_ptr;

  /*  Only free the layer mask if we're freeing from the redo
   *  stack and it's a layer add, or if we're freeing from
   *  the undo stack and it's a layer remove
   */
  if ((state == REDO && lmu->undo_type == 0) ||
      (state == UNDO && lmu->undo_type == 1))
    layer_mask_delete (lmu->mask);

  g_free (lmu);
}


/*********************************/
/*  New Channel Undo               */

int
undo_push_channel (GImage *gimage,
		   void   *cu_ptr)
{
  ChannelUndo *cu;
  Undo *new;
  int size;

  cu = (ChannelUndo *) cu_ptr;

  /*  increment the dirty flag for this gimage  */
  gimage_dirty (gimage);
  drawable_dirty (GIMP_DRAWABLE(cu->channel));

  size = channel_size (cu->channel) + sizeof (ChannelUndo);

  if ((new = undo_push (gimage, size, CHANNEL_UNDO)))
    {
      new->data          = cu_ptr;
      new->pop_func      = undo_pop_channel;
      new->free_func     = undo_free_channel;

      return TRUE;
    }
  else
    {
      if (cu->undo_type == 1)
	channel_delete (cu->channel);
      g_free (cu);
      return FALSE;
    }
}


int
undo_pop_channel (GImage *gimage,
		  int     state,
		  int     type,
		  void   *cu_ptr)
{
  ChannelUndo *cu;

  cu = (ChannelUndo *) cu_ptr;

  switch (state)
    {
    case UNDO:
      gimage_clean (gimage);
      drawable_clean (GIMP_DRAWABLE(cu->channel));
      break;
    case REDO:
      gimage_dirty (gimage);
      drawable_dirty (GIMP_DRAWABLE(cu->channel));
      break;
    }

  /*  remove channel  */
  if ((state == UNDO && cu->undo_type == 0) ||
      (state == REDO && cu->undo_type == 1))
    {
      /*  record the current position  */
      cu->prev_position = gimage_get_channel_index (gimage, cu->channel);

      /*  remove the channel  */
      gimage->channels = g_slist_remove (gimage->channels, cu->channel);

      /*  set the previous channel  */
      gimage_set_active_channel (gimage, cu->prev_channel);

      /*  update the area  */
      drawable_update (GIMP_DRAWABLE(cu->channel), 0, 0, GIMP_DRAWABLE(cu->channel)->width, GIMP_DRAWABLE(cu->channel)->height);
    }
  /*  restore channel  */
  else
    {
      /*  record the active channel  */
      cu->prev_channel = gimage->active_channel;

      /*  add the new channel  */
      gimage->channels = g_slist_insert (gimage->channels, cu->channel, cu->prev_position);

      /*  set the new channel  */
      gimage_set_active_channel (gimage, cu->channel);

      /*  update the area  */
      drawable_update (GIMP_DRAWABLE(cu->channel), 0, 0, GIMP_DRAWABLE(cu->channel)->width, GIMP_DRAWABLE(cu->channel)->height);
    }

  return TRUE;
}


void
undo_free_channel (int   state,
		   void *cu_ptr)
{
  ChannelUndo *cu;

  cu = (ChannelUndo *) cu_ptr;

  /*  Only free the channel if we're freeing from the redo
   *  stack and it's a channel add, or if we're freeing from
   *  the undo stack and it's a channel remove
   */
  if ((state == REDO && cu->undo_type == 0) ||
      (state == UNDO && cu->undo_type == 1))
    channel_delete (cu->channel);

  g_free (cu);
}



/*********************************/
/*  Channel Mod Undo               */

int
undo_push_channel_mod (GImage *gimage,
		       void   *channel_ptr)
{
  Channel *channel;
  TileManager *tiles;
  Undo *new;
  void **data;
  int size;

  channel = (Channel *) channel_ptr;

  /*  increment the dirty flag for this gimage  */
  gimage_dirty (gimage);
  drawable_dirty (GIMP_DRAWABLE(channel));

  tiles = GIMP_DRAWABLE(channel)->tiles;
  size = GIMP_DRAWABLE(channel)->width * GIMP_DRAWABLE(channel)->height + sizeof (void *) * 2;

  if ((new = undo_push (gimage, size, CHANNEL_MOD)))
    {
      data               = (void **) g_malloc (sizeof (void *) * 2);
      new->data          = data;
      new->pop_func      = undo_pop_channel_mod;
      new->free_func     = undo_free_channel_mod;

      data[0] = channel_ptr;
      data[1] = (void *) tiles;

      return TRUE;
    }
  else
    {
      tile_manager_destroy (tiles);
      return FALSE;
    }
}


int
undo_pop_channel_mod (GImage *gimage,
		      int     state,
		      int     type,
		      void   *data_ptr)
{
  void **data;
  TileManager *tiles;
  TileManager *temp;
  Channel *channel;

  data = (void **) data_ptr;
  channel = (Channel *) data[0];

  switch (state)
    {
    case UNDO:
      gimage_clean (gimage);
      drawable_clean (GIMP_DRAWABLE(channel));
      break;
    case REDO:
      gimage_dirty (gimage);
      drawable_dirty (GIMP_DRAWABLE(channel));
      break;
    }

  tiles = (TileManager *) data[1];

  /*  Issue the first update  */
  drawable_update (GIMP_DRAWABLE(channel), 0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height);

  temp = GIMP_DRAWABLE(channel)->tiles;
  GIMP_DRAWABLE(channel)->tiles = tiles;
  GIMP_DRAWABLE(channel)->width = tiles->levels[0].width;
  GIMP_DRAWABLE(channel)->height = tiles->levels[0].height;

  /*  Set the new buffer  */
  data[1] = temp;

  /*  Issue the second update  */
  drawable_update (GIMP_DRAWABLE(channel), 0, 0, GIMP_DRAWABLE(channel)->width, GIMP_DRAWABLE(channel)->height);

  return TRUE;
}


void
undo_free_channel_mod (int   state,
		       void *data_ptr)
{
  void ** data;

  data = (void **) data_ptr;
  tile_manager_destroy ((TileManager *) data[1]);
  g_free (data);
}


/**************************************/
/*  Floating Selection to Layer Undo  */

int
undo_push_fs_to_layer (GImage *gimage,
		       void   *fsu_ptr)
{
  FStoLayerUndo *fsu;
  Undo *new;
  int size;

  fsu = (FStoLayerUndo *) fsu_ptr;

  /*  increment the dirty flag for this gimage  */
  gimage_dirty (gimage);
  drawable_dirty (GIMP_DRAWABLE(fsu->layer));

  size = sizeof (FStoLayerUndo);

  if ((new = undo_push (gimage, size, CHANNEL_MOD)))
    {
      new->data          = fsu_ptr;
      new->pop_func      = undo_pop_fs_to_layer;
      new->free_func     = undo_free_fs_to_layer;

      return TRUE;
    }
  else
    {
      tile_manager_destroy (fsu->layer->fs.backing_store);
      g_free (fsu);
      return FALSE;
    }
}


int
undo_pop_fs_to_layer (GImage *gimage,
		      int     state,
		      int     type,
		      void   *fsu_ptr)
{
  FStoLayerUndo *fsu;

  fsu = (FStoLayerUndo *) fsu_ptr;

  switch (state)
    {
    case UNDO:
      gimage_clean (gimage);
      drawable_clean (GIMP_DRAWABLE(fsu->layer));
      break;
    case REDO:
      gimage_dirty (gimage);
      drawable_dirty (GIMP_DRAWABLE(fsu->layer));
      break;
    }

  switch (state)
    {
    case UNDO:
      /*  Update the preview for the floating sel  */
      drawable_invalidate_preview (GIMP_DRAWABLE(fsu->layer));

      fsu->layer->fs.drawable = fsu->drawable;
      gimage->active_layer = fsu->layer;
      gimage->floating_sel = fsu->layer;

      /*  restore the contents of the drawable  */
      floating_sel_store (fsu->layer, GIMP_DRAWABLE(fsu->layer)->offset_x, GIMP_DRAWABLE(fsu->layer)->offset_y,
			  GIMP_DRAWABLE(fsu->layer)->width, GIMP_DRAWABLE(fsu->layer)->height);
      fsu->layer->fs.initial = TRUE;

      /*  clear the selection  */
      layer_invalidate_boundary (fsu->layer);

      /*  Update the preview for the gimage and underlying drawable  */
      drawable_invalidate_preview (GIMP_DRAWABLE(fsu->layer));
      break;

    case REDO:
      /*  restore the contents of the drawable  */
      floating_sel_restore (fsu->layer, GIMP_DRAWABLE(fsu->layer)->offset_x, GIMP_DRAWABLE(fsu->layer)->offset_y,
			    GIMP_DRAWABLE(fsu->layer)->width, GIMP_DRAWABLE(fsu->layer)->height);

      /*  Update the preview for the gimage and underlying drawable  */
      drawable_invalidate_preview (GIMP_DRAWABLE(fsu->layer));

      /*  clear the selection  */
      layer_invalidate_boundary (fsu->layer);

      /*  update the pointers  */
      fsu->layer->fs.drawable = NULL;
      gimage->floating_sel = NULL;

      /*  Update the fs drawable  */
      drawable_invalidate_preview (GIMP_DRAWABLE(fsu->layer));
      break;
    }

  return TRUE;
}


void
undo_free_fs_to_layer (int   state,
		       void *fsu_ptr)
{
  FStoLayerUndo *fsu;

  fsu = (FStoLayerUndo *) fsu_ptr;

  if (state == UNDO)
    tile_manager_destroy (fsu->layer->fs.backing_store);
  g_free (fsu);
}


/***********************************/
/*  Floating Selection Rigor Undo  */

int
undo_push_fs_rigor (GImage *gimage,
		    int     layer_id)
{
  Undo *new;
  int size;

  size = sizeof (int);

  if ((new = undo_push (gimage, size, FS_RIGOR)))
    {
      new->data          = g_new (int, 1);
      new->pop_func      = undo_pop_fs_rigor;
      new->free_func     = undo_free_fs_rigor;

      *((int *) new->data) = layer_id;

      return TRUE;
    }
  else
    return FALSE;
}


int
undo_pop_fs_rigor (GImage *gimage,
		   int     state,
		   int     type,
		   void   *layer_ptr)
{
  int layer_id;
  Layer *floating_layer;

  layer_id = *((int *) layer_ptr);
  if ((floating_layer = layer_get_ID (layer_id)) == NULL)
    return FALSE;
  if (! layer_is_floating_sel (floating_layer))
    return FALSE;

  switch (state)
    {
    case UNDO:
      /*  restore the contents of drawable the floating layer is attached to  */
      if (floating_layer->fs.initial == FALSE)
	floating_sel_restore (floating_layer,
			      GIMP_DRAWABLE(floating_layer)->offset_x, GIMP_DRAWABLE(floating_layer)->offset_y,
			      GIMP_DRAWABLE(floating_layer)->width, GIMP_DRAWABLE(floating_layer)->height);
      floating_layer->fs.initial = TRUE;
      break;

    case REDO:
      /*  store the affected area from the drawable in the backing store  */
      floating_sel_store (floating_layer,
			  GIMP_DRAWABLE(floating_layer)->offset_x, GIMP_DRAWABLE(floating_layer)->offset_y,
			  GIMP_DRAWABLE(floating_layer)->width, GIMP_DRAWABLE(floating_layer)->height);
      floating_layer->fs.initial = TRUE;
      break;
    }

  return TRUE;
}


void
undo_free_fs_rigor (int   state,
		    void *layer_ptr)
{
  g_free (layer_ptr);
}


/***********************************/
/*  Floating Selection Relax Undo  */

int
undo_push_fs_relax (GImage *gimage,
		    int     layer_id)
{
  Undo *new;
  int size;

  size = sizeof (int);

  if ((new = undo_push (gimage, size, FS_RELAX)))
    {
      new->data          = g_new (int, 1);
      new->pop_func      = undo_pop_fs_relax;
      new->free_func     = undo_free_fs_relax;

      *((int *) new->data) = layer_id;

      return TRUE;
    }
  else
    return FALSE;
}


int
undo_pop_fs_relax (GImage *gimage,
		   int     state,
		   int     type,
		   void   *layer_ptr)
{
  int layer_id;
  Layer *floating_layer;

  layer_id = *((int *) layer_ptr);
  if ((floating_layer = layer_get_ID (layer_id)) == NULL)
    return FALSE;
  if (! layer_is_floating_sel (floating_layer))
    return FALSE;

  switch (state)
    {
    case UNDO:
      /*  store the affected area from the drawable in the backing store  */
      floating_sel_store (floating_layer,
			  GIMP_DRAWABLE(floating_layer)->offset_x, GIMP_DRAWABLE(floating_layer)->offset_y,
			  GIMP_DRAWABLE(floating_layer)->width, GIMP_DRAWABLE(floating_layer)->height);
      floating_layer->fs.initial = TRUE;
      break;

    case REDO:
      /*  restore the contents of drawable the floating layer is attached to  */
      if (floating_layer->fs.initial == FALSE)
	floating_sel_restore (floating_layer,
			      GIMP_DRAWABLE(floating_layer)->offset_x, GIMP_DRAWABLE(floating_layer)->offset_y,
			      GIMP_DRAWABLE(floating_layer)->width, GIMP_DRAWABLE(floating_layer)->height);
      floating_layer->fs.initial = TRUE;
      break;
    }

  return TRUE;
}


void
undo_free_fs_relax (int   state,
		    void *layer_ptr)
{
  g_free (layer_ptr);
}


/*********************/
/*  GImage Mod Undo  */

int
undo_push_gimage_mod (GImage *gimage)
{
  Undo *new;
  int *data;
  int size;

  /*  increment the dirty flag for this gimage  */
  gimage_dirty (gimage);

  size = sizeof (int) * 3;

  if ((new = undo_push (gimage, size, GIMAGE_MOD)))
    {
      data               = (int *) g_malloc (sizeof (int) * 3);
      new->data          = data;
      new->pop_func      = undo_pop_gimage_mod;
      new->free_func     = undo_free_gimage_mod;

      data[0] = gimage->width;
      data[1] = gimage->height;
      data[2] = gimage->base_type;

      return TRUE;
    }

  return FALSE;
}


int
undo_pop_gimage_mod (GImage *gimage,
		     int     state,
		     int     type,
		     void   *data_ptr)
{
  int *data;
  int tmp;

  switch (state)
    {
    case UNDO:
      gimage_clean (gimage);
      break;
    case REDO:
      gimage_dirty (gimage);
      break;
    }

  data = (int *) data_ptr;
  tmp = data[0];
  data[0] = gimage->width;
  gimage->width = tmp;
  tmp = data[1];
  data[1] = gimage->height;
  gimage->height = tmp;
  tmp = data[2];
  data[2] = gimage->base_type;
  gimage->base_type = tmp;

  gimage_projection_realloc (gimage);

  gimage_mask_invalidate (gimage);
  channel_invalidate_previews (gimage);
  layer_invalidate_previews (gimage);
  gimage_invalidate_preview (gimage);
  gdisplays_update_full (gimage);
  gdisplays_update_title (gimage);

  indexed_palette_update_image_list ();

  if (gimage->width != (int) data[0] || gimage->height != (int) data[1])
    shrink_wrap = TRUE;

  return TRUE;
}


void
undo_free_gimage_mod (int   state,
		      void *data_ptr)
{
  g_free (data_ptr);
}


/***********/
/*  Guide  */

typedef struct _GuideUndo GuideUndo;

struct _GuideUndo
{
  GImage *gimage;
  Guide *guide;
  Guide orig;
};

int
undo_push_guide (GImage *gimage,
		 void   *guide)
{
  Undo *new;
  GuideUndo *data;
  long size;

  /*  increment the dirty flag for this gimage  */
  gimage_dirty (gimage);

  size = sizeof (GuideUndo);

  if ((new = undo_push (gimage, size, GIMAGE_MOD)))
    {
      ((Guide *)(guide))->ref_count++;
      data               = g_new (GuideUndo, 1);
      new->data          = data;
      new->pop_func      = undo_pop_guide;
      new->free_func     = undo_free_guide;

      data->gimage = gimage;
      data->guide = guide;
      data->orig = *(data->guide);

      return TRUE;
    }

  return FALSE;
}


int
undo_pop_guide (GImage *gimage,
		int     state,
		int     type,
		void   *data_ptr)
{
  GuideUndo *data;
  Guide tmp;
  int tmp_ref;

  data = data_ptr;

  gdisplays_expose_guide (gimage, data->guide);
  gdisplays_expose_guide (gimage, &data->orig);

  tmp_ref = data->guide->ref_count;
  tmp = *(data->guide);
  *(data->guide) = data->orig;
  data->guide->ref_count = tmp_ref;
  data->orig = tmp;

  switch (state)
    {
    case UNDO:
      gimage_clean (gimage);
      break;
    case REDO:
      gimage_dirty (gimage);
      break;
    }

  return TRUE;
}


void
undo_free_guide (int   state,
		 void *data_ptr)
{
  GuideUndo *data;

  data = data_ptr;

  data->guide->ref_count--;
  if (data->guide->position < 0 && data->guide->ref_count <= 0)
      gimage_delete_guide (data->gimage, data->guide);

  g_free (data_ptr);
}
