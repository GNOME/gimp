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
#include "canvas.h"
#include "drawable.h"
#include "errors.h"
#include "floating_sel.h"
#include "general.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "gimprc.h"
#include "info_dialog.h"
#include "interface.h"
#include "layers_dialog.h"
#include "paint_funcs_area.h"
#include "palette.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "transform_core.h"
#include "transform_tool.h"
#include "tools.h"
#include "undo.h"

#include "layer_pvt.h"
#include "drawable_pvt.h"

#define    SQR(x) ((x) * (x))

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif  /*  M_PI  */

/*  variables  */
static TranInfo    old_trans_info;
InfoDialog *       transform_info = NULL;

/*  forward function declarations  */
static int         transform_core_bounds  (Tool *, void *);
static void *      transform_core_recalc  (Tool *, void *);
static double      cubic                  (double, int, int, int, int);


#define BILINEAR(jk,j1k,jk1,j1k1,dx,dy) \
                ((1-dy) * ((1-dx)*jk + dx*j1k) + \
		    dy  * ((1-dx)*jk1 + dx*j1k1))

#define REF_TILE(i,x,y) \
     tile[i] = tile_manager_get_tile (float_tiles, x, y, 0); \
     tile_ref (tile[i]); \
     src[i] = tile[i]->data + tile[i]->bpp * (tile[i]->ewidth * ((y) % TILE_HEIGHT) + ((x) % TILE_WIDTH));


void
transform_core_button_press (tool, bevent, gdisp_ptr)
     Tool *tool;
     GdkEventButton *bevent;
     gpointer gdisp_ptr;
{
  TransformCore * transform_core;
  GDisplay * gdisp;
  Layer * layer;
  int dist;
  int closest_dist;
  int x, y;
  int i;
  int off_x, off_y;

  gdisp = (GDisplay *) gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  transform_core->bpressed = TRUE; /* ALT */

  tool->drawable = gimage_active_drawable (gdisp->gimage);

  /*  Save the current transformation info  */
  for (i = 0; i < TRAN_INFO_SIZE; i++)
    old_trans_info [i] = transform_core->trans_info [i];

  /*  if we have already displayed the bounding box and handles,
   *  check to make sure that the display which currently owns the
   *  tool is the one which just received the button pressed event
   */
  if ((transform_core->function >= CREATING) && (gdisp_ptr == tool->gdisp_ptr) &&
      transform_core->interactive)
    {
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

  /*  if the cursor is clicked inside the current selection, show the
   *  bounding box and handles...
   */
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, FALSE, FALSE);
  if ((layer = gimage_get_active_layer (gdisp->gimage))) 
    {
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      if (x >= off_x && y >= off_y &&
	  x < (off_x + drawable_width (GIMP_DRAWABLE(layer))) &&
	  y < (off_y + drawable_height (GIMP_DRAWABLE(layer))))
	if (gimage_mask_is_empty (gdisp->gimage) ||
	    gimage_mask_value (gdisp->gimage, x, y))
	  {
	    if (layer->mask != NULL && GIMP_DRAWABLE(layer->mask))
	      {
		g_message ("Transformations do not work on\nlayers that contain layer masks.");
		tool->state = INACTIVE;
		return;
	      }
	    
	    /*  If the tool is already active, clear the current state and reset  */
	    if (tool->state == ACTIVE)
	      transform_core_reset (tool, gdisp_ptr);
	    
	    /*  Set the pointer to the gdisplay that owns this tool  */
	    tool->gdisp_ptr = gdisp_ptr;
	    tool->state = ACTIVE;
	    
	    /*  Grab the pointer if we're in non-interactive mode  */
	    if (!transform_core->interactive)
	      gdk_pointer_grab (gdisp->canvas->window, FALSE,
				(GDK_POINTER_MOTION_HINT_MASK |
				 GDK_BUTTON1_MOTION_MASK |
				 GDK_BUTTON_RELEASE_MASK),
				NULL, NULL, bevent->time);
	    
	    /*  Find the transform bounds for some tools (like scale, perspective)
	     *  that actually need the bounds for initializing
	     */
	    transform_core_bounds (tool, gdisp_ptr);
	    
	    /*  Initialize the transform tool  */
	    (* transform_core->trans_func) (tool, gdisp_ptr, INIT);
	    
	    /*  Recalculate the transform tool  */
	    transform_core_recalc (tool, gdisp_ptr);
	    
	    /*  start drawing the bounding box and handles...  */
	    draw_core_start (transform_core->core, gdisp->canvas->window, tool);
	    
	    /*  recall this function to find which handle we're dragging  */
	    if (transform_core->interactive)
	      transform_core_button_press (tool, bevent, gdisp_ptr);
	  }
    }
}

void
transform_core_button_release (tool, bevent, gdisp_ptr)
     Tool *tool;
     GdkEventButton *bevent;
     gpointer gdisp_ptr;
{
  GDisplay *gdisp;
  TransformCore *transform_core;
  Canvas *new_tiles;
  TransformUndo *tu;
  int first_transform;
  int new_layer;
  int i, x, y;

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
      /*  We're going to dirty this image, but we want to keep the tool
	  around
      */

      tool->preserve = TRUE;

      /*  Start a transform undo group  */
      undo_push_group_start (gdisp->gimage, TRANSFORM_CORE_UNDO);

      /*  If original is NULL, then this is the first transformation  */
      first_transform = (transform_core->original) ? FALSE : TRUE;

      /*  If we're in interactive mode, and haven't yet done any
       *  transformations, we need to copy the current selection to
       *  the transform tool's private selection pointer, so that the
       *  original source can be repeatedly modified.
       */
      if (first_transform) 
	transform_core->original = transform_core_cut (gdisp->gimage,
						       gimage_active_drawable (gdisp->gimage),
						       &new_layer);
      else
	new_layer = FALSE;

      /*  Send the request for the transformation to the tool...
       */
      new_tiles = (* transform_core->trans_func) (tool, gdisp_ptr, FINISH);

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
	  tu->first = first_transform;
	  tu->original = NULL;

	  /* Make a note of the new current drawable (since we may have
	     a floating selection, etc now. */
	  
	  tool->drawable = gimage_active_drawable (gdisp->gimage);

	  undo_push_transform (gdisp->gimage, (void *) tu);
	}

      /*  push the undo group end  */
      undo_push_group_end (gdisp->gimage);

      /*  We're done dirtying the image, and would like to be restarted
	  if the image gets dirty while the tool exists
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
      gdisplays_flush ();
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
    }

  /*  if this tool is non-interactive, make it inactive after use  */
  if (!transform_core->interactive)
    tool->state = INACTIVE;
}

void
transform_core_motion (tool, mevent, gdisp_ptr)
     Tool *tool;
     GdkEventMotion *mevent;
     gpointer gdisp_ptr;
{
  GDisplay *gdisp;
  TransformCore *transform_core;


  gdisp = (GDisplay *) gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  if(transform_core->bpressed == FALSE)
  {
    /* hey we have not got the button press yet
     * so go away.
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
transform_core_cursor_update (tool, mevent, gdisp_ptr)
     Tool *tool;
     GdkEventMotion *mevent;
     gpointer gdisp_ptr;
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
      case FLIP_HORZ: ctype = GDK_SB_H_DOUBLE_ARROW; break;
      case FLIP_VERT: ctype = GDK_SB_V_DOUBLE_ARROW; break;
      default: break;
      }

  gdisplay_install_tool_cursor (gdisp, ctype);
}


void
transform_core_control (tool, action, gdisp_ptr)
     Tool *tool;
     int action;
     gpointer gdisp_ptr;
{
  TransformCore * transform_core;

  transform_core = (TransformCore *) tool->private;

  switch (action)
    {
    case PAUSE :
      draw_core_pause (transform_core->core, tool);
      break;
    case RESUME :
      if (transform_core_recalc (tool, gdisp_ptr))
	draw_core_resume (transform_core->core, tool);
      else
	{
	  info_dialog_popdown (transform_info);
	  tool->state = INACTIVE;
	}
      break;
    case HALT :
      transform_core_reset (tool, gdisp_ptr);
      break;
    }
}

void
transform_core_no_draw (tool)
     Tool * tool;
{
  return;
}

void
transform_core_draw (tool)
     Tool * tool;
{
  int x1, y1, x2, y2, x3, y3, x4, y4;
  TransformCore * transform_core;
  GDisplay * gdisp;
  int srw, srh;

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

  /*  draw the tool handles  */
  gdk_draw_rectangle (transform_core->core->win, transform_core->core->gc, 0,
		      x1 - (srw >> 1), y1 - (srh >> 1), srw, srh);
  gdk_draw_rectangle (transform_core->core->win, transform_core->core->gc, 0,
		      x2 - (srw >> 1), y2 - (srh >> 1), srw, srh);
  gdk_draw_rectangle (transform_core->core->win, transform_core->core->gc, 0,
		      x3 - (srw >> 1), y3 - (srh >> 1), srw, srh);
  gdk_draw_rectangle (transform_core->core->win, transform_core->core->gc, 0,
		      x4 - (srw >> 1), y4 - (srh >> 1), srw, srh);
}

Tool *
transform_core_new (type, interactive)
     int type;
     int interactive;
{
  Tool * tool;
  TransformCore * private;
  int i;

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (TransformCore *) g_malloc (sizeof (TransformCore));

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

  tool->type = type;
  tool->state = INACTIVE;
  tool->scroll_lock = 1;    /*  Do not allow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;

  tool->preserve = FALSE;   /*  Destroy when the image is dirtied. */

  tool->button_press_func = transform_core_button_press;
  tool->button_release_func = transform_core_button_release;
  tool->motion_func = transform_core_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = transform_core_cursor_update;
  tool->control_func = transform_core_control;

  return tool;
}

void
transform_core_free (tool)
     Tool *tool;
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
    canvas_delete (transform_core->original);

  /*  If there is an information dialog, free it up  */
  if (transform_info)
    info_dialog_free (transform_info);
  transform_info = NULL;

  /*  Finally, free the transform tool itself  */
  g_free (transform_core);
}

void
transform_bounding_box (tool)
     Tool * tool;
{
  TransformCore * transform_core;

  transform_core = (TransformCore *) tool->private;

  transform_point (transform_core->transform,
		   transform_core->x1, transform_core->y1,
		   &transform_core->tx1, &transform_core->ty1);
  transform_point (transform_core->transform,
		   transform_core->x2, transform_core->y1,
		   &transform_core->tx2, &transform_core->ty2);
  transform_point (transform_core->transform,
		   transform_core->x1, transform_core->y2,
		   &transform_core->tx3, &transform_core->ty3);
  transform_point (transform_core->transform,
		   transform_core->x2, transform_core->y2,
		   &transform_core->tx4, &transform_core->ty4);
}

void
transform_point (m, x, y, nx, ny)
     Matrix m;
     double x, y;
     double *nx, *ny;
{
  double xx, yy, ww;

  xx = m[0][0] * x + m[0][1] * y + m[0][2];
  yy = m[1][0] * x + m[1][1] * y + m[1][2];
  ww = m[2][0] * x + m[2][1] * y + m[2][2];

  if (!ww)
    ww = 1.0;

  *nx = xx / ww;
  *ny = yy / ww;
}

void
mult_matrix (m1, m2)
     Matrix m1, m2;
{
  Matrix result;
  int i, j, k;

  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      {
	result [i][j] = 0.0;
	for (k = 0; k < 3; k++)
	  result [i][j] += m1 [i][k] * m2[k][j];
      }

  /*  copy the result into matrix 2  */
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      m2 [i][j] = result [i][j];
}

void
identity_matrix (m)
     Matrix m;
{
  int i, j;

  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      m[i][j] = (i == j) ? 1 : 0;

}

void
translate_matrix (m, x, y)
     Matrix m;
     double x, y;
{
  Matrix trans;

  identity_matrix (trans);
  trans[0][2] = x;
  trans[1][2] = y;
  mult_matrix (trans, m);
}

void
scale_matrix (m, x, y)
     Matrix m;
     double x, y;
{
  Matrix scale;

  identity_matrix (scale);
  scale[0][0] = x;
  scale[1][1] = y;
  mult_matrix (scale, m);
}

void
rotate_matrix (m, theta)
     Matrix m;
     double theta;
{
  Matrix rotate;
  double cos_theta, sin_theta;

  cos_theta = cos (theta);
  sin_theta = sin (theta);

  identity_matrix (rotate);
  rotate[0][0] = cos_theta;
  rotate[0][1] = -sin_theta;
  rotate[1][0] = sin_theta;
  rotate[1][1] = cos_theta;
  mult_matrix (rotate, m);
}

void
xshear_matrix (m, shear)
     Matrix m;
     double shear;
{
  Matrix shear_m;

  identity_matrix (shear_m);
  shear_m[0][1] = shear;
  mult_matrix (shear_m, m);
}

void
yshear_matrix (m, shear)
     Matrix m;
     double shear;
{
  Matrix shear_m;

  identity_matrix (shear_m);
  shear_m[1][0] = shear;
  mult_matrix (shear_m, m);
}

/*  find the determinate for a 3x3 matrix  */
static double
determinate (Matrix m)
{
  int i;
  double det = 0;

  for (i = 0; i < 3; i ++)
    {
      det += m[0][i] * m[1][(i+1)%3] * m[2][(i+2)%3];
      det -= m[2][i] * m[1][(i+1)%3] * m[0][(i+2)%3];
    }

  return det;
}

/*  find the cofactor matrix of a matrix  */
static void
cofactor (Matrix m, Matrix m_cof)
{
  int i, j;
  int x1, y1;
  int x2, y2;

  x1 = y1 = x2 = y2 = 0;

  for (i = 0; i < 3; i++)
    {
      switch (i)
	{
	case 0 : y1 = 1; y2 = 2; break;
	case 1 : y1 = 0; y2 = 2; break;
	case 2 : y1 = 0; y2 = 1; break;
	}
      for (j = 0; j < 3; j++)
	{
	  switch (j)
	    {
	    case 0 : x1 = 1; x2 = 2; break;
	    case 1 : x1 = 0; x2 = 2; break;
	    case 2 : x1 = 0; x2 = 1; break;
	    }
	  m_cof[i][j] = (m[x1][y1] * m[x2][y2] - m[x1][y2] * m[x2][y1]) *
	    (((i+j) % 2) ? -1 : 1);
	}
    }
}

/*  find the inverse of a 3x3 matrix  */
static void
invert (Matrix m, Matrix m_inv)
{
  double det = determinate (m);
  int i, j;

  if (det == 0.0)
    return;

  /*  Find the cofactor matrix of m, store it in m_inv  */
  cofactor (m, m_inv);

  /*  divide by the determinate  */
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      m_inv[i][j] = m_inv[i][j] / det;
}

void
transform_core_reset(tool, gdisp_ptr)
     Tool * tool;
     void * gdisp_ptr;
{
  TransformCore * transform_core;
  GDisplay * gdisp;

  transform_core = (TransformCore *) tool->private;
  gdisp = (GDisplay *) gdisp_ptr;

  if (transform_core->original)
    canvas_delete (transform_core->original);
  transform_core->original = NULL;

  /*  inactivate the tool  */
  transform_core->function = CREATING;
  draw_core_stop (transform_core->core, tool);
  info_dialog_popdown (transform_info);
  tool->state = INACTIVE;
}

static int
transform_core_bounds (tool, gdisp_ptr)
     Tool *tool;
     void *gdisp_ptr;
{
  GDisplay * gdisp;
  TransformCore * transform_core;
  Canvas * tiles;
  GimpDrawable *drawable;
  int offset_x, offset_y;

  gdisp = (GDisplay *) gdisp_ptr;
  transform_core = (TransformCore *) tool->private;
  tiles = transform_core->original;
  drawable = gimage_active_drawable (gdisp->gimage);

  /*  find the boundaries  */
  if (tiles)
    {
      transform_core->x1 = canvas_fixme_getx (tiles);
      transform_core->y1 = canvas_fixme_gety (tiles);
      transform_core->x2 = canvas_fixme_getx (tiles) + canvas_width (tiles);
      transform_core->y2 = canvas_fixme_gety (tiles) + canvas_height (tiles);
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

  return TRUE;
}

static void *
transform_core_recalc (tool, gdisp_ptr)
     Tool * tool;
     void * gdisp_ptr;
{
  TransformCore * transform_core;

  transform_core = (TransformCore *) tool->private;

  transform_core_bounds (tool, gdisp_ptr);
  return (* transform_core->trans_func) (tool, gdisp_ptr, RECALC);
}

/*  Actually carry out a transformation  */
Canvas *
transform_core_do (gimage, drawable, float_tiles, interpolation, matrix)
     GImage *gimage;
     GimpDrawable *drawable;
     Canvas *float_tiles;
     int interpolation;
     Matrix matrix;
{
#if 0
  PixelArea destPR;
  Canvas *tiles;
  Matrix m;
  int itx, ity;
  int tx1, ty1, tx2, ty2;
  int width, height;
  int alpha;
  int bytes, b;
  int x, y;
  int sx, sy;
  int plus_x, plus_y;
  int plus2_x, plus2_y;
  int minus_x, minus_y;
  int x1, y1, x2, y2;
  double dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4;
  double xinc, yinc, winc;
  double tx, ty, tw;
  double ttx = 0.0, tty = 0.0;
  double dx = 0.0, dy = 0.0;
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

  /*  Find the inverse of the transformation matrix  */
  invert (matrix, m);

  x1 = float_tiles->x;
  y1 = float_tiles->y;
  x2 = x1 + float_tiles->levels[0].width;
  y2 = y1 + float_tiles->levels[0].height;

  transform_point (matrix, x1, y1, &dx1, &dy1);
  transform_point (matrix, x2, y1, &dx2, &dy2);
  transform_point (matrix, x1, y2, &dx3, &dy3);
  transform_point (matrix, x2, y2, &dx4, &dy4);

  /*  Find the bounding coordinates  */
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

  /*  Get the new temporary buffer for the transformed result  */
  tiles = tile_manager_new ((tx2 - tx1), (ty2 - ty1), float_tiles->levels[0].bpp);
  pixelarea_init (&destPR, tiles, 0, 0, (tx2 - tx1), (ty2 - ty1), TRUE);
  tiles->x = tx1;
  tiles->y = ty1;

  width = tiles->levels[0].width;
  height = tiles->levels[0].height;
  bytes = tiles->levels[0].bpp;

  dest = (unsigned char *) g_malloc (width * bytes);

  xinc = m[0][0];
  yinc = m[1][0];
  winc = m[2][0];

  for (y = ty1; y < ty2; y++)
    {
      /*  When we calculate the inverse transformation, we should transform
       *  the center of each destination pixel...
       */
      tx = xinc * (tx1 + 0.5) + m[0][1] * (y + 0.5) + m[0][2];
      ty = yinc * (tx1 + 0.5) + m[1][1] * (y + 0.5) + m[1][2];
      tw = winc * (tx1 + 0.5) + m[2][1] * (y + 0.5) + m[2][2];
      d = dest;
      for (x = tx1; x < tx2; x++)
	{
	  /*  normalize homogeneous coords  */
	  if (tw == 0.0)
	    g_message ("homogeneous coordinate = 0...\n");
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

	  /*  tx & ty are the coordinates of the point in the original
	   *  selection's floating buffer.  Make sure they're within bounds
	   */
	  if (ttx < 0)
	    itx = (int) (ttx - 0.999999);
	  else
	    itx = (int) ttx;

	  if (tty < 0)
	    ity = (int) (tty - 0.999999);
	  else
	    ity = (int) tty;

	  /*  if interpolation is on, get the fractional error  */
	  if (interpolation)
	    {
	      dx = ttx - itx;
	      dy = tty - ity;
	    }

	  if (itx >= x1 && itx < x2 && ity >= y1 && ity < y2)
	    {
	      /*  x, y coordinates into source tiles  */
	      sx = itx - x1;
	      sy = ity - y1;

	      /*  Set the destination pixels  */
	      if (interpolation)
		{
		  plus_x = (itx < (x2 - 1)) ? 1 : 0;
		  plus_y = (ity < (y2 - 1)) ? 1 : 0;

		  if (cubic_interpolation)
		    {
		      minus_x = (itx > x1) ? -1 : 0;
		      plus2_x = ((itx + 1) < (x2 - 1)) ? 2 : plus_x;

		      minus_y = (ity > y1) ? -1 : 0;
		      plus2_y = ((ity + 1) < (y2 - 1)) ? 2 : plus_y;

		      REF_TILE (0, sx + minus_x, sy + minus_y);
		      REF_TILE (1, sx, sy + minus_y);
		      REF_TILE (2, sx + plus_x, sy + minus_y);
		      REF_TILE (3, sx + plus2_x, sy + minus_y);
		      REF_TILE (4, sx + minus_x, sy);
		      REF_TILE (5, sx, sy);
		      REF_TILE (6, sx + plus_x, sy);
		      REF_TILE (7, sx + plus2_x, sy);
		      REF_TILE (8, sx + minus_x, sy + plus_y);
		      REF_TILE (9, sx, sy + plus_y);
		      REF_TILE (10, sx + plus_x, sy + plus_y);
		      REF_TILE (11, sx + plus2_x, sy + plus_y);
		      REF_TILE (12, sx + minus_x, sy + plus2_y);
		      REF_TILE (13, sx, sy + plus2_y);
		      REF_TILE (14, sx + plus_x, sy + plus2_y);
		      REF_TILE (15, sx + plus2_x, sy + plus2_y);

		      a[0] = (minus_y * minus_x) ? src[0][alpha] : 0;
		      a[1] = (minus_y) ? src[1][alpha] : 0;
		      a[2] = (minus_y * plus_x) ? src[2][alpha] : 0;
		      a[3] = (minus_y * plus2_x) ? src[3][alpha] : 0;

		      a[4] = (minus_x) ? src[4][alpha] : 0;
		      a[5] = src[5][alpha];
		      a[6] = (plus_x) ? src[6][alpha] : 0;
		      a[7] = (plus2_x) ? src[7][alpha] : 0;

		      a[8] = (plus_y * minus_x) ? src[8][alpha] : 0;
		      a[9] = (plus_y) ? src[9][alpha] : 0;
		      a[10] = (plus_y * plus_x) ? src[10][alpha] : 0;
		      a[11] = (plus_y * plus2_x) ? src[11][alpha] : 0;

		      a[12] = (plus2_y * minus_x) ? src[12][alpha] : 0;
		      a[13] = (plus2_y) ? src[13][alpha] : 0;
		      a[14] = (plus2_y * plus_x) ? src[14][alpha] : 0;
		      a[15] = (plus2_y * plus2_x) ? src[15][alpha] : 0;

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
			tile_unref (tile[b], FALSE);
		    }
		  else  /*  linear  */
		    {
		      REF_TILE (0, sx, sy);
		      REF_TILE (1, sx + plus_x, sy);
		      REF_TILE (2, sx, sy + plus_y);
		      REF_TILE (3, sx + plus_x, sy + plus_y);

		      /*  Need special treatment for the alpha channel  */
		      if (plus_x == 0 && plus_y == 0)
			{
			  a[0] = src[0][alpha];
			  a[1] = a[2] = a[3] = 0;
			}
		      else if (plus_x == 0)
			{
			  a[0] = src[0][alpha];
			  a[2] = src[2][alpha];
			  a[1] = a[3] = 0;
			}
		      else if (plus_y == 0)
			{
			  a[0] = src[0][alpha];
			  a[1] = src[1][alpha];
			  a[2] = a[3] = 0;
			}
		      else
			{
			  a[0] = src[0][alpha];
			  a[1] = src[1][alpha];
			  a[2] = src[2][alpha];
			  a[3] = src[3][alpha];
			}

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
			tile_unref (tile[b], FALSE);
		    }
		}
	      else  /*  no interpolation  */
		{
		  REF_TILE (0, sx, sy);

		  for (b = 0; b < bytes; b++)
		    *d++ = src[0][b];

		  tile_unref (tile[0], FALSE);
		}
	    }
	  else
	    {
	      /*  increment the destination pointers  */
	      for (b = 0; b < bytes; b++)
		*d++ = bg_col[b];
	    }
	  /*  increment the transformed coordinates  */
	  tx += xinc;
	  ty += yinc;
	  tw += winc;
	}

      /*  set the pixel region row  */
      pixelarea_write_row (&destPR, 0, (y - ty1), width, dest);
    }

  g_free (dest);
  return tiles;
#endif
  return NULL;
}


Canvas *
transform_core_cut (gimage, drawable, new_layer)
     GImage *gimage;
     GimpDrawable *drawable;
     int *new_layer;
{
  Canvas *tiles;

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
transform_core_paste (gimage, drawable, tiles, new_layer)
     GImage *gimage;
     GimpDrawable *drawable;
     Canvas *tiles;
     int new_layer;
{
  Layer * layer;
  Layer * floating_layer;

  if (new_layer)
    {
      layer = layer_from_tiles (gimage, drawable, tiles, "Transformation", OPAQUE_OPACITY, NORMAL_MODE);
      GIMP_DRAWABLE(layer)->offset_x = canvas_fixme_getx (tiles);
      GIMP_DRAWABLE(layer)->offset_y = canvas_fixme_gety (tiles);

      /*  Start a group undo  */
      undo_push_group_start (gimage, EDIT_PASTE_UNDO);

      floating_sel_attach (layer, drawable);

      /*  End the group undo  */
      undo_push_group_end (gimage);

      /*  Free the tiles  */
      canvas_delete (tiles);

      active_tool_layer = layer;

      return layer;
    }
  else
    {
      if ((layer = drawable_layer ( (drawable))) == NULL)
	return NULL;

      layer_add_alpha (layer);
      floating_layer = gimage_floating_sel (gimage);

      if (floating_layer)
	floating_sel_relax (floating_layer, TRUE);

      gdisplays_update_area (gimage->ID,
			     GIMP_DRAWABLE(layer)->offset_x, GIMP_DRAWABLE(layer)->offset_y,
			     GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height);

      /*  Push an undo  */
      undo_push_layer_mod (gimage, layer);

      /*  set the current layer's data  */
      GIMP_DRAWABLE(layer)->tiles = tiles;

      /*  Fill in the new layer's attributes  */
      GIMP_DRAWABLE(layer)->width = canvas_width (tiles);;
      GIMP_DRAWABLE(layer)->height = canvas_height (tiles);
      /* GIMP_DRAWABLE(layer)->bytes = tiles->levels[0].bpp; */
      GIMP_DRAWABLE(layer)->offset_x = canvas_fixme_getx (tiles);
      GIMP_DRAWABLE(layer)->offset_y = canvas_fixme_gety (tiles);

      if (floating_layer)
	floating_sel_rigor (floating_layer, TRUE);

      drawable_update (GIMP_DRAWABLE(layer), 0, 0, GIMP_DRAWABLE(layer)->width, GIMP_DRAWABLE(layer)->height);

      return layer;
    }
}


static double
cubic (dx, jm1, j, jp1, jp2)
     double dx;
     int jm1, j, jp1, jp2;
{
  double dx1, dx2, dx3;
  double h1, h2, h3, h4;
  double result;

  /*  constraint parameter = -1  */
  dx1 = fabs (dx);
  dx2 = dx1 * dx1;
  dx3 = dx2 * dx1;
  h1 = dx3 - 2 * dx2 + 1;
  result = h1 * j;

  dx1 = fabs (dx - 1.0);
  dx2 = dx1 * dx1;
  dx3 = dx2 * dx1;
  h2 = dx3 - 2 * dx2 + 1;
  result += h2 * jp1;

  dx1 = fabs (dx - 2.0);
  dx2 = dx1 * dx1;
  dx3 = dx2 * dx1;
  h3 = -dx3 + 5 * dx2 - 8 * dx1 + 4;
  result += h3 * jp2;

  dx1 = fabs (dx + 1.0);
  dx2 = dx1 * dx1;
  dx3 = dx2 * dx1;
  h4 = -dx3 + 5 * dx2 - 8 * dx1 + 4;
  result += h4 * jm1;

  if (result < 0.0)
    result = 0.0;
  if (result > 255.0)
    result = 255.0;

  return result;
}
