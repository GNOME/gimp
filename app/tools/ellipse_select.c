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
#include "appenv.h"
#include "edit_selection.h"
#include "ellipse_select.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "rect_select.h"
/*  private header file for rect_select data structure  */
#include "rect_selectP.h"

#define NO  0
#define YES 1

SelectionOptions *ellipse_options = NULL;
void ellipse_select (GImage *, int, int, int, int, int, int, int, double);

static Argument *ellipse_select_invoker (Argument *);

/*************************************/
/*  Ellipsoidal selection apparatus  */

void
ellipse_select (gimage, x, y, w, h, op, antialias, feather, feather_radius)
     GImage *gimage;
     int x, y;
     int w, h;
     int op;
     int antialias;
     int feather;
     double feather_radius;
{
  Channel * new_mask;

  /*  if applicable, replace the current selection  */
  if (op == REPLACE)
    gimage_mask_clear (gimage);
  else
    gimage_mask_undo (gimage);

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather)
    {
      new_mask = channel_new_mask (gimage->ID, gimage->width, gimage->height);
      channel_combine_ellipse (new_mask, ADD, x, y, w, h, antialias);
      channel_feather (new_mask, gimage_get_mask (gimage),
		       feather_radius, op, 0, 0);
      channel_delete (new_mask);
    }
  else if (op == INTERSECT)
    {
      new_mask = channel_new_mask (gimage->ID, gimage->width, gimage->height);
      channel_combine_ellipse (new_mask, ADD, x, y, w, h, antialias);
      channel_combine_mask (gimage_get_mask (gimage), new_mask, op, 0, 0);
      channel_delete (new_mask);
    }
  else
    channel_combine_ellipse (gimage_get_mask (gimage), op, x, y, w, h, antialias);
}

void
ellipse_select_draw (tool)
     Tool *tool;
{
  GDisplay * gdisp;
  EllipseSelect * ellipse_sel;
  int x1, y1, x2, y2;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  ellipse_sel = (EllipseSelect *) tool->private;

  x1 = MINIMUM (ellipse_sel->x, ellipse_sel->x + ellipse_sel->w);
  y1 = MINIMUM (ellipse_sel->y, ellipse_sel->y + ellipse_sel->h);
  x2 = MAXIMUM (ellipse_sel->x, ellipse_sel->x + ellipse_sel->w);
  y2 = MAXIMUM (ellipse_sel->y, ellipse_sel->y + ellipse_sel->h);

  gdisplay_transform_coords (gdisp, x1, y1, &x1, &y1, 0);
  gdisplay_transform_coords (gdisp, x2, y2, &x2, &y2, 0);

  gdk_draw_arc (ellipse_sel->core->win,
		ellipse_sel->core->gc, 0,
		x1, y1, (x2 - x1), (y2 - y1), 0, 23040);
}

Tool *
tools_new_ellipse_select ()
{
  Tool *tool;
  EllipseSelect *private;

  /*  The tool options  */
  if (!ellipse_options)
    ellipse_options = create_selection_options (ELLIPSE_SELECT);

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (EllipseSelect *) g_malloc (sizeof (EllipseSelect));

  private->core = draw_core_new (ellipse_select_draw);
  /*  Make the selection static, not blinking  */
  private->x = private->y = 0;
  private->w = private->h = 0;

  tool->type = ELLIPSE_SELECT;
  tool->state = INACTIVE;
  tool->scroll_lock = 0;  /*  Allow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = rect_select_button_press;
  tool->button_release_func = rect_select_button_release;
  tool->motion_func = rect_select_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = rect_select_cursor_update;
  tool->control_func = rect_select_control;
  tool->preserve = TRUE;

  return tool;
}

void
tools_free_ellipse_select (tool)
     Tool *tool;
{
  EllipseSelect *ellipse_sel;

  ellipse_sel = (EllipseSelect *) tool->private;

  draw_core_free (ellipse_sel->core);
  g_free (ellipse_sel);
}

/*  The ellipse_select procedure definition  */
ProcArg ellipse_select_args[] =
{
  { PDB_IMAGE,
    "image",
    "The image"
  },
  { PDB_FLOAT,
    "x",
    "x coordinate of upper-left corner of ellipse bounding box"
  },
  { PDB_FLOAT,
    "y",
    "y coordinate of upper-left corner of ellipse bounding box"
  },
  { PDB_FLOAT,
    "width",
    "the width of the ellipse: width > 0"
  },
  { PDB_FLOAT,
    "height",
    "the height of the ellipse: height > 0"
  },
  { PDB_INT32,
    "operation",
    "the selection operation: { ADD (0), SUB (1), REPLACE (2), INTERSECT (3) }"
  },
  { PDB_INT32,
    "antialias",
    "antialiasing On/Off"
  },
  { PDB_INT32,
    "feather",
    "feather option for selections"
  },
  { PDB_FLOAT,
    "feather_radius",
    "radius for feather operation"
  }
};

ProcRecord ellipse_select_proc =
{
  "gimp_ellipse_select",
  "Create an elliptical selection over the specified image",
  "This tool creates an elliptical selection over the specified image.  The elliptical region can be either added to, subtracted from, or replace the contents of the previous selection mask.  If antialiasing is turned on, the edges of the elliptical region will contain intermediate values which give the appearance of a sharper, less pixelized edge.  This should be set as TRUE most of the time.  If the feather option is enabled, the resulting selection is blurred before combining.  The blur is a gaussian blur with the specified feather radius.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  9,
  ellipse_select_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { ellipse_select_invoker } },
};


static Argument *
ellipse_select_invoker (args)
     Argument *args;
{
  int success = TRUE;
  GImage *gimage;
  int op;
  int antialias;
  int feather;
  double x, y;
  double w, h;
  double feather_radius;
  int int_value;

  op = REPLACE;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  x, y, w, h  */
  if (success)
    {
      x = args[1].value.pdb_float;
      y = args[2].value.pdb_float;
      w = args[3].value.pdb_float;
      h = args[4].value.pdb_float;
    }
  /*  operation  */
  if (success)
    {
      int_value = args[5].value.pdb_int;
      switch (int_value)
	{
	case 0: op = ADD; break;
	case 1: op = SUB; break;
	case 2: op = REPLACE; break;
	case 3: op = INTERSECT; break;
	default: success = FALSE;
	}
    }
  /*  antialiasing?  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      antialias = (int_value) ? TRUE : FALSE;
    }
  /*  feathering  */
  if (success)
    {
      int_value = args[7].value.pdb_int;
      feather = (int_value) ? TRUE : FALSE;
    }
  /*  feather radius  */
  if (success)
    {
      feather_radius = args[8].value.pdb_float;
    }

  /*  call the ellipse_select procedure  */
  if (success)
    ellipse_select (gimage, (int) x, (int) y, (int) w, (int) h,
		    op, antialias, feather, feather_radius);

  return procedural_db_return_args (&ellipse_select_proc, success);
}
