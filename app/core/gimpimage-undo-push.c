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

#include "appenv.h"
#include "by_color_select.h"
#include "channel.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "gdisplay_ops.h"
#include "gimage_mask.h"
#include "gimpparasite.h"
#include "gimprc.h"
#include "layer.h"
#include "paint_core.h"
#include "paint_funcs.h"
#include "parasitelist.h"
#include "path_transform.h"
#include "tools.h"
#include "transform_core.h"
#include "undo.h"

#include "drawable_pvt.h"
#include "layer_pvt.h"
#include "channel_pvt.h"
#include "tile_manager_pvt.h"
#include "tile.h"			/* ick. */

#include "libgimp/gimpparasite.h"

#include "libgimp/gimpintl.h"


/*#define DEBUG*/

#ifdef DEBUG
#define TRC(x) printf x
#else
#define TRC(x)
#endif


typedef enum
{
  UNDO = 0,
  REDO = 1
} UndoState;


typedef gboolean  (* UndoPopFunc)  (GImage *, UndoState, UndoType, gpointer);
typedef void      (* UndoFreeFunc) (UndoState, UndoType, gpointer);

typedef struct _Undo Undo;

struct _Undo
{
  UndoType      type;           /* undo type                           */
  gpointer      data;           /* data to implement the undo, NULL for group */
  glong         bytes;          /* size of undo item                   */
  gboolean      dirties_image;  /* TRUE if undo mutates image */
  gboolean      group_boundary; /* TRUE if this is the start/end of a group */

  UndoPopFunc   pop_func;      /* function pointer to undo pop proc   */
  UndoFreeFunc  free_func;     /* function with specifics for freeing */
};

/*  Pop functions  */

static gboolean undo_pop_image            (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_mask             (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_layer_displace   (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_transform        (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_paint            (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_layer            (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_layer_mod        (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_layer_mask       (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_channel          (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_channel_mod      (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_fs_to_layer      (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_fs_rigor         (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_fs_relax         (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_gimage_mod       (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_guide            (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_parasite         (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_qmask            (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_resolution       (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_layer_rename     (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_layer_reposition (GImage *,
					   UndoState, UndoType, gpointer);
static gboolean undo_pop_cantundo         (GImage *,
					   UndoState, UndoType, gpointer);


/*  Free functions  */

static void     undo_free_image           (UndoState, UndoType, gpointer);
static void     undo_free_mask            (UndoState, UndoType, gpointer);
static void     undo_free_layer_displace  (UndoState, UndoType, gpointer);
static void     undo_free_transform       (UndoState, UndoType, gpointer);
static void     undo_free_paint           (UndoState, UndoType, gpointer);
static void     undo_free_layer           (UndoState, UndoType, gpointer);
static void     undo_free_layer_mod       (UndoState, UndoType, gpointer);
static void     undo_free_layer_mask      (UndoState, UndoType, gpointer);
static void     undo_free_channel         (UndoState, UndoType, gpointer);
static void     undo_free_channel_mod     (UndoState, UndoType, gpointer);
static void     undo_free_fs_to_layer     (UndoState, UndoType, gpointer);
static void     undo_free_fs_rigor        (UndoState, UndoType, gpointer);
static void     undo_free_fs_relax        (UndoState, UndoType, gpointer);
static void     undo_free_gimage_mod      (UndoState, UndoType, gpointer);
static void     undo_free_guide           (UndoState, UndoType, gpointer);
static void     undo_free_parasite        (UndoState, UndoType, gpointer);
static void     undo_free_qmask           (UndoState, UndoType, gpointer);
static void     undo_free_resolution      (UndoState, UndoType, gpointer);
static void     undo_free_layer_rename    (UndoState, UndoType, gpointer);
static void     undo_free_layer_reposition(UndoState, UndoType, gpointer);
static void     undo_free_cantundo        (UndoState, UndoType, gpointer);


/*  Sizing functions  */
static gint          layer_size         (Layer    *layer);
static gint          channel_size       (Channel  *channel);

static const gchar * undo_type_to_name  (UndoType  undo_type);

static Undo        * undo_new           (UndoType  undo_type, 
					 glong     size, 
					 gboolean  dirties_image);


static gboolean shrink_wrap = FALSE;


static gint
layer_size (Layer *layer)
{
  gint size;

  size =
    sizeof (Layer) + 
    GIMP_DRAWABLE (layer)->width * GIMP_DRAWABLE (layer)->height *
    GIMP_DRAWABLE (layer)->bytes + 
    strlen (GIMP_DRAWABLE (layer)->name);

  if (layer_get_mask (layer))
    size += channel_size (GIMP_CHANNEL (layer_get_mask (layer)));

  return size;
}


static gint
channel_size (Channel *channel)
{
  gint size;

  size =
    sizeof (Channel) +
    GIMP_DRAWABLE (channel)->width * GIMP_DRAWABLE (channel)->height +
    strlen (GIMP_DRAWABLE (channel)->name);

  return size;
}


static void
undo_free_list (GImage     *gimage,
		UndoState   state,
		GSList     *list)
{
  GSList *orig;
  Undo   *undo;

  orig = list;

  while (list)
    {
      undo = (Undo *) list->data;
      if (undo)
	{
	  if (undo->free_func)
	    (*undo->free_func) (state, undo->type, undo->data);
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
  GSList   *list;
  GSList   *last;
  Undo     *object;
  gboolean  in_group = FALSE;

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

	  gimp_image_undo_event (gimage, UNDO_EXPIRED);
	}
      else
	{
	  object = (Undo *) list->data;
	  if (object->group_boundary)
	    in_group = (in_group) ? FALSE : TRUE;

	  /*  Make sure last points to only a single item, or the
	   *  tail of an aggregate undo item
	   */
	  if (!in_group)
	    last = list;

	  list = g_slist_next (list);
	}
    }

  if (last)
    return gimage->undo_stack;
  else
    return NULL;
}


/* Allocate and initialise a new Undo.  Leaves data and function
 * pointers zeroed ready to be filled in by caller. */
static Undo *
undo_new (UndoType  type, 
	  glong     size, 
	  gboolean  dirties_image)
{
  Undo *new;

  new = g_new0 (Undo, 1);

  new->type          = type;
  new->bytes         = size;
  new->dirties_image = dirties_image;

  return new;
}


static gboolean
undo_free_up_space (GImage *gimage)
{
  /* If there are 0 levels of undo return FALSE.  */
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
undo_push (GImage   *gimage,
	   glong     size,
	   UndoType  type,
	   gboolean  dirties_image)
{
  Undo *new;

  /* Does this undo dirty the image?  If so, we always want to mark
   * image dirty, even if we can't actually push the undo. */
  if (dirties_image)
      gimp_image_dirty (gimage);

  if (! gimage->undo_on)
    return NULL;

  size += sizeof (Undo);

  if (gimage->redo_stack)
    {
      undo_free_list (gimage, REDO, gimage->redo_stack);
      gimage->redo_stack = NULL;

      /* If the image was dirty, but could become clean by redo-ing
       * some actions, then it should now become 'infinitely' dirty.
       * This is because we've just nuked the actions that would allow
       * the image to become clean again.  The only hope for salvation
       * is to save the image now!  -- austin */
      if (gimage->dirty < 0)
	gimage->dirty = 10000;
    }

  if (gimage->pushing_undo_group == UNDO_NULL)
    if (! undo_free_up_space (gimage))
      return NULL;

  new = undo_new (type, size, dirties_image);

  gimage->undo_bytes += size;

  /*  only increment levels if not in a group  */
  if (gimage->pushing_undo_group == UNDO_NULL)
      gimage->undo_levels ++;

  gimage->undo_stack = g_slist_prepend (gimage->undo_stack, (gpointer) new);
  TRC (("undo_push: %s\n", undo_type_to_name (type)));

  /* lastly, tell people about the newly pushed undo (must come after
   * modification of undo_stack).  */
  if (gimage->pushing_undo_group == UNDO_NULL)
      gimp_image_undo_event (gimage, UNDO_PUSHED);

  return new;
}


static gboolean
pop_stack (GImage     *gimage,
	   GSList    **stack_ptr,
	   GSList    **unstack_ptr,
	   UndoState   state)
{
  Undo     *object;
  GSList   *stack;
  GSList   *tmp;
  gint      status = 0;
  gint      in_group = 0;
  gint      x, y;
  GDisplay *gdisp;

  /*  Keep popping until we pop a valid object
   *  or get to the end of a group if we're in one
   */
  while (*stack_ptr)
    {
      stack = *stack_ptr;

      object = (Undo *) stack->data;
      if (object->group_boundary)
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
	  TRC (("undo_pop: %s\n", undo_type_to_name (object->type)));
	  status = (* object->pop_func) (gimage, state, object->type,
					 object->data);

	  if (object->dirties_image)
	    {
	      switch (state)
		{
		case UNDO:
		  gimp_image_clean (gimage);
		  break;
		case REDO:
		  gimp_image_dirty (gimage);
		  break;
		}
	    }

	  if (!in_group)
	    gimage->undo_levels += (state == UNDO) ? -1 : 1;
	}

      *unstack_ptr = g_slist_prepend (*unstack_ptr, (gpointer) object);

      tmp = stack;
      *stack_ptr = g_slist_next (*stack_ptr);
      tmp->next = NULL;
      g_slist_free (tmp);

      if (status && !in_group)
	{
	  /*  Flush any image updates and displays  */
	  gdisp = gdisplay_active();
	  if (gdisp != NULL)
	    {
	      if (gdisp->disp_xoffset || gdisp->disp_yoffset)
		{
		  gdk_window_get_size (gdisp->canvas->window, &x, &y);
		  if (gdisp->disp_yoffset)
		    {
		      gdisplay_expose_area (gdisp, 0, 0,
					    gdisp->disp_width,
					    gdisp->disp_yoffset);
		      gdisplay_expose_area (gdisp, 0, gdisp->disp_yoffset + y,
					    gdisp->disp_width,
					    gdisp->disp_height);
		    }
		  if (gdisp->disp_xoffset)
		    {
		      gdisplay_expose_area (gdisp, 0, 0,
					    gdisp->disp_xoffset,
					    gdisp->disp_height);
		      gdisplay_expose_area (gdisp, gdisp->disp_xoffset + x, 0,
					    gdisp->disp_width,
					    gdisp->disp_height);
		    }
		}
	    }
	  gdisplays_flush ();

	  /*  If the shrink_wrap flag was set  */
	  if (shrink_wrap)
	    {
	      gdisplays_resize_cursor_label (gimage);
	      gdisplays_shrink_wrap (gimage);
	      shrink_wrap = FALSE;
	    }

	  /* let others know that we just popped an action */
	  gimp_image_undo_event (gimage,
				 (state == UNDO)? UNDO_POPPED : UNDO_REDO);

	  return TRUE;
	}
    }

  return FALSE;
}


gboolean
undo_pop (GImage *gimage)
{
  /* Very bad idea to pop an action off the undo stack if we're in the
   * middle of a group, since the whole group won't be popped.  Might
   * leave unbalanced group start marker earlier in the stack too,
   * causing much confusion when it's later reached and
   * mis-interpreted as a group start.  */
  g_return_val_if_fail (gimage->pushing_undo_group == UNDO_NULL, FALSE);

  return pop_stack (gimage, &gimage->undo_stack, &gimage->redo_stack, UNDO);
}


gboolean
undo_redo (GImage *gimage)
{
  /* ditto for redo stack */
  g_return_val_if_fail (gimage->pushing_undo_group == UNDO_NULL, FALSE);

  return pop_stack (gimage, &gimage->redo_stack, &gimage->undo_stack, REDO);
}



/* Return the name of the action that would be used if undo_pop or
 * undo_redo is called, or NULL if there are no actions pushed on the
 * stack.  */
static const gchar *
undo_get_topitem_name (GSList *stack)
{
  Undo *object;

  if (!stack)
    return NULL;

  object = stack->data;

  /* For group boundaries, the type of the boundary marker is the
   * type of the whole group (but each individual action in the
   * group retains its own type so layer/channel unrefs work
   * correctly).
   */

  return undo_type_to_name (object->type);
}

/* Return the type of the top undo action */
static UndoType
undo_get_topitem_type (GSList *stack)
{
  Undo *object;

  if (!stack)
    return UNDO_NULL;

  object = stack->data;

  /* For group boundaries, the type of the boundary marker is the
   * type of the whole group (but each individual action in the
   * group retains its own type so layer/channel unrefs work
   * correctly).
   */

  return object->type;
}


const gchar *
undo_get_undo_name (GImage *gimage)
{
  g_return_val_if_fail (gimage != NULL, NULL);

  /* don't want to encourage undo while a group is open */
  if (gimage->pushing_undo_group != UNDO_NULL)
    return NULL;

  return undo_get_topitem_name (gimage->undo_stack);
}

UndoType
undo_get_undo_top_type (GImage *gimage)
{
  g_return_val_if_fail (gimage != NULL, UNDO_NULL);

  /* don't want to encourage undo while a group is open */
  if (gimage->pushing_undo_group != UNDO_NULL)
    return UNDO_NULL;

  return undo_get_topitem_type (gimage->undo_stack);
}


const gchar *
undo_get_redo_name (GImage *gimage)
{
  g_return_val_if_fail (gimage != NULL, NULL);

  /* don't want to encourage redo while a group is open */
  if (gimage->pushing_undo_group != UNDO_NULL)
    return NULL;

  return undo_get_topitem_name (gimage->redo_stack);
}


static void
undo_map_over_stack (GSList      *stack, 
		     undo_map_fn  fn, 
		     gpointer     data)
{
  gboolean  in_group = FALSE;
  gint      count = 0;
  Undo     *object;

  while (stack)
    {
      object = (Undo *) stack->data;
      if (object->group_boundary)
	in_group = !in_group;

      /* Keep track of group length.  0 means not in group (or group
       * end marker), 1 is group start marker, >= 2 are group
       * members.
       */
      if (in_group)
	count++;
      else
	count = 0;

      /* Is this a group end marker or non-grouped action? */
      if (count == 0)
	{
	  if (fn (undo_type_to_name (object->type), data))
	    return; /* early termination option exercised */
	}

      stack = g_slist_next (stack);
    }
}

void
undo_map_over_undo_stack (GImage      *gimage, 
			  undo_map_fn  fn, 
			  gpointer     data)
{
  /* shouldn't have group open */
  g_return_if_fail (gimage->pushing_undo_group == UNDO_NULL);
  undo_map_over_stack (gimage->undo_stack, fn, data);
}

void
undo_map_over_redo_stack (GImage      *gimage, 
			  undo_map_fn  fn, 
			  gpointer     data)
{
  /* shouldn't have group open */
  g_return_if_fail (gimage->pushing_undo_group == UNDO_NULL);
  undo_map_over_stack (gimage->redo_stack, fn, data);
}


void
undo_free (GImage *gimage)
{
  undo_free_list (gimage, UNDO, gimage->undo_stack);
  undo_free_list (gimage, REDO, gimage->redo_stack);

  gimage->undo_stack  = NULL;
  gimage->redo_stack  = NULL;
  gimage->undo_bytes  = 0;
  gimage->undo_levels = 0;

  /* If the image was dirty, but could become clean by redo-ing
   * some actions, then it should now become 'infinitely' dirty.
   * This is because we've just nuked the actions that would allow
   * the image to become clean again.  The only hope for salvation
   * is to save the image now!  -- austin
   */
  if (gimage->dirty < 0)
    gimage->dirty = 10000;

  /* The same applies to the case where the image would become clean
   * due to undo actions, but since user can't undo without an undo
   * stack, that's not so much a problem.
   */
  gimp_image_undo_event (gimage, UNDO_FREE);
}


/**********************/
/*  group Undo funcs  */

gboolean
undo_push_group_start (GImage    *gimage,
		       UndoType   type)
{
  Undo *boundary_marker;

  if (! gimage->undo_on)
    return FALSE;

  /* Bad idea to push 0 as the group type, since that won't
   * be recognized as the start of the group later.
   */
  g_return_val_if_fail (type != 0, FALSE);

  gimage->group_count ++;
  TRC (("group_start (%s)\n", undo_type_to_name (type)));

  /*  If we're already in a group...ignore  */
  if (gimage->group_count > 1)
    return TRUE;

  if (gimage->redo_stack)
    {
      undo_free_list (gimage, REDO, gimage->redo_stack);
      gimage->redo_stack = NULL;
      /* If the image was dirty, but could become clean by redo-ing
       * some actions, then it should now become 'infinitely' dirty.
       * This is because we've just nuked the actions that would allow
       * the image to become clean again.  The only hope for salvation
       * is to save the image now!  -- austin
       */
      if (gimage->dirty < 0)
	gimage->dirty = 10000;
    }

  if (! undo_free_up_space (gimage))
    return FALSE;

  boundary_marker = undo_new (type, sizeof (Undo), FALSE);
  gimage->undo_bytes += boundary_marker->bytes;
  boundary_marker->group_boundary = TRUE;

  gimage->pushing_undo_group = type;
  gimage->undo_stack = g_slist_prepend (gimage->undo_stack, boundary_marker);
  gimage->undo_levels++;

  return TRUE;
}


gboolean
undo_push_group_end (GImage *gimage)
{
  Undo *boundary_marker;

  if (! gimage->undo_on)
    return FALSE;

  gimage->group_count --;
  TRC (("group end\n"));

  if (gimage->group_count == 0)
    {
      boundary_marker = undo_new (gimage->pushing_undo_group,
				  sizeof (Undo), FALSE);
      boundary_marker->group_boundary = TRUE;
      gimage->undo_stack = g_slist_prepend (gimage->undo_stack,
					    boundary_marker);
      gimage->undo_bytes += boundary_marker->bytes;

      gimage->pushing_undo_group = UNDO_NULL;

      /* Do it here, since undo_push doesn't emit this event while in the
       * middle of a group */
      gimp_image_undo_event (gimage, UNDO_PUSHED);
    }

  return TRUE;
}


/*********************************/
/*  Image Undo functions         */

typedef struct _ImageUndo ImageUndo;

struct _ImageUndo
{
  TileManager  *tiles;
  GimpDrawable *drawable;
  gint          x1, y1, x2, y2;
  gboolean      sparse;
};


gboolean
undo_push_image (GImage       *gimage,
		 GimpDrawable *drawable,
		 gint          x1,
		 gint          y1,
		 gint          x2,
		 gint          y2)
{
  glong        size;
  Undo        *new;
  ImageUndo   *image_undo;
  TileManager *tiles;
  PixelRegion  srcPR, destPR;

  x1 = CLAMP (x1, 0, drawable_width (drawable));
  y1 = CLAMP (y1, 0, drawable_height (drawable));
  x2 = CLAMP (x2, 0, drawable_width (drawable));
  y2 = CLAMP (y2, 0, drawable_height (drawable));

  size = (x2 - x1) * (y2 - y1) * drawable_bytes (drawable) + sizeof (gpointer) * 2;

  if ((new = undo_push (gimage, size, IMAGE_UNDO, TRUE)))
    {
      image_undo = g_new (ImageUndo, 1);

      /*  If we cannot create a new temp buf--either because our parameters are
       *  degenerate or something else failed, simply return an unsuccessful push.
       */
      tiles = tile_manager_new ((x2 - x1), (y2 - y1), drawable_bytes (drawable));
      pixel_region_init (&srcPR, drawable_data (drawable),
			 x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&destPR, tiles,
			 0, 0, (x2 - x1), (y2 - y1), TRUE);
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


gboolean
undo_push_image_mod (GImage       *gimage,
		     GimpDrawable *drawable,
		     gint          x1,
		     gint          y1,
		     gint          x2,
		     gint          y2,
		     gpointer      tiles_ptr,
		     gboolean      sparse)
{
  glong  size;
  gint   dwidth, dheight;
  Undo  *new;
  ImageUndo   *image_undo;
  TileManager *tiles;

  if (! tiles_ptr)
    return FALSE;

  dwidth = drawable_width (drawable);
  dheight = drawable_height (drawable);

  x1 = CLAMP (x1, 0, dwidth);
  y1 = CLAMP (y1, 0, dheight);
  x2 = CLAMP (x2, 0, dwidth);
  y2 = CLAMP (y2, 0, dheight);

  tiles = (TileManager *) tiles_ptr;
  size = tiles->width * tiles->height *
    tiles->bpp + sizeof (gpointer) * 2;

  if ((new = undo_push (gimage, size, IMAGE_MOD_UNDO, TRUE)))
    {
      image_undo = g_new (ImageUndo, 1);
      image_undo->tiles    = tiles;
      image_undo->drawable = drawable;
      image_undo->x1       = x1;
      image_undo->y1       = y1;
      image_undo->x2       = x2;
      image_undo->y2       = y2;
      image_undo->sparse   = sparse;

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


static gboolean
undo_pop_image (GImage    *gimage,
		UndoState  state,
		UndoType   type,
		gpointer   image_undo_ptr)
{
  ImageUndo   *image_undo;
  TileManager *tiles;
  PixelRegion  PR1, PR2;
  gint x, y;
  gint w, h;

  image_undo = (ImageUndo *) image_undo_ptr;
  tiles = image_undo->tiles;

  /*  some useful values  */
  x = image_undo->x1;
  y = image_undo->y1;

  if (image_undo->sparse == FALSE)
    {
      w = tiles->width;
      h = tiles->height;

      pixel_region_init (&PR1, tiles,
			 0, 0, w, h, TRUE);
      pixel_region_init (&PR2, drawable_data (image_undo->drawable),
			 x, y, w, h, TRUE);

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
	      src_tile = tile_manager_get_tile (tiles, j, i, FALSE, FALSE);
	      if (tile_is_valid (src_tile) == TRUE)
		{
		  /* swap tiles, not pixels! */

		  src_tile = tile_manager_get_tile (tiles, j, i, TRUE, FALSE /* TRUE */);
		  dest_tile = tile_manager_get_tile (drawable_data (image_undo->drawable), j, i, TRUE, FALSE /* TRUE */);

		  tile_manager_map_tile (tiles, j, i, dest_tile);
		  tile_manager_map_tile (drawable_data (image_undo->drawable), j, i, src_tile);
#if 0
		  swap_pixels (tile_data_pointer (src_tile, 0, 0),
			       tile_data_pointer (dest_tile, 0, 0),
			       tile_size (src_tile));
#endif
		  
		  tile_release (dest_tile, FALSE /* TRUE */);
		  tile_release (src_tile, FALSE /* TRUE */);
		}
	    }
	}
    }

  drawable_update (image_undo->drawable, x, y, w, h);

  return TRUE;
}


static void
undo_free_image (UndoState  state,
		 UndoType   type,
		 gpointer   image_undo_ptr)
{
  ImageUndo *image_undo;

  image_undo = (ImageUndo *) image_undo_ptr;

  tile_manager_destroy (image_undo->tiles);
  g_free (image_undo);
}


/******************************/
/*  Mask Undo functions  */

gboolean
undo_push_mask (GImage   *gimage,
		gpointer  mask_ptr)
{
  Undo     *new;
  MaskUndo *mask_undo;
  gint      size;

  mask_undo = (MaskUndo *) mask_ptr;

  if (mask_undo->tiles)
    size = mask_undo->tiles->width * mask_undo->tiles->height;
  else
    size = 0;

  if ((new = undo_push (gimage, size, MASK_UNDO, FALSE)))
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


static gboolean
undo_pop_mask (GImage     *gimage,
	       UndoState   state,
	       UndoType    type,
	       gpointer    mask_ptr)
{
  MaskUndo    *mask_undo;
  TileManager *new_tiles;
  Channel     *sel_mask;
  PixelRegion  srcPR, destPR;
  gint   selection;
  gint   x1, y1, x2, y2;
  gint   width, height;
  guchar empty = 0;

  width = height = 0;

  mask_undo = (MaskUndo *) mask_ptr;

  /*  save current selection mask  */
  sel_mask = gimage_get_mask (gimage);
  selection = channel_bounds (sel_mask, &x1, &y1, &x2, &y2);
  pixel_region_init (&srcPR, GIMP_DRAWABLE (sel_mask)->tiles,
		     x1, y1, (x2 - x1), (y2 - y1), FALSE);

  if (selection)
    {
      new_tiles = tile_manager_new (srcPR.w, srcPR.h, 1);
      pixel_region_init (&destPR, new_tiles,
			 0, 0, srcPR.w, srcPR.h, TRUE);

      copy_region (&srcPR, &destPR);

      pixel_region_init (&srcPR, GIMP_DRAWABLE (sel_mask)->tiles,
			 x1, y1, (x2 - x1), (y2 - y1), TRUE);
      color_region (&srcPR, &empty);
    }
  else
    new_tiles = NULL;

  if (mask_undo->tiles)
    {
      width = mask_undo->tiles->width;
      height = mask_undo->tiles->height;
      pixel_region_init (&srcPR, mask_undo->tiles,
			 0, 0, width, height, FALSE);
      pixel_region_init (&destPR, GIMP_DRAWABLE (sel_mask)->tiles,
			 mask_undo->x, mask_undo->y, width, height, TRUE);
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
      sel_mask->x2 = GIMP_DRAWABLE (sel_mask)->width;
      sel_mask->y2 = GIMP_DRAWABLE (sel_mask)->height;
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


static void
undo_free_mask (UndoState  state,
		UndoType   type,
		gpointer   mask_ptr)
{
  MaskUndo *mask_undo;

  mask_undo = (MaskUndo *) mask_ptr;
  if (mask_undo->tiles)
    tile_manager_destroy (mask_undo->tiles);
  g_free (mask_undo);
}


/***************************************/
/*  Layer displacement Undo functions  */

typedef struct _LayerDisplayUndo LayerDisplaceUndo;

struct _LayerDisplayUndo 
{
  gint      info[3];
  PathUndo *path_undo;
};

gboolean
undo_push_layer_displace (GImage    *gimage,
			  GimpLayer *layer)
{
  Undo              *new;
  LayerDisplaceUndo *ldu;

  if ((new = undo_push (gimage, 12, LAYER_DISPLACE_UNDO, TRUE)))
    {
      new->data          = g_new (LayerDisplaceUndo, 1);
      new->pop_func      = undo_pop_layer_displace;
      new->free_func     = undo_free_layer_displace;

      ldu = (LayerDisplaceUndo *) new->data;
      ldu->info[0] = drawable_ID (GIMP_DRAWABLE (layer));
      ldu->info[1] = GIMP_DRAWABLE (layer)->offset_x;
      ldu->info[2] = GIMP_DRAWABLE (layer)->offset_y;
      ldu->path_undo = path_transform_start_undo (gimage);

      return TRUE;
    }
  else
    return FALSE;
}


static gboolean
undo_pop_layer_displace (GImage     *gimage,
			 UndoState   state,
			 UndoType    type,
			 gpointer    info_ptr)
{
  Layer *layer;
  gint   old_offsets[2];
  LayerDisplaceUndo *ldu;

  ldu = (LayerDisplaceUndo *) info_ptr;
  layer = layer_get_ID (ldu->info[0]);
  if (layer)
    {
      old_offsets[0] = GIMP_DRAWABLE (layer)->offset_x;
      old_offsets[1] = GIMP_DRAWABLE (layer)->offset_y;
      drawable_update (GIMP_DRAWABLE (layer), 0, 0,
		       GIMP_DRAWABLE (layer)->width,
		       GIMP_DRAWABLE (layer)->height);

      GIMP_DRAWABLE (layer)->offset_x = ldu->info[1];
      GIMP_DRAWABLE (layer)->offset_y = ldu->info[2];
      drawable_update (GIMP_DRAWABLE (layer), 0, 0,
		       GIMP_DRAWABLE (layer)->width,
		       GIMP_DRAWABLE (layer)->height);
      
      if (layer->mask) 
	{
	  GIMP_DRAWABLE (layer->mask)->offset_x = ldu->info[1];
	  GIMP_DRAWABLE (layer->mask)->offset_y = ldu->info[2];
	  drawable_update (GIMP_DRAWABLE (layer->mask), 0, 0,
			   GIMP_DRAWABLE (layer->mask)->width,
			   GIMP_DRAWABLE (layer->mask)->height);
	}


      /*  invalidate the selection boundary because of a layer modification  */
      layer_invalidate_boundary (layer);

      ldu->info[1] = old_offsets[0];
      ldu->info[2] = old_offsets[1];

      /* Now undo paths bits */
      if (ldu->path_undo)
	path_transform_do_undo (gimage, ldu->path_undo);

      return TRUE;
    }
  else
    return FALSE;
}


static void
undo_free_layer_displace (UndoState  state,
			  UndoType   type,
			  gpointer   info_ptr)
{
  LayerDisplaceUndo * ldu;

  ldu = (LayerDisplaceUndo *) info_ptr;

  /* Free mem held for paths undo stuff */
  if (ldu->path_undo)
    path_transform_free_undo (ldu->path_undo);  

  g_free (info_ptr);
}


/*********************************/
/*  Transform Undo               */

gboolean
undo_push_transform (GImage   *gimage,
		     gpointer  tu_ptr)
{
  Undo *new;

  if ((new = undo_push (gimage, 
			sizeof (TransformUndo), TRANSFORM_UNDO, FALSE)))
    {
      new->data      = tu_ptr;
      new->pop_func  = undo_pop_transform;
      new->free_func = undo_free_transform;

      return TRUE;
    }
  else
    {
      g_free (tu_ptr);
      return FALSE;
    }
}


static gboolean
undo_pop_transform (GImage    *gimage,
		    UndoState  state,
		    UndoType   type,
		    gpointer   tu_ptr)
{
  TransformCore *tc;
  TransformUndo *tu;
  TileManager   *temp;
  gdouble d;
  gint    i;

  /* Can't have ANY tool selected - maybe a plugin running */
  if (active_tool == NULL)
    return TRUE;

  tc = (TransformCore *) active_tool->private;
  tu = (TransformUndo *) tu_ptr;

  path_transform_do_undo (gimage,tu->path_undo);

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

  /*  swap the original buffer--the source buffer for repeated transforms
   */
  temp = tu->original;
  tu->original = tc->original;
  tc->original = temp;

  /*  If we're re-implementing the first transform, reactivate tool  */
  if (state == REDO && tc->original)
    {
      active_tool->state = ACTIVE;
      draw_core_resume (tc->core, active_tool);
    }

  return TRUE;
}


static void
undo_free_transform (UndoState   state,
		     UndoType    type,
		     gpointer    tu_ptr)
{
  TransformUndo * tu;

  tu = (TransformUndo *) tu_ptr;
  if (tu->original)
    tile_manager_destroy (tu->original);
  path_transform_free_undo (tu->path_undo);
  g_free (tu);
}


/*********************************/
/*  Paint Undo                   */

gboolean
undo_push_paint (GImage   *gimage,
		 gpointer  pu_ptr)
{
  Undo *new;

  if ((new = undo_push (gimage, 
			sizeof (PaintUndo), PAINT_UNDO, FALSE)))
    {
      new->data      = pu_ptr;
      new->pop_func  = undo_pop_paint;
      new->free_func = undo_free_paint;

      return TRUE;
    }
  else
    {
      g_free (pu_ptr);
      return FALSE;
    }
}


static gboolean
undo_pop_paint (GImage    *gimage,
		UndoState  state,
		UndoType   type,
		gpointer   pu_ptr)
{
  PaintCore *pc;
  PaintUndo *pu;
  gdouble    tmp;

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


static void
undo_free_paint (UndoState  state,
		 UndoType   type,
		 gpointer   pu_ptr)
{
  PaintUndo *pu;

  pu = (PaintUndo *) pu_ptr;
  g_free (pu);
}


/*********************************/
/*  New Layer Undo               */

gboolean
undo_push_layer (GImage   *gimage,
		 UndoType  type,
		 gpointer  lu_ptr)
{
  LayerUndo *lu;
  Undo      *new;
  gint       size;

  lu = (LayerUndo *) lu_ptr;

  g_return_val_if_fail (type == LAYER_ADD_UNDO ||
			type == LAYER_REMOVE_UNDO,
			FALSE);

  size = layer_size (lu->layer) + sizeof (LayerUndo);

  if ((new = undo_push (gimage, size, type, TRUE)))
    {
      new->data      = lu_ptr;
      new->pop_func  = undo_pop_layer;
      new->free_func = undo_free_layer;

      return TRUE;
    }
  else
    {
      /*  if this is a remove layer, delete the layer  */
      if (type == LAYER_REMOVE_UNDO)
	layer_unref (lu->layer);
      g_free (lu);
      return FALSE;
    }
}


static gboolean
undo_pop_layer (GImage    *gimage,
		UndoState  state,
		UndoType   type,
		gpointer   lu_ptr)
{
  LayerUndo *lu;

  lu = (LayerUndo *) lu_ptr;

  /*  remove layer  */
  if ((state == UNDO && type == LAYER_ADD_UNDO) ||
      (state == REDO && type == LAYER_REMOVE_UNDO))
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

      drawable_update (GIMP_DRAWABLE (lu->layer), 0, 0,
		       GIMP_DRAWABLE (lu->layer)->width,
		       GIMP_DRAWABLE (lu->layer)->height);
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
      gimage->layers = g_slist_insert (gimage->layers, lu->layer,
				       lu->prev_position);
      gimage->layer_stack = g_slist_prepend (gimage->layer_stack, lu->layer);
      gimage->active_layer = lu->layer;

      drawable_update (GIMP_DRAWABLE (lu->layer), 0, 0,
		       GIMP_DRAWABLE (lu->layer)->width,
		       GIMP_DRAWABLE (lu->layer)->height);
    }

  return TRUE;
}


static void
undo_free_layer (UndoState  state,
		 UndoType   type,
		 gpointer   lu_ptr)
{
  LayerUndo *lu;

  lu = (LayerUndo *) lu_ptr;

  /*  Only free the layer if we're freeing from the redo
   *  stack and it's a layer add, or if we're freeing from
   *  the undo stack and it's a layer remove
   */
  if ((state == REDO && type == LAYER_ADD_UNDO) ||
      (state == UNDO && type == LAYER_REMOVE_UNDO))
    layer_unref (lu->layer);

  g_free (lu);
}


/*********************************/
/*  Layer Mod Undo               */

gboolean
undo_push_layer_mod (GImage   *gimage,
		     gpointer  layer_ptr)
{
  Layer       *layer;
  Undo        *new;
  TileManager *tiles;
  gpointer    *data;
  gint         size;

  layer = (Layer *) layer_ptr;

  tiles    =  GIMP_DRAWABLE (layer)->tiles;
  tiles->x =  GIMP_DRAWABLE (layer)->offset_x;
  tiles->y =  GIMP_DRAWABLE (layer)->offset_y;
  size     = (GIMP_DRAWABLE (layer)->width * GIMP_DRAWABLE (layer)->height * 
	      GIMP_DRAWABLE (layer)->bytes + sizeof (gpointer) * 3);

  if ((new = undo_push (gimage, size, LAYER_MOD, TRUE)))
    {
      data           = g_new (gpointer, 3);
      new->data      = data;
      new->pop_func  = undo_pop_layer_mod;
      new->free_func = undo_free_layer_mod;

      data[0] = layer_ptr;
      data[1] = (gpointer) tiles;
      data[2] = (gpointer) ((glong) GIMP_DRAWABLE (layer)->type);

      return TRUE;
    }
  else
    {
      tile_manager_destroy (tiles);
      return FALSE;
    }
}


static gboolean
undo_pop_layer_mod (GImage    *gimage,
		    UndoState  state,
		    UndoType   type,
		    gpointer   data_ptr)
{
  gpointer    *data;
  gint         layer_type;
  TileManager *tiles;
  TileManager *temp;
  Layer       *layer;

  data = (gpointer *) data_ptr;
  layer = (Layer *) data[0];

  tiles = (TileManager *) data[1];

  /*  Issue the first update  */
  gdisplays_update_area (gimage,
			 GIMP_DRAWABLE (layer)->offset_x,
			 GIMP_DRAWABLE (layer)->offset_y,
			 GIMP_DRAWABLE (layer)->width,
			 GIMP_DRAWABLE (layer)->height);

  /*  Create a tile manager to store the current layer contents  */
  temp       = GIMP_DRAWABLE (layer)->tiles;
  temp->x    = GIMP_DRAWABLE (layer)->offset_x;
  temp->y    = GIMP_DRAWABLE (layer)->offset_y;
  layer_type = (glong) data[2];
  data[2]    = (gpointer) ((glong) GIMP_DRAWABLE (layer)->type);

  /*  restore the layer's data  */
  GIMP_DRAWABLE (layer)->tiles     = tiles;
  GIMP_DRAWABLE (layer)->offset_x  = tiles->x;
  GIMP_DRAWABLE (layer)->offset_y  = tiles->y;
  GIMP_DRAWABLE (layer)->width     = tiles->width;
  GIMP_DRAWABLE (layer)->height    = tiles->height;
  GIMP_DRAWABLE (layer)->bytes     = tiles->bpp;
  GIMP_DRAWABLE (layer)->type      = layer_type;
  GIMP_DRAWABLE (layer)->has_alpha = TYPE_HAS_ALPHA (layer_type);

  if (layer->mask) 
    {
      GIMP_DRAWABLE (layer->mask)->offset_x = tiles->x;
      GIMP_DRAWABLE (layer->mask)->offset_y = tiles->y;
    }

  /*  If the layer type changed, update the gdisplay titles  */
  if (GIMP_DRAWABLE (layer)->type != (glong) data[2])
    gdisplays_update_title (GIMP_DRAWABLE (layer)->gimage);

  /*  Set the new tile manager  */
  data[1] = temp;

  /*  Issue the second update  */
  drawable_update (GIMP_DRAWABLE (layer), 0, 0, 
		   GIMP_DRAWABLE (layer)->width,
		   GIMP_DRAWABLE(layer)->height);

  return TRUE;
}


static void
undo_free_layer_mod (UndoState  state,
		     UndoType   type,
		     gpointer   data_ptr)
{
  gpointer *data;

  data = (gpointer *) data_ptr;
  tile_manager_destroy ((TileManager *) data[1]);
  g_free (data);
}


/*********************************/
/*  Layer Mask Undo               */

gboolean
undo_push_layer_mask (GImage   *gimage,
		      UndoType  type,
		      gpointer  lmu_ptr)
{
  LayerMaskUndo *lmu;
  Undo *new;
  gint  size;

  lmu = (LayerMaskUndo *) lmu_ptr;

  g_return_val_if_fail (type == LAYER_MASK_ADD_UNDO ||
			type == LAYER_MASK_REMOVE_UNDO,
			FALSE);

  size = channel_size (GIMP_CHANNEL (lmu->mask)) + sizeof (LayerMaskUndo);

  if ((new = undo_push (gimage, size, type, TRUE)))
    {
      new->data      = lmu_ptr;
      new->pop_func  = undo_pop_layer_mask;
      new->free_func = undo_free_layer_mask;

      return TRUE;
    }
  else
    {
      if (type == LAYER_MASK_REMOVE_UNDO)
	layer_mask_delete (lmu->mask);
      g_free (lmu);
      return FALSE;
    }
}


static gboolean
undo_pop_layer_mask (GImage    *gimage,
		     UndoState  state,
		     UndoType   type,
		     gpointer   lmu_ptr)
{
  LayerMaskUndo *lmu;

  lmu = (LayerMaskUndo *) lmu_ptr;

  /*  remove layer mask  */
  if ((state == UNDO && type == LAYER_MASK_ADD_UNDO) ||
      (state == REDO && type == LAYER_MASK_REMOVE_UNDO))
    {
      /*  remove the layer mask  */
      lmu->layer->mask       = NULL;
      lmu->layer->apply_mask = FALSE;
      lmu->layer->edit_mask  = FALSE;
      lmu->layer->show_mask  = FALSE;

      /*  if this is redoing a remove operation &
       *  the mode of application was DISCARD or
       *  this is undoing an add...
       */
      if ((state == REDO && lmu->mode == DISCARD) || state == UNDO)
	drawable_update (GIMP_DRAWABLE (lmu->layer), 0, 0, 
			 GIMP_DRAWABLE (lmu->layer)->width,
			 GIMP_DRAWABLE (lmu->layer)->height);
    }
  /*  restore layer  */
  else
    {
      lmu->layer->mask       = lmu->mask;
      lmu->layer->apply_mask = lmu->apply_mask;
      lmu->layer->edit_mask  = lmu->edit_mask;
      lmu->layer->show_mask  = lmu->show_mask;

      gimage_set_layer_mask_edit (gimage, lmu->layer, lmu->edit_mask);

      /*  if this is undoing a remove operation &
       *  the mode of application was DISCARD or
       *  this is redoing an add
       */
      if ((state == UNDO && lmu->mode == DISCARD) || state == REDO)
	drawable_update (GIMP_DRAWABLE (lmu->layer), 0, 0, 
			 GIMP_DRAWABLE (lmu->layer)->width,
			 GIMP_DRAWABLE (lmu->layer)->height);
    }

  return TRUE;
}


static void
undo_free_layer_mask (UndoState  state,
		      UndoType   type,
		      gpointer   lmu_ptr)
{
  LayerMaskUndo *lmu;

  lmu = (LayerMaskUndo *) lmu_ptr;

  /*  Only free the layer mask if we're freeing from the redo
   *  stack and it's a layer add, or if we're freeing from
   *  the undo stack and it's a layer remove
   */
  if ((state == REDO && type == LAYER_MASK_ADD_UNDO) ||
      (state == UNDO && type == LAYER_MASK_REMOVE_UNDO))
    layer_mask_delete (lmu->mask);

  g_free (lmu);
}


/*********************************/
/*  New Channel Undo             */

gboolean
undo_push_channel (GImage   *gimage,
		   UndoType  type,
		   gpointer  cu_ptr)
{
  ChannelUndo *cu;
  Undo *new;
  gint  size;

  g_return_val_if_fail (type == CHANNEL_ADD_UNDO ||
			type == CHANNEL_REMOVE_UNDO,
			FALSE);

  cu = (ChannelUndo *) cu_ptr;

  size = channel_size (cu->channel) + sizeof (ChannelUndo);

  if ((new = undo_push (gimage, size, type, TRUE)))
    {
      new->data      = cu_ptr;
      new->pop_func  = undo_pop_channel;
      new->free_func = undo_free_channel;

      return TRUE;
    }
  else
    {
      if (type == CHANNEL_REMOVE_UNDO)
	channel_delete (cu->channel);
      g_free (cu);
      return FALSE;
    }
}


static gboolean
undo_pop_channel (GImage    *gimage,
		  UndoState  state,
		  UndoType   type,
		  gpointer   cu_ptr)
{
  ChannelUndo *cu;

  cu = (ChannelUndo *) cu_ptr;

  /*  remove channel  */
  if ((state == UNDO && type == CHANNEL_ADD_UNDO) ||
      (state == REDO && type == CHANNEL_REMOVE_UNDO))
    {
      /*  record the current position  */
      cu->prev_position = gimage_get_channel_index (gimage, cu->channel);

      /*  remove the channel  */
      gimage->channels = g_slist_remove (gimage->channels, cu->channel);

      /*  set the previous channel  */
      gimage_set_active_channel (gimage, cu->prev_channel);

      /*  update the area  */
      drawable_update (GIMP_DRAWABLE (cu->channel), 0, 0, 
		       GIMP_DRAWABLE (cu->channel)->width,
		       GIMP_DRAWABLE (cu->channel)->height);
    }
  /*  restore channel  */
  else
    {
      /*  record the active channel  */
      cu->prev_channel = gimage->active_channel;

      /*  add the new channel  */
      gimage->channels = g_slist_insert (gimage->channels, cu->channel,
					 cu->prev_position);

      /*  set the new channel  */
      gimage_set_active_channel (gimage, cu->channel);

      /*  update the area  */
      drawable_update (GIMP_DRAWABLE (cu->channel), 0, 0, 
		       GIMP_DRAWABLE (cu->channel)->width,
		       GIMP_DRAWABLE (cu->channel)->height);
    }

  return TRUE;
}


static void
undo_free_channel (UndoState  state,
		   UndoType   type,
		   gpointer   cu_ptr)
{
  ChannelUndo *cu;

  cu = (ChannelUndo *) cu_ptr;

  /*  Only free the channel if we're freeing from the redo
   *  stack and it's a channel add, or if we're freeing from
   *  the undo stack and it's a channel remove
   */
  if ((state == REDO && type == CHANNEL_ADD_UNDO) ||
      (state == UNDO && type == CHANNEL_REMOVE_UNDO))
    channel_delete (cu->channel);

  g_free (cu);
}



/*********************************/
/*  Channel Mod Undo               */

gboolean
undo_push_channel_mod (GImage   *gimage,
		       gpointer  channel_ptr)
{
  Channel     *channel;
  TileManager *tiles;
  Undo        *new;
  gpointer    *data;
  gint         size;

  channel = (Channel *) channel_ptr;

  tiles = GIMP_DRAWABLE (channel)->tiles;
  size  = GIMP_DRAWABLE (channel)->width * GIMP_DRAWABLE (channel)->height +
    sizeof (gpointer) * 2;

  if ((new = undo_push (gimage, size, CHANNEL_MOD, TRUE)))
    {
      data           = g_new (gpointer, 2);
      new->data      = data;
      new->pop_func  = undo_pop_channel_mod;
      new->free_func = undo_free_channel_mod;

      data[0] = channel_ptr;
      data[1] = (gpointer) tiles;

      return TRUE;
    }
  else
    {
      tile_manager_destroy (tiles);
      return FALSE;
    }
}


static gboolean
undo_pop_channel_mod (GImage    *gimage,
		      UndoState  state,
		      UndoType   type,
		      gpointer   data_ptr)
{
  gpointer    *data;
  TileManager *tiles;
  TileManager *temp;
  Channel     *channel;

  data = (gpointer *) data_ptr;
  channel = (Channel *) data[0];

  tiles = (TileManager *) data[1];

  /*  Issue the first update  */
  drawable_update (GIMP_DRAWABLE (channel), 0, 0, 
		   GIMP_DRAWABLE (channel)->width,
		   GIMP_DRAWABLE (channel)->height);

  temp = GIMP_DRAWABLE (channel)->tiles;
  GIMP_DRAWABLE (channel)->tiles = tiles;
  GIMP_DRAWABLE (channel)->width = tiles->width;
  GIMP_DRAWABLE (channel)->height = tiles->height;
  GIMP_CHANNEL (channel)->bounds_known = FALSE; /* #4840. set to False because bounds 
                                                  reflect previous tile set */

  /*  Set the new buffer  */
  data[1] = temp;

  /*  Issue the second update  */
  drawable_update (GIMP_DRAWABLE (channel), 0, 0, 
		   GIMP_DRAWABLE (channel)->width,
		   GIMP_DRAWABLE (channel)->height);

  return TRUE;
}


static void
undo_free_channel_mod (UndoState  state,
		       UndoType   type,
		       gpointer   data_ptr)
{
  gpointer *data;

  data = (gpointer *) data_ptr;
  tile_manager_destroy ((TileManager *) data[1]);
  g_free (data);
}


/**************************************/
/*  Floating Selection to Layer Undo  */

gboolean
undo_push_fs_to_layer (GImage   *gimage,
		       gpointer  fsu_ptr)
{
  FStoLayerUndo *fsu;
  Undo *new;

  fsu = (FStoLayerUndo *) fsu_ptr;

  if ((new = undo_push (gimage, 
			sizeof (FStoLayerUndo), FS_TO_LAYER_UNDO, TRUE)))
    {
      new->data      = fsu_ptr;
      new->pop_func  = undo_pop_fs_to_layer;
      new->free_func = undo_free_fs_to_layer;

      return TRUE;
    }
  else
    {
      tile_manager_destroy (fsu->layer->fs.backing_store);
      g_free (fsu);
      return FALSE;
    }
}


static gboolean
undo_pop_fs_to_layer (GImage    *gimage,
		      UndoState  state,
		      UndoType   type,
		      gpointer   fsu_ptr)
{
  FStoLayerUndo *fsu;

  fsu = (FStoLayerUndo *) fsu_ptr;

  switch (state)
    {
    case UNDO:
      /*  Update the preview for the floating sel  */
      gimp_drawable_invalidate_preview (GIMP_DRAWABLE (fsu->layer), TRUE);

      fsu->layer->fs.drawable = fsu->drawable;
      gimage->active_layer = fsu->layer;
      gimage->floating_sel = fsu->layer;

      /*  restore the contents of the drawable  */
      floating_sel_store (fsu->layer, 
			  GIMP_DRAWABLE (fsu->layer)->offset_x, 
			  GIMP_DRAWABLE (fsu->layer)->offset_y,
			  GIMP_DRAWABLE (fsu->layer)->width, 
			  GIMP_DRAWABLE (fsu->layer)->height);
      fsu->layer->fs.initial = TRUE;

      /*  clear the selection  */
      layer_invalidate_boundary (fsu->layer);

      /*  Update the preview for the gimage and underlying drawable  */
      gimp_drawable_invalidate_preview (GIMP_DRAWABLE (fsu->layer), TRUE);
      break;

    case REDO:
      /*  restore the contents of the drawable  */
      floating_sel_restore (fsu->layer, 
			    GIMP_DRAWABLE (fsu->layer)->offset_x, 
			    GIMP_DRAWABLE (fsu->layer)->offset_y,
			    GIMP_DRAWABLE (fsu->layer)->width, 
			    GIMP_DRAWABLE (fsu->layer)->height);

      /*  Update the preview for the gimage and underlying drawable  */
      gimp_drawable_invalidate_preview (GIMP_DRAWABLE (fsu->layer), TRUE);

      /*  clear the selection  */
      layer_invalidate_boundary (fsu->layer);

      /*  update the pointers  */
      fsu->layer->fs.drawable = NULL;
      gimage->floating_sel = NULL;

      /*  Update the fs drawable  */
      gimp_drawable_invalidate_preview (GIMP_DRAWABLE (fsu->layer), TRUE);
      break;
    }

  return TRUE;
}


static void
undo_free_fs_to_layer (UndoState  state,
		       UndoType   type,
		       gpointer   fsu_ptr)
{
  FStoLayerUndo *fsu;

  fsu = (FStoLayerUndo *) fsu_ptr;

  if (state == UNDO)
    tile_manager_destroy (fsu->layer->fs.backing_store);
  g_free (fsu);
}


/***********************************/
/*  Floating Selection Rigor Undo  */

gboolean
undo_push_fs_rigor (GImage *gimage,
		    gint   layer_ID)
{
  Undo *new;

  if ((new = undo_push (gimage, 
			sizeof (gint32), FS_RIGOR, FALSE)))
    {
      new->data      = g_new (gint32, 1);
      new->pop_func  = undo_pop_fs_rigor;
      new->free_func = undo_free_fs_rigor;

      *((gint32 *) new->data) = layer_ID;

      return TRUE;
    }
  else
    return FALSE;
}


static gboolean
undo_pop_fs_rigor (GImage    *gimage,
		   UndoState  state,
		   UndoType   type,
		   gpointer   layer_ptr)
{
  gint32  layer_ID;
  Layer  *floating_layer;

  layer_ID = *((gint32 *) layer_ptr);

  if ((floating_layer = layer_get_ID (layer_ID)) == NULL)
    return FALSE;

  if (! layer_is_floating_sel (floating_layer))
    return FALSE;

  switch (state)
    {
    case UNDO:
      /*  restore the contents of drawable the floating layer is attached to  */
      if (floating_layer->fs.initial == FALSE)
	floating_sel_restore (floating_layer,
			      GIMP_DRAWABLE (floating_layer)->offset_x, 
			      GIMP_DRAWABLE (floating_layer)->offset_y,
			      GIMP_DRAWABLE (floating_layer)->width, 
			      GIMP_DRAWABLE (floating_layer)->height);
      floating_layer->fs.initial = TRUE;
      break;

    case REDO:
      /*  store the affected area from the drawable in the backing store  */
      floating_sel_store (floating_layer,
			  GIMP_DRAWABLE (floating_layer)->offset_x, 
			  GIMP_DRAWABLE (floating_layer)->offset_y,
			  GIMP_DRAWABLE (floating_layer)->width, 
			  GIMP_DRAWABLE (floating_layer)->height);
      floating_layer->fs.initial = TRUE;
      break;
    }

  return TRUE;
}


static void
undo_free_fs_rigor (UndoState  state,
		    UndoType   type,
		    gpointer   layer_ptr)
{
  g_free (layer_ptr);
}


/***********************************/
/*  Floating Selection Relax Undo  */

gboolean
undo_push_fs_relax (GImage *gimage,
		    gint32  layer_ID)
{
  Undo *new;

  if ((new = undo_push (gimage, 
			sizeof (gint32), FS_RELAX, FALSE)))
    {
      new->data      = g_new (gint32, 1);
      new->pop_func  = undo_pop_fs_relax;
      new->free_func = undo_free_fs_relax;

      *((gint32 *) new->data) = layer_ID;

      return TRUE;
    }
  else
    return FALSE;
}


static gboolean
undo_pop_fs_relax (GImage    *gimage,
		   UndoState  state,
		   UndoType   type,
		   gpointer   layer_ptr)
{
  gint32  layer_ID;
  Layer  *floating_layer;

  layer_ID = *((gint32 *) layer_ptr);

  if ((floating_layer = layer_get_ID (layer_ID)) == NULL)
    return FALSE;

  if (! layer_is_floating_sel (floating_layer))
    return FALSE;

  switch (state)
    {
    case UNDO:
      /*  store the affected area from the drawable in the backing store  */
      floating_sel_store (floating_layer,
			  GIMP_DRAWABLE (floating_layer)->offset_x, 
			  GIMP_DRAWABLE (floating_layer)->offset_y,
			  GIMP_DRAWABLE (floating_layer)->width, 
			  GIMP_DRAWABLE (floating_layer)->height);
      floating_layer->fs.initial = TRUE;
      break;

    case REDO:
      /*  restore the contents of drawable the floating layer is attached to  */
      if (floating_layer->fs.initial == FALSE)
	floating_sel_restore (floating_layer,
			      GIMP_DRAWABLE (floating_layer)->offset_x, 
			      GIMP_DRAWABLE (floating_layer)->offset_y,
			      GIMP_DRAWABLE (floating_layer)->width, 
			      GIMP_DRAWABLE (floating_layer)->height);
      floating_layer->fs.initial = TRUE;
      break;
    }

  return TRUE;
}


static void
undo_free_fs_relax (UndoState  state,
		    UndoType   type,
		    gpointer   layer_ptr)
{
  g_free (layer_ptr);
}


/*********************/
/*  GImage Mod Undo  */

gboolean
undo_push_gimage_mod (GImage *gimage)
{
  Undo *new;
  gint *data;

  if ((new = undo_push (gimage, 
			sizeof (gint) * 3, GIMAGE_MOD, TRUE)))
    {
      data           = g_new (gint, 3);
      new->data      = data;
      new->pop_func  = undo_pop_gimage_mod;
      new->free_func = undo_free_gimage_mod;

      data[0] = gimage->width;
      data[1] = gimage->height;
      data[2] = gimage->base_type;

      return TRUE;
    }

  return FALSE;
}


static gboolean
undo_pop_gimage_mod (GImage    *gimage,
		     UndoState  state,
		     UndoType   type,
		     gpointer   data_ptr)
{
  gint *data;
  gint  tmp;

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

  gimp_image_colormap_changed (gimage, -1);

  if (gimage->width != (int) data[0] || gimage->height != (int) data[1])
    shrink_wrap = TRUE;

  return TRUE;
}


static void
undo_free_gimage_mod (UndoState  state,
		      UndoType   type,
		      gpointer   data_ptr)
{
  g_free (data_ptr);
}

/***********/
/*  Qmask  */

typedef struct _QmaskUndo QmaskUndo;

struct _QmaskUndo
{
  GImage   *gimage;
  gboolean  qmask;
};


gboolean
undo_push_qmask (GImage *gimage)
{
  Undo      *new;
  QmaskUndo *data;

  if ((new = undo_push (gimage, 
			sizeof (QmaskUndo), QMASK_UNDO, TRUE)))
    {
      data = g_new (QmaskUndo, 1);
      new->data      = data;
      new->pop_func  = undo_pop_qmask;
      new->free_func = undo_free_qmask;

      data->gimage = gimage;
      data->qmask  = gimage->qmask_state;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_qmask (GImage    *gimage,
		UndoState  state,
		UndoType   type,
		gpointer   data_ptr)
{
  QmaskUndo *data;
  gint       tmp;

  data = (QmaskUndo *) data_ptr;
  
  tmp = gimage->qmask_state;
  gimage->qmask_state = data->qmask;
  data->qmask = tmp;
  
  /*  make sure the buttons on all displays are updated  */
  gdisplays_flush ();

  return TRUE;
}


static void
undo_free_qmask (UndoState  state,
		 UndoType   type,
		 gpointer   data_ptr)
{
  g_free (data_ptr);
}

/***********/
/*  Guide  */

typedef struct _GuideUndo GuideUndo;

struct _GuideUndo
{
  GImage *gimage;
  Guide  *guide;
  Guide   orig;
};

gboolean
undo_push_guide (GImage   *gimage,
		 gpointer  guide)
{
  Undo      *new;
  GuideUndo *data;

  if ((new = undo_push (gimage, 
			sizeof (GuideUndo), GUIDE_UNDO, TRUE)))
    {
      ((Guide *)(guide))->ref_count++;
      
      data           = g_new (GuideUndo, 1);
      new->data      = data;
      new->pop_func  = undo_pop_guide;
      new->free_func = undo_free_guide;

      data->gimage = gimage;
      data->guide  = guide;
      data->orig   = *(data->guide);

      return TRUE;
    }

  return FALSE;
}


static gboolean
undo_pop_guide (GImage    *gimage,
		UndoState  state,
		UndoType   type,
		gpointer   data_ptr)
{
  GuideUndo *data;
  Guide      tmp;
  gint       tmp_ref;

  data = data_ptr;

  gdisplays_expose_guide (gimage, data->guide);

  tmp_ref = data->guide->ref_count;
  tmp = *(data->guide);
  *(data->guide) = data->orig;
  data->guide->ref_count = tmp_ref;
  data->orig = tmp;

  gdisplays_expose_guide (gimage, data->guide);

  return TRUE;
}


static void
undo_free_guide (UndoState  state,
		 UndoType   type,
		 gpointer   data_ptr)
{
  GuideUndo *data;

  data = data_ptr;

  data->guide->ref_count--;
  if (data->guide->position < 0 && data->guide->ref_count <= 0)
    {
      gimp_image_remove_guide (data->gimage, data->guide);
      g_free (data->guide);
    }
  g_free (data);
}

/****************/
/*  Resolution  */

typedef struct _ResolutionUndo ResolutionUndo;

struct _ResolutionUndo
{
  gdouble  xres;
  gdouble  yres;
  GimpUnit unit;
};

gboolean
undo_push_resolution (GImage *gimage)
{
  Undo           *new;
  ResolutionUndo *data;

  if ((new = undo_push (gimage, 
			sizeof (ResolutionUndo), RESOLUTION_UNDO, TRUE)))
    {
      data           = g_new (ResolutionUndo, 1);
      new->data      = data;
      new->pop_func  = undo_pop_resolution;
      new->free_func = undo_free_resolution;

      data->xres = gimage->xresolution;
      data->yres = gimage->yresolution;
      data->unit = gimage->unit;

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_resolution (GImage    *gimage,
		     UndoState  state,
		     UndoType   type,
		     gpointer   data_ptr)
{
  ResolutionUndo *data;
  gdouble         tmpres;
  GimpUnit        tmpunit;

  data = data_ptr;

  tmpres = gimage->xresolution;
  gimage->xresolution = data->xres;
  data->xres = tmpres;

  tmpres = gimage->yresolution;
  gimage->yresolution = data->yres;
  data->yres = tmpres;

  if (data->unit != gimage->unit)
    {
      tmpunit = gimage->unit;
      gimage->unit = data->unit;
      data->unit = tmpunit;
    }

  /* really just want to recalc size and repaint */
  shrink_wrap = TRUE;

  return TRUE;
}

static void
undo_free_resolution (UndoState  state,
		      UndoType   type,
		      gpointer   data_ptr)
{
  g_free (data_ptr);
}


/************/
/* Parasite */

typedef struct _ParasiteUndo ParasiteUndo;

struct _ParasiteUndo
{
  GImage       *gimage;
  GimpDrawable *drawable;
  GimpParasite *parasite;
  gchar        *name;
};

gboolean
undo_push_image_parasite (GImage   *gimage,
			  gpointer  parasite)
{
  Undo         *new;
  ParasiteUndo *data;

  if ((new = undo_push (gimage, 
			sizeof (ParasiteUndo), PARASITE_ATTACH_UNDO, TRUE)))
    {
      data           = g_new (ParasiteUndo, 1);
      new->data      = data;
      new->pop_func  = undo_pop_parasite;
      new->free_func = undo_free_parasite;

      data->gimage   = gimage;
      data->drawable = NULL;
      data->name     = g_strdup (gimp_parasite_name (parasite));
      data->parasite = gimp_parasite_copy (gimp_image_parasite_find (gimage, data->name));

      return TRUE;
    }

  return FALSE;
}

gboolean
undo_push_image_parasite_remove (GImage      *gimage,
				 const gchar *name)
{
  Undo         *new;
  ParasiteUndo *data;

  if ((new = undo_push (gimage, 
			sizeof (ParasiteUndo), PARASITE_REMOVE_UNDO, TRUE)))
    {
      data           = g_new (ParasiteUndo, 1);
      new->data      = data;
      new->pop_func  = undo_pop_parasite;
      new->free_func = undo_free_parasite;

      data->gimage = gimage;
      data->drawable = NULL;
      data->name     = g_strdup (name);
      data->parasite = gimp_parasite_copy (gimp_image_parasite_find (gimage, data->name));

      return TRUE;
    }

  return FALSE;
}

gboolean
undo_push_drawable_parasite (GImage       *gimage,
			     GimpDrawable *drawable,
			     gpointer      parasite)
{
  Undo         *new;
  ParasiteUndo *data;

  if ((new = undo_push (gimage, 
			sizeof (ParasiteUndo), GIMAGE_MOD, TRUE)))
    {
      data           = g_new (ParasiteUndo, 1);
      new->data      = data;
      new->pop_func  = undo_pop_parasite;
      new->free_func = undo_free_parasite;

      data->gimage   = NULL;
      data->drawable = drawable;
      data->name     = g_strdup (gimp_parasite_name (parasite));
      data->parasite = gimp_parasite_copy (gimp_drawable_parasite_find (drawable, data->name));
      return TRUE;
    }

  return FALSE;
}

gboolean
undo_push_drawable_parasite_remove (GImage       *gimage,
				    GimpDrawable *drawable,
				    const gchar  *name)
{
  Undo         *new;
  ParasiteUndo *data;

  if ((new = undo_push (gimage, 
			sizeof (ParasiteUndo), GIMAGE_MOD, TRUE)))
    {
      data           = g_new (ParasiteUndo, 1);
      new->data      = data;
      new->pop_func  = undo_pop_parasite;
      new->free_func = undo_free_parasite;

      data->gimage   = NULL;
      data->drawable = drawable;
      data->name     = g_strdup (name);
      data->parasite = gimp_parasite_copy (gimp_drawable_parasite_find (drawable, data->name));
      return TRUE;
    }

  return FALSE;
}


static gboolean
undo_pop_parasite (GImage    *gimage,
		   UndoState  state,
		   UndoType   type,
		   gpointer   data_ptr)
{
  ParasiteUndo *data;
  GimpParasite *tmp;

  data = data_ptr;

  tmp = data->parasite;
  
  if (data->gimage)
    {
      data->parasite =
	gimp_parasite_copy (gimp_image_parasite_find (gimage, data->name));

      if (tmp)
	parasite_list_add (data->gimage->parasites, tmp);
      else
	parasite_list_remove (data->gimage->parasites, data->name);
    }
  else if (data->drawable)
    {
      data->parasite =
	gimp_parasite_copy (gimp_drawable_parasite_find (data->drawable,
							 data->name));
      if (tmp)
	parasite_list_add (data->drawable->parasites, tmp);
      else
	parasite_list_remove (data->drawable->parasites, data->name);
    }
  else
    {
      data->parasite = gimp_parasite_copy (gimp_parasite_find (data->name));
      if (tmp)
	gimp_parasite_attach (tmp);
      else
	gimp_parasite_detach (data->name);
    }
    
  if (tmp)
    gimp_parasite_free (tmp);

  return TRUE;
}


static void
undo_free_parasite (UndoState  state,
		    UndoType   type,
		    gpointer   data_ptr)
{
  ParasiteUndo *data;

  data = data_ptr;

  if (data->parasite)
    gimp_parasite_free (data->parasite);
  if (data->name)
    g_free (data->name);

  g_free (data_ptr);
}



/*************/
/* Layer re-position */

typedef struct _LayerRepositionUndo LayerRepositionUndo;

struct _LayerRepositionUndo
{
  GimpLayer *layer;
  gint       old_position;
};

gboolean
undo_push_layer_reposition (GImage    *gimage, 
			    GimpLayer *layer)
{
  Undo                *new;
  LayerRepositionUndo *data;

  if ((new = undo_push (gimage, 
			sizeof (LayerRepositionUndo), LAYER_REPOSITION_UNDO, TRUE)))
    {
      data           = g_new (LayerRepositionUndo, 1);
      new->data      = data;
      new->pop_func  = undo_pop_layer_reposition;
      new->free_func = undo_free_layer_reposition;

      data->layer        = layer;
      data->old_position = g_slist_index (gimage->layers, layer);

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_layer_reposition (GImage    *gimage,
			   UndoState  state,
			   UndoType   type,
			   void      *data_ptr)
{
  LayerRepositionUndo *data = data_ptr;
  gint tmp;

  /* what's the layer's current index? */
  tmp = g_slist_index (gimage->layers, data->layer);

  gimp_image_position_layer (gimage, data->layer, data->old_position, FALSE);

  data->old_position = tmp;

  return TRUE;
}

static void
undo_free_layer_reposition (UndoState  state,
			    UndoType   type,
			    void      *data_ptr)
{
  g_free (data_ptr);
}


/*************/
/* Layer name change */

typedef struct _LayerRenameUndo LayerRenameUndo;

struct _LayerRenameUndo
{
  GimpLayer *layer;
  gchar     *old_name;
};

gboolean
undo_push_layer_rename (GImage    *gimage, 
			GimpLayer *layer)
{
  Undo            *new;
  LayerRenameUndo *data;

  if ((new = undo_push (gimage, 
			sizeof (LayerRenameUndo), LAYER_RENAME_UNDO, TRUE)))
    {
      data           = g_new (LayerRenameUndo, 1);
      new->data      = data;
      new->pop_func  = undo_pop_layer_rename;
      new->free_func = undo_free_layer_rename;

      data->layer    = layer;
      data->old_name = g_strdup (layer_get_name (layer));

      return TRUE;
    }

  return FALSE;
}

static gboolean
undo_pop_layer_rename (GImage    *gimage,
		       UndoState  state,
		       UndoType   type,
		       void      *data_ptr)
{
  LayerRenameUndo *data = data_ptr;
  gchar *tmp;

  tmp = g_strdup (layer_get_name (data->layer));
  layer_set_name (data->layer, data->old_name);
  g_free (data->old_name);
  data->old_name = tmp;

  return TRUE;
}

static void
undo_free_layer_rename (UndoState  state,
			UndoType   type,
			gpointer   data_ptr)
{
  LayerRenameUndo *data = data_ptr;

  g_free (data->old_name);
  g_free (data);
}



/*************/
/* Something for which programmer is too lazy to write an undo 
 * function for.
 */

gboolean
undo_push_cantundo (GImage      *gimage,
		    const gchar *action)
{
  Undo *new;

  /* This is the sole purpose of this type of undo: the ability to
   * mark an image as having been mutated, without really providing
   * any adequate undo facility. */

  new = undo_push (gimage, 0, GIMAGE_MOD, TRUE);
  if (!new)
    return FALSE;

  new->data      = (void*) action;
  new->pop_func  = undo_pop_cantundo;
  new->free_func = undo_free_cantundo;

  return TRUE;
}

static gboolean
undo_pop_cantundo (GImage    *gimage,
		   UndoState  state,
		   UndoType   type,
		   gpointer   data_ptr)
{
  gchar *action = data_ptr;

  switch (state)
    {
    case UNDO:
      g_message (_("Can't undo %s"), action);
      break;

    case REDO:
      break;
    }

  return TRUE;
}


static void
undo_free_cantundo (UndoState  state,
		    UndoType   type,
		    gpointer   data_ptr)
{
  /* nothing to free */
}




/* A "ok" to the name means I've checked where it's used and
   it seems plausible.  -- austin 23/9/99 */
static struct undo_name_t
{
  UndoType     type;
  const gchar *name;
}
undo_name[] =
{
  { UNDO_NULL,     		N_("<<invalid>>") },
  { IMAGE_UNDO,		        N_("image") },
  { IMAGE_MOD_UNDO,		N_("image mod") },
  { MASK_UNDO,			N_("mask") },
  { LAYER_DISPLACE_UNDO,	N_("layer move") }, /* ok */
  { TRANSFORM_UNDO,		N_("transform") },
  { PAINT_UNDO,		        N_("paint") },
  { LAYER_ADD_UNDO,		N_("new layer") },
  { LAYER_REMOVE_UNDO,		N_("delete layer") },
  { LAYER_MOD,			N_("layer mod") },
  { LAYER_MASK_ADD_UNDO,	N_("add layer mask") },  /* ok */
  { LAYER_MASK_REMOVE_UNDO,	N_("delete layer mask") }, /* ok */
  { LAYER_RENAME_UNDO,		N_("rename layer") },
  { LAYER_REPOSITION_UNDO,	N_("layer reposition") }, /* ok */
  { CHANNEL_ADD_UNDO,		N_("new channel") },
  { CHANNEL_REMOVE_UNDO,	N_("delete channel") },
  { CHANNEL_MOD,		N_("channel mod") },
  { FS_TO_LAYER_UNDO,		N_("FS to layer") }, /* ok */
  { GIMAGE_MOD,		        N_("gimage") },
  { FS_RIGOR,			N_("FS rigor") },
  { FS_RELAX,			N_("FS relax") },
  { GUIDE_UNDO,		        N_("guide") },
  { TEXT_UNDO,			N_("text") },
  { FLOAT_MASK_UNDO,		N_("float selection") },
  { EDIT_PASTE_UNDO,		N_("paste") },
  { EDIT_CUT_UNDO,		N_("cut") },
  { TRANSFORM_CORE_UNDO,	N_("transform core") },
  { PAINT_CORE_UNDO,		N_("paint core") },
  { FLOATING_LAYER_UNDO,	N_("floating layer") }, /* unused! */
  { LINKED_LAYER_UNDO,		N_("linked layer") },
  { LAYER_APPLY_MASK_UNDO,	N_("apply layer mask") }, /* ok */
  { LAYER_MERGE_UNDO,		N_("layer merge") },
  { FS_ANCHOR_UNDO,		N_("FS anchor") },
  { GIMAGE_MOD_UNDO,		N_("gimage mod") },
  { CROP_UNDO,			N_("crop") },
  { LAYER_SCALE_UNDO,		N_("layer scale") },
  { LAYER_RESIZE_UNDO,		N_("layer resize") },
  { QMASK_UNDO,		        N_("quickmask") },
  { PARASITE_ATTACH_UNDO,	N_("attach parasite") },
  { PARASITE_REMOVE_UNDO,	N_("remove parasite") },
  { RESOLUTION_UNDO,		N_("resolution change") },
  { IMAGE_SCALE_UNDO,		N_("image scale") },
  { IMAGE_RESIZE_UNDO,		N_("image resize") },
  { MISC_UNDO,			N_("misc") }
};
#define NUM_NAMES (sizeof (undo_name) / sizeof (struct undo_name_t))


static const gchar *
undo_type_to_name (UndoType type)
{
  gint i;

  for (i=0; i < NUM_NAMES; i++)
    if (undo_name[i].type == type)
      return gettext (undo_name[i].name);

  /* no name found */
  return "";
}
