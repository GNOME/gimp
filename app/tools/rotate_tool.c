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
#include "rotate_tool.h"
#include "selection.h"
#include "tools.h"
#include "transform_core.h"
#include "transform_tool.h"
#include "undo.h"

#include "tile_manager_pvt.h"

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /* M_PI */

/*  index into trans_info array  */
#define ANGLE          0
#define REAL_ANGLE     1
#define EPSILON        0.018  /*  ~ 1 degree  */
#define FIFTEEN_DEG    (M_PI / 12.0)

/*  variables local to this file  */
char          angle_buf  [MAX_INFO_BUF];

/*  forward function declarations  */
static void *      rotate_tool_rotate  (GImage *, GimpDrawable *, double, TileManager *, int, Matrix);
static void *      rotate_tool_recalc  (Tool *, void *);
static void        rotate_tool_motion  (Tool *, void *);
static void        rotate_info_update  (Tool *);
static Argument *  rotate_invoker      (Argument *);

void *
rotate_tool_transform (tool, gdisp_ptr, state)
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
	  transform_info = info_dialog_new ("Rotation Information");
	  info_dialog_add_field (transform_info, "Angle: ", angle_buf);
	}

      transform_core->trans_info[ANGLE]      = 0.0;
      transform_core->trans_info[REAL_ANGLE] = 0.0;

      return NULL;
      break;

    case MOTION :
      rotate_tool_motion (tool, gdisp_ptr);

      return (rotate_tool_recalc (tool, gdisp_ptr));
      break;

    case RECALC :
      return (rotate_tool_recalc (tool, gdisp_ptr));
      break;

    case FINISH :
      return rotate_tool_rotate (gdisp->gimage, gimage_active_drawable (gdisp->gimage),
				 transform_core->trans_info[ANGLE], transform_core->original,
				 transform_tool_smoothing (), transform_core->transform);
      break;
    }

  return NULL;
}

Tool *
tools_new_rotate_tool ()
{
  Tool * tool;
  TransformCore * private;

  tool = transform_core_new (ROTATE, INTERACTIVE);

  private = tool->private;

  /*  set the rotation specific transformation attributes  */
  private->trans_func = rotate_tool_transform;
  private->trans_info[ANGLE]      = 0.0;
  private->trans_info[REAL_ANGLE] = 0.0;

  /*  assemble the transformation matrix  */
  identity_matrix (private->transform);

  return tool;
}

void
tools_free_rotate_tool (tool)
     Tool * tool;
{
  transform_core_free (tool);
}

static void
rotate_info_update (tool)
     Tool * tool;
{
  GDisplay * gdisp;
  TransformCore * transform_core;
  double angle;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  angle = (transform_core->trans_info[ANGLE] * 180.0) / M_PI;

  sprintf (angle_buf, "%0.2f", angle);

  info_dialog_update (transform_info);
  info_dialog_popup (transform_info);
}

static void
rotate_tool_motion (tool, gdisp_ptr)
     Tool * tool;
     void * gdisp_ptr;
{
  TransformCore * transform_core;
  double angle1, angle2, angle;
  double cx, cy;
  double x1, y1, x2, y2;

  transform_core = (TransformCore *) tool->private;

  cx = (transform_core->x1 + transform_core->x2) / 2.0;
  cy = (transform_core->y1 + transform_core->y2) / 2.0;

  x1 = transform_core->curx - cx;
  x2 = transform_core->lastx - cx;
  y1 = cy - transform_core->cury;
  y2 = cy - transform_core->lasty;

  /*  find the first angle  */
  angle1 = atan2 (y1, x1);

  /*  find the angle  */
  angle2 = atan2 (y2, x2);

  angle = angle2 - angle1;

  if (angle > M_PI || angle < -M_PI)
    angle = angle2 - ((angle1 < 0) ? 2*M_PI + angle1 : angle1 - 2*M_PI);

  /*  increment the transform tool's angle  */
  transform_core->trans_info[REAL_ANGLE] += angle;

  /*  limit the angle to between 0 and 360 degrees  */
  if (transform_core->trans_info[REAL_ANGLE] < - M_PI)
    transform_core->trans_info[REAL_ANGLE] = 2 * M_PI - transform_core->trans_info[REAL_ANGLE];
  else if (transform_core->trans_info[REAL_ANGLE] > M_PI)
    transform_core->trans_info[REAL_ANGLE] = transform_core->trans_info[REAL_ANGLE] - 2 * M_PI;

  /* constrain the angle to 15-degree multiples if ctrl is held down */
  
  if (transform_core->state & ControlMask)
    transform_core->trans_info[ANGLE] = FIFTEEN_DEG * (int) ((transform_core->trans_info[REAL_ANGLE] +
							      FIFTEEN_DEG / 2.0) /
							     FIFTEEN_DEG);
  else
    transform_core->trans_info[ANGLE] = transform_core->trans_info[REAL_ANGLE];
}

static void *
rotate_tool_recalc (tool, gdisp_ptr)
     Tool * tool;
     void * gdisp_ptr;
{
  TransformCore * transform_core;
  GDisplay * gdisp;
  double cx, cy;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  cx = (transform_core->x1 + transform_core->x2) / 2.0;
  cy = (transform_core->y1 + transform_core->y2) / 2.0;

  /*  assemble the transformation matrix  */
  identity_matrix  (transform_core->transform);
  translate_matrix (transform_core->transform, -cx, -cy);
  rotate_matrix    (transform_core->transform, transform_core->trans_info[ANGLE]);
  translate_matrix (transform_core->transform, +cx, +cy);

  /*  transform the bounding box  */
  transform_bounding_box (tool);

  /*  update the information dialog  */
  rotate_info_update (tool);

  return (void *) 1;
}


/*  This procedure returns a valid pointer to a new selection if the
 *  requested angle is a multiple of 90 degrees...
 */

static void *
rotate_tool_rotate (gimage, drawable, angle, float_tiles, interpolation, matrix)
     GImage *gimage;
     GimpDrawable *drawable;
     double angle;
     TileManager *float_tiles;
     int interpolation;
     Matrix matrix;
{
  return transform_core_do (gimage, drawable, float_tiles, interpolation, matrix);
}


/*  The rotate procedure definition  */
ProcArg rotate_args[] =
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
  { PDB_FLOAT,
    "angle",
    "the angle of rotation (radians)",
  }
};

ProcArg rotate_out_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the rotated drawable"
  }
};

ProcRecord rotate_proc =
{
  "gimp_rotate",
  "Rotate the specified drawable about its center through the specified angle",
  "This tool rotates the specified drawable if no selection exists.  If a selection exists, the portion of the drawable which lies under the selection is cut from the drawable and made into a floating selection which is then rotated by the specified amount.  The interpolation parameter can be set to TRUE to indicate that either linear or cubic interpolation should be used to smooth the resulting rotated drawable.  The return value is the ID of the rotated drawable.  If there was no selection, this will be equal to the drawable ID supplied as input.  Otherwise, this will be the newly created and rotated drawable.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  4,
  rotate_args,

  /*  Output arguments  */
  1,
  rotate_out_args,

  /*  Exec method  */
  { { rotate_invoker } },
};


static Argument *
rotate_invoker (args)
     Argument *args;
{
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  int interpolation;
  double angle;
  int int_value;
  TileManager *float_tiles;
  TileManager *new_tiles;
  Matrix matrix;
  int new_layer;
  Layer *layer;
  Argument *return_args;

  drawable = NULL;
  layer = NULL;

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
  /*  angle of rotation  */
  if (success)
    angle = args[3].value.pdb_float;

  /*  call the rotate procedure  */
  if (success)
    {
      double cx, cy;

      /*  Start a transform undo group  */
      undo_push_group_start (gimage, TRANSFORM_CORE_UNDO);

      /*  Cut/Copy from the specified drawable  */
      float_tiles = transform_core_cut (gimage, drawable, &new_layer);

      cx = float_tiles->x + float_tiles->levels[0].width / 2.0;
      cy = float_tiles->y + float_tiles->levels[0].height / 2.0;

      /*  assemble the transformation matrix  */
      identity_matrix  (matrix);
      translate_matrix (matrix, -cx, -cy);
      rotate_matrix    (matrix, angle);
      translate_matrix (matrix, +cx, +cy);

      /*  rotate the buffer  */
      new_tiles = rotate_tool_rotate (gimage, drawable, angle, float_tiles, interpolation, matrix);

      /*  free the cut/copied buffer  */
      tile_manager_destroy (float_tiles);

      if (new_tiles)
	success = (layer = transform_core_paste (gimage, drawable, new_tiles, new_layer)) != NULL;
      else
	success = FALSE;

      /*  push the undo group end  */
      undo_push_group_end (gimage);
    }

  return_args = procedural_db_return_args (&rotate_proc, success);

  if (success)
    return_args[1].value.pdb_int = drawable_ID (GIMP_DRAWABLE(layer));

  return return_args;
}
