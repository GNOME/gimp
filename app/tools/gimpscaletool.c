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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpimage.h"

#include "gui/info-dialog.h"

#include "gimpscaletool.h"
#include "tool_manager.h"
#include "tool_options.h"
#include "transform_options.h"

#include "gdisplay.h"
#include "gimpprogress.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


/*  forward function declarations  */
static TileManager * gimp_scale_tool_transform      (GimpTransformTool  *tool,
					             GDisplay           *gdisp,
					             TransformState      state);
static void          gimp_scale_tool_recalc         (GimpScaleTool      *tool,
					             GDisplay           *gdisp);
static void          gimp_scale_tool_motion         (GimpScaleTool      *tool,
 					             GDisplay           *gdisp);
static void          gimp_scale_tool_info_update    (GimpScaleTool      *tool);

static void          gimp_scale_tool_size_changed   (GtkWidget          *widget,
        				             gpointer            data);
static void          gimp_scale_tool_unit_changed   (GtkWidget          *widget,
					             gpointer            data);
static void          gimp_scale_tool_class_init     (GimpScaleToolClass *klass);

static void          gimp_scale_tool_init           (GimpScaleTool      *sc_tool);


/*  storage for information dialog fields  */
static gchar      orig_width_buf[MAX_INFO_BUF];
static gchar      orig_height_buf[MAX_INFO_BUF];
static gdouble    size_vals[2];
static gchar      x_ratio_buf[MAX_INFO_BUF];
static gchar      y_ratio_buf[MAX_INFO_BUF];

/*  needed for original size unit update  */
static GtkWidget *sizeentry = NULL;

static GimpTransformToolClass *parent_class = NULL;

static TransformOptions *scale_options = NULL;


void
gimp_scale_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_SCALE_TOOL,
                              FALSE,
                              "gimp:scale_tool",
                              _("Scale Tool"),
                              _("Scale the layer or selection"),
                              N_("/Tools/Transform Tools/Scale"), "<shift>T",
                              NULL, "tools/transform.html",
                              GIMP_STOCK_TOOL_SCALE);
}

GType
gimp_scale_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpScaleTool",
        sizeof (GimpScaleTool),
        sizeof (GimpScaleToolClass),
        (GtkClassInitFunc) gimp_scale_tool_class_init,
        (GtkObjectInitFunc) gimp_scale_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      tool_type = gtk_type_unique (GIMP_TYPE_TRANSFORM_TOOL, &tool_info);
    }

  return tool_type;
}

static void
gimp_scale_tool_class_init (GimpScaleToolClass *klass)
{
  GtkObjectClass         *object_class;
  GimpToolClass          *tool_class;
  GimpTransformToolClass *transform_class;

  object_class      = (GtkObjectClass *) klass;
  tool_class        = (GimpToolClass *) klass;
  transform_class   = (GimpTransformToolClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  transform_class->transform = gimp_scale_tool_transform;
}

static void
gimp_scale_tool_init (GimpScaleTool *sc_tool)
{
  GimpTool          *tool;
  GimpTransformTool *tr_tool;

  tool    = GIMP_TOOL (sc_tool);
  tr_tool = GIMP_TRANSFORM_TOOL (sc_tool); 

  if (! scale_options)
    {
      scale_options = transform_options_new (GIMP_TYPE_SCALE_TOOL,
                                            transform_options_reset);

      tool_manager_register_tool_options (GIMP_TYPE_SCALE_TOOL,
                                          (GimpToolOptions *) scale_options);
    }

  tool->tool_cursor = GIMP_RESIZE_TOOL_CURSOR;

  /*  set the scale specific transformation attributes  */
  tr_tool->trans_info[X0] = 0.0;
  tr_tool->trans_info[Y0] = 0.0;
  tr_tool->trans_info[X1] = 0.0;
  tr_tool->trans_info[Y1] = 0.0;

  /*  assemble the transformation matrix  */
  gimp_matrix3_identity (tr_tool->transform); /* FIXME name is confusing */
}

static TileManager *
gimp_scale_tool_transform (GimpTransformTool  *tr_tool,
		           GDisplay           *gdisp,
		           TransformState      state)
{
  GimpScaleTool   *sc_tool;
  GtkWidget       *spinbutton;
  GimpTool        *tool;

  sc_tool = GIMP_SCALE_TOOL (tr_tool);
  tool    = GIMP_TOOL (sc_tool);


  switch (state)
    {
    case TRANSFORM_INIT:
      size_vals[0] = tr_tool->x2 - tr_tool->x1;
      size_vals[1] = tr_tool->y2 - tr_tool->y1;

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
				       G_CALLBACK (gimp_scale_tool_size_changed),
				       tool);
	  g_signal_connect (G_OBJECT (sizeentry), "unit_changed",
			    G_CALLBACK (gimp_scale_tool_unit_changed),
			    tool);

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

      g_signal_handlers_block_by_func (G_OBJECT (sizeentry), 
                                       gimp_scale_tool_size_changed,
                                       tool);
      g_signal_handlers_block_by_func (G_OBJECT (sizeentry), 
                                       gimp_scale_tool_unit_changed,
                                       tool);

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

      g_signal_handlers_unblock_by_func (G_OBJECT (sizeentry), 
                                         gimp_scale_tool_size_changed,
                                         tool);
      g_signal_handlers_unblock_by_func (G_OBJECT (sizeentry), 
                                         gimp_scale_tool_unit_changed,
                                         tool);

      tr_tool->trans_info [X0] = (double) tr_tool->x1;
      tr_tool->trans_info [Y0] = (double) tr_tool->y1;
      tr_tool->trans_info [X1] = (double) tr_tool->x2;
      tr_tool->trans_info [Y1] = (double) tr_tool->y2;

      return NULL;
      break;

    case TRANSFORM_MOTION:
      gimp_scale_tool_motion (sc_tool, gdisp);
      gimp_scale_tool_recalc (sc_tool, gdisp);
      break;

    case TRANSFORM_RECALC:
      gimp_scale_tool_recalc (sc_tool, gdisp);
      break;

    case TRANSFORM_FINISH:
      gtk_widget_set_sensitive (GTK_WIDGET (transform_info->shell), FALSE);
      return gimp_scale_tool_scale (gdisp->gimage,
			            gimp_image_active_drawable (gdisp->gimage),
			            gdisp,
			            tr_tool->trans_info,
			            tr_tool->original,
			            gimp_transform_tool_smoothing (),
			            tr_tool->transform);
      break;
    }

  return NULL;
}


static void
gimp_scale_tool_info_update (GimpScaleTool *sc_tool)
{
  GimpTool          *tool;
  GimpTransformTool *tr_tool;
  gdouble            ratio_x, ratio_y;
  gint               x1, y1, x2, y2, x3, y3, x4, y4;
  GimpUnit           unit;
  gdouble            unit_factor;
  gchar              format_buf[16];

  static GimpUnit  label_unit = GIMP_UNIT_PIXEL;

  tool    = GIMP_TOOL (sc_tool);
  tr_tool = GIMP_TRANSFORM_TOOL (sc_tool);
  unit    = gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (sizeentry));;

  /*  Find original sizes  */
  x1 = tr_tool->x1;
  y1 = tr_tool->y1;
  x2 = tr_tool->x2;
  y2 = tr_tool->y2;

  if (unit != GIMP_UNIT_PERCENT)
    label_unit = unit;

  unit_factor = gimp_unit_get_factor (label_unit);

  if (label_unit) /* unit != GIMP_UNIT_PIXEL */
    {
      g_snprintf (format_buf, sizeof (format_buf), "%%.%df %s",
		  gimp_unit_get_digits (label_unit) + 1,
		  gimp_unit_get_symbol (label_unit));
      g_snprintf (orig_width_buf, MAX_INFO_BUF, format_buf,
		  (x2 - x1) * unit_factor / tool->gdisp->gimage->xresolution);
      g_snprintf (orig_height_buf, MAX_INFO_BUF, format_buf,
		  (y2 - y1) * unit_factor / tool->gdisp->gimage->yresolution);
    }
  else /* unit == GIMP_UNIT_PIXEL */
    {
      g_snprintf (orig_width_buf, MAX_INFO_BUF, "%d", x2 - x1);
      g_snprintf (orig_height_buf, MAX_INFO_BUF, "%d", y2 - y1);
    }

  /*  Find current sizes  */
  x3 = (int) tr_tool->trans_info [X0];
  y3 = (int) tr_tool->trans_info [Y0];
  x4 = (int) tr_tool->trans_info [X1];
  y4 = (int) tr_tool->trans_info [Y1];

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
gimp_scale_tool_size_changed (GtkWidget *widget,
	         	      gpointer   data)
{
  GimpTool          *tool;
  GimpTransformTool *tr_tool;
  GimpDrawTool      *dr_tool;
  gint               width;
  gint               height;

  tool = GIMP_TOOL(data);

  if (tool)
    {
      tr_tool = GIMP_TRANSFORM_TOOL(tool);
      dr_tool = GIMP_DRAW_TOOL(tool);

      width  = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0));
      height = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1));

      if ((width != (tr_tool->trans_info[X1] -
		     tr_tool->trans_info[X0])) ||
	  (height != (tr_tool->trans_info[Y1] -
		      tr_tool->trans_info[Y0])))
	{
	  gimp_draw_tool_pause (dr_tool);
	  tr_tool->trans_info[X1] =
	    tr_tool->trans_info[X0] + width;
	  tr_tool->trans_info[Y1] =
	    tr_tool->trans_info[Y0] + height;
	  gimp_scale_tool_recalc (GIMP_SCALE_TOOL(tool), tool->gdisp);
	  gimp_draw_tool_resume (dr_tool);
	}
    }
}

static void
gimp_scale_tool_unit_changed (GtkWidget *widget,
	         	      gpointer   data)
{
  gimp_scale_tool_info_update (GIMP_SCALE_TOOL (data));
}

static void
gimp_scale_tool_motion (GimpScaleTool *sc_tool,
		        GDisplay      *gdisp)
{
  GimpTransformTool *tr_tool;
  gdouble            ratio;
  gdouble           *x1;
  gdouble           *y1;
  gdouble           *x2;
  gdouble           *y2;
  gint               w, h;
  gint               dir_x, dir_y;
  gint               diff_x, diff_y;

  tr_tool = GIMP_TRANSFORM_TOOL(sc_tool);

  diff_x = tr_tool->curx - tr_tool->lastx;
  diff_y = tr_tool->cury - tr_tool->lasty;

  switch (tr_tool->function)
    {
    case TRANSFORM_HANDLE_1:
      x1 = &tr_tool->trans_info [X0];
      y1 = &tr_tool->trans_info [Y0];
      x2 = &tr_tool->trans_info [X1];
      y2 = &tr_tool->trans_info [Y1];
      dir_x = dir_y = 1;
      break;
    case TRANSFORM_HANDLE_2:
      x1 = &tr_tool->trans_info [X1];
      y1 = &tr_tool->trans_info [Y0];
      x2 = &tr_tool->trans_info [X0];
      y2 = &tr_tool->trans_info [Y1];
      dir_x = -1;
      dir_y = 1;
      break;
    case TRANSFORM_HANDLE_3:
      x1 = &tr_tool->trans_info [X0];
      y1 = &tr_tool->trans_info [Y1];
      x2 = &tr_tool->trans_info [X1];
      y2 = &tr_tool->trans_info [Y0];
      dir_x = 1;
      dir_y = -1;
      break;
    case TRANSFORM_HANDLE_4:
      x1 = &tr_tool->trans_info [X1];
      y1 = &tr_tool->trans_info [Y1];
      x2 = &tr_tool->trans_info [X0];
      y2 = &tr_tool->trans_info [Y0];
      dir_x = dir_y = -1;
      break;
    case TRANSFORM_HANDLE_CENTER:
      return;
    default:
      return;
    }

  /*  if just the mod1 key is down, affect only the height  */
  if (tr_tool->state & GDK_MOD1_MASK &&
      ! (tr_tool->state & GDK_CONTROL_MASK))
    diff_x = 0;
  /*  if just the control key is down, affect only the width  */
  else if (tr_tool->state & GDK_CONTROL_MASK &&
	   ! (tr_tool->state & GDK_MOD1_MASK))
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
  if (tr_tool->state & GDK_CONTROL_MASK &&
      tr_tool->state & GDK_MOD1_MASK)
    {
      ratio = (double) (tr_tool->x2 - tr_tool->x1) /
        (double) (tr_tool->y2 - tr_tool->y1);

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
gimp_scale_tool_recalc (GimpScaleTool *sc_tool,
		        GDisplay      *gdisp)
{
  GimpTransformTool *tr_tool;
  gint               x1, y1, x2, y2;
  gint               diffx, diffy;
  gint               cx, cy;
  gdouble            scalex, scaley;

  tr_tool = GIMP_TRANSFORM_TOOL (sc_tool);

  x1 = (int) tr_tool->trans_info [X0];
  y1 = (int) tr_tool->trans_info [Y0];
  x2 = (int) tr_tool->trans_info [X1];
  y2 = (int) tr_tool->trans_info [Y1];

  scalex = scaley = 1.0;
  if (tr_tool->x2 - tr_tool->x1)
    scalex = (double) (x2 - x1) / (double) (tr_tool->x2 - tr_tool->x1);
  if (tr_tool->y2 - tr_tool->y1)
    scaley = (double) (y2 - y1) / (double) (tr_tool->y2 - tr_tool->y1);

  switch (tr_tool->function)
    {
    case TRANSFORM_HANDLE_1:
      cx = x2;  cy = y2;
      diffx = x2 - tr_tool->x2;
      diffy = y2 - tr_tool->y2;
      break;
    case TRANSFORM_HANDLE_2:
      cx = x1;  cy = y2;
      diffx = x1 - tr_tool->x1;
      diffy = y2 - tr_tool->y2;
      break;
    case TRANSFORM_HANDLE_3:
      cx = x2;  cy = y1;
      diffx = x2 - tr_tool->x2;
      diffy = y1 - tr_tool->y1;
      break;
    case TRANSFORM_HANDLE_4:
      cx = x1;  cy = y1;
      diffx = x1 - tr_tool->x1;
      diffy = y1 - tr_tool->y1;
      break;
    case TRANSFORM_HANDLE_CENTER:
      cx = x1; cy = y1;
      diffx = diffy = 0;
      break;
    default:
      cx = x1; cy = y1;
      diffx = diffy = 0;
      break;
    }

  /*  assemble the transformation matrix  */
  gimp_matrix3_identity  (tr_tool->transform);
  gimp_matrix3_translate (tr_tool->transform,
			  (double) -cx + diffx, (double) -cy + diffy);
  gimp_matrix3_scale     (tr_tool->transform, scalex, scaley);
  gimp_matrix3_translate (tr_tool->transform, (double) cx, (double) cy);

  /*  transform the bounding box  */
  gimp_transform_tool_transform_bounding_box (tr_tool);

  /*  update the information dialog  */
  gimp_scale_tool_info_update (sc_tool);
}

TileManager *
gimp_scale_tool_scale (GimpImage    *gimage,
		       GimpDrawable *drawable,
		       GDisplay     *gdisp,
		       gdouble      *trans_info,
		       TileManager  *float_tiles,
		       gboolean      interpolation,
		       GimpMatrix3   matrix)
{
  GimpProgress *progress;
  TileManager  *ret;

  progress = progress_start (gdisp, _("Scaling..."), FALSE, NULL, NULL);

  ret = gimp_transform_tool_do (gimage, drawable, float_tiles,
			        interpolation, matrix,
			        progress ? progress_update_and_flush :
			        (GimpProgressFunc) NULL,
			        progress);

  if (progress)
    progress_end (progress);

  return ret;
}
