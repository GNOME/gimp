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
#include <math.h>
#include "appenv.h"
#include "drawable.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "info_dialog.h"
#include "palette.h"
#include "shear_tool.h"
#include "selection.h"
#include "tools.h"
#include "transform_core.h"
#include "transform_tool.h"
#include "undo.h"

#include "tile_manager_pvt.h"

/*  index into trans_info array  */
#define HORZ_OR_VERT   0
#define XSHEAR         1
#define YSHEAR         2

/*  types of shearing  */
#define HORZ           1
#define VERT           2

/*  the minimum movement before direction of shear can be determined (pixels) */
#define MIN_MOVE       5

/*  variables local to this file  */
static int         direction_unknown;
static char        xshear_buf  [MAX_INFO_BUF];
static char        yshear_buf  [MAX_INFO_BUF];

/*  forward function declarations  */
static void *      shear_tool_shear   (GImage *, GimpDrawable *, TileManager *, int, Matrix);
static void *      shear_tool_recalc  (Tool *, void *);
static void        shear_tool_motion  (Tool *, void *);
static void        shear_info_update  (Tool *);
static Argument *  shear_invoker      (Argument *);

/*  Info dialog callback funtions  */
static void        shear_x_mag_changed (GtkWidget *, gpointer);
static void        shear_y_mag_changed (GtkWidget *, gpointer);

void *
shear_tool_transform (tool, gdisp_ptr, state)
     Tool * tool;
     gpointer gdisp_ptr;
     int state;
{
  TransformCore * transform_core;
  GDisplay *gdisp;

  transform_core = (TransformCore *) tool->private;
  gdisp = (GDisplay *) gdisp_ptr;

  switch (state)
    {
    case INIT :
      if (!transform_info)
	{
	  transform_info = info_dialog_new ("Shear Information");
	  info_dialog_add_field (transform_info, "X Shear Magnitude: ", xshear_buf, shear_x_mag_changed, tool);
	  info_dialog_add_field (transform_info, "Y Shear Magnitude: ", yshear_buf, shear_y_mag_changed, tool);
	}
      direction_unknown = 1;
      transform_core->trans_info[HORZ_OR_VERT] = HORZ;
      transform_core->trans_info[XSHEAR] = 0.0;
      transform_core->trans_info[YSHEAR] = 0.0;

      return NULL;
      break;

    case MOTION :
      shear_tool_motion (tool, gdisp_ptr);

      return (shear_tool_recalc (tool, gdisp_ptr));
      break;

    case RECALC :
      return (shear_tool_recalc (tool, gdisp_ptr));
      break;

    case FINISH :
      direction_unknown = 1;
      return shear_tool_shear (gdisp->gimage, gimage_active_drawable (gdisp->gimage),
			       transform_core->original, transform_tool_smoothing (),
			       transform_core->transform);
      break;
    }

  return NULL;
}


Tool *
tools_new_shear_tool ()
{
  Tool * tool;
  TransformCore * private;

  tool = transform_core_new (SHEAR, INTERACTIVE);

  private = tool->private;

  /*  set the rotation specific transformation attributes  */
  private->trans_func = shear_tool_transform;

  /*  assemble the transformation matrix  */
  identity_matrix (private->transform);

  return tool;
}


void
tools_free_shear_tool (tool)
     Tool * tool;
{
  transform_core_free (tool);
}

static void
shear_info_update (tool)
     Tool * tool;
{
  TransformCore * transform_core;

  transform_core = (TransformCore *) tool->private;

  sprintf (xshear_buf, "%0.2f", transform_core->trans_info[XSHEAR]);
  sprintf (yshear_buf, "%0.2f", transform_core->trans_info[YSHEAR]);

  info_dialog_update (transform_info);
  info_dialog_popup (transform_info);
}

static void
shear_x_mag_changed (GtkWidget *w,
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

      if (value != transform_core->trans_info[XSHEAR])
	{
	  draw_core_pause (transform_core->core, tool);
	  transform_core->trans_info[XSHEAR] = value;
	  shear_tool_recalc (tool, gdisp);
	  draw_core_resume (transform_core->core, tool);
	}
      
      g_free (str);
    }
}

static void
shear_y_mag_changed (GtkWidget *w,
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

      if (value != transform_core->trans_info[YSHEAR])
	{
	  draw_core_pause (transform_core->core, tool);
	  transform_core->trans_info[YSHEAR] = value;
	  shear_tool_recalc (tool, gdisp);
	  draw_core_resume (transform_core->core, tool);
	}
      
      g_free (str);
    }
}

static void
shear_tool_motion (tool, gdisp_ptr)
     Tool * tool;
     void * gdisp_ptr;
{
  TransformCore * transform_core;
  int diffx, diffy;
  int dir;

  transform_core = (TransformCore *) tool->private;

  diffx = transform_core->curx - transform_core->lastx;
  diffy = transform_core->cury - transform_core->lasty;

  /*  If we haven't yet decided on which way to control shearing
   *  decide using the maximum differential
   */

  if (direction_unknown)
    {
      if (abs(diffx) > MIN_MOVE || abs(diffy) > MIN_MOVE)
	{
	  if (abs(diffx) > abs(diffy))
	    {
	      transform_core->trans_info[HORZ_OR_VERT] = HORZ;
	      transform_core->trans_info[VERT] = 0.0;
	    }
	  else
	    {
	      transform_core->trans_info[HORZ_OR_VERT] = VERT;
	      transform_core->trans_info[HORZ] = 0.0;
	    }

	  direction_unknown = 0;
	}
      /*  set the current coords to the last ones  */
      else
	{
	  transform_core->curx = transform_core->lastx;
	  transform_core->cury = transform_core->lasty;
	}
    }

  /*  if the direction is known, keep track of the magnitude  */
  if (!direction_unknown)
    {
      dir = transform_core->trans_info[HORZ_OR_VERT];
      switch (transform_core->function)
	{
	case HANDLE_1 :
	  if (dir == HORZ)
	    transform_core->trans_info[XSHEAR] -= diffx;
	  else
	    transform_core->trans_info[YSHEAR] -= diffy;
	  break;
	case HANDLE_2 :
	  if (dir == HORZ)
	    transform_core->trans_info[XSHEAR] -= diffx;
	  else
	    transform_core->trans_info[YSHEAR] += diffy;
	  break;
	case HANDLE_3 :
	  if (dir == HORZ)
	    transform_core->trans_info[XSHEAR] += diffx;
	  else
	    transform_core->trans_info[YSHEAR] -= diffy;
	  break;
	case HANDLE_4 :
	  if (dir == HORZ)
	    transform_core->trans_info[XSHEAR] += diffx;
	  else
	    transform_core->trans_info[YSHEAR] += diffy;
	  break;
	default :
	  return;
	}
    }
}


static void *
shear_tool_recalc (tool, gdisp_ptr)
     Tool * tool;
     void * gdisp_ptr;
{
  TransformCore * transform_core;
  GDisplay * gdisp;
  float width, height;
  float cx, cy;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  cx = (transform_core->x1 + transform_core->x2) / 2.0;
  cy = (transform_core->y1 + transform_core->y2) / 2.0;

  width = transform_core->x2 - transform_core->x1;
  height = transform_core->y2 - transform_core->y1;

  if (width == 0)
    width = 1;
  if (height == 0)
    height = 1;

  /*  assemble the transformation matrix  */
  identity_matrix  (transform_core->transform);
  translate_matrix (transform_core->transform, -cx, -cy);

  /*  shear matrix  */
  if (transform_core->trans_info[HORZ_OR_VERT] == HORZ)
    xshear_matrix (transform_core->transform,
		   (float) transform_core->trans_info [XSHEAR] / height);
  else
    yshear_matrix (transform_core->transform,
		   (float) transform_core->trans_info [YSHEAR] / width);

  translate_matrix (transform_core->transform, +cx, +cy);

  /*  transform the bounding box  */
  transform_bounding_box (tool);

  /*  update the information dialog  */
  shear_info_update (tool);

  return (void *) 1;
}


static void *
shear_tool_shear (gimage, drawable, float_tiles, interpolation, matrix)
     GImage *gimage;
     GimpDrawable *drawable;
     TileManager *float_tiles;
     int interpolation;
     Matrix matrix;
{
  return transform_core_do (gimage, drawable, float_tiles, interpolation, matrix);
}


/*  The shear procedure definition  */
ProcArg shear_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_DRAWABLE,
    "drawable",
    "the affected drawable"
  },
  { PDB_INT32,
    "interpolation",
    "whether to use interpolation"
  },
  { PDB_INT32,
    "shear_type",
    "Type of shear: { HORIZONTAL (0), VERTICAL (1) }"
  },
  { PDB_FLOAT,
    "magnitude",
    "the magnitude of the shear"
  }
};

ProcArg shear_out_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the sheard drawable"
  }
};

ProcRecord shear_proc =
{
  "gimp_shear",
  "Shear the specified drawable about its center by the specified magnitude",
  "This tool shears the specified drawable if no selection exists.  If a selection exists, the portion of the drawable which lies under the selection is cut from the drawable and made into a floating selection which is then sheard by the specified amount.  The interpolation parameter can be set to TRUE to indicate that either linear or cubic interpolation should be used to smooth the resulting sheard drawable.  The return value is the ID of the sheard drawable.  If there was no selection, this will be equal to the drawable ID supplied as input.  Otherwise, this will be the newly created and sheard drawable.  The shear type parameter indicates whether the shear will be applied horizontally or vertically.  The magnitude can be either positive or negative and indicates the extent (in pixels) to shear by.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  5,
  shear_args,

  /*  Output arguments  */
  1,
  shear_out_args,

  /*  Exec method  */
  { { shear_invoker } },
};


static Argument *
shear_invoker (args)
     Argument *args;
{
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  int interpolation;
  int shear_type;
  double shear_magnitude;
  int int_value;
  TileManager *float_tiles;
  TileManager *new_tiles;
  Matrix matrix;
  int new_layer;
  Layer *layer;
  Argument *return_args;

  drawable = NULL;
  shear_type  = HORZ;
  layer       = NULL;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  interpolation  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      interpolation = (int_value) ? TRUE : FALSE;
    }
  /*  shear type */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      switch (int_value)
	{
	case 0: shear_type = HORZ; break;
	case 1: shear_type = VERT; break;
	default: success = FALSE;
	}
    }
  /*  shear extents  */
  if (success)
    {
      shear_magnitude = args[4].value.pdb_float;
    }

  /*  call the shear procedure  */
  if (success)
    {
      double cx, cy;

      /*  Start a transform undo group  */
      undo_push_group_start (gimage, TRANSFORM_CORE_UNDO);

      /*  Cut/Copy from the specified drawable  */
      float_tiles = transform_core_cut (gimage, drawable, &new_layer);

      cx = float_tiles->x + float_tiles->levels[0].width / 2.0;
      cy = float_tiles->y + float_tiles->levels[0].height / 2.0;

      identity_matrix  (matrix);
      translate_matrix (matrix, -cx, -cy);
      /*  shear matrix  */
      if (shear_type == HORZ)
	xshear_matrix (matrix, shear_magnitude / float_tiles->levels[0].height);
      else if (shear_type == VERT)
	yshear_matrix (matrix, shear_magnitude / float_tiles->levels[0].width);
      translate_matrix (matrix, +cx, +cy);

      /*  shear the buffer  */
      new_tiles = shear_tool_shear (gimage, drawable, float_tiles, interpolation, matrix);

      /*  free the cut/copied buffer  */
      tile_manager_destroy (float_tiles);

      if (new_tiles)
	success = (layer = transform_core_paste (gimage, drawable, new_tiles, new_layer)) != NULL;
      else
	success = FALSE;

      /*  push the undo group end  */
      undo_push_group_end (gimage);
    }

  return_args = procedural_db_return_args (&shear_proc, success);

  if (success)
    return_args[1].value.pdb_int = drawable_ID (GIMP_DRAWABLE(layer));

  return return_args;
}
