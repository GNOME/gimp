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
#include "shear_tool.h"
#include "selection.h"
#include "transform_tool.h"
#include "undo.h"

#include "tile_manager_pvt.h"

#include "config.h"
#include "libgimp/gimpintl.h"
#include "libgimp/gimpmath.h"

/*  index into trans_info array  */
#define HORZ_OR_VERT 0
#define XSHEAR       1
#define YSHEAR       2

/*  the minimum movement before direction of shear can be determined (pixels) */
#define MIN_MOVE     5

/*  variables local to this file  */
static gdouble  xshear_val;
static gdouble  yshear_val;

/*  forward function declarations  */
static void   shear_tool_recalc  (Tool *, void *);
static void   shear_tool_motion  (Tool *, void *);
static void   shear_info_update  (Tool *);

/*  Info dialog callback funtions  */
static void   shear_x_mag_changed (GtkWidget *widget, gpointer data);
static void   shear_y_mag_changed (GtkWidget *widget, gpointer data);

TileManager *
shear_tool_transform (Tool           *tool,
		      gpointer        gdisp_ptr,
		      TransformState  state)
{
  TransformCore *transform_core;
  GDisplay       *gdisp;

  transform_core = (TransformCore *) tool->private;
  gdisp = (GDisplay *) gdisp_ptr;

  switch (state)
    {
    case INIT:
      if (!transform_info)
	{
	  transform_info = info_dialog_new (_("Shear Information"),
					    gimp_standard_help_func,
					    "tools/transform_shear.html");

	  info_dialog_add_spinbutton (transform_info,
				      _("Shear Magnitude X:"),
				      &xshear_val,
				      -65536, 65536, 1, 15, 1, 1, 0,
				      shear_x_mag_changed, tool);

	  info_dialog_add_spinbutton (transform_info,
				      _("Y:"),
				      &yshear_val,
				      -65536, 65536, 1, 15, 1, 1, 0,
				      shear_y_mag_changed, tool);
	}
      gtk_widget_set_sensitive (GTK_WIDGET (transform_info->shell), TRUE);
      transform_core->trans_info[HORZ_OR_VERT] = ORIENTATION_UNKNOWN;
      transform_core->trans_info[XSHEAR] = 0.0;
      transform_core->trans_info[YSHEAR] = 0.0;

      return NULL;
      break;

    case MOTION:
      shear_tool_motion (tool, gdisp_ptr);
      shear_tool_recalc (tool, gdisp_ptr);
      break;

    case RECALC:
      shear_tool_recalc (tool, gdisp_ptr);
      break;

    case FINISH:
      gtk_widget_set_sensitive (GTK_WIDGET (transform_info->shell), FALSE);
      return shear_tool_shear (gdisp->gimage,
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
tools_new_shear_tool (void)
{
  Tool          *tool;
  TransformCore *private;

  tool = transform_core_new (SHEAR, TRUE);

  private = tool->private;

  /*  set the rotation specific transformation attributes  */
  private->trans_func = shear_tool_transform;

  /*  assemble the transformation matrix  */
  gimp_matrix3_identity (private->transform);

  return tool;
}

void
tools_free_shear_tool (Tool *tool)
{
  transform_core_free (tool);
}

static void
shear_info_update (Tool *tool)
{
  TransformCore *transform_core;

  transform_core = (TransformCore *) tool->private;

  xshear_val = transform_core->trans_info[XSHEAR];
  yshear_val = transform_core->trans_info[YSHEAR];

  info_dialog_update (transform_info);
  info_dialog_popup (transform_info);
}

static void
shear_x_mag_changed (GtkWidget *widget,
		     gpointer   data)
{
  Tool          *tool;
  TransformCore *transform_core;
  GDisplay      *gdisp;
  gint           value;

  tool = (Tool *) data;

  if (tool)
    {
      gdisp = (GDisplay *) tool->gdisp_ptr;
      transform_core = (TransformCore *) tool->private;

      value = GTK_ADJUSTMENT (widget)->value;

      if (value != transform_core->trans_info[XSHEAR])
	{
	  draw_core_pause (transform_core->core, tool);
	  transform_core->trans_info[XSHEAR] = value;
	  shear_tool_recalc (tool, gdisp);
	  draw_core_resume (transform_core->core, tool);
	}
    }
}

static void
shear_y_mag_changed (GtkWidget *widget,
		     gpointer   data)
{
  Tool          *tool;
  TransformCore *transform_core;
  GDisplay      *gdisp;
  gint           value;

  tool = (Tool *) data;

  if (tool)
    {
      gdisp = (GDisplay *) tool->gdisp_ptr;
      transform_core = (TransformCore *) tool->private;

      value = GTK_ADJUSTMENT (widget)->value;

      if (value != transform_core->trans_info[YSHEAR])
	{
	  draw_core_pause (transform_core->core, tool);
	  transform_core->trans_info[YSHEAR] = value;
	  shear_tool_recalc (tool, gdisp);
	  draw_core_resume (transform_core->core, tool);
	}
    }
}

static void
shear_tool_motion (Tool *tool,
		   void *gdisp_ptr)
{
  TransformCore *transform_core;
  gint           diffx, diffy;
  gint           dir;

  transform_core = (TransformCore *) tool->private;

  diffx = transform_core->curx - transform_core->lastx;
  diffy = transform_core->cury - transform_core->lasty;

  /*  If we haven't yet decided on which way to control shearing
   *  decide using the maximum differential
   */

  if (transform_core->trans_info[HORZ_OR_VERT] == ORIENTATION_UNKNOWN)
    {
      if (abs (diffx) > MIN_MOVE || abs (diffy) > MIN_MOVE)
	{
	  if (abs (diffx) > abs (diffy))
	    {
	      transform_core->trans_info[HORZ_OR_VERT] = ORIENTATION_HORIZONTAL;
	      transform_core->trans_info[ORIENTATION_VERTICAL] = 0.0;
	    }
	  else
	    {
	      transform_core->trans_info[HORZ_OR_VERT] = ORIENTATION_VERTICAL;
	      transform_core->trans_info[ORIENTATION_HORIZONTAL] = 0.0;
	    }
	}
      /*  set the current coords to the last ones  */
      else
	{
	  transform_core->curx = transform_core->lastx;
	  transform_core->cury = transform_core->lasty;
	}
    }

  /*  if the direction is known, keep track of the magnitude  */
  if (transform_core->trans_info[HORZ_OR_VERT] != ORIENTATION_UNKNOWN)
    {
      dir = transform_core->trans_info[HORZ_OR_VERT];
      switch (transform_core->function)
	{
	case HANDLE_1:
	  if (dir == ORIENTATION_HORIZONTAL)
	    transform_core->trans_info[XSHEAR] -= diffx;
	  else
	    transform_core->trans_info[YSHEAR] -= diffy;
	  break;
	case HANDLE_2:
	  if (dir == ORIENTATION_HORIZONTAL)
	    transform_core->trans_info[XSHEAR] -= diffx;
	  else
	    transform_core->trans_info[YSHEAR] += diffy;
	  break;
	case HANDLE_3:
	  if (dir == ORIENTATION_HORIZONTAL)
	    transform_core->trans_info[XSHEAR] += diffx;
	  else
	    transform_core->trans_info[YSHEAR] -= diffy;
	  break;
	case HANDLE_4:
	  if (dir == ORIENTATION_HORIZONTAL)
	    transform_core->trans_info[XSHEAR] += diffx;
	  else
	    transform_core->trans_info[YSHEAR] += diffy;
	  break;
	default:
	  break;
	}
    }
}

static void
shear_tool_recalc (Tool *tool,
		   void *gdisp_ptr)
{
  TransformCore *transform_core;
  GDisplay      *gdisp;
  gfloat         width, height;
  gfloat         cx, cy;

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
  gimp_matrix3_identity  (transform_core->transform);
  gimp_matrix3_translate (transform_core->transform, -cx, -cy);

  /*  shear matrix  */
  if (transform_core->trans_info[HORZ_OR_VERT] == ORIENTATION_HORIZONTAL)
    gimp_matrix3_xshear (transform_core->transform,
			 (float) transform_core->trans_info [XSHEAR] / height);
  else
    gimp_matrix3_yshear (transform_core->transform,
			 (float) transform_core->trans_info [YSHEAR] / width);

  gimp_matrix3_translate (transform_core->transform, +cx, +cy);

  /*  transform the bounding box  */
  transform_core_transform_bounding_box (tool);

  /*  update the information dialog  */
  shear_info_update (tool);
}

TileManager *
shear_tool_shear (GimpImage    *gimage,
		  GimpDrawable *drawable,
		  GDisplay     *gdisp,
		  TileManager  *float_tiles,
		  gboolean      interpolation,
		  GimpMatrix3   matrix)
{
  gimp_progress *progress;
  TileManager   *ret;

  progress = progress_start (gdisp, _("Shearing..."), FALSE, NULL, NULL);

  ret = transform_core_do (gimage, drawable, float_tiles,
			   interpolation, matrix,
			   progress ? progress_update_and_flush : NULL,
			   progress);

  if (progress)
    progress_end (progress);

  return ret;
}
