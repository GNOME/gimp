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
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"
#include "gui/gui-types.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimpimage.h"

#include "gui/info-dialog.h"

#include "display/gimpdisplay.h"

#include "gimprotatetool.h"
#include "tool_manager.h"
#include "tool_options.h"
#include "transform_options.h"

#include "gimpprogress.h"
#include "undo.h"
#include "path_transform.h"

#include "libgimp/gimpintl.h"


/*  index into trans_info array  */
#define ANGLE        0
#define REAL_ANGLE   1
#define CENTER_X     2
#define CENTER_Y     3

#define EPSILON      0.018  /*  ~ 1 degree  */
#define FIFTEEN_DEG  (G_PI / 12.0)


/*  local function declarations  */
static void          gimp_rotate_tool_class_init  (GimpRotateToolClass *klass);
static void          gimp_rotate_tool_init        (GimpRotateTool      *rotate_tool);

static TileManager * gimp_rotate_tool_transform (GimpTransformTool *transform_tool,
						 GDisplay          *gdisp,
						 TransformState     state);

static void          rotate_tool_recalc         (GimpTool       *tool,
						 GDisplay       *gdisp);
static void          rotate_tool_motion         (GimpTool       *tool,
						 GDisplay       *gdisp);
static void          rotate_info_update         (GimpTool       *tool);

static void          rotate_angle_changed       (GtkWidget      *entry,
						 gpointer        data);
static void          rotate_center_changed      (GtkWidget      *entry,
						 gpointer        data);


/*  variables local to this file  */
static gdouble    angle_val;
static gdouble    center_vals[2];

/*  needed for size update  */
static GtkWidget *sizeentry = NULL;

static GimpTransformToolClass *parent_class = NULL;

static TransformOptions *rotate_options = NULL;


/*  public functions  */

void 
gimp_rotate_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_ROTATE_TOOL,
                              FALSE,
			      "gimp:rotate_tool",
			      _("Rotate Tool"),
			      _("Rotate the layer or selection"),
			      N_("/Tools/Transform Tools/Rotate"), "<shift>R",
			      NULL, "tools/rotate.html",
			      GIMP_STOCK_TOOL_ROTATE);
}

GType
gimp_rotate_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpRotateToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_rotate_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpRotateTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_rotate_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_TRANSFORM_TOOL,
					  "GimpRotateTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_rotate_tool_class_init (GimpRotateToolClass *klass)
{
  GimpTransformToolClass *trans_class;

  trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  trans_class->transform = gimp_rotate_tool_transform;
}

static void
gimp_rotate_tool_init (GimpRotateTool *rotate_tool)
{
  GimpTool          *tool;
  GimpTransformTool *tr_tool;

  tool    = GIMP_TOOL (rotate_tool);
  tr_tool = GIMP_TRANSFORM_TOOL (rotate_tool);

  if (! rotate_options)
    {
      rotate_options = transform_options_new (GIMP_TYPE_ROTATE_TOOL,
					      transform_options_reset);

      tool_manager_register_tool_options (GIMP_TYPE_ROTATE_TOOL,
                                          (GimpToolOptions *) rotate_options);
    }

  tool->tool_cursor   = GIMP_ROTATE_TOOL_CURSOR;

  tr_tool->trans_info[ANGLE]      = 0.0;
  tr_tool->trans_info[REAL_ANGLE] = 0.0;
  tr_tool->trans_info[CENTER_X]   = 0.0;
  tr_tool->trans_info[CENTER_Y]   = 0.0;

  /*  assemble the transformation matrix  */
  gimp_matrix3_identity (tr_tool->transform);
}

TileManager *
gimp_rotate_tool_rotate (GimpImage    *gimage,
			 GimpDrawable *drawable,
			 GDisplay     *gdisp,
			 gdouble       angle,
			 TileManager  *float_tiles,
			 gboolean      interpolation,
			 GimpMatrix3   matrix)
{
  GimpProgress *progress;
  TileManager  *ret;

  progress = progress_start (gdisp, _("Rotating..."), FALSE, NULL, NULL);

  ret = gimp_transform_tool_do (gimage, drawable, float_tiles,
				interpolation, matrix,
				progress ? progress_update_and_flush :
				(GimpProgressFunc) NULL,
				progress);

  if (progress)
    progress_end (progress);

  return ret;
}

static TileManager *
gimp_rotate_tool_transform (GimpTransformTool  *transform_tool,
			    GDisplay           *gdisp,
			    TransformState      state)
{
  GimpTool      *tool;
  GtkWidget     *widget;
  GtkWidget     *spinbutton2;

  tool = GIMP_TOOL (transform_tool);

  switch (state)
    {
    case TRANSFORM_INIT:
      angle_val = 0.0;
      center_vals[0] = transform_tool->cx;
      center_vals[1] = transform_tool->cy;

      if (!transform_info)
	{
	  transform_info = info_dialog_new (_("Rotation Information"),
					    gimp_standard_help_func,
					    "tools/transform_rotate.html");

	  widget =
	    info_dialog_add_spinbutton (transform_info, _("Angle:"),
					&angle_val,
					-180, 180, 1, 15, 1, 1, 2,
					G_CALLBACK (rotate_angle_changed),
					tool);
	  gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (widget), TRUE);

	  /*  this looks strange (-180, 181), but it works  */
	  widget = info_dialog_add_scale (transform_info, "", &angle_val,
					  -180, 181, 0.01, 0.1, 1, -1,
					  G_CALLBACK (rotate_angle_changed),
					  tool);
	  gtk_widget_set_usize (widget, 180, 0);

	  spinbutton2 =
	    info_dialog_add_spinbutton (transform_info, _("Center X:"), NULL,
					-1, 1, 1, 10, 1, 1, 2, NULL, NULL);
	  sizeentry =
	    info_dialog_add_sizeentry (transform_info, _("Y:"),
				       center_vals, 1,
				       gdisp->gimage->unit, "%a",
				       TRUE, TRUE, FALSE,
				       GIMP_SIZE_ENTRY_UPDATE_SIZE,
				       G_CALLBACK (rotate_center_changed),
				       tool);

	  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (sizeentry),
				     GTK_SPIN_BUTTON (spinbutton2), NULL);

	  gtk_table_set_row_spacing (GTK_TABLE (transform_info->info_table),
				     1, 6);
	  gtk_table_set_row_spacing (GTK_TABLE (transform_info->info_table),
				     2, 0);
	}

      g_signal_handlers_block_by_func (G_OBJECT (sizeentry), 
                                       rotate_center_changed,
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
					     -65536,
					     65536 + gdisp->gimage->width);
      gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 1,
					     -65536,
					     65536 + gdisp->gimage->height);

      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (sizeentry), 0,
				transform_tool->x1, transform_tool->x2);
      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (sizeentry), 1,
				transform_tool->y1, transform_tool->y2);

      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 0,
				  center_vals[0]);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 1,
				  center_vals[1]);

      gtk_widget_set_sensitive (transform_info->shell, TRUE);

      g_signal_handlers_unblock_by_func (G_OBJECT (sizeentry), 
                                         rotate_center_changed,
                                         tool);

      transform_tool->trans_info[ANGLE] = angle_val;
      transform_tool->trans_info[REAL_ANGLE] = angle_val;
      transform_tool->trans_info[CENTER_X] = center_vals[0];
      transform_tool->trans_info[CENTER_Y] = center_vals[1];

      return NULL;
      break;

    case TRANSFORM_MOTION:
      rotate_tool_motion (tool, gdisp);
      rotate_tool_recalc (tool, gdisp);
      break;

    case TRANSFORM_RECALC:
      rotate_tool_recalc (tool, gdisp);
      break;

    case TRANSFORM_FINISH:
      gtk_widget_set_sensitive (GTK_WIDGET (transform_info->shell), FALSE);
      return gimp_rotate_tool_rotate (gdisp->gimage,
       			      gimp_image_active_drawable (gdisp->gimage),
       			      gdisp,
       			      transform_tool->trans_info[ANGLE],
       			      transform_tool->original,
       			      gimp_transform_tool_smoothing (),
       			      transform_tool->transform);
      break;
    }

  return NULL;
}

static void
rotate_info_update (GimpTool *tool)
{
  GimpTransformTool *transform_tool;

  transform_tool = GIMP_TRANSFORM_TOOL (tool);

  angle_val      = gimp_rad_to_deg (transform_tool->trans_info[ANGLE]);
  center_vals[0] = transform_tool->cx;
  center_vals[1] = transform_tool->cy;

  info_dialog_update (transform_info);
  info_dialog_popup (transform_info);
}

static void
rotate_angle_changed (GtkWidget *widget,
		      gpointer   data)
{
  GimpTool          *tool;
  GimpDrawTool      *draw_tool;
  GimpTransformTool *transform_tool;
  gdouble            value;

  tool = (GimpTool *) data;

  if (tool)
    {
      transform_tool = GIMP_TRANSFORM_TOOL (tool);
      draw_tool      = GIMP_DRAW_TOOL (tool);

      value = gimp_deg_to_rad (GTK_ADJUSTMENT (widget)->value);

      if (value != transform_tool->trans_info[ANGLE])
	{
	  gimp_draw_tool_pause (draw_tool);      
	  transform_tool->trans_info[ANGLE] = value;
	  rotate_tool_recalc (tool, tool->gdisp);
	  gimp_draw_tool_resume (draw_tool);
	}
    }
}

static void
rotate_center_changed (GtkWidget *widget,
		       gpointer   data)
{
  GimpTool          *tool;
  GimpDrawTool      *draw_tool;
  GimpTransformTool *transform_tool;
  gint               cx;
  gint               cy;

  tool = (GimpTool *) data;

  if (tool)
    {
      transform_tool = GIMP_TRANSFORM_TOOL (tool);
      draw_tool      = GIMP_DRAW_TOOL (tool);

      cx = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0));
      cy = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1));

      if ((cx != transform_tool->cx) ||
	  (cy != transform_tool->cy))
	{
	  gimp_draw_tool_pause (draw_tool);      
	  transform_tool->cx = cx;
	  transform_tool->cy = cy;
	  rotate_tool_recalc (tool, tool->gdisp);
	  gimp_draw_tool_resume (draw_tool);
	}
    }
}

static void
rotate_tool_motion (GimpTool *tool,
		    GDisplay *gdisp)
{
  GimpTransformTool *transform_tool;
  gdouble            angle1, angle2, angle;
  gdouble            cx, cy;
  gdouble            x1, y1, x2, y2;

  transform_tool = GIMP_TRANSFORM_TOOL (tool);

  if (transform_tool->function == TRANSFORM_HANDLE_CENTER)
    {
      transform_tool->cx = transform_tool->curx;
      transform_tool->cy = transform_tool->cury;

      return;
    }

  cx = transform_tool->cx;
  cy = transform_tool->cy;

  x1 = transform_tool->curx - cx;
  x2 = transform_tool->lastx - cx;
  y1 = cy - transform_tool->cury;
  y2 = cy - transform_tool->lasty;

  /*  find the first angle  */
  angle1 = atan2 (y1, x1);

  /*  find the angle  */
  angle2 = atan2 (y2, x2);

  angle = angle2 - angle1;

  if (angle > G_PI || angle < -G_PI)
    angle = angle2 - ((angle1 < 0) ? 2.0 * G_PI + angle1 : angle1 - 2.0 * G_PI);

  /*  increment the transform tool's angle  */
  transform_tool->trans_info[REAL_ANGLE] += angle;

  /*  limit the angle to between 0 and 360 degrees  */
  if (transform_tool->trans_info[REAL_ANGLE] < - G_PI)
    transform_tool->trans_info[REAL_ANGLE] =
      2.0 * G_PI - transform_tool->trans_info[REAL_ANGLE];
  else if (transform_tool->trans_info[REAL_ANGLE] > G_PI)
    transform_tool->trans_info[REAL_ANGLE] =
      transform_tool->trans_info[REAL_ANGLE] - 2.0 * G_PI;

  /*  constrain the angle to 15-degree multiples if ctrl is held down  */
  if (transform_tool->state & GDK_CONTROL_MASK)
    transform_tool->trans_info[ANGLE] =
      FIFTEEN_DEG * (int) ((transform_tool->trans_info[REAL_ANGLE] +
			    FIFTEEN_DEG / 2.0) /
			   FIFTEEN_DEG);
  else
    transform_tool->trans_info[ANGLE] = transform_tool->trans_info[REAL_ANGLE];
}

static void
rotate_tool_recalc (GimpTool *tool,
		    GDisplay *gdisp)
{
  GimpTransformTool *transform_tool;
  gdouble            cx, cy;

  transform_tool = GIMP_TRANSFORM_TOOL (tool);

  cx = transform_tool->cx;
  cy = transform_tool->cy;

  /*  assemble the transformation matrix  */
  gimp_matrix3_identity  (transform_tool->transform);
  gimp_matrix3_translate (transform_tool->transform, -cx, -cy);
  gimp_matrix3_rotate    (transform_tool->transform,
			  transform_tool->trans_info[ANGLE]);
  gimp_matrix3_translate (transform_tool->transform, +cx, +cy);

  /*  transform the bounding box  */
  gimp_transform_tool_transform_bounding_box (transform_tool);

  /*  update the information dialog  */
  rotate_info_update (tool);
}
