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

#include "appenv.h"
#include "actionarea.h"
#include "cursorutil.h"
#include "drawable.h"
#include "errors.h"
#include "floating_sel.h"
#include "general.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "info_dialog.h"
#include "interface.h"
#include "palette.h"
#include "transform_core.h"
#include "transform_tool.h"
#include "temp_buf.h"
#include "tools.h"
#include "undo.h"
#include "layer_pvt.h"
#include "drawable_pvt.h"
#include "tile_manager_pvt.h"
#include "tile.h"			/* ick. */

#include "libgimp/gimpintl.h"
#include "libgimp/gimpmath.h"

#define BILINEAR(jk,j1k,jk1,j1k1,dx,dy) \
                ((1-dy) * (jk + dx * (j1k - jk)) + \
                    dy  * (jk1 + dx * (j1k1 - jk1)))

/*  variables  */
static TranInfo    old_trans_info;
InfoDialog *       transform_info = NULL;
static gboolean    transform_info_inited = FALSE;

/*  forward function declarations  */
static int         transform_core_bounds      (Tool *, void *);
static void *      transform_core_recalc      (Tool *, void *);
static void        transform_core_doit        (Tool *, gpointer);
static double      cubic                      (double, int, int, int, int);
static void        transform_core_setup_grid  (Tool *);
static void        transform_core_grid_recalc (TransformCore *);

/* Hmmm... Should be in a headerfile but which? */
void          paths_draw_current(GDisplay *,DrawCore *,GimpMatrix);

#define BILINEAR(jk,j1k,jk1,j1k1,dx,dy) \
                ((1-dy) * (jk + dx * (j1k - jk)) + \
		    dy  * (jk1 + dx * (j1k1 - jk1)))

/* access interleaved pixels */
#define CUBIC_ROW(dx, row, step) \
  cubic(dx, (row)[0], (row)[step], (row)[step+step], (row)[step+step+step])
#define CUBIC_SCALED_ROW(dx, row, step, i) \
  cubic(dx, (row)[0] * (row)[i], \
            (row)[step] * (row)[step + i], \
            (row)[step+step]* (row)[step+step + i], \
            (row)[step+step+step] * (row)[step+step+step + i])


#define REF_TILE(i,x,y) \
     tile[i] = tile_manager_get_tile (float_tiles, x, y, TRUE, FALSE); \
     src[i] = tile_data_pointer (tile[i], (x) % TILE_WIDTH, (y) % TILE_HEIGHT);


/* This should be migrated to pixel_region or similar... */
/* PixelSurround describes a (read-only)
 *  region around a pixel in a tile manager
 */

typedef struct _PixelSurround {
  Tile* tile;
  TileManager* mgr;
  unsigned char* buff;
  int buff_size;
  int bpp;
  int w;
  int h;
  unsigned char bg[MAX_CHANNELS];
  int row_stride;
} PixelSurround;

static void pixel_surround_init(PixelSurround * ps, TileManager* t,
    int w, int h, unsigned char bg[MAX_CHANNELS]) {
  int i;
  for (i = 0; i < MAX_CHANNELS; ++i) {
    ps->bg[i] = bg[i];
  }
  ps->tile = 0;
  ps->mgr = t;
  ps->bpp = tile_manager_level_bpp(t);
  ps->w = w;
  ps->h = h;
  /* make sure buffer is big enough */
  ps->buff_size = w * h * ps->bpp;
  ps->buff = g_malloc(ps->buff_size);
  ps->row_stride = 0;
}

/* return a pointer to a buffer which contains all the surrounding pixels */
/* strategy: if we are in the middle of a tile, use the tile storage */
/* otherwise just copy into out own malloced buffer and return that */

static unsigned char* pixel_surround_lock(PixelSurround* ps, int x, int y) {

  int i, j;
  unsigned char* k;
  unsigned char* ptr;

  ps->tile = tile_manager_get_tile(ps->mgr, x, y, TRUE, FALSE);

  i = x % TILE_WIDTH;
  j = y % TILE_HEIGHT;

  /* do we have the whole region? */
  if (ps->tile && (i < (tile_ewidth(ps->tile) - ps->w)) &&
                  (j < (tile_eheight(ps->tile) - ps->h))) {
    ps->row_stride = tile_ewidth(ps->tile) * ps->bpp;
    /* is this really the correct way? */
    return tile_data_pointer(ps->tile, i, j);
  }

  /* nope, do this the hard way (for now) */
  if (ps->tile) {
    tile_release(ps->tile, FALSE);
    ps->tile = 0;
  }

  /* copy pixels, one by one */
  /* no, this is not the best way, but it's much better than before */
  ptr = ps->buff;
  for (j = y; j < y+ps->h; ++j) {
    for (i = x; i < x+ps->w; ++i) {
      Tile* tile = tile_manager_get_tile (ps->mgr, i, j, TRUE, FALSE);
      if (tile) {
        unsigned char* buff = tile_data_pointer (tile, i % TILE_WIDTH, j % TILE_HEIGHT);
        for (k = buff; k < buff+ps->bpp; ++k, ++ptr) {
          *ptr = *k;
	}
        tile_release(tile, FALSE);
      } else {
	for (k = ps->bg; k < ps->bg+ps->bpp; ++k, ++ptr) {
          *ptr = *k;
	}
      }
    }
  }
  ps->row_stride = ps->w * ps->bpp;
  return ps->buff;
}

static int pixel_surround_rowstride(PixelSurround* ps) {
  return ps->row_stride;
}

static void pixel_surround_release(PixelSurround* ps) {
  /* always get new tile (for now), so release the old one */
  if (ps->tile) {
    tile_release(ps->tile, FALSE);
    ps->tile = 0;
  }
}

static void pixel_surround_clear(PixelSurround* ps) {
  if (ps->buff) {
    g_free(ps->buff);
    ps->buff = 0;
    ps->buff_size = 0;
  }
}



static void
transform_ok_callback (GtkWidget *w,
		       gpointer   client_data)
{
  Tool *tool;

  tool = (Tool *) client_data;
  transform_core_doit (tool, tool->gdisp_ptr);
}

static void
transform_reset_callback (GtkWidget *w,
			  gpointer   client_data)
{
  Tool *tool;
  TransformCore *transform_core;
  int i;

  tool = (Tool *) client_data;
  transform_core = (TransformCore *) tool->private;

  /*  stop the current tool drawing process  */
  draw_core_pause (transform_core->core, tool);
  
  /*  Restore the previous transformation info  */
  for (i = 0; i < TRAN_INFO_SIZE; i++)
    transform_core->trans_info [i] = old_trans_info [i];
  /*  recalculate the tool's transformation matrix  */
  transform_core_recalc (tool, tool->gdisp_ptr);
  
  /*  resume drawing the current tool  */
  draw_core_resume (transform_core->core, tool);
}


static ActionAreaItem action_items[] = 
{
  { NULL, transform_ok_callback, NULL, NULL },
  { N_("Reset"), transform_reset_callback, NULL, NULL },
};
static gint n_action_items = sizeof (action_items) / sizeof (action_items[0]);

static const char *action_labels[] =
{
  N_("Rotate"),
  N_("Scale"),
  N_("Shear"),
  N_("Transform")
};

void
transform_core_button_press (Tool           *tool,
			     GdkEventButton *bevent,
			     gpointer        gdisp_ptr)
{
  TransformCore * transform_core;
  GDisplay * gdisp;
  Layer * layer;
  GimpDrawable * drawable;
  int dist;
  int closest_dist;
  int x, y;
  int i;
  int off_x, off_y;

  gdisp = (GDisplay *) gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  transform_core->bpressed = TRUE; /* ALT */

  drawable = gimage_active_drawable (gdisp->gimage);

  if (transform_core->function == CREATING && tool->state == ACTIVE)
    {
      /*  Save the current transformation info  */
      for (i = 0; i < TRAN_INFO_SIZE; i++)
	old_trans_info [i] = transform_core->trans_info [i];
    }

  /*  if we have already displayed the bounding box and handles,
   *  check to make sure that the display which currently owns the
   *  tool is the one which just received the button pressed event
   */
  if ((gdisp == tool->gdisp_ptr) && transform_core->interactive)
    {
      /*  start drawing the bounding box and handles...  */
      draw_core_start (transform_core->core, gdisp->canvas->window, tool);
	    
      x = bevent->x;
      y = bevent->y;

      closest_dist = SQR (x - transform_core->sx1) + SQR (y - transform_core->sy1);
      transform_core->function = HANDLE_1;

      dist = SQR (x - transform_core->sx2) + SQR (y - transform_core->sy2);
      if (dist < closest_dist)
	{
	  closest_dist = dist;
	  transform_core->function = HANDLE_2;
	}

      dist = SQR (x - transform_core->sx3) + SQR (y - transform_core->sy3);
      if (dist < closest_dist)
	{
	  closest_dist = dist;
	  transform_core->function = HANDLE_3;
	}

      dist = SQR (x - transform_core->sx4) + SQR (y - transform_core->sy4);
      if (dist < closest_dist)
	{
	  closest_dist = dist;
	  transform_core->function = HANDLE_4;
	}

      if (tool->type == ROTATE
	  && (SQR (x - transform_core->scx) +
	      SQR (y - transform_core->scy)) <= 100)
	{
	  transform_core->function = HANDLE_CENTER;
	}

      /*  Save the current pointer position  */
      gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
				   &transform_core->startx,
				   &transform_core->starty, TRUE, 0);
      transform_core->lastx = transform_core->startx;
      transform_core->lasty = transform_core->starty;

      gdk_pointer_grab (gdisp->canvas->window, FALSE,
			GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			NULL, NULL, bevent->time);

      tool->state = ACTIVE;
      return;
    }


  /* Initialisation stuff: if the cursor is clicked inside the current
   * selection, show the bounding box and handles...  */
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, FALSE, FALSE);
  if ((layer = gimage_get_active_layer (gdisp->gimage))) 
    {
      drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);
      if (x >= off_x && y >= off_y &&
	  x < (off_x + drawable_width (GIMP_DRAWABLE(layer))) &&
	  y < (off_y + drawable_height (GIMP_DRAWABLE(layer))))
	if (gimage_mask_is_empty (gdisp->gimage) ||
	    gimage_mask_value (gdisp->gimage, x, y))
	  {
	    if (layer->mask != NULL && GIMP_DRAWABLE (layer->mask))
	      {
		g_message (_("Transformations do not work on\nlayers that contain layer masks."));
		tool->state = INACTIVE;
		return;
	      }

	    /* If the tool is already active, clear the current state
             * and reset */
	    if (tool->state == ACTIVE)
	      transform_core_reset (tool, gdisp_ptr);

	    /*  Set the pointer to the active display  */
	    tool->gdisp_ptr = gdisp;
	    tool->drawable = drawable;
	    tool->state = ACTIVE;

	    /*  Grab the pointer if we're in non-interactive mode  */
	    if (!transform_core->interactive)
	      gdk_pointer_grab (gdisp->canvas->window, FALSE,
				(GDK_POINTER_MOTION_HINT_MASK |
				 GDK_BUTTON1_MOTION_MASK |
				 GDK_BUTTON_RELEASE_MASK),
				NULL, NULL, bevent->time);

	    /* Find the transform bounds for some tools (like scale,
	     * perspective) that actually need the bounds for
	     * initializing */
	    transform_core_bounds (tool, gdisp_ptr);

	    /*  Calculate the grid line endpoints  */
	    if (transform_tool_show_grid ())
	      transform_core_setup_grid (tool);

	    /*  Initialize the transform tool  */
	    (* transform_core->trans_func) (tool, gdisp_ptr, INIT);

	    if (transform_info != NULL && !transform_info_inited)
	      {
		action_items[0].label = action_labels[tool->type - ROTATE];
		action_items[0].user_data = tool;
		action_items[1].user_data = tool;
		build_action_area (GTK_DIALOG (transform_info->shell),
				   action_items, n_action_items, 0);
		transform_info_inited = TRUE;
	      }

	    /*  Recalculate the transform tool  */
	    transform_core_recalc (tool, gdisp_ptr);

	    /*  recall this function to find which handle we're dragging  */
	    if (transform_core->interactive)
	      transform_core_button_press (tool, bevent, gdisp_ptr);
	  }
    }
}

void
transform_core_button_release (Tool           *tool,
			       GdkEventButton *bevent,
			       gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  TransformCore *transform_core;
  int i;

  gdisp = (GDisplay *) gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  transform_core->bpressed = FALSE; /* ALT */

  /*  if we are creating, there is nothing to be done...exit  */
  if (transform_core->function == CREATING && transform_core->interactive)
    return;

  /*  release of the pointer grab  */
  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  /*  if the 3rd button isn't pressed, transform the selected mask  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      /* Shift-clicking is another way to approve the transform  */
      if ((bevent->state & GDK_SHIFT_MASK) || (tool->type == FLIP))
	{
	  transform_core_doit (tool, gdisp_ptr);
	}
      else
	{
	  /*  Only update the paths preview */
	  paths_transform_current_path(gdisp->gimage,transform_core->transform,TRUE);
	}
    }
  else
    {
      /*  stop the current tool drawing process  */
      draw_core_pause (transform_core->core, tool);

      /*  Restore the previous transformation info  */
      for (i = 0; i < TRAN_INFO_SIZE; i++)
	transform_core->trans_info [i] = old_trans_info [i];
      /*  recalculate the tool's transformation matrix  */
      transform_core_recalc (tool, gdisp_ptr);

      /*  resume drawing the current tool  */
      draw_core_resume (transform_core->core, tool);

      /* Update the paths preview */
      paths_transform_current_path(gdisp->gimage,transform_core->transform,TRUE);
    }

  /*  if this tool is non-interactive, make it inactive after use  */
  if (!transform_core->interactive)
    tool->state = INACTIVE;
}

void
transform_core_doit (Tool     *tool,
		     gpointer  gdisp_ptr)
{
  GDisplay *gdisp;
  void *pundo;
  TransformCore *transform_core;
  TileManager *new_tiles;
  TransformUndo *tu;
  int new_layer;
  int i, x, y;

  gimp_add_busy_cursors();

  gdisp = (GDisplay *) gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  /* undraw the tool before we muck around with the transform matrix */
  draw_core_pause (transform_core->core, tool);

  /*  We're going to dirty this image, but we want to keep the tool
   *  around
   */
  tool->preserve = TRUE;

  /*  Start a transform undo group  */
  undo_push_group_start (gdisp->gimage, TRANSFORM_CORE_UNDO);

  /*  With the old UI, if original is NULL, then this is the
      first transformation. In the new UI, it is always so, yes?  */
  g_assert (transform_core->original == NULL);

  /* If we're in interactive mode, we need to copy the current
   *  selection to the transform tool's private selection pointer, so
   *  that the original source can be repeatedly modified.
   */
  transform_core->original = transform_core_cut (gdisp->gimage,
						 gimage_active_drawable (gdisp->gimage),
						 &new_layer);

  pundo = paths_transform_start_undo(gdisp->gimage);

  /*  Send the request for the transformation to the tool...
   */
  new_tiles = (* transform_core->trans_func) (tool, gdisp_ptr, FINISH);

  (* transform_core->trans_func) (tool, gdisp_ptr, INIT);
  
  transform_core_recalc (tool, gdisp_ptr);

  if (new_tiles)
    {
      /*  paste the new transformed image to the gimage...also implement
       *  undo...
       */
      transform_core_paste (gdisp->gimage, gimage_active_drawable (gdisp->gimage),
			    new_tiles, new_layer);

      /*  create and initialize the transform_undo structure  */
      tu = (TransformUndo *) g_malloc (sizeof (TransformUndo));
      tu->tool_ID = tool->ID;
      tu->tool_type = tool->type;
      for (i = 0; i < TRAN_INFO_SIZE; i++)
	tu->trans_info[i] = old_trans_info[i];
      tu->original = NULL;
      tu->path_undo = pundo;

      /*  Make a note of the new current drawable (since we may have
       *  a floating selection, etc now.
       */
      tool->drawable = gimage_active_drawable (gdisp->gimage);

      undo_push_transform (gdisp->gimage, (void *) tu);
    }

  /*  push the undo group end  */
  undo_push_group_end (gdisp->gimage);

  /*  We're done dirtying the image, and would like to be restarted
   *  if the image gets dirty while the tool exists
   */
  tool->preserve = FALSE;

  /*  Flush the gdisplays  */
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

  gimp_remove_busy_cursors(NULL);

  gdisplays_flush ();

  transform_core_reset (tool, gdisp_ptr);

  /*  if this tool is non-interactive, make it inactive after use  */
  if (!transform_core->interactive)
    tool->state = INACTIVE;
}


void
transform_core_motion (Tool           *tool,
		       GdkEventMotion *mevent,
		       gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  TransformCore *transform_core;

  gdisp = (GDisplay *) gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  if(transform_core->bpressed == FALSE)
  {
    /*  hey we have not got the button press yet
     *  so go away.
     */
    return;
  }

  /*  if we are creating or this tool is non-interactive, there is
   *  nothing to be done so exit.
   */
  if (transform_core->function == CREATING || !transform_core->interactive)
    return;

  /*  stop the current tool drawing process  */
  draw_core_pause (transform_core->core, tool);

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &transform_core->curx,
			       &transform_core->cury, TRUE, 0);
  transform_core->state = mevent->state;

  /*  recalculate the tool's transformation matrix  */
  (* transform_core->trans_func) (tool, gdisp_ptr, MOTION);

  transform_core->lastx = transform_core->curx;
  transform_core->lasty = transform_core->cury;

  /*  resume drawing the current tool  */
  draw_core_resume (transform_core->core, tool);
}


void
transform_core_cursor_update (Tool           *tool,
			      GdkEventMotion *mevent,
			      gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  TransformCore *transform_core;
  Layer *layer;
  int use_transform_cursor = FALSE;
  GdkCursorType ctype = GDK_TOP_LEFT_ARROW;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, FALSE, FALSE);
  if ((layer = gimage_get_active_layer (gdisp->gimage)))
    if (x >= GIMP_DRAWABLE(layer)->offset_x && y >= GIMP_DRAWABLE(layer)->offset_y &&
	x < (GIMP_DRAWABLE(layer)->offset_x + GIMP_DRAWABLE(layer)->width) &&
	y < (GIMP_DRAWABLE(layer)->offset_y + GIMP_DRAWABLE(layer)->height))
      {
	if (gimage_mask_is_empty (gdisp->gimage) ||
	    gimage_mask_value (gdisp->gimage, x, y))
	  use_transform_cursor = TRUE;
      }

  if (use_transform_cursor)
    /*  ctype based on transform tool type  */
    switch (tool->type)
      {
      case ROTATE: ctype = GDK_EXCHANGE; break;
      case SCALE: ctype = GDK_SIZING; break;
      case SHEAR: ctype = GDK_TCROSS; break;
      case PERSPECTIVE: ctype = GDK_TCROSS; break;
      default: break;
      }

  gdisplay_install_tool_cursor (gdisp, ctype);
}


void
transform_core_control (Tool       *tool,
			ToolAction  action,
			gpointer    gdisp_ptr)
{
  TransformCore * transform_core;

  transform_core = (TransformCore *) tool->private;

  switch (action)
    {
    case PAUSE:
      draw_core_pause (transform_core->core, tool);
      break;

    case RESUME:
      if (transform_core_recalc (tool, gdisp_ptr))
	draw_core_resume (transform_core->core, tool);
      else
	{
	  info_dialog_popdown (transform_info);
	  tool->state = INACTIVE;
	}
      break;

    case HALT:
      transform_core_reset (tool, gdisp_ptr);
      break;

    default:
      break;
    }
}

void
transform_core_no_draw (Tool *tool)
{
  return;
}

void
transform_core_draw (Tool *tool)
{
  int x1, y1, x2, y2, x3, y3, x4, y4;
  TransformCore * transform_core;
  GDisplay * gdisp;
  int srw, srh;
  int i, k, gci;
  int xa, ya, xb, yb;

  gdisp = tool->gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  gdisplay_transform_coords (gdisp, transform_core->tx1, transform_core->ty1,
			     &transform_core->sx1, &transform_core->sy1, 0);
  gdisplay_transform_coords (gdisp, transform_core->tx2, transform_core->ty2,
			     &transform_core->sx2, &transform_core->sy2, 0);
  gdisplay_transform_coords (gdisp, transform_core->tx3, transform_core->ty3,
			     &transform_core->sx3, &transform_core->sy3, 0);
  gdisplay_transform_coords (gdisp, transform_core->tx4, transform_core->ty4,
			     &transform_core->sx4, &transform_core->sy4, 0);

  x1 = transform_core->sx1;  y1 = transform_core->sy1;
  x2 = transform_core->sx2;  y2 = transform_core->sy2;
  x3 = transform_core->sx3;  y3 = transform_core->sy3;
  x4 = transform_core->sx4;  y4 = transform_core->sy4;

  /*  find the handles' width and height  */
  srw = 10;
  srh = 10;

  /*  draw the bounding box  */
  gdk_draw_line (transform_core->core->win, transform_core->core->gc,
		 x1, y1, x2, y2);
  gdk_draw_line (transform_core->core->win, transform_core->core->gc,
		 x2, y2, x4, y4);
  gdk_draw_line (transform_core->core->win, transform_core->core->gc,
		 x3, y3, x4, y4);
  gdk_draw_line (transform_core->core->win, transform_core->core->gc,
		 x3, y3, x1, y1);

  /*  Draw the grid */

  if ((transform_core->grid_coords != NULL) && 
      (transform_core->tgrid_coords != NULL) &&
      ((tool->type != PERSPECTIVE) ||
       ((transform_core->transform[0][0] >=0.0) && 
	(transform_core->transform[1][1] >=0.0))))
    {

      gci = 0;
      k = transform_core->ngx + transform_core->ngy;
      for (i = 0; i < k; i++)
	{
	  gdisplay_transform_coords (gdisp, transform_core->tgrid_coords[gci],
				     transform_core->tgrid_coords[gci+1],
				     &xa, &ya, 0);
	  gdisplay_transform_coords (gdisp, transform_core->tgrid_coords[gci+2],
				     transform_core->tgrid_coords[gci+3],
				     &xb, &yb, 0);
      
	  gdk_draw_line (transform_core->core->win, transform_core->core->gc,
			 xa, ya, xb, yb);
	  gci += 4;
	}
    }

  /*  draw the tool handles  */
  gdk_draw_rectangle (transform_core->core->win, transform_core->core->gc, 0,
		      x1 - (srw >> 1), y1 - (srh >> 1), srw, srh);
  gdk_draw_rectangle (transform_core->core->win, transform_core->core->gc, 0,
		      x2 - (srw >> 1), y2 - (srh >> 1), srw, srh);
  gdk_draw_rectangle (transform_core->core->win, transform_core->core->gc, 0,
		      x3 - (srw >> 1), y3 - (srh >> 1), srw, srh);
  gdk_draw_rectangle (transform_core->core->win, transform_core->core->gc, 0,
		      x4 - (srw >> 1), y4 - (srh >> 1), srw, srh);

  /*  draw the center  */
  if (tool->type == ROTATE)
    {
      gdisplay_transform_coords (gdisp, transform_core->tcx, transform_core->tcy,
				 &transform_core->scx, &transform_core->scy, 0);

      gdk_draw_arc (transform_core->core->win, transform_core->core->gc, 1,
		    transform_core->scx - (srw >> 1),
		    transform_core->scy - (srh >> 1),
		    srw, srh, 0, 23040);
    }

  if(transform_tool_showpath())
    {
      GimpMatrix tmp_matrix;
      if (transform_tool_direction () == TRANSFORM_CORRECTIVE)
	{
	  gimp_matrix_invert (transform_core->transform,tmp_matrix);
	}
      else
	{
	  gimp_matrix_duplicate(transform_core->transform,tmp_matrix);
	}

      paths_draw_current(gdisp,transform_core->core,tmp_matrix);
    }
}

Tool *
transform_core_new (ToolType type,
		    int      interactive)
{
  Tool * tool;
  TransformCore * private;
  int i;

  tool = tools_new_tool (type);
  private = g_new (TransformCore, 1);

  private->interactive = interactive;

  if (interactive)
    private->core = draw_core_new (transform_core_draw);
  else
    private->core = draw_core_new (transform_core_no_draw);

  private->function = CREATING;
  private->original = NULL;

  private->bpressed = FALSE;

  for (i = 0; i < TRAN_INFO_SIZE; i++)
    private->trans_info[i] = 0;

  private->grid_coords = private->tgrid_coords = NULL;

  tool->scroll_lock = TRUE;   /*  Disallow scrolling  */
  tool->preserve    = FALSE;  /*  Don't preserve on drawable change  */

  tool->private = (void *) private;

  tool->button_press_func   = transform_core_button_press;
  tool->button_release_func = transform_core_button_release;
  tool->motion_func         = transform_core_motion;
  tool->cursor_update_func  = transform_core_cursor_update;
  tool->control_func        = transform_core_control;

  return tool;
}

void
transform_core_free (Tool *tool)
{
  TransformCore * transform_core;

  transform_core = (TransformCore *) tool->private;

  /*  Make sure the selection core is not visible  */
  if (tool->state == ACTIVE)
    draw_core_stop (transform_core->core, tool);

  /*  Free the selection core  */
  draw_core_free (transform_core->core);

  /*  Free up the original selection if it exists  */
  if (transform_core->original)
    tile_manager_destroy (transform_core->original);

  /*  If there is an information dialog, free it up  */
  if (transform_info)
    info_dialog_free (transform_info);
  transform_info = NULL;
  transform_info_inited = FALSE;

  /*  Free the grid line endpoint arrays if they exist */ 
  if (transform_core->grid_coords != NULL)
    g_free (transform_core->grid_coords);
  if (transform_core->tgrid_coords != NULL)
    g_free (transform_core->tgrid_coords);

  /*  Finally, free the transform tool itself  */
  g_free (transform_core);
}

void
transform_bounding_box (Tool *tool)
{
  TransformCore * transform_core;
  int i, k;
  int gci;
  GDisplay * gdisp;

  transform_core = (TransformCore *) tool->private;

  gimp_matrix_transform_point (transform_core->transform,
			       transform_core->x1, transform_core->y1,
			       &transform_core->tx1, &transform_core->ty1);
  gimp_matrix_transform_point (transform_core->transform,
			       transform_core->x2, transform_core->y1,
			       &transform_core->tx2, &transform_core->ty2);
  gimp_matrix_transform_point (transform_core->transform,
			       transform_core->x1, transform_core->y2,
			       &transform_core->tx3, &transform_core->ty3);
  gimp_matrix_transform_point (transform_core->transform,
			       transform_core->x2, transform_core->y2,
			       &transform_core->tx4, &transform_core->ty4);

  if (tool->type == ROTATE)
    gimp_matrix_transform_point (transform_core->transform,
				 transform_core->cx, transform_core->cy,
				 &transform_core->tcx, &transform_core->tcy);

  if (transform_core->grid_coords != NULL &&
      transform_core->tgrid_coords != NULL)
    {
      gci = 0;
      k  = (transform_core->ngx + transform_core->ngy) * 2;
      for (i = 0; i < k; i++)
	{
	  gimp_matrix_transform_point (transform_core->transform,
				       transform_core->grid_coords[gci],
				       transform_core->grid_coords[gci+1],
				       &(transform_core->tgrid_coords[gci]),
				       &(transform_core->tgrid_coords[gci+1]));
	  gci += 2;
	}
    }

  gdisp = (GDisplay *) tool->gdisp_ptr;
}

void
transform_core_reset (Tool *tool,
		      void *gdisp_ptr)
{
  TransformCore * transform_core;
  GDisplay * gdisp;

  transform_core = (TransformCore *) tool->private;
  gdisp = (GDisplay *) gdisp_ptr;

  if (transform_core->original)
    tile_manager_destroy (transform_core->original);
  transform_core->original = NULL;

  /*  inactivate the tool  */
  transform_core->function = CREATING;
  draw_core_stop (transform_core->core, tool);
  info_dialog_popdown (transform_info);

  tool->state = INACTIVE;
  tool->gdisp_ptr = NULL;
  tool->drawable = NULL;
}

static int
transform_core_bounds (Tool *tool,
		       void *gdisp_ptr)
{
  GDisplay * gdisp;
  TransformCore * transform_core;
  TileManager * tiles;
  GimpDrawable *drawable;
  int offset_x, offset_y;

  gdisp = (GDisplay *) gdisp_ptr;
  transform_core = (TransformCore *) tool->private;
  tiles = transform_core->original;
  drawable = gimage_active_drawable (gdisp->gimage);

  /*  find the boundaries  */
  if (tiles)
    {
      transform_core->x1 = tiles->x;
      transform_core->y1 = tiles->y;
      transform_core->x2 = tiles->x + tiles->width;
      transform_core->y2 = tiles->y + tiles->height;
    }
  else
    {
      drawable_offsets (drawable, &offset_x, &offset_y);
      drawable_mask_bounds (drawable,
			    &transform_core->x1, &transform_core->y1,
			    &transform_core->x2, &transform_core->y2);
      transform_core->x1 += offset_x;
      transform_core->y1 += offset_y;
      transform_core->x2 += offset_x;
      transform_core->y2 += offset_y;
    }
  transform_core->cx = (transform_core->x1 + transform_core->x2) / 2;
  transform_core->cy = (transform_core->y1 + transform_core->y2) / 2;

  /*  changing the bounds invalidates any grid we may have  */
  transform_core_grid_recalc (transform_core);

  return TRUE;
}

void
transform_core_grid_density_changed ()
{
  TransformCore * transform_core;
  
  transform_core = (TransformCore *) active_tool->private;

  if (transform_core->function == CREATING)
    return;

  draw_core_pause (transform_core->core, active_tool);
  transform_core_grid_recalc (transform_core);
  transform_bounding_box (active_tool);
  draw_core_resume (transform_core->core, active_tool);
}

void
transform_core_showpath_changed (gint type)
{
  TransformCore * transform_core;

  transform_core = (TransformCore *) active_tool->private;

  if (transform_core->function == CREATING)
    return;

  if (type)
    draw_core_pause (transform_core->core, active_tool);
  else
    draw_core_resume (transform_core->core, active_tool);
}

static void
transform_core_grid_recalc (TransformCore *transform_core)
{
  if (transform_core->grid_coords != NULL)
    {
      g_free (transform_core->grid_coords);
      transform_core->grid_coords = NULL;
    }
  if (transform_core->tgrid_coords != NULL)
    {
      g_free (transform_core->tgrid_coords);
      transform_core->tgrid_coords = NULL;
    }
  if (transform_tool_show_grid())
    transform_core_setup_grid (active_tool);
}

static void
transform_core_setup_grid (Tool *tool)
{
  TransformCore * transform_core;
  int i, gci;
  double *coords;

  transform_core = (TransformCore *) tool->private;
      
  /*  We use the transform_tool_grid_size function only here, even
   *  if the user changes the grid size in the middle of an
   *  operation, nothing happens.
   */
  transform_core->ngx =
    (transform_core->x2 - transform_core->x1) / transform_tool_grid_size ();
  if (transform_core->ngx > 0)
    transform_core->ngx--;
  transform_core->ngy =
    (transform_core->y2 - transform_core->y1) / transform_tool_grid_size ();
  if (transform_core->ngy > 0)
    transform_core->ngy--;
  transform_core->grid_coords = coords = (double *)
    g_malloc ((transform_core->ngx + transform_core->ngy) * 4
	      * sizeof(double));
  transform_core->tgrid_coords = (double *)
    g_malloc ((transform_core->ngx + transform_core->ngy) * 4
	      * sizeof(double));
  
  gci = 0;
  for (i = 1; i <= transform_core->ngx; i++)
    {
      coords[gci] = transform_core->x1 +
	((double) i)/(transform_core->ngx + 1) *
	(transform_core->x2 - transform_core->x1);
      coords[gci+1] = transform_core->y1;
      coords[gci+2] = coords[gci];
      coords[gci+3] = transform_core->y2;
      gci += 4;
    }
  for (i = 1; i <= transform_core->ngy; i++)
    {
      coords[gci] = transform_core->x1;
      coords[gci+1] = transform_core->y1 +
	((double) i)/(transform_core->ngy + 1) *
	(transform_core->y2 - transform_core->y1);
      coords[gci+2] = transform_core->x2;
      coords[gci+3] = coords[gci+1];
      gci += 4;
    }
}

static void *
transform_core_recalc (Tool *tool,
		       void *gdisp_ptr)
{
  TransformCore * transform_core;

  transform_core = (TransformCore *) tool->private;

  transform_core_bounds (tool, gdisp_ptr);
  return (* transform_core->trans_func) (tool, gdisp_ptr, RECALC);
}

/*  Actually carry out a transformation  */
TileManager *
transform_core_do (GImage          *gimage,
                   GimpDrawable    *drawable,
                   TileManager     *float_tiles,
                   int              interpolation,
                   GimpMatrix       matrix,
                   progress_func_t  progress_callback,
                   gpointer         progress_data)
{
  PixelRegion destPR;
  TileManager *tiles;
  GimpMatrix m;
  GimpMatrix im;
  int itx, ity;
  int tx1, ty1, tx2, ty2;
  int width, height;
  int alpha;
  int bytes, b;
  int x, y;
  int sx, sy;
  int x1, y1, x2, y2;
  double xinc, yinc, winc;
  double tx, ty, tw;
  double ttx = 0.0, tty = 0.0;
  unsigned char * dest, * d;
  unsigned char * src[16];
  Tile *tile[16];
  unsigned char bg_col[MAX_CHANNELS];
  int i;
  double a_val, a_recip;
  int newval;

  PixelSurround surround;

  alpha = 0;

  /*  turn interpolation off for simple transformations (e.g. rot90)  */
  if (gimp_matrix_is_simple (matrix)
      || interpolation_type == NEAREST_NEIGHBOR_INTERPOLATION)
    interpolation = FALSE;

  /*  Get the background color  */
  gimage_get_background (gimage, drawable, bg_col);

  switch (drawable_type (drawable))
    {
    case RGB_GIMAGE: case RGBA_GIMAGE:
      bg_col[ALPHA_PIX] = TRANSPARENT_OPACITY;
      alpha = 3;
      break;
    case GRAY_GIMAGE: case GRAYA_GIMAGE:
      bg_col[ALPHA_G_PIX] = TRANSPARENT_OPACITY;
      alpha = 1;
      break;
    case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
      bg_col[ALPHA_I_PIX] = TRANSPARENT_OPACITY;
      alpha = 1;
      /*  If the gimage is indexed color, ignore smoothing value  */
      interpolation = 0;
      break;
    }

  if (transform_tool_direction () == TRANSFORM_CORRECTIVE)
    {
      /*  keep the original matrix here, so we dont need to recalculate 
	  the inverse later  */   
      gimp_matrix_duplicate (matrix, m);
      gimp_matrix_invert (matrix, im);
      matrix = im;
    }
  else
    {
      /*  Find the inverse of the transformation matrix  */
      gimp_matrix_invert (matrix, m);
    }

  paths_transform_current_path(gimage,matrix,FALSE);

  x1 = float_tiles->x;
  y1 = float_tiles->y;
  x2 = x1 + float_tiles->width;
  y2 = y1 + float_tiles->height;

  /*  Find the bounding coordinates  */
  if (active_tool && transform_tool_clip ())
    {
      tx1 = x1;
      ty1 = y1;
      tx2 = x2;
      ty2 = y2;
    }
  else
    {
      double dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4;

      gimp_matrix_transform_point (matrix, x1, y1, &dx1, &dy1);
      gimp_matrix_transform_point (matrix, x2, y1, &dx2, &dy2);
      gimp_matrix_transform_point (matrix, x1, y2, &dx3, &dy3);
      gimp_matrix_transform_point (matrix, x2, y2, &dx4, &dy4);

      tx1 = MINIMUM (dx1, dx2);
      tx1 = MINIMUM (tx1, dx3);
      tx1 = MINIMUM (tx1, dx4);
      ty1 = MINIMUM (dy1, dy2);
      ty1 = MINIMUM (ty1, dy3);
      ty1 = MINIMUM (ty1, dy4);
      tx2 = MAXIMUM (dx1, dx2);
      tx2 = MAXIMUM (tx2, dx3);
      tx2 = MAXIMUM (tx2, dx4);
      ty2 = MAXIMUM (dy1, dy2);
      ty2 = MAXIMUM (ty2, dy3);
      ty2 = MAXIMUM (ty2, dy4);
    }

  /*  Get the new temporary buffer for the transformed result  */
  tiles = tile_manager_new ((tx2 - tx1), (ty2 - ty1), float_tiles->bpp);
  pixel_region_init (&destPR, tiles, 0, 0, (tx2 - tx1), (ty2 - ty1), TRUE);
  tiles->x = tx1;
  tiles->y = ty1;

  /* initialise the pixel_surround accessor */
  if (interpolation) {
    if (interpolation_type == CUBIC_INTERPOLATION) {
      pixel_surround_init(&surround, float_tiles, 4, 4, bg_col);
    } else {
      pixel_surround_init(&surround, float_tiles, 2, 2, bg_col);
    }
  } else {
    /* not actually useful, keeps the code cleaner */
    pixel_surround_init(&surround, float_tiles, 1, 1, bg_col);
  }

  width = tiles->width;
  height = tiles->height;
  bytes = tiles->bpp;

  dest = (unsigned char *) g_malloc (width * bytes);

  xinc = m[0][0];
  yinc = m[1][0];
  winc = m[2][0];

  /* these loops could be rearranged, depending on which bit of code
   * you'd most like to write more than once.
   */

  for (y = ty1; y < ty2; y++)
    {
      if (progress_callback && !(y & 0xf))
	(*progress_callback)(ty1, ty2, y, progress_data);

      /* set up inverse transform steps */
      tx = xinc * tx1 + m[0][1] * y + m[0][2];
      ty = yinc * tx1 + m[1][1] * y + m[1][2];
      tw = winc * tx1 + m[2][1] * y + m[2][2];

      d = dest;
      for (x = tx1; x < tx2; x++)
	{
	  /*  normalize homogeneous coords  */
	  if (tw == 0.0)
	    g_message (_("homogeneous coordinate = 0...\n"));
	  else if (tw != 1.0)
	    {
	      ttx = tx / tw;
	      tty = ty / tw;
	    }
	  else
	    {
	      ttx = tx;
	      tty = ty;
	    }

          /*  Set the destination pixels  */

          if (interpolation)
       	    {

              if (interpolation_type == CUBIC_INTERPOLATION)
       	        {

                  /*  ttx & tty are the subpixel coordinates of the point in the original
                   *  selection's floating buffer.  We need the four integer pixel coords
	           *  around them: itx to itx + 3, ity to ity + 3
                   */
                  itx = floor(ttx);
                  ity = floor(tty);

		  /* check if any part of our region overlaps the buffer */

                  if ((itx + 2) >= x1 && (itx - 1) < x2 &&
                      (ity + 2) >= y1 && (ity - 1) < y2 )
                    {
                      unsigned char* data;
                      int row;
                      double dx, dy;
                      unsigned char* start;

		      /* lock the pixel surround */
                      data = pixel_surround_lock(&surround, itx - 1 - x1, ity - 1 - y1);

                      row = pixel_surround_rowstride(&surround);

                      /* the fractional error */
                      dx = ttx - itx;
                      dy = tty - ity;

		      /* calculate alpha of result */
                      start = &data[alpha];
                      a_val = cubic (dy,
		                     CUBIC_ROW (dx, start, bytes),
		                     CUBIC_ROW (dx, start + row, bytes),
				     CUBIC_ROW (dx, start + row + row, bytes),
				     CUBIC_ROW (dx, start + row + row + row, bytes));

		      if (a_val <= 0.0)
			{
			  a_recip = 0.0;
                          d[alpha] = 0;
			}
                      else if (a_val > 255.0)
			{
		  	  a_recip = 1.0 / a_val;
                          d[alpha] = 255;
			}
		      else
			{
			  a_recip = 1.0 / a_val;
                          d[alpha] = RINT(a_val);
			}

                      /* for colour channels c, 
                       * result = bicubic(c * alpha) / bicubic(alpha)
	               */
                      for (i = -alpha; i < 0; ++i) {
                        start = &data[alpha];
                        newval = RINT(a_recip * cubic (dy,
	                             CUBIC_SCALED_ROW (dx, start, bytes, i),
		                     CUBIC_SCALED_ROW (dx, start + row, bytes, i),
		                     CUBIC_SCALED_ROW (dx, start + row + row, bytes, i),
		     	             CUBIC_SCALED_ROW (dx, start + row + row + row, bytes, i)));
                        if (newval <= 0) {
                          *d++ = 0;
                        } else if (newval > 255) {
                          *d++ = 255;
		        } else {
                          *d++ = newval;
		        }
		      }

		      d++;

                      pixel_surround_release(&surround);

		    }
                  else /* not in source range */
                    {
                      /*  increment the destination pointers  */
                      for (b = 0; b < bytes; b++)
                        *d++ = bg_col[b];
                    }

                }

       	      else  /*  linear  */
                {

                  itx = floor(ttx);
                  ity = floor(tty);

		  /* expand source area to cover interpolation region */
		  /* (which runs from itx to itx + 1, same in y) */
                  if ((itx + 1) >= x1 && itx < x2 &&
                      (ity + 1) >= y1 && ity < y2 )
                    {
                      unsigned char* data;
                      int row;
                      double dx, dy;
                      unsigned char* chan;

		      /* lock the pixel surround */
                      data = pixel_surround_lock(&surround, itx - x1, ity - y1);

                      row = pixel_surround_rowstride(&surround);

                      /* the fractional error */
                      dx = ttx - itx;
                      dy = tty - ity;

		      /* calculate alpha value of result pixel */
                      chan = &data[alpha];
                      a_val = BILINEAR (chan[0], chan[bytes], chan[row], chan[row+bytes], dx, dy);
                      if (a_val <= 0.0) {
                        a_recip = 0.0;
                        d[alpha] = 0.0;
		      } else if (a_val >= 255.0) {
                        a_recip = 1.0 / a_val;
                        d[alpha] = 255;
		      } else {
                        a_recip = 1.0 / a_val;
                        d[alpha] = RINT(a_val);
		      }

		      /* for colour channels c,
		       * result = bilinear(c * alpha) / bilinear(alpha)
		       */
                      for (i = -alpha; i < 0; ++i) {
                        chan = &data[alpha];
                        newval = RINT(a_recip * 
                          BILINEAR (chan[0] * chan[i],
                                    chan[bytes] * chan[bytes+i],
                                    chan[row] * chan[row+i],
                                    chan[row+bytes] * chan[row+bytes+i], dx, dy));
                        if (newval <= 0) {
                          *d++ = 0;
			} else if (newval > 255) {
                          *d++ = 255;
			} else {
                          *d++ = newval;
			}
		      }
		      /* already set alpha */
                      d++;

                      pixel_surround_release(&surround);
		    }

                  else /* not in source range */
                    {
                      /*  increment the destination pointers  */
                      for (b = 0; b < bytes; b++)
                        *d++ = bg_col[b];
                    }
		}
	    }
          else  /*  no interpolation  */
            {
              itx = RINT(ttx);
              ity = RINT(tty);

              if (itx >= x1 && itx < x2 &&
                  ity >= y1 && ity < y2 )
                {
                  /*  x, y coordinates into source tiles  */
                  sx = itx - x1;
                  sy = ity - y1;

                  REF_TILE (0, sx, sy);

                  for (b = 0; b < bytes; b++)
                    *d++ = src[0][b];

                  tile_release (tile[0], FALSE);
		}
              else /* not in source range */
                {
                  /*  increment the destination pointers  */
                  for (b = 0; b < bytes; b++)
                    *d++ = bg_col[b];
                }
	    }
	  /*  increment the transformed coordinates  */
	  tx += xinc;
	  ty += yinc;
	  tw += winc;
	}

      /*  set the pixel region row  */
      pixel_region_set_row (&destPR, 0, (y - ty1), width, dest);
    }

  pixel_surround_clear(&surround);

  g_free (dest);
  return tiles;
}


TileManager *
transform_core_cut (GImage       *gimage,
		    GimpDrawable *drawable,
		    int          *new_layer)
{
  TileManager *tiles;

  /*  extract the selected mask if there is a selection  */
  if (! gimage_mask_is_empty (gimage))
    {
      tiles = gimage_mask_extract (gimage, drawable, TRUE, TRUE);
      *new_layer = TRUE;
    }
  /*  otherwise, just copy the layer  */
  else
    {
      tiles = gimage_mask_extract (gimage, drawable, FALSE, TRUE);
      *new_layer = FALSE;
    }

  return tiles;
}


/*  Paste a transform to the gdisplay  */
Layer *
transform_core_paste (GImage       *gimage,
		      GimpDrawable *drawable,
		      TileManager  *tiles,
		      int           new_layer)
{
  Layer * layer;
  Layer * floating_layer;

  if (new_layer)
    {
      layer = layer_from_tiles (gimage, drawable, tiles, _("Transformation"),
				OPAQUE_OPACITY, NORMAL_MODE);
      GIMP_DRAWABLE(layer)->offset_x = tiles->x;
      GIMP_DRAWABLE(layer)->offset_y = tiles->y;

      /*  Start a group undo  */
      undo_push_group_start (gimage, EDIT_PASTE_UNDO);

      floating_sel_attach (layer, drawable);

      /*  End the group undo  */
      undo_push_group_end (gimage);

      /*  Free the tiles  */
      tile_manager_destroy (tiles);

      return layer;
    }
  else
    {
      if (GIMP_IS_LAYER(drawable))
	layer=GIMP_LAYER(drawable);
      else
	return NULL;

      layer_add_alpha (layer);
      floating_layer = gimage_floating_sel (gimage);

      if (floating_layer)
	floating_sel_relax (floating_layer, TRUE);

      gdisplays_update_area (gimage,
			     GIMP_DRAWABLE(layer)->offset_x, GIMP_DRAWABLE(layer)->offset_y,
			     GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height);

      /*  Push an undo  */
      undo_push_layer_mod (gimage, layer);

      /*  set the current layer's data  */
      GIMP_DRAWABLE(layer)->tiles = tiles;

      /*  Fill in the new layer's attributes  */
      GIMP_DRAWABLE(layer)->width = tiles->width;
      GIMP_DRAWABLE(layer)->height = tiles->height;
      GIMP_DRAWABLE(layer)->bytes = tiles->bpp;
      GIMP_DRAWABLE(layer)->offset_x = tiles->x;
      GIMP_DRAWABLE(layer)->offset_y = tiles->y;

      if (floating_layer)
	floating_sel_rigor (floating_layer, TRUE);

      drawable_update (GIMP_DRAWABLE(layer), 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height);

      /* if we were operating on the floating selection, then it's boundary 
       * and previews need invalidating */
      if (layer == floating_layer)
	floating_sel_invalidate (floating_layer);

      return layer;
    }
}

/* Note: cubic function no longer clips result */
static double
cubic (double dx,
       int    jm1,
       int    j,
       int    jp1,
       int    jp2)
{
  double result;

#if 0
  /* Equivalent to Gimp 1.1.1 and earlier - some ringing */
  result = ((( ( - jm1 + j - jp1 + jp2 ) * dx +
               ( jm1 + jm1 - j - j + jp1 - jp2 ) ) * dx +
               ( - jm1 + jp1 ) ) * dx + j );
  /* Recommended by Mitchell and Netravali - too blurred? */
  result = ((( ( - 7 * jm1 + 21 * j - 21 * jp1 + 7 * jp2 ) * dx +
               ( 15 * jm1 - 36 * j + 27 * jp1 - 6 * jp2 ) ) * dx +
               ( - 9 * jm1 + 9 * jp1 ) ) * dx + (jm1 + 16 * j + jp1) ) / 18.0;
#else

  /* Catmull-Rom - not bad */
  result = ((( ( - jm1 + 3 * j - 3 * jp1 + jp2 ) * dx +
               ( 2 * jm1 - 5 * j + 4 * jp1 - jp2 ) ) * dx +
               ( - jm1 + jp1 ) ) * dx + (j + j) ) / 2.0;

#endif

  return result;
}


