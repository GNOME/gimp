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

#include "config.h"

#include "appenv.h"
#include "drawable.h"
#include "gdisplay.h"
#include "gimage_mask.h"
#include "info_dialog.h"
#include "scale_tool.h"
#include "selection.h"
#include "transform_tool.h"
#include "undo.h"

#include "tile_manager_pvt.h"

#include "libgimp/gimpmath.h"
#include "libgimp/gimplimits.h"
#include "libgimp/gimpintl.h"


/*  storage for information dialog fields  */
static gchar      orig_width_buf[MAX_INFO_BUF];
static gchar      orig_height_buf[MAX_INFO_BUF];
static gdouble    size_vals[2];
static gchar      x_ratio_buf[MAX_INFO_BUF];
static gchar      y_ratio_buf[MAX_INFO_BUF];

/*  needed for original size unit update  */
static GtkWidget *sizeentry;

/*  forward function declarations  */
static void   scale_tool_recalc  (Tool *, void *);
static void   scale_tool_motion  (Tool *, void *);
static void   scale_info_update  (Tool *);

/*  callback functions for the info dialog fields  */
static void   scale_size_changed (GtkWidget *widget, gpointer data);
static void   scale_unit_changed (GtkWidget *widget, gpointer data);

TileManager *
scale_tool_transform (Tool           *tool,
		      gpointer        gdisp_ptr,
		      TransformState  state)
{
  GDisplay      *gdisp;
  TransformCore *transform_core;
  GtkWidget     *spinbutton;

  gdisp = (GDisplay *) gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  switch (state)
    {
    case INIT:
      size_vals[0] = transform_core->x2 - transform_core->x1;
      size_vals[1] = transform_core->y2 - transform_core->y1;

      if (!transform_info)
	{
	  transform_info = info_dialog_new (_("Scaling Information"),
					    gimp_standard_help_func,
					    "tools/transform_scale.html");

	  info_dialog_add_label (transform_info, _("Original Width:"),
				 orig_width_buf);
	  info_dialog_add_label (transform_info, _("Height:"),
				 orig_height_buf);

	  spinbutton =
	    info_dialog_add_spinbutton (transform_info, _("Current Width:"),
					NULL, -1, 1, 1, 10, 1, 1, 2, NULL, NULL);
	  sizeentry =
	    info_dialog_add_sizeentry (transform_info, _("Height:"),
				       size_vals, 1,
				       gdisp->gimage->unit, "%a",
				       TRUE, TRUE, FALSE,
				       GIMP_SIZE_ENTRY_UPDATE_SIZE,
				       scale_size_changed, tool);
	  gtk_signal_connect (GTK_OBJECT (sizeentry), "unit_changed",
			      scale_unit_changed, tool);

	  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (sizeentry),
				     GTK_SPIN_BUTTON (spinbutton), NULL);

	  info_dialog_add_label (transform_info, _("Scale Ratio X:"),
				 x_ratio_buf);
	  info_dialog_add_label (transform_info, _("Y:"),
				 y_ratio_buf);

	  gtk_table_set_row_spacing (GTK_TABLE (transform_info->info_table),
				     1, 4);
	  gtk_table_set_row_spacing (GTK_TABLE (transform_info->info_table),
				     2, 0);
	}

      gtk_signal_handler_block_by_data (GTK_OBJECT (sizeentry), tool);

      gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (sizeentry),
				gdisp->gimage->unit);
      if (gdisp->dot_for_dot)
	gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (sizeentry), GIMP_UNIT_PIXEL);

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry), 0,
				      gdisp->gimage->xresolution, FALSE);
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry), 1,
				      gdisp->gimage->yresolution, FALSE);

      gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 0,
					     GIMP_MIN_IMAGE_SIZE,
					     GIMP_MAX_IMAGE_SIZE);
      gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 1,
					     GIMP_MIN_IMAGE_SIZE,
					     GIMP_MAX_IMAGE_SIZE);

      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (sizeentry), 0,
				0, size_vals[0]);
      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (sizeentry), 1,
				0, size_vals[1]);

      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 0,
				  size_vals[0]);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 1,
				  size_vals[1]);

      gtk_widget_set_sensitive (GTK_WIDGET (transform_info->shell), TRUE);

      gtk_signal_handler_unblock_by_data (GTK_OBJECT (sizeentry), tool);

      transform_core->trans_info [X0] = (double) transform_core->x1;
      transform_core->trans_info [Y0] = (double) transform_core->y1;
      transform_core->trans_info [X1] = (double) transform_core->x2;
      transform_core->trans_info [Y1] = (double) transform_core->y2;

      return NULL;
      break;

    case MOTION:
      scale_tool_motion (tool, gdisp_ptr);
      scale_tool_recalc (tool, gdisp_ptr);
      break;

    case RECALC:
      scale_tool_recalc (tool, gdisp_ptr);
      break;

    case FINISH:
      gtk_widget_set_sensitive (GTK_WIDGET (transform_info->shell), FALSE);
      return scale_tool_scale (gdisp->gimage,
			       gimage_active_drawable (gdisp->gimage),
			       gdisp,
			       transform_core->trans_info,
			       transform_core->original,
			       transform_tool_smoothing (),
			       transform_core->transform);
      break;
    }

  return NULL;
}

Tool *
tools_new_scale_tool (void)
{
  Tool          *tool;
  TransformCore *private;

  tool = transform_core_new (SCALE, TRUE);

  private = tool->private;

  /*  set the scale specific transformation attributes  */
  private->trans_func = scale_tool_transform;
  private->trans_info[X0] = 0.0;
  private->trans_info[Y0] = 0.0;
  private->trans_info[X1] = 0.0;
  private->trans_info[Y1] = 0.0;

  /*  assemble the transformation matrix  */
  gimp_matrix3_identity (private->transform);

  return tool;
}

void
tools_free_scale_tool (Tool *tool)
{
  transform_core_free (tool);
}

static void
scale_info_update (Tool *tool)
{
  GDisplay      *gdisp;
  TransformCore *transform_core;
  gdouble        ratio_x, ratio_y;
  gint           x1, y1, x2, y2, x3, y3, x4, y4;
  GimpUnit       unit;
  gdouble        unit_factor;
  gchar          format_buf[16];

  static GimpUnit  label_unit = GIMP_UNIT_PIXEL;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  transform_core = (TransformCore *) tool->private;
  unit = gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (sizeentry));;

  /*  Find original sizes  */
  x1 = transform_core->x1;
  y1 = transform_core->y1;
  x2 = transform_core->x2;
  y2 = transform_core->y2;

  if (unit != GIMP_UNIT_PERCENT)
    label_unit = unit;

  unit_factor = gimp_unit_get_factor (label_unit);

  if (label_unit) /* unit != GIMP_UNIT_PIXEL */
    {
      g_snprintf (format_buf, sizeof (format_buf), "%%.%df %s",
		  gimp_unit_get_digits (label_unit) + 1,
		  gimp_unit_get_symbol (label_unit));
      g_snprintf (orig_width_buf, MAX_INFO_BUF, format_buf,
		  (x2 - x1) * unit_factor / gdisp->gimage->xresolution);
      g_snprintf (orig_height_buf, MAX_INFO_BUF, format_buf,
		  (y2 - y1) * unit_factor / gdisp->gimage->yresolution);
    }
  else /* unit == GIMP_UNIT_PIXEL */
    {
      g_snprintf (orig_width_buf, MAX_INFO_BUF, "%d", x2 - x1);
      g_snprintf (orig_height_buf, MAX_INFO_BUF, "%d", y2 - y1);
    }

  /*  Find current sizes  */
  x3 = (int) transform_core->trans_info [X0];
  y3 = (int) transform_core->trans_info [Y0];
  x4 = (int) transform_core->trans_info [X1];
  y4 = (int) transform_core->trans_info [Y1];

  size_vals[0] = x4 - x3;
  size_vals[1] = y4 - y3;

  ratio_x = ratio_y = 0.0;

  if (x2 - x1)
    ratio_x = (double) (x4 - x3) / (double) (x2 - x1);
  if (y2 - y1)
    ratio_y = (double) (y4 - y3) / (double) (y2 - y1);

  g_snprintf (x_ratio_buf, MAX_INFO_BUF, "%0.2f", ratio_x);
  g_snprintf (y_ratio_buf, MAX_INFO_BUF, "%0.2f", ratio_y);

  info_dialog_update (transform_info);
  info_dialog_popup (transform_info);
}

static void
scale_size_changed (GtkWidget *widget,
		    gpointer   data)
{
  Tool          *tool;
  TransformCore *transform_core;
  GDisplay      *gdisp;
  gint           width;
  gint           height;

  tool = (Tool *)data;

  if (tool)
    {
      gdisp = (GDisplay *) tool->gdisp_ptr;
      transform_core = (TransformCore *) tool->private;

      width = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0));
      height = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1));

      if ((width != (transform_core->trans_info[X1] -
		     transform_core->trans_info[X0])) ||
	  (height != (transform_core->trans_info[Y1] -
		      transform_core->trans_info[Y0])))
	{
	  draw_core_pause (transform_core->core, tool);
	  transform_core->trans_info[X1] =
	    transform_core->trans_info[X0] + width;
	  transform_core->trans_info[Y1] =
	    transform_core->trans_info[Y0] + height;
	  scale_tool_recalc (tool, gdisp);
	  draw_core_resume (transform_core->core, tool);
	}
    }
}

static void
scale_unit_changed (GtkWidget *widget,
		    gpointer   data)
{
  scale_info_update ((Tool *) data);
}

static void
scale_tool_motion (Tool *tool,
		   void *gdisp_ptr)
{
  GDisplay      *gdisp;
  TransformCore *transform_core;
  gdouble        ratio;
  gdouble       *x1;
  gdouble       *y1;
  gdouble       *x2;
  gdouble       *y2;
  gint           w, h;
  gint           dir_x, dir_y;
  gint           diff_x, diff_y;

  gdisp = (GDisplay *) gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  diff_x = transform_core->curx - transform_core->lastx;
  diff_y = transform_core->cury - transform_core->lasty;

  switch (transform_core->function)
    {
    case HANDLE_1:
      x1 = &transform_core->trans_info [X0];
      y1 = &transform_core->trans_info [Y0];
      x2 = &transform_core->trans_info [X1];
      y2 = &transform_core->trans_info [Y1];
      dir_x = dir_y = 1;
      break;
    case HANDLE_2:
      x1 = &transform_core->trans_info [X1];
      y1 = &transform_core->trans_info [Y0];
      x2 = &transform_core->trans_info [X0];
      y2 = &transform_core->trans_info [Y1];
      dir_x = -1;
      dir_y = 1;
      break;
    case HANDLE_3:
      x1 = &transform_core->trans_info [X0];
      y1 = &transform_core->trans_info [Y1];
      x2 = &transform_core->trans_info [X1];
      y2 = &transform_core->trans_info [Y0];
      dir_x = 1;
      dir_y = -1;
      break;
    case HANDLE_4:
      x1 = &transform_core->trans_info [X1];
      y1 = &transform_core->trans_info [Y1];
      x2 = &transform_core->trans_info [X0];
      y2 = &transform_core->trans_info [Y0];
      dir_x = dir_y = -1;
      break;
    default :
      return;
    }

  /*  if just the mod1 key is down, affect only the height  */
  if (transform_core->state & GDK_MOD1_MASK &&
      ! (transform_core->state & GDK_CONTROL_MASK))
    diff_x = 0;
  /*  if just the control key is down, affect only the width  */
  else if (transform_core->state & GDK_CONTROL_MASK &&
	   ! (transform_core->state & GDK_MOD1_MASK))
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

  /*  if both the control key & mod1 keys are down,
   *  keep the aspect ratio intact 
   */
  if (transform_core->state & GDK_CONTROL_MASK &&
      transform_core->state & GDK_MOD1_MASK)
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

static void
scale_tool_recalc (Tool *tool,
		   void *gdisp_ptr)
{
  TransformCore *transform_core;
  GDisplay      *gdisp;
  gint           x1, y1, x2, y2;
  gint           diffx, diffy;
  gint           cx, cy;
  gdouble        scalex, scaley;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  transform_core = (TransformCore *) tool->private;

  x1 = (int) transform_core->trans_info [X0];
  y1 = (int) transform_core->trans_info [Y0];
  x2 = (int) transform_core->trans_info [X1];
  y2 = (int) transform_core->trans_info [Y1];

  scalex = scaley = 1.0;
  if (transform_core->x2 - transform_core->x1)
    scalex = (double) (x2 - x1) / (double) (transform_core->x2 - transform_core->x1);
  if (transform_core->y2 - transform_core->y1)
    scaley = (double) (y2 - y1) / (double) (transform_core->y2 - transform_core->y1);

  switch (transform_core->function)
    {
    case HANDLE_1:
      cx = x2;  cy = y2;
      diffx = x2 - transform_core->x2;
      diffy = y2 - transform_core->y2;
      break;
    case HANDLE_2:
      cx = x1;  cy = y2;
      diffx = x1 - transform_core->x1;
      diffy = y2 - transform_core->y2;
      break;
    case HANDLE_3:
      cx = x2;  cy = y1;
      diffx = x2 - transform_core->x2;
      diffy = y1 - transform_core->y1;
      break;
    case HANDLE_4:
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
  gimp_matrix3_identity  (transform_core->transform);
  gimp_matrix3_translate (transform_core->transform,
			  (double) -cx + diffx, (double) -cy + diffy);
  gimp_matrix3_scale     (transform_core->transform, scalex, scaley);
  gimp_matrix3_translate (transform_core->transform, (double) cx, (double) cy);

  /*  transform the bounding box  */
  transform_core_transform_bounding_box (tool);

  /*  update the information dialog  */
  scale_info_update (tool);
}

TileManager *
scale_tool_scale (GImage       *gimage,
		  GimpDrawable *drawable,
		  GDisplay     *gdisp,
		  gdouble      *trans_info,
		  TileManager  *float_tiles,
		  gboolean      interpolation,
		  GimpMatrix3   matrix)
{
  gimp_progress *progress;
  TileManager   *ret;

  progress = progress_start (gdisp, _("Scaling..."), FALSE, NULL, NULL);

  ret = transform_core_do (gimage, drawable, float_tiles,
			   interpolation, matrix,
			   progress ? progress_update_and_flush :
			   (progress_func_t) NULL,
			   progress);

  if (progress)
    progress_end (progress);

  return ret;
}
