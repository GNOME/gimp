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
#include <math.h>
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

#define SQR(x)  ((x) * (x))

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif  /*  M_PI  */

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
void               paths_draw_current         (GDisplay *, DrawCore *,
					       GimpMatrix);

#define REF_TILE(i,x,y) \
     tile[i] = tile_manager_get_tile (float_tiles, x, y, TRUE, FALSE); \
     src[i] = tile_data_pointer (tile[i], (x) % TILE_WIDTH, (y) % TILE_HEIGHT);


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
  double src_a[16][MAX_CHANNELS];
  Tile *tile[16];
  int a[16];
  unsigned char bg_col[MAX_CHANNELS];
  int i;
  double a_val, a_mult, a_recip;
  int newval;

  alpha = 0;

  /*  turn interpolation off for simple transformations (e.g. rot90)  */
  if (gimp_matrix_is_simple (matrix))
    interpolation = FALSE;

  /*  Get the background color  */
  /*  Check - background colour only works for non-interpolated transforms? */
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

  width = tiles->width;
  height = tiles->height;
  bytes = tiles->bpp;

  dest = (unsigned char *) g_malloc (width * bytes);

  xinc = m[0][0];
  yinc = m[1][0];
  winc = m[2][0];

  for (y = ty1; y < ty2; y++)
    {
      if (progress_callback && !(y & 0xf))
        (*progress_callback)(ty1, ty2, y, progress_data);

      /*  When we calculate the inverse transformation, we should transform
       *  the center of each destination pixel...
       */

      /* Actually, no. Relabel them in your head, if you must  */

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
              /*  ttx & tty are the coordinates of the point in the original
               *  selection's floating buffer.  Make sure they're within bounds
               */
              itx = floor(ttx);
              ity = floor(tty);

              if (cubic_interpolation)
                {

                  /* expand source area to cover interpolation region */
                  /* (which runs from itx - 1 to itx + 2, same in y) */
                  if ((itx + 2) >= x1 && (itx - 1) < x2 &&
                      (ity + 2) >= y1 && (ity - 1) < y2 )
                    {
                      /*  to hold clipping info */
                      int use_x[4], use_y[4];
                      /* to hold index */
                      int index_x[4], index_y[4];
                      int xy_i;

                      /*  get the fractional error  */
                      double dx, dy;
                      dx = ttx - itx;
                      dy = tty - ity;

                      /* setup indices and clip flags */
                      /* now correctly handles very small images (why?) */

                      for (xy_i = 0; xy_i < 4; ++xy_i) 
                        {
                          use_x[xy_i] = 1;
                          use_y[xy_i] = 1;
                          index_x[xy_i] = (itx - 1) - x1 + xy_i;
                          index_y[xy_i] = (ity - 1) - y1 + xy_i;
                        }

                      /* ugly, ugly code */

                      /* clip x to left hand edge */
                      if ((itx - 1) < x1) {
                        use_x[0] = 0;
                        index_x[0] = 0;
                        if (itx < x1) {
                          use_x[1] = 0;
                          index_x[1] = 0;
                          if ((itx + 1) < x1) {
                            use_x[2] = 0;
                            index_x[2] = 0;
                          }
                        }
                      }
                      /* clip x to right hand edge */
                      if ((itx + 2) >= x2) {
                        int xlimit = x2 - 1 - x1;
                        use_x[3] = 0;
                        index_x[3] = xlimit;
                        if ((itx + 1) >= x2) {
                          use_x[2] = 0;
                          index_x[2] = xlimit;
                          if (itx >= x2) {
                            use_x[1] = 0;
                            index_x[1] = xlimit;
                          }
                        }
                      }
                      /* clip y to top edge */
                      if ((ity - 1) < y1) {
                        use_y[0] = 0;
                        index_y[0] = 0;
                        if (ity < y1) {
                          use_y[1] = 0;
                          index_y[1] = 0;
                          if ((ity + 1) < y1) {
                            use_y[2] = 0;
                            index_y[2] = 0;
                          }
                        }
                      }
                      /* clip y to bottom edge */
                      if ((ity + 2) >= y2) {
                        int ylimit = y2 - 1 - y1;
                        use_y[3] = 0;
                        index_y[3] = ylimit;
                        if ((ity + 1) >= y2) {
                          use_y[2] = 0;
                          index_y[2] = ylimit;
                          if (ity >= y2) {
                            use_y[1] = 0;
                            index_y[1] = ylimit;
                          }
                        }
                      }


                      REF_TILE (0, index_x[0], index_y[0]);
                      REF_TILE (1, index_x[1], index_y[0]);
                      REF_TILE (2, index_x[2], index_y[0]);
                      REF_TILE (3, index_x[3], index_y[0]);
                      REF_TILE (4, index_x[0], index_y[1]);
                      REF_TILE (5, index_x[1], index_y[1]);
                      REF_TILE (6, index_x[2], index_y[1]);
                      REF_TILE (7, index_x[3], index_y[1]);
                      REF_TILE (8, index_x[0], index_y[2]);
                      REF_TILE (9, index_x[1], index_y[2]);
                      REF_TILE (10, index_x[2], index_y[2]);
                      REF_TILE (11, index_x[3], index_y[2]);
                      REF_TILE (12, index_x[0], index_y[3]);
                      REF_TILE (13, index_x[1], index_y[3]);
                      REF_TILE (14, index_x[2], index_y[3]);
                      REF_TILE (15, index_x[3], index_y[3]);

                      a[0] = (use_x[0] && use_y[0]) ? src[0][alpha] : 0;
                      a[1] = (use_x[1] && use_y[0]) ? src[1][alpha] : 0;
                      a[2] = (use_x[2] && use_y[0]) ? src[2][alpha] : 0;
                      a[3] = (use_x[3] && use_y[0]) ? src[3][alpha] : 0;

                      a[4] = (use_x[0] && use_y[1]) ? src[4][alpha] : 0;
                      a[5] = (use_x[1] && use_y[1]) ? src[5][alpha] : 0;
                      a[6] = (use_x[2] && use_y[1]) ? src[6][alpha] : 0;
                      a[7] = (use_x[3] && use_y[1]) ? src[7][alpha] : 0;

                      a[8] = (use_x[0] && use_y[2]) ? src[8][alpha] : 0;
                      a[9] = (use_x[1] && use_y[2]) ? src[9][alpha] : 0;
                      a[10] = (use_x[2] && use_y[2]) ? src[10][alpha] : 0;
                      a[11] = (use_x[3] && use_y[2]) ? src[11][alpha] : 0;

                      a[12] = (use_x[0] && use_y[3]) ? src[12][alpha] : 0;
                      a[13] = (use_x[1] && use_y[3]) ? src[13][alpha] : 0;
                      a[14] = (use_x[2] && use_y[3]) ? src[14][alpha] : 0;
                      a[15] = (use_x[3] && use_y[3]) ? src[15][alpha] : 0;

                      a_val = cubic (dy,
                                    cubic (dx, a[0], a[1], a[2], a[3]),
                                    cubic (dx, a[4], a[5], a[6], a[7]),
                                    cubic (dx, a[8], a[9], a[10], a[11]),
                                    cubic (dx, a[12], a[13], a[14], a[15]));

                      if (a_val != 0)
                        a_recip = 255.0 / a_val;
                      else
                        a_recip = 0.0;

                      /* premultiply the alpha */
                      for (i = 0; i < 16; i++)
                        {
                          a_mult = a[i] * (1.0 / 255.0);
                          for (b = 0; b < alpha; b++)
                            src_a[i][b] = src[i][b] * a_mult;
                        }

                      for (b = 0; b < alpha; b++)
                        {
                          newval =
                            a_recip *
                            cubic (dy,
                                   cubic (dx, src_a[0][b], src_a[1][b], src_a[2][b], src_a[3][b]),
                                   cubic (dx, src_a[4][b], src_a[5][b], src_a[6][b], src_a[7][b]),
                                   cubic (dx, src_a[8][b], src_a[9][b], src_a[10][b], src_a[11][b]),
                                   cubic (dx, src_a[12][b], src_a[13][b], src_a[14][b], src_a[15][b]));
                          if ((newval & 0x100) == 0)
                            *d++ = newval;
                          else if (newval < 0)
                            *d++ = 0;
                          else
                            *d++ = 255;
                        }

                      *d++ = a_val;

                      for (b = 0; b < 16; b++)
                        tile_release (tile[b], FALSE);
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

                  /* expand source area to cover interpolation region */
                  /* (which runs from itx to itx + 1, same in y) */
                  if ((itx + 1) >= x1 && itx < x2 &&
                      (ity + 1) >= y1 && ity < y2 )
                    {
                      /*  to hold clipping info */
                      int use_x[2], use_y[2];
                      /* to hold index */
                      int index_x[2], index_y[2];

                      /*  get the fractional error  */
                      double dx, dy;
                      dx = ttx - itx;
                      dy = tty - ity;

                      /* setup indices and clip flags */
                      /* now correctly handles very small images (why?) */

                      /*  x, y coordinates into source tiles  */
                      sx = itx - x1;
                      sy = ity - y1;

                      use_x[0] = (itx >= x1);
                      index_x[0] = use_x[0] ? sx : 0;

                      use_x[1] = ((itx + 1) < x2);
                      index_x[1] = use_x[1] ? sx + 1 : x2 - 1 - x1;

                      use_y[0] = (ity >= y1);
                      index_y[0] = use_y[0] ? sy : 0;

                      use_y[1] = ((ity + 1) < y2);
                      index_y[1] = use_y[1] ? sy + 1 : y2 - 1 - y1;

                      REF_TILE (0, index_x[0], index_y[0]);
                      REF_TILE (1, index_x[1], index_y[0]);
                      REF_TILE (2, index_x[0], index_y[1]);
                      REF_TILE (3, index_x[1], index_y[1]);

                      /*  Need special treatment for the alpha channel  */
                      a[0] = (use_x[0] && use_y[0]) ? src[0][alpha] : 0;
                      a[1] = (use_x[1] && use_y[0]) ? src[1][alpha] : 0;
                      a[2] = (use_x[0] && use_y[1]) ? src[2][alpha] : 0;
                      a[3] = (use_x[1] && use_y[1]) ? src[3][alpha] : 0;

                      /*  The alpha channel  */
                      a_val = BILINEAR (a[0], a[1], a[2], a[3], dx, dy);

                      if (a_val != 0)
                        a_recip = 255.0 / a_val;
                      else
                        a_recip = 0.0;

                      /* premultiply the alpha */
                      for (i = 0; i < 4; i++)
                        {
                          a_mult = a[i] * (1.0 / 255.0);
                          for (b = 0; b < alpha; b++)
                            src_a[i][b] = src[i][b] * a_mult;
                        }

                      for (b = 0; b < alpha; b++)
                        *d++ = a_recip * BILINEAR (src_a[0][b], src_a[1][b], src_a[2][b], src_a[3][b], dx, dy);

                      *d++ = a_val;

                      for (b = 0; b < 4; b++)
                        tile_release (tile[b], FALSE);
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
              itx = rint(ttx);
              ity = rint(tty);

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
  if (result < 0.0)
    result = 0.0;
  if (result > 255.0)
    result = 255.0;

  return result;
}
