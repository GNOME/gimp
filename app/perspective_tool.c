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
#include "perspective_tool.h"
#include "selection.h"
#include "tools.h"
#include "transform_core.h"
#include "transform_tool.h"
#include "undo.h"

#include "tile_manager_pvt.h"

#define X0 0
#define Y0 1
#define X1 2
#define Y1 3
#define X2 4
#define Y2 5
#define X3 6
#define Y3 7

/*  storage for information dialog fields  */
static char        matrix_row_buf [3][MAX_INFO_BUF];

/*  forward function declarations  */
static void *      perspective_tool_perspective (GImage *, GimpDrawable *, TileManager *, int, Matrix);
static void        perspective_find_transform   (double *, Matrix);
static void *      perspective_tool_recalc      (Tool *, void *);
static void        perspective_tool_motion      (Tool *, void *);
static void        perspective_info_update      (Tool *);
static Argument *  perspective_invoker          (Argument *);

void *
perspective_tool_transform (tool, gdisp_ptr, state)
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
	  transform_info = info_dialog_new ("Perspective Transform Information");
	  info_dialog_add_field (transform_info, "Matrix: ",
				 matrix_row_buf[0], NULL, NULL);
	  info_dialog_add_field (transform_info, "        ",
				 matrix_row_buf[1], NULL, NULL);
	  info_dialog_add_field (transform_info, "        ",
				 matrix_row_buf[2], NULL, NULL);
	}

      transform_core->trans_info [X0] = (double) transform_core->x1;
      transform_core->trans_info [Y0] = (double) transform_core->y1;
      transform_core->trans_info [X1] = (double) transform_core->x2;
      transform_core->trans_info [Y1] = (double) transform_core->y1;
      transform_core->trans_info [X2] = (double) transform_core->x1;
      transform_core->trans_info [Y2] = (double) transform_core->y2;
      transform_core->trans_info [X3] = (double) transform_core->x2;
      transform_core->trans_info [Y3] = (double) transform_core->y2;

      return NULL;
      break;

    case MOTION :
      perspective_tool_motion (tool, gdisp_ptr);

      return (perspective_tool_recalc (tool, gdisp_ptr));
      break;

    case RECALC :
      return (perspective_tool_recalc (tool, gdisp_ptr));
      break;

    case FINISH :
      /*  Let the transform core handle the inverse mapping  */
      return perspective_tool_perspective (gdisp->gimage, gimage_active_drawable (gdisp->gimage),
					   transform_core->original, transform_tool_smoothing (),
					   transform_core->transform);
      break;
    }

  return NULL;
}


Tool *
tools_new_perspective_tool ()
{
  Tool * tool;
  TransformCore * private;

  tool = transform_core_new (PERSPECTIVE, INTERACTIVE);

  private = tool->private;

  /*  set the rotation specific transformation attributes  */
  private->trans_func = perspective_tool_transform;
  private->trans_info[X0] = 0;
  private->trans_info[Y0] = 0;
  private->trans_info[X1] = 0;
  private->trans_info[Y1] = 0;
  private->trans_info[X2] = 0;
  private->trans_info[Y2] = 0;
  private->trans_info[X3] = 0;
  private->trans_info[Y3] = 0;

  /*  assemble the transformation matrix  */
  identity_matrix (private->transform);

  return tool;
}


void
tools_free_perspective_tool (tool)
     Tool * tool;
{
  transform_core_free (tool);
}


static void
perspective_info_update (tool)
     Tool * tool;
{
  TransformCore * transform_core;
  int i;
  
  transform_core = (TransformCore *) tool->private;

  for (i = 0; i < 3; i++)
    {
      char *p = matrix_row_buf[i];
      int j;
      
      for (j = 0; j < 3; j++)
	{
	  p += sprintf (p, "%10.3g", transform_core->transform[i][j]);
	}
    }

  info_dialog_update (transform_info);
  info_dialog_popup (transform_info);
  
  return;
}


static void
perspective_tool_motion (tool, gdisp_ptr)
     Tool * tool;
     void * gdisp_ptr;
{
  GDisplay * gdisp;
  TransformCore * transform_core;
  int diff_x, diff_y;

  gdisp = (GDisplay *) gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  diff_x = transform_core->curx - transform_core->lastx;
  diff_y = transform_core->cury - transform_core->lasty;

  switch (transform_core->function)
    {
    case HANDLE_1 :
      transform_core->trans_info [X0] += diff_x;
      transform_core->trans_info [Y0] += diff_y;
      break;
    case HANDLE_2 :
      transform_core->trans_info [X1] += diff_x;
      transform_core->trans_info [Y1] += diff_y;
      break;
    case HANDLE_3 :
      transform_core->trans_info [X2] += diff_x;
      transform_core->trans_info [Y2] += diff_y;
      break;
    case HANDLE_4 :
      transform_core->trans_info [X3] += diff_x;
      transform_core->trans_info [Y3] += diff_y;
      break;
    default :
      return;
    }
}

static void *
perspective_tool_recalc (tool, gdisp_ptr)
     Tool * tool;
     void * gdisp_ptr;
{
  TransformCore * transform_core;
  GDisplay * gdisp;
  Matrix m;
  double cx, cy;
  double scalex, scaley;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  /*  determine the perspective transform that maps from
   *  the unit cube to the trans_info coordinates
   */
  perspective_find_transform (transform_core->trans_info, m);

  cx     = transform_core->x1;
  cy     = transform_core->y1;
  scalex = 1.0;
  scaley = 1.0;
  if (transform_core->x2 - transform_core->x1)
    scalex = 1.0 / (transform_core->x2 - transform_core->x1);
  if (transform_core->y2 - transform_core->y1)
    scaley = 1.0 / (transform_core->y2 - transform_core->y1);

  /*  assemble the transformation matrix  */
  identity_matrix  (transform_core->transform);
  translate_matrix (transform_core->transform, -cx, -cy);
  scale_matrix     (transform_core->transform, scalex, scaley);
  mult_matrix      (m, transform_core->transform);

  /*  transform the bounding box  */
  transform_bounding_box (tool);

  /*  update the information dialog  */
  perspective_info_update (tool);

  return (void *) 1;
}


static void
perspective_find_transform (coords, m)
     double * coords;
     Matrix m;
{
  double dx1, dx2, dx3, dy1, dy2, dy3;
  double det1, det2;

  dx1 = coords[X1] - coords[X3];
  dx2 = coords[X2] - coords[X3];
  dx3 = coords[X0] - coords[X1] + coords[X3] - coords[X2];

  dy1 = coords[Y1] - coords[Y3];
  dy2 = coords[Y2] - coords[Y3];
  dy3 = coords[Y0] - coords[Y1] + coords[Y3] - coords[Y2];

  /*  Is the mapping affine?  */
  if ((dx3 == 0.0) && (dy3 == 0.0))
    {
      m[0][0] = coords[X1] - coords[X0];
      m[0][1] = coords[X3] - coords[X1];
      m[0][2] = coords[X0];
      m[1][0] = coords[Y1] - coords[Y0];
      m[1][1] = coords[Y3] - coords[Y1];
      m[1][2] = coords[Y0];
      m[2][0] = 0.0;
      m[2][1] = 0.0;
    }
  else
    {
      det1 = dx3 * dy2 - dy3 * dx2;
      det2 = dx1 * dy2 - dy1 * dx2;
      m[2][0] = det1 / det2;
      det1 = dx1 * dy3 - dy1 * dx3;
      det2 = dx1 * dy2 - dy1 * dx2;
      m[2][1] = det1 / det2;

      m[0][0] = coords[X1] - coords[X0] + m[2][0] * coords[X1];
      m[0][1] = coords[X2] - coords[X0] + m[2][1] * coords[X2];
      m[0][2] = coords[X0];

      m[1][0] = coords[Y1] - coords[Y0] + m[2][0] * coords[Y1];
      m[1][1] = coords[Y2] - coords[Y0] + m[2][1] * coords[Y2];
      m[1][2] = coords[Y0];
    }

  m[2][2] = 1.0;
}


static void *
perspective_tool_perspective (gimage, drawable, float_tiles, interpolation, matrix)
     GImage *gimage;
     GimpDrawable *drawable;
     TileManager *float_tiles;
     int interpolation;
     Matrix matrix;
{
  return transform_core_do (gimage, drawable, float_tiles, interpolation, matrix);
}


/*  The perspective procedure definition  */
ProcArg perspective_args[] =
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
    "x0",
    "the new x coordinate of upper-left corner of original bounding box"
  },
  { PDB_FLOAT,
    "y0",
    "the new y coordinate of upper-left corner of original bounding box"
  },
  { PDB_FLOAT,
    "x1",
    "the new x coordinate of upper-right corner of original bounding box"
  },
  { PDB_FLOAT,
    "y1",
    "the new y coordinate of upper-right corner of original bounding box"
  },
  { PDB_FLOAT,
    "x2",
    "the new x coordinate of lower-left corner of original bounding box"
  },
  { PDB_FLOAT,
    "y2",
    "the new y coordinate of lower-left corner of original bounding box"
  },
  { PDB_FLOAT,
    "x3",
    "the new x coordinate of lower-right corner of original bounding box"
  },
  { PDB_FLOAT,
    "y3",
    "the new y coordinate of lower-right corner of original bounding box"
  }
};

ProcArg perspective_out_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the newly mapped drawable"
  }
};

ProcRecord perspective_proc =
{
  "gimp_perspective",
  "Perform a possibly non-affine transformation on the specified drawable",
  "This tool performs a possibly non-affine transformation on the specified drawable by allowing the corners of the original bounding box to be arbitrarily remapped to any values.  The specified drawable is remapped if no selection exists.  However, if a selection exists, the portion of the drawable which lies under the selection is cut from the drawable and made into a floating selection which is then remapped as specified.  The interpolation parameter can be set to TRUE to indicate that either linear or cubic interpolation should be used to smooth the resulting remapped drawable.  The return value is the ID of the remapped drawable.  If there was no selection, this will be equal to the drawable ID supplied as input.  Otherwise, this will be the newly created and remapped drawable.  The 4 coordinates specify the new locations of each corner of the original bounding box.  By specifying these values, any affine transformation (rotation, scaling, translation) can be affected.  Additionally, these values can be specified such that the resulting transformed drawable will appear to have been projected via a perspective transform.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  11,
  perspective_args,

  /*  Output arguments  */
  1,
  perspective_out_args,

  /*  Exec method  */
  { { perspective_invoker } },
};


static Argument *
perspective_invoker (args)
     Argument *args;
{
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  int interpolation;
  double trans_info[8];
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
  /*  perspective extents  */
  if (success)
    {
      trans_info[X0] = args[3].value.pdb_float;
      trans_info[Y0] = args[4].value.pdb_float;
      trans_info[X1] = args[5].value.pdb_float;
      trans_info[Y1] = args[6].value.pdb_float;
      trans_info[X2] = args[7].value.pdb_float;
      trans_info[Y2] = args[8].value.pdb_float;
      trans_info[X3] = args[9].value.pdb_float;
      trans_info[Y3] = args[10].value.pdb_float;
    }

  /*  call the perspective procedure  */
  if (success)
    {
      double cx, cy;
      double scalex, scaley;
      Matrix m;

      /*  Start a transform undo group  */
      undo_push_group_start (gimage, TRANSFORM_CORE_UNDO);

      /*  Cut/Copy from the specified drawable  */
      float_tiles = transform_core_cut (gimage, drawable, &new_layer);

      /*  determine the perspective transform that maps from
       *  the unit cube to the trans_info coordinates
       */
      perspective_find_transform (trans_info, m);

      cx     = float_tiles->x;
      cy     = float_tiles->y;
      scalex = 1.0;
      scaley = 1.0;
      if (float_tiles->width)
	scalex = 1.0 / float_tiles->width;
      if (float_tiles->height)
	scaley = 1.0 / float_tiles->height;

      /*  assemble the transformation matrix  */
      identity_matrix  (matrix);
      translate_matrix (matrix, -cx, -cy);
      scale_matrix     (matrix, scalex, scaley);
      mult_matrix      (m, matrix);

      /*  perspective the buffer  */
      new_tiles = perspective_tool_perspective (gimage, drawable, float_tiles, interpolation, matrix);

      /*  free the cut/copied buffer  */
      tile_manager_destroy (float_tiles);

      if (new_tiles)
	success = (layer = transform_core_paste (gimage, drawable, new_tiles, new_layer)) != NULL;
      else
	success = FALSE;

      /*  push the undo group end  */
      undo_push_group_end (gimage);
    }

  return_args = procedural_db_return_args (&perspective_proc, success);

  if (success)
    return_args[1].value.pdb_int = drawable_ID (GIMP_DRAWABLE(layer));

  return return_args;
}
