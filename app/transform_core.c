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
static gfloat      cubic                  (double, gfloat, gfloat, gfloat, gfloat);


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
	    (gimage_mask_value (gdisp->gimage, x, y) != 0))
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
	x < (GIMP_DRAWABLE(layer)->offset_x + drawable_width (GIMP_DRAWABLE(layer))) &&
	y < (GIMP_DRAWABLE(layer)->offset_y + drawable_height (GIMP_DRAWABLE(layer))))
      {
	if (gimage_mask_is_empty (gdisp->gimage) ||
	    (gimage_mask_value (gdisp->gimage, x, y) != 0))
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




/* ---------------------------------------------------------------

                       texturemap area

   this is a fairly general texture mapper that should work on
   arbitrary precision/tiling images.
   
*/

#define BILINEAR(jk,j1k,jk1,j1k1,dx,dy) \
                ((1-dy) * ((1-dx)*jk + dx*j1k) + \
		    dy  * ((1-dx)*jk1 + dx*j1k1))

typedef struct _InputTile InputTile;
typedef struct _InputPixel InputPixel;
typedef struct _TMState TMState;


/* Types of interpolation that can be done */
typedef enum
{
  INTERPOLATION_NONE,
  INTERPOLATION_BILINEAR,
  INTERPOLATION_CUBIC
} Interpolation;


/* a reffed tile */
struct _InputTile
{
  /* number of input pixels on this tile */
  gint refs;

  /* the portion for this tile */
  gint x0, y0, x1, y1;

  /* the data pointer */
  void * data;

  /* the strides */
  gint rs, ps;
};


/* an input pixel */
struct _InputPixel
{
  /* is this pixel valid */
  guint do_this_one;
  
  /* the coords of this point in the input image */
  gint x;
  gint y;

  /* the tile we're on */
  InputTile * tile;
  
  /* the raw pixel data */
  void * data;

  /* the alpha channel value */
  gfloat alpha;

  /* the alpha-premultiplied pixel data */
  gfloat premul_data[MAX_CHANNELS];
};


/* a routine that interpolates some input pixels */
typedef void (*TMInterpolateFunc) (TMState *);


/* the current state of a texture mapping operation */
struct _TMState
{
  /* the area we're mapping to */
  PixelArea * output;

  /* the area we're mapping from */
  PixelArea * input;

  /* maps output pixels back to input image */
  Matrix m;

  /* type of smoothing to perform */
  Interpolation in;

  /* color for output pixels with no input */
  PixelRow * color;

  /* function to interpolate pixels */
  TMInterpolateFunc ifunc;

  
  /* input canvas */
  Canvas * c;

  /* input bound box */
  gint c_x, c_y, c_x1, c_y1;

  /* exact position of output pixel in input image */
  gdouble tx, ty;

  /* integer part of tx/ty */
  gint itx, ity;

  /* fractional part of tx/ty */
  gdouble dx, dy;

  /* how many pixels are we interpolating */
  gint num_points;

  /* input tiles */
  InputTile tiles[16];
  
  /* input pixels */
  InputPixel pixels[16];

  
  /* output canvas */
  Canvas * dst;

  /* output offsets */
  gint dstx, dsty;

  /* output pixel data */
  void * data;     

  /* which channel is the alpha */
  gint alpha;
};



/* the 8 bit core */
static void 
tm_interpolate_u8  (
                    TMState * s
                    )
{
#define O    ((guint8*)s->data)            /* output pixel */
#define D(n) ((guint8*)s->pixels[n].data)  /* input pixel(s) */
#define P(n) (s->pixels[n].premul_data)    /* premul input pixel(s) */
#define A(n) (s->pixels[n].alpha)          /* input pixel(s) alpha */  

  gint b, i;

  /* grab the alpha values and pre-multiply the input pixels */
  if (s->in != INTERPOLATION_NONE)
    {
      for (i = 0; i < s->num_points; i++)
        {
          if (D(i))
            {
              A(i) = D(i)[s->alpha] / 255.0;
              for (b = 0; b < s->alpha; b++)
                P(i)[b] = D(i)[b] * A(i) / 255.0;
            }
          else
            {
              A(i) = 0;
              for (b = 0; b < s->alpha; b++)
                P(i)[b] = 0;
            }
        }
    }
  
  /* interpolate the input values to an output value */
  switch (s->in)
    {
    case INTERPOLATION_NONE:
      {
        for (b = 0; b <= s->alpha; b++)
          O[b] = D(0)[b];
      }
      break;

    case INTERPOLATION_BILINEAR:
      {
        gdouble a_recip = 0.0;
        gdouble a_val = BILINEAR (A(0), A(1), A(2), A(3),
                                  s->dx, s->dy);

        if (a_val != 0)
          {
            a_recip = 255.0 / a_val;
            a_val = a_val * 255.0;
          }
        
        for (b = 0; b < s->alpha; b++)
          {
            O[b] =
              a_recip *
              BILINEAR (P(0)[b], P(1)[b], P(2)[b], P(3)[b],
                        s->dx, s->dy);
          }
        
        O[s->alpha] = a_val;
      }
      break;

    case INTERPOLATION_CUBIC:
      {
        gfloat a_recip = 0.0;
        gfloat a_val = cubic (s->dy,
                               cubic (s->dx, A(0),  A(1),  A(2),  A(3)),
                               cubic (s->dx, A(4),  A(5),  A(6),  A(7)),
                               cubic (s->dx, A(8),  A(9),  A(10), A(11)),
                               cubic (s->dx, A(12), A(13), A(14), A(15)));
        
        if (a_val != 0)
          {
            a_recip = 255.0 / a_val;
            a_val = a_val * 255.0;
          }
        
        for (b = 0; b < s->alpha; b++)
          {
            gint newval =
              a_recip *
              cubic (s->dy,
                     cubic (s->dx, P(0)[b],  P(1)[b], P(2)[b],  P(3)[b]),
                     cubic (s->dx, P(4)[b],  P(5)[b], P(6)[b],  P(7)[b]),
                     cubic (s->dx, P(8)[b],  P(9)[b], P(10)[b], P(11)[b]),
                     cubic (s->dx, P(12)[b], P(13)[b], P(14)[b], P(15)[b]));
            
            O[b] = CLAMP (newval, 0, 255);
          }
        
        O[s->alpha] = a_val;
      }
      break;
    }

#undef O
#undef D
#undef P
#undef A
}


/* the 16 bit core */
static void 
tm_interpolate_u16  (
                     TMState * s
                     )
{
#define O    ((guint16*)s->data)            /* output pixel */
#define D(n) ((guint16*)s->pixels[n].data)  /* input pixel(s) */
#define P(n) (s->pixels[n].premul_data)     /* premul input pixel(s) */
#define A(n) (s->pixels[n].alpha)           /* input pixel(s) alpha */  

  gint b, i;

  /* grab the alpha values and pre-multiply the input pixels */
  if (s->in != INTERPOLATION_NONE)
    {
      for (i = 0; i < s->num_points; i++)
        {
          if (D(i))
            {
              A(i) = D(i)[s->alpha] / 65535.0;
              for (b = 0; b < s->alpha; b++)
                P(i)[b] = D(i)[b] * A(i) / 65535.0;
            }
          else
            {
              A(i) = 0;
              for (b = 0; b < s->alpha; b++)
                P(i)[b] = 0;
            }
        }
    }
  
  /* interpolate the input values to an output value */
  switch (s->in)
    {
    case INTERPOLATION_NONE:
      {
        for (b = 0; b <= s->alpha; b++)
          O[b] = D(0)[b];
      }
      break;

    case INTERPOLATION_BILINEAR:
      {
        gdouble a_recip = 0.0;
        gdouble a_val = BILINEAR (A(0), A(1), A(2), A(3),
                                  s->dx, s->dy);

        if (a_val != 0)
          {
            a_recip = 65535.0 / a_val;
            a_val = a_val * 65535.0;
          }
        
        for (b = 0; b < s->alpha; b++)
          {
            O[b] =
              a_recip *
              BILINEAR (P(0)[b], P(1)[b], P(2)[b], P(3)[b],
                        s->dx, s->dy);
          }
        
        O[s->alpha] = a_val;
      }
      break;

    case INTERPOLATION_CUBIC:
      {
        gfloat a_recip = 0.0;
        gfloat a_val = cubic (s->dy,
                               cubic (s->dx, A(0),  A(1),  A(2),  A(3)),
                               cubic (s->dx, A(4),  A(5),  A(6),  A(7)),
                               cubic (s->dx, A(8),  A(9),  A(10), A(11)),
                               cubic (s->dx, A(12), A(13), A(14), A(15)));
        
        if (a_val != 0)
          {
            a_recip = 65535.0 / a_val;
            a_val = a_val * 65535.0;
          }
        
        for (b = 0; b < s->alpha; b++)
          {
            gint newval =
              a_recip *
              cubic (s->dy,
                     cubic (s->dx, P(0)[b],  P(1)[b], P(2)[b],  P(3)[b]),
                     cubic (s->dx, P(4)[b],  P(5)[b], P(6)[b],  P(7)[b]),
                     cubic (s->dx, P(8)[b],  P(9)[b], P(10)[b], P(11)[b]),
                     cubic (s->dx, P(12)[b], P(13)[b], P(14)[b], P(15)[b]));
            
            O[b] = CLAMP (newval, 0, 65535);
          }
        
        O[s->alpha] = a_val;
      }
      break;
    }

#undef O
#undef D
#undef P
#undef A
}


/* the float core */
static void 
tm_interpolate_float  (
                       TMState * s
                       )
{
#define O    ((gfloat*)s->data)            /* output pixel */
#define D(n) ((gfloat*)s->pixels[n].data)  /* input pixel(s) */
#define P(n) (s->pixels[n].premul_data)    /* premul input pixel(s) */
#define A(n) (s->pixels[n].alpha)          /* input pixel(s) alpha */  

  gint b, i;

  /* grab the alpha values and pre-multiply the input pixels */
  if (s->in != INTERPOLATION_NONE)
    {
      for (i = 0; i < s->num_points; i++)
        {
          if (D(i))
            {
              A(i) = D(i)[s->alpha];
              for (b = 0; b < s->alpha; b++)
                P(i)[b] = D(i)[b] * A(i);
            }
          else
            {
              A(i) = 0;
              for (b = 0; b < s->alpha; b++)
                P(i)[b] = 0;
            }
        }
    }
  
  /* interpolate the input values to an output value */
  switch (s->in)
    {
    case INTERPOLATION_NONE:
      {
        for (b = 0; b <= s->alpha; b++)
          O[b] = D(0)[b];
      }
      break;

    case INTERPOLATION_BILINEAR:
      {
        gdouble a_recip = 0.0;
        gdouble a_val = BILINEAR (A(0), A(1), A(2), A(3),
                                  s->dx, s->dy);

        if (a_val != 0)
          {
            a_recip = 1.0 / a_val;
          }
        
        for (b = 0; b < s->alpha; b++)
          {
            O[b] =
              a_recip *
              BILINEAR (P(0)[b], P(1)[b], P(2)[b], P(3)[b],
                        s->dx, s->dy);
          }
        
        O[s->alpha] = a_val;
      }
      break;

    case INTERPOLATION_CUBIC:
      {
        gfloat a_recip = 0.0;
        gfloat a_val = cubic (s->dy,
                               cubic (s->dx, A(0),  A(1),  A(2),  A(3)),
                               cubic (s->dx, A(4),  A(5),  A(6),  A(7)),
                               cubic (s->dx, A(8),  A(9),  A(10), A(11)),
                               cubic (s->dx, A(12), A(13), A(14), A(15)));
        
        if (a_val != 0)
          {
            a_recip = 1.0 / a_val;
          }
        
        for (b = 0; b < s->alpha; b++)
          {
            gfloat newval =
              a_recip *
              cubic (s->dy,
                     cubic (s->dx, P(0)[b],  P(1)[b], P(2)[b],  P(3)[b]),
                     cubic (s->dx, P(4)[b],  P(5)[b], P(6)[b],  P(7)[b]),
                     cubic (s->dx, P(8)[b],  P(9)[b], P(10)[b], P(11)[b]),
                     cubic (s->dx, P(12)[b], P(13)[b], P(14)[b], P(15)[b]));
            
            O[b] = CLAMP (newval, 0.0, 1.0);
          }
        
        O[s->alpha] = a_val;
      }
      break;
    }

#undef O
#undef D
#undef P
#undef A
}


static TMInterpolateFunc 
tm_interpolate_funcs (
                      Tag tag
                      )
     
{
  switch (tag_precision (tag))
    {
    case PRECISION_U8:
      return tm_interpolate_u8;
    case PRECISION_U16:
      return tm_interpolate_u16;
    case PRECISION_FLOAT:
      return tm_interpolate_float;
    case PRECISION_FLOAT16:
    default:
      return NULL;
    } 
}



#define REF_INIT(_n, _x, _y, _b) \
  s->pixels[_n].do_this_one = (_b); \
  s->pixels[_n].x = (_x); \
  s->pixels[_n].y = (_y); \
  s->pixels[_n].tile = NULL; \
  s->pixels[_n].data = NULL

#define REF_TILE(_t, _x, _y) \
  do { \
    gint x = (_x); gint y = (_y); \
    canvas_portion_refro (s->c, x, y); \
    (_t)->refs = 0; \
    (_t)->x0 = canvas_portion_x (s->c, x, y); \
    (_t)->y0 = canvas_portion_y (s->c, x, y); \
    x = (_t)->x0; y = (_t)->y0; \
    (_t)->x1 = x + canvas_portion_width (s->c, x, y); \
    (_t)->y1 = y + canvas_portion_height (s->c, x, y); \
    (_t)->data = canvas_portion_data (s->c, x, y); \
    (_t)->rs = canvas_portion_rowstride (s->c, x, y); \
    (_t)->ps = tag_bytes (canvas_tag (s->c)); \
  } while (0)

#define UNREF_TILE(_t) \
  do { \
    if ((_t)->data != NULL) \
      { \
        (_t)->refs = 0; \
        canvas_portion_unref (s->c, (_t)->x0, (_t)->y0); \
        (_t)->data = NULL; \
      } \
  } while (0)

#define REF_PIXEL(_p, _t) \
  do { \
    (_t)->refs++; \
    (_p)->tile = (_t); \
    (_p)->data = \
      ((guchar*) (_t)->data) + \
      (((_p)->x - (_t)->x0) * (_t)->ps) + \
      (((_p)->y - (_t)->y0) * (_t)->rs); \
    } while (0)

#define UNREF_PIXEL(_p) \
  do { \
    if ((_p)->tile) \
      { \
        (_p)->tile->refs--; \
      } \
      (_p)->tile = NULL; \
      (_p)->data = NULL; \
  } while (0)



static void
texture_map_ref (
                 TMState * s
                 )
{
  guint try_again = TRUE;
  gint i, j;

  /* get the offset into the input image */
  gint x = s->itx - s->c_x;
  gint y = s->ity - s->c_y;

  /* check which neighbours to ref */
  gint x1 = (s->itx < (s->c_x1 - 1)) ? 1: 0;
  gint y1 = (s->ity < (s->c_y1 - 1)) ? 1: 0;
  gint x2 = (s->itx < (s->c_x1 - 2)) ? 1: 0;
  gint y2 = (s->ity < (s->c_y1 - 2)) ? 1: 0;
  gint xa = (s->itx > s->c_x) ? 1: 0;
  gint ya = (s->ity > s->c_y) ? 1: 0;

  /* identify the pixels we want to ref */
  switch (s->in)
    {
    case INTERPOLATION_CUBIC:
      REF_INIT (0,  x-1, y-1, xa*ya);
      REF_INIT (1,  x,   y-1, ya);
      REF_INIT (2,  x+1, y-1, x1*ya);
      REF_INIT (3,  x+2, y-1, x2*ya);
      REF_INIT (4,  x-1, y,   xa);
      REF_INIT (5,  x,   y,   1);
      REF_INIT (6,  x+1, y,   x1);
      REF_INIT (7,  x+2, y,   x2);
      REF_INIT (8,  x-1, y+1, xa*y1);
      REF_INIT (9,  x,   y+1, y1);
      REF_INIT (10, x+1, y+1, x1*y1);
      REF_INIT (11, x+2, y+1, x2*y1);
      REF_INIT (12, x-1, y+2, xa*y2);
      REF_INIT (13, x,   y+2, y2);
      REF_INIT (14, x+1, y+2, x1*y2);
      REF_INIT (15, x+2, y+2, x2*y2);
      break;
      
    case INTERPOLATION_BILINEAR:
      REF_INIT (0, x,   y,   1);
      REF_INIT (1, x+1, y,   x1);
      REF_INIT (2, x,   y+1, y1);
      REF_INIT (3, x+1, y+1, x1*y1);
      break;
      
    case INTERPOLATION_NONE:
      REF_INIT (0, x, y, 1);
      break;
    }
     
  /* loop until all pixels reffed */
  while (try_again == TRUE)
    {
      /* assume everything will work this time */
      try_again = FALSE;
      
      /* make a pass across all pixels */
      for (i = 0; i < s->num_points; i++)
        {
          InputPixel * p = &s->pixels[i];
          InputTile * first_free = NULL;
          
          /* see if this one is okay as is */
          if ((p->do_this_one == FALSE) || (p->tile != NULL))
            continue;
          
          /* check if this one is on a pre-reffed tile */
          for (j = 0; j < s->num_points; j++)
            {
              InputTile * t = &s->tiles[j];
              
              if (t->data == NULL)
                {
                  if (first_free == NULL)
                    first_free = t;
                }
              else if ((p->x >= t->x0) && (p->x < t->x1) &&
                       (p->y >= t->y0) && (p->y < t->y1))
                {
                  REF_PIXEL (p, t);
                  break;
                }
            }

          /* see if we need to nuke a tile */
          if (p->tile == NULL)
            {
              if (first_free != NULL)
                {
                  REF_TILE (first_free, p->x, p->y);
                  REF_PIXEL (p, first_free);
                }
              else
                {
                  try_again = TRUE;
                }
            }
        }

      /* if we need to free up tiles, do so and try again */
      if (try_again == TRUE)
        {
          for (j = 0; j < s->num_points; j++)
            {
              if (s->tiles[j].refs == 0)
                {
                  UNREF_TILE (&s->tiles[j]);
                }
            }
        }
    }
}

static void
texture_map_unref (
                   TMState * s
                   )
{
  gint i;

  for (i = 0; i < s->num_points; i++)
    {
      if (s->pixels[i].do_this_one == TRUE)
        {
          UNREF_PIXEL (&s->pixels[i]);
        }
    }
}

static void
texture_map_unref_all (
                       TMState * s
                       )
{
  gint j;

  for (j = 0; j < s->num_points; j++)
    {
      UNREF_TILE (&s->tiles[j]);
    }
}

#undef REF_INIT
#undef REF_TILE
#undef UNREF_TILE
#undef REF_PIXEL
#undef UNREF_PIXEL


/* map the output pixel back to the input image.  if it hits,
   interpolate the input pixel(s) to create the output pixel */
static void 
texture_map_pixel  (
                    TMState * s,
                    gint x,
                    gint y
                    )
{
  gdouble tw;

  /* map the output pixel to the input image */
  s->tx = (s->m[0][2]) + (s->m[0][0] * (x + 0.5)) + (s->m[0][1] * (y + 0.5));
  s->ty = (s->m[1][2]) + (s->m[1][0] * (x + 0.5)) + (s->m[1][1] * (y + 0.5));

  /* normalize the coords */
  tw = (s->m[2][2]) + (s->m[2][0] * (x + 0.5)) + (s->m[2][1] * (y + 0.5));
  if ((tw != 1.0) && (tw != 0.0))
    {
      s->tx = s->tx / tw;
      s->ty = s->ty / tw;
    }

  /* round coords to integers and save fractional error */
  s->itx = (int) ((s->tx < 0) ? (s->tx - 0.999999) : s->tx);
  s->dx = s->tx - s->itx;

  s->ity = (int) ((s->ty < 0) ? (s->ty - 0.999999) : s->ty);
  s->dy = s->ty - s->ity;

  /* check if the pixel fell inside the input image */
  if ((s->itx >= s->c_x) && (s->itx < s->c_x1) &&
      (s->ity >= s->c_y) && (s->ity < s->c_y1))
    {
      /* bring in the input data */
      texture_map_ref (s);

      /* do the interpolation */
      s->ifunc (s);

      /* release the input pixels */
      texture_map_unref (s);
    }
}

/* create each output pixel from one or more input pixels */
static void 
texture_map_area2  (
                    TMState * s
                    )
{
  void * pag;

  /* hit each portion of the destination */
  for (pag = pixelarea_register (1, s->output);
       pag;
       pag = pixelarea_process (pag))
    {
      gint xx, yy, ww, hh;
      gint x, y, w, h;
      PixelArea area;
      gint ps, rs;
      guchar * d;

      /* get the portion dimensions */
      x = pixelarea_x (s->output);
      y = pixelarea_y (s->output);
      w = pixelarea_width (s->output);
      h = pixelarea_height (s->output);

      /* get the data pointer and strides */
      d = pixelarea_data (s->output);
      rs = pixelarea_rowstride (s->output);
      ps = tag_bytes (pixelarea_tag (s->output));
      
      /* fill this portion with the default color */
      pixelarea_init (&area, s->dst,
                      x, y,
                      w, h,
                      TRUE);
      color_area (&area, s->color);
      
      /* do all pixels in this portion */
      for (yy = y, hh = h; hh; yy++, hh--)
        for (xx = x, ww = w; ww; xx++, ww--)
          {
            s->data = d + ((yy-y) * rs) + ((xx-x) * ps);
            texture_map_pixel (s, xx + s->dstx, yy + s->dsty);
          }
    }

  /* clean up dangling refs */
  texture_map_unref_all (s);
}


/* error check inputs, dispatch, and execute */
static void 
texture_map_area  (
                   PixelArea * output,
                   PixelArea * input,
                   Matrix m,
                   Interpolation in,
                   PixelRow * color
                   )
{
  Tag ot, it, ct;
  TMState state;
  int i, j;
  
  /* check args */
  g_return_if_fail (output != NULL);
  g_return_if_fail (input != NULL);
  g_return_if_fail (color != NULL);
  g_return_if_fail (m != NULL);
  
  /* check the tags */
  ot = pixelarea_tag (output);
  it = pixelarea_tag (input);
  ct = pixelrow_tag (color);
  g_return_if_fail (tag_equal (ot, it) != FALSE);
  g_return_if_fail (tag_equal (ot, ct) != FALSE);
  
  /* save the inputs */
  state.output = output;
  state.input = input;
  state.color = color;
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      state.m[i][j] = m[i][j];
  
  /* get the reffing and interpolation functions */
  state.ifunc = tm_interpolate_funcs (ot);
  g_return_if_fail (state.ifunc != NULL);
  
  /* figure out which one is the alpha channel */
  state.alpha = 0;
  switch (tag_format (ot))
    {
    case FORMAT_RGB:     state.alpha = 3; break;
    case FORMAT_GRAY:    state.alpha = 1; break;
    case FORMAT_INDEXED: state.alpha = 1; break;
    }
  g_return_if_fail (state.alpha != 0);

  /* decide how many pixels we're interpolating */
  state.in = in;
  state.num_points = 0;
  switch (in)
    {
    case INTERPOLATION_CUBIC:    state.num_points = 16; break;
    case INTERPOLATION_BILINEAR: state.num_points = 4; break;
    case INTERPOLATION_NONE:     state.num_points = 1; break;
    }
  g_return_if_fail (state.num_points != 0);
  for (i = 0; i < state.num_points; i++)
    state.tiles[i].data = NULL;
  
  /* get the src region of interest */
#define FIXME
  state.c = input->canvas;
  state.c_x = canvas_fixme_getx (state.c);
  state.c_y = canvas_fixme_gety (state.c);
  state.c_x1 = state.c_x + canvas_width (state.c);
  state.c_y1 = state.c_y + canvas_height (state.c);

  /* get the dst canvas and offsets */
#define FIXME
  state.dst = output->canvas;
  state.dstx = canvas_fixme_getx (state.dst);
  state.dsty = canvas_fixme_gety (state.dst);

  /* okay, everything looks good, let's do it */
  texture_map_area2 (&state);
}


/* convert an input image with forward transform to an output image
   with reverse transform and execute it */
Canvas * 
transform_core_do  (
                    GImage * gimage,
                    GimpDrawable * drawable,
                    Canvas * float_tiles,
                    int interpolation,
                    Matrix matrix
                    )
{
  gint x1, x2, y1, y2;
  gint tx1, tx2, ty1, ty2;
  Tag tag;
  Canvas *tiles;
  Matrix m;
  Interpolation i;
  unsigned char bg_col[TAG_MAX_BYTES];
  PixelRow color; 
  PixelArea inputPR;
  PixelArea outputPR;

  /* get the input and output bounding box */
  {
    double dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4;
    
    x1 = canvas_fixme_getx (float_tiles);
    y1 = canvas_fixme_gety (float_tiles);
    x2 = x1 + canvas_width (float_tiles);
    y2 = y1 + canvas_height (float_tiles);

    transform_point (matrix, x1, y1, &dx1, &dy1);
    transform_point (matrix, x2, y1, &dx2, &dy2);
    transform_point (matrix, x1, y2, &dx3, &dy3);
    transform_point (matrix, x2, y2, &dx4, &dy4);

    tx1 = MIN (MIN (dx1, dx2), MIN (dx3, dx4));
    ty1 = MIN (MIN (dy1, dy2), MIN (dy3, dy4));
    tx2 = MAX (MAX (dx1, dx2), MAX (dx3, dx4));
    ty2 = MAX (MAX (dy1, dy2), MAX (dy3, dy4));
  }

  /* find out what sort of data we're dealing with */
  tag = canvas_tag (float_tiles);
  
  /* get a result image */
  tiles = canvas_new (tag,
                      (tx2 - tx1), (ty2 - ty1),
                      STORAGE_TILED);
  canvas_fixme_setx (tiles, tx1);
  canvas_fixme_sety (tiles, ty1);

  /* Find the inverse of the transformation matrix  */
  invert (matrix, m);

  /* decide what sort of interpolation to use */
  i = INTERPOLATION_NONE;
  if (interpolation && (tag_format (tag) != FORMAT_INDEXED))
    {
      if (cubic_interpolation)
        i = INTERPOLATION_CUBIC;
      else
        i = INTERPOLATION_BILINEAR;
    }
  
  /* get the default color */
  pixelrow_init (&color, tag, bg_col, 1);
  palette_get_transparent (&color);
  
  /* do the transformation */
  pixelarea_init (&outputPR, tiles,
                  0, 0,
                  0, 0,
                  TRUE);
  pixelarea_init (&inputPR, float_tiles,
                  0, 0,
                  0, 0,
                  FALSE);
  texture_map_area (&outputPR, &inputPR, m, i, &color);
  
  /* return transformed image */
  return tiles;
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
			     drawable_width (GIMP_DRAWABLE(layer)), drawable_height (GIMP_DRAWABLE(layer)));

      /*  Push an undo  */
      undo_push_layer_mod (gimage, layer);

      /*  set the current layer's data  */
      GIMP_DRAWABLE(layer)->tiles = tiles;

      /*  Fill in the new layer's attributes  */
      GIMP_DRAWABLE(layer)->offset_x = canvas_fixme_getx (tiles);
      GIMP_DRAWABLE(layer)->offset_y = canvas_fixme_gety (tiles);

      if (floating_layer)
	floating_sel_rigor (floating_layer, TRUE);

      drawable_update (GIMP_DRAWABLE(layer),
                       0, 0,
                       0, 0);

      return layer;
    }
}

static gfloat
cubic (
       double dx,
       gfloat jm1,
       gfloat j,
       gfloat jp1,
       gfloat jp2
       )
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
  if (result > 1.0)
    result = 1.0;

  return (gfloat) result;
}
