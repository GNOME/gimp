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
#include <stdio.h>
#include "appenv.h"
#include "drawable.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "info_dialog.h"
#include "scale_tool.h"
#include "selection.h"
#include "tools.h"
#include "transform_core.h"
#include "transform_tool.h"
#include "undo.h"

#include "tile_manager_pvt.h"

#include "libgimp/gimpintl.h"

#define X1 0
#define Y1 1
#define X2 2
#define Y2 3

/*  storage for information dialog fields  */
char          orig_width_buf  [MAX_INFO_BUF];
char          orig_height_buf [MAX_INFO_BUF];
char          width_buf       [MAX_INFO_BUF];
char          height_buf      [MAX_INFO_BUF];
char          x_ratio_buf     [MAX_INFO_BUF];
char          y_ratio_buf     [MAX_INFO_BUF];

/*  forward function declarations  */
static void *      scale_tool_scale   (GImage *, GimpDrawable *, double *, TileManager *, int, GimpMatrix);
static void *      scale_tool_recalc  (Tool *, void *);
static void        scale_tool_motion  (Tool *, void *);
static void        scale_info_update  (Tool *);
static Argument *  scale_invoker      (Argument *);

/*  callback functions for the info dialog entries  */
static void        scale_width_changed  (GtkWidget *entry, gpointer data);
static void        scale_height_changed (GtkWidget *entry, gpointer data);

void *
scale_tool_transform (tool, gdisp_ptr, state)
     Tool * tool;
     gpointer gdisp_ptr;
     int state;
{
  GDisplay * gdisp;
  TransformCore * transform_core;

  gdisp = (GDisplay *) gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  switch (state)
    {
    case INIT :
      if (!transform_info)
	{
	  transform_info = info_dialog_new (_("Scaling Information"));
	  info_dialog_add_field (transform_info, _("Original Width: "), orig_width_buf, NULL, NULL);
	  info_dialog_add_field (transform_info, _("Original Height: "), orig_height_buf, NULL, NULL);
	  info_dialog_add_field (transform_info, _("Current Width: "), width_buf, scale_width_changed, tool);
	  info_dialog_add_field (transform_info, _("Current Height: "), height_buf, scale_height_changed, tool);
	  info_dialog_add_field (transform_info, _("X Scale Ratio: "), x_ratio_buf, NULL, NULL);
	  info_dialog_add_field (transform_info, _("Y Scale Ratio: "), y_ratio_buf, NULL, NULL);
	}

      transform_core->trans_info [X1] = (double) transform_core->x1;
      transform_core->trans_info [Y1] = (double) transform_core->y1;
      transform_core->trans_info [X2] = (double) transform_core->x2;
      transform_core->trans_info [Y2] = (double) transform_core->y2;

      return NULL;
      break;

    case MOTION :
      scale_tool_motion (tool, gdisp_ptr);

      return (scale_tool_recalc (tool, gdisp_ptr));
      break;

    case RECALC :
      return (scale_tool_recalc (tool, gdisp_ptr));
      break;

    case FINISH :
      return scale_tool_scale (gdisp->gimage, gimage_active_drawable (gdisp->gimage),
			       transform_core->trans_info, transform_core->original,
			       transform_tool_smoothing (), transform_core->transform);
      break;
    }

  return NULL;
}


Tool *
tools_new_scale_tool ()
{
  Tool * tool;
  TransformCore * private;

  tool = transform_core_new (SCALE, INTERACTIVE);

  private = tool->private;

  /*  set the rotation specific transformation attributes  */
  private->trans_func = scale_tool_transform;
  private->trans_info[X1] = 0;
  private->trans_info[Y1] = 0;
  private->trans_info[X2] = 0;
  private->trans_info[Y2] = 0;

  /*  assemble the transformation matrix  */
  gimp_matrix_identity (private->transform);

  return tool;
}


void
tools_free_scale_tool (tool)
     Tool * tool;
{
  transform_core_free (tool);
}


static void
scale_info_update (tool)
     Tool * tool;
{
  GDisplay * gdisp;
  TransformCore * transform_core;
  double ratio_x, ratio_y;
  int x1, y1, x2, y2, x3, y3, x4, y4;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  /*  Find original sizes  */
  x1 = transform_core->x1;
  y1 = transform_core->y1;
  x2 = transform_core->x2;
  y2 = transform_core->y2;
  sprintf (orig_width_buf, "%d", x2 - x1);
  sprintf (orig_height_buf, "%d", y2 - y1);

  /*  Find current sizes  */
  x3 = (int) transform_core->trans_info [X1];
  y3 = (int) transform_core->trans_info [Y1];
  x4 = (int) transform_core->trans_info [X2];
  y4 = (int) transform_core->trans_info [Y2];

  sprintf (width_buf, "%d", x4 - x3);
  sprintf (height_buf, "%d", y4 - y3);

  ratio_x = ratio_y = 0.0;

  if (x2 - x1)
    ratio_x = (double) (x4 - x3) / (double) (x2 - x1);
  if (y2 - y1)
    ratio_y = (double) (y4 - y3) / (double) (y2 - y1);

  sprintf (x_ratio_buf, "%0.2f", ratio_x);
  sprintf (y_ratio_buf, "%0.2f", ratio_y);

  info_dialog_update (transform_info);
  info_dialog_popup (transform_info);
}

static void
scale_width_changed (GtkWidget *w,
		     gpointer  data)
{
  Tool * tool;
  TransformCore * transform_core;
  GDisplay * gdisp;
  gchar *str;
  int value;

  tool = (Tool *)data;

  if (tool)
    {
      gdisp = (GDisplay *) tool->gdisp_ptr;
      transform_core = (TransformCore *) tool->private;

      str = g_strdup (gtk_entry_get_text (GTK_ENTRY (w)));
      value = (int) atof(str);

      if (value != (transform_core->trans_info[X2] - transform_core->trans_info[X1]))
	{
	  draw_core_pause (transform_core->core, tool);
	  transform_core->trans_info[X2] = transform_core->trans_info[X1] + value;
	  scale_tool_recalc (tool, gdisp);
	  draw_core_resume (transform_core->core, tool);
	}
      
      g_free (str);
    }
}

static void
scale_height_changed (GtkWidget *w,
		      gpointer  data)
{
  Tool * tool;
  TransformCore * transform_core;
  GDisplay * gdisp;
  gchar *str;
  int value;

  tool = (Tool *)data;

  if (tool)
    {
      gdisp = (GDisplay *) tool->gdisp_ptr;
      transform_core = (TransformCore *) tool->private;

      str = g_strdup (gtk_entry_get_text (GTK_ENTRY (w)));
      value = (int) atof(str);

      if (value != (transform_core->trans_info[Y2] - transform_core->trans_info[Y1]))
	{
	  draw_core_pause (transform_core->core, tool);
	  transform_core->trans_info[Y2] = transform_core->trans_info[Y1] + value;
	  scale_tool_recalc (tool, gdisp);
	  draw_core_resume (transform_core->core, tool);
	}
      
      g_free (str);
    }
}

static void
scale_tool_motion (tool, gdisp_ptr)
     Tool * tool;
     void * gdisp_ptr;
{
  GDisplay * gdisp;
  TransformCore * transform_core;
  double ratio;
  double *x1, *y1;
  double *x2, *y2;
  int w, h;
  int dir_x, dir_y;
  int diff_x, diff_y;

  gdisp = (GDisplay *) gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  diff_x = transform_core->curx - transform_core->lastx;
  diff_y = transform_core->cury - transform_core->lasty;

  switch (transform_core->function)
    {
    case HANDLE_1 :
      x1 = &transform_core->trans_info [X1];
      y1 = &transform_core->trans_info [Y1];
      x2 = &transform_core->trans_info [X2];
      y2 = &transform_core->trans_info [Y2];
      dir_x = dir_y = 1;
      break;
    case HANDLE_2 :
      x1 = &transform_core->trans_info [X2];
      y1 = &transform_core->trans_info [Y1];
      x2 = &transform_core->trans_info [X1];
      y2 = &transform_core->trans_info [Y2];
      dir_x = -1;
      dir_y = 1;
      break;
    case HANDLE_3 :
      x1 = &transform_core->trans_info [X1];
      y1 = &transform_core->trans_info [Y2];
      x2 = &transform_core->trans_info [X2];
      y2 = &transform_core->trans_info [Y1];
      dir_x = 1;
      dir_y = -1;
      break;
    case HANDLE_4 :
      x1 = &transform_core->trans_info [X2];
      y1 = &transform_core->trans_info [Y2];
      x2 = &transform_core->trans_info [X1];
      y2 = &transform_core->trans_info [Y1];
      dir_x = dir_y = -1;
      break;
    default :
      return;
    }

  /*  if just the control key is down, affect only the height  */
  if (transform_core->state & ControlMask && ! (transform_core->state & ShiftMask))
    diff_x = 0;
  /*  if just the shift key is down, affect only the width  */
  else if (transform_core->state & ShiftMask && ! (transform_core->state & ControlMask))
    diff_y = 0;

  *x1 += diff_x;
  *y1 += diff_y;

  if (dir_x > 0)
    {
      if (*x1 >= *x2) *x1 = *x2 - 1;
    }
  else
    {
      if (*x1 <= *x2) *x1 = *x2 + 1;
    }

  if (dir_y > 0)
    {
      if (*y1 >= *y2) *y1 = *y2 - 1;
    }
  else
    {
      if (*y1 <= *y2) *y1 = *y2 + 1;
    }

  /*  if both the control key & shift keys are down, keep the aspect ratio intact  */
  if (transform_core->state & GDK_CONTROL_MASK && transform_core->state & GDK_SHIFT_MASK)
    {
      ratio = (double) (transform_core->x2 - transform_core->x1) /
        (double) (transform_core->y2 - transform_core->y1);

      w = ABS ((*x2 - *x1));
      h = ABS ((*y2 - *y1));

      if (w > h * ratio)
        h = w / ratio;
      else
        w = h * ratio;

      *y1 = *y2 - dir_y * h;
      *x1 = *x2 - dir_x * w;
    }
}

static void *
scale_tool_recalc (tool, gdisp_ptr)
     Tool * tool;
     void * gdisp_ptr;
{
  TransformCore * transform_core;
  GDisplay * gdisp;
  int x1, y1, x2, y2;
  int diffx, diffy;
  int cx, cy;
  double scalex, scaley;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  transform_core = (TransformCore *) tool->private;
  x1 = (int) transform_core->trans_info [X1];
  y1 = (int) transform_core->trans_info [Y1];
  x2 = (int) transform_core->trans_info [X2];
  y2 = (int) transform_core->trans_info [Y2];

  scalex = scaley = 1.0;
  if (transform_core->x2 - transform_core->x1)
    scalex = (double) (x2 - x1) / (double) (transform_core->x2 - transform_core->x1);
  if (transform_core->y2 - transform_core->y1)
    scaley = (double) (y2 - y1) / (double) (transform_core->y2 - transform_core->y1);

  switch (transform_core->function)
    {
    case HANDLE_1 :
      cx = x2;  cy = y2;
      diffx = x2 - transform_core->x2;
      diffy = y2 - transform_core->y2;
      break;
    case HANDLE_2 :
      cx = x1;  cy = y2;
      diffx = x1 - transform_core->x1;
      diffy = y2 - transform_core->y2;
      break;
    case HANDLE_3 :
      cx = x2;  cy = y1;
      diffx = x2 - transform_core->x2;
      diffy = y1 - transform_core->y1;
      break;
    case HANDLE_4 :
      cx = x1;  cy = y1;
      diffx = x1 - transform_core->x1;
      diffy = y1 - transform_core->y1;
      break;
    default :
      cx = x1; cy = y1;
      diffx = diffy = 0;
      break;
    }

  /*  assemble the transformation matrix  */
  gimp_matrix_identity  (transform_core->transform);
  gimp_matrix_translate (transform_core->transform, (double) -cx + diffx, (double) -cy + diffy);
  gimp_matrix_scale     (transform_core->transform, scalex, scaley);
  gimp_matrix_translate (transform_core->transform, (double) cx, (double) cy);

  /*  transform the bounding box  */
  transform_bounding_box (tool);

  /*  update the information dialog  */
  scale_info_update (tool);

  return (void *) 1;
}

static void *
scale_tool_scale (gimage, drawable, trans_info, float_tiles, interpolation, matrix)
     GImage *gimage;
     GimpDrawable *drawable;
     double *trans_info;
     TileManager *float_tiles;
     int interpolation;
     GimpMatrix matrix;
{
  return transform_core_do (gimage, drawable, float_tiles, interpolation, matrix);
}


/*  The scale procedure definition  */
ProcArg scale_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    N_("the affected drawable")
  },
  { PDB_INT32,
    "interpolation",
    N_("whether to use interpolation")
  },
  { PDB_FLOAT,
    "x1",
    N_("the x coordinate of the upper-left corner of newly scaled region")
  },
  { PDB_FLOAT,
    "y1",
    N_("the y coordinate of the upper-left corner of newly scaled region")
  },
  { PDB_FLOAT,
    "x2",
    N_("the x coordinate of the lower-right corner of newly scaled region")
  },
  { PDB_FLOAT,
    "y2",
    N_("the y coordinate of the lower-right corner of newly scaled region")
  }
};

ProcArg scale_out_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    N_("the scaled drawable")
  }
};

ProcRecord scale_proc =
{
  "gimp_scale",
  N_("Scale the specified drawable"),
  N_("This tool scales the specified drawable if no selection exists.  If a selection exists, the portion of the drawable which lies under the selection is cut from the drawable and made into a floating selection which is then scaled by the specified amount.  The interpolation parameter can be set to TRUE to indicate that either linear or cubic interpolation should be used to smooth the resulting scaled drawable.  The return value is the ID of the scaled drawable.  If there was no selection, this will be equal to the drawable ID supplied as input.  Otherwise, this will be the newly created and scaled drawable."),
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  6,
  scale_args,

  /*  Output arguments  */
  1,
  scale_out_args,

  /*  Exec method  */
  { { scale_invoker } },
};


static Argument *
scale_invoker (args)
     Argument *args;
{
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  int interpolation;
  double trans_info[4];
  int int_value;
  TileManager *float_tiles;
  TileManager *new_tiles;
  GimpMatrix matrix;
  int new_layer;
  Layer *layer;
  Argument *return_args;

  drawable = NULL;
  layer = NULL;

  /*  the drawable  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL)                                        
        success = FALSE;
      else
        gimage = drawable_gimage (drawable);
    }
  /*  interpolation  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      interpolation = (int_value) ? TRUE : FALSE;
    }
  /*  scale extents  */
  if (success)
    {
      trans_info[X1] = args[2].value.pdb_float;
      trans_info[Y1] = args[3].value.pdb_float;
      trans_info[X2] = args[4].value.pdb_float;
      trans_info[Y2] = args[5].value.pdb_float;

      if (trans_info[X1] >= trans_info[X2] ||
	  trans_info[Y1] >= trans_info[Y2])
	success = FALSE;
    }

  /*  call the scale procedure  */
  if (success)
    {
      double scalex, scaley;

      /*  Start a transform undo group  */
      undo_push_group_start (gimage, TRANSFORM_CORE_UNDO);

      /*  Cut/Copy from the specified drawable  */
      float_tiles = transform_core_cut (gimage, drawable, &new_layer);

      scalex = scaley = 1.0;
      if (float_tiles->width)
	scalex = (trans_info[X2] - trans_info[X1]) / (double) float_tiles->width;
      if (float_tiles->height)
	scaley = (trans_info[Y2] - trans_info[Y1]) / (double) float_tiles->height;

      /*  assemble the transformation matrix  */
      gimp_matrix_identity  (matrix);
      gimp_matrix_translate (matrix, float_tiles->x, float_tiles->y);
      gimp_matrix_scale     (matrix, scalex, scaley);
      gimp_matrix_translate (matrix, trans_info[X1], trans_info[Y1]);

      /*  scale the buffer  */
      new_tiles = scale_tool_scale (gimage, drawable, trans_info,
				    float_tiles, interpolation, matrix);

      /*  free the cut/copied buffer  */
      tile_manager_destroy (float_tiles);

      if (new_tiles)
	success = (layer = transform_core_paste (gimage, drawable, new_tiles, new_layer)) != NULL;
      else
	success = FALSE;

      /*  push the undo group end  */
      undo_push_group_end (gimage);
    }

  return_args = procedural_db_return_args (&scale_proc, success);

  if (success)
    return_args[1].value.pdb_int = drawable_ID (GIMP_DRAWABLE(layer));

  return return_args;
}
