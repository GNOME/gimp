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

#include "libgimp/gimpintl.h"

#include "tile_manager_pvt.h"


/*  storage for information dialog fields  */
static char        matrix_row_buf [3][MAX_INFO_BUF];

/*  forward function declarations  */
static void *      perspective_tool_recalc      (Tool *, void *);
static void        perspective_tool_motion      (Tool *, void *);
static void        perspective_info_update      (Tool *);

void *
perspective_tool_transform (Tool     *tool,
			    gpointer  gdisp_ptr,
			    int       state)
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
	  transform_info =
	    info_dialog_new (_("Perspective Transform Information"),
			     tools_help_func, NULL);
	  info_dialog_add_label (transform_info, _("Matrix:"),
				 matrix_row_buf[0]);
	  info_dialog_add_label (transform_info, "", matrix_row_buf[1]);
	  info_dialog_add_label (transform_info, "", matrix_row_buf[2]);
	}
      gtk_widget_set_sensitive (GTK_WIDGET (transform_info->shell), TRUE);

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
      gtk_widget_set_sensitive (GTK_WIDGET (transform_info->shell), FALSE);
      return
	perspective_tool_perspective (gdisp->gimage,
				      gimage_active_drawable (gdisp->gimage),
				      gdisp,
				      transform_core->original,
				      transform_tool_smoothing (),
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
  gimp_matrix_identity (private->transform);

  return tool;
}


void
tools_free_perspective_tool (Tool *tool)
{
  transform_core_free (tool);
}


static void
perspective_info_update (Tool *tool)
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
	  p += g_snprintf (p, MAX_INFO_BUF - (p - matrix_row_buf[i]),
			   "%10.3g", transform_core->transform[i][j]);
	}
    }

  info_dialog_update (transform_info);
  info_dialog_popup (transform_info);
  
  return;
}


static void
perspective_tool_motion (Tool *tool,
			 void *gdisp_ptr)
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
perspective_tool_recalc (Tool *tool,
			 void *gdisp_ptr)
{
  TransformCore * transform_core;
  GDisplay * gdisp;
  GimpMatrix m;
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
  gimp_matrix_identity  (transform_core->transform);
  gimp_matrix_translate (transform_core->transform, -cx, -cy);
  gimp_matrix_scale     (transform_core->transform, scalex, scaley);
  gimp_matrix_mult      (m, transform_core->transform);

  /*  transform the bounding box  */
  transform_bounding_box (tool);

  /*  update the information dialog  */
  perspective_info_update (tool);

  return (void *) 1;
}


void
perspective_find_transform (double     *coords,
			    GimpMatrix  m)
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


void *
perspective_tool_perspective (GImage       *gimage,
			      GimpDrawable *drawable,
			      GDisplay     *gdisp,
			      TileManager  *float_tiles,
			      int           interpolation,
			      GimpMatrix    matrix)
{
  void *ret;
  gimp_progress *progress;

  progress = progress_start (gdisp, _("Perspective..."), FALSE, NULL, NULL);

  ret = transform_core_do (gimage, drawable, float_tiles,
			   interpolation, matrix,
			   progress? progress_update_and_flush:NULL, progress);
  if (progress)
    progress_end (progress);

  return ret;
}
