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
#include "gui/gui-types.h"

#include "core/gimpimage.h"
#include "core/gimpdrawable-transform.h"
#include "core/gimpdrawable-transform-utils.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"

#include "gui/info-dialog.h"

#include "gimpscaletool.h"
#include "transform_options.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void          gimp_scale_tool_class_init   (GimpScaleToolClass *klass);

static void          gimp_scale_tool_init         (GimpScaleTool      *sc_tool);

static TileManager * gimp_scale_tool_transform    (GimpTransformTool  *tr_tool,
                                                   GimpDisplay        *gdisp,
                                                   TransformState      state);

static void          gimp_scale_tool_recalc       (GimpTransformTool  *tr_tool,
                                                   GimpDisplay        *gdisp);
static void          gimp_scale_tool_motion       (GimpTransformTool  *tr_tool,
                                                   GimpDisplay        *gdisp);
static void          gimp_scale_tool_info_update  (GimpTransformTool  *tr_tool);

static void          gimp_scale_tool_size_changed (GtkWidget          *widget,
                                                   gpointer            data);
static void          gimp_scale_tool_unit_changed (GtkWidget          *widget,
                                                   gpointer            data);


/*  storage for information dialog fields  */
static gchar      orig_width_buf[MAX_INFO_BUF];
static gchar      orig_height_buf[MAX_INFO_BUF];
static gdouble    size_vals[2];
static gchar      x_ratio_buf[MAX_INFO_BUF];
static gchar      y_ratio_buf[MAX_INFO_BUF];

/*  needed for original size unit update  */
static GtkWidget *sizeentry = NULL;

static GimpTransformToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_scale_tool_register (Gimp                     *gimp,
                          GimpToolRegisterCallback  callback)
{
  (* callback) (gimp,
                GIMP_TYPE_SCALE_TOOL,
                transform_options_new,
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
      static const GTypeInfo tool_info =
      {
        sizeof (GimpScaleToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_scale_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpScaleTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_scale_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_TRANSFORM_TOOL,
					  "GimpScaleTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}


/*  private functions  */

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
gimp_scale_tool_init (GimpScaleTool *scale_tool)
{
  GimpTool *tool;

  tool = GIMP_TOOL (scale_tool);

  tool->tool_cursor = GIMP_RESIZE_TOOL_CURSOR;
}

static TileManager *
gimp_scale_tool_transform (GimpTransformTool *transform_tool,
		           GimpDisplay       *gdisp,
		           TransformState     state)
{
  switch (state)
    {
    case TRANSFORM_INIT:
      size_vals[0] = transform_tool->x2 - transform_tool->x1;
      size_vals[1] = transform_tool->y2 - transform_tool->y1;

      if (! transform_tool->info_dialog)
	{
          GtkWidget *spinbutton;

	  transform_tool->info_dialog =
            info_dialog_new (_("Scaling Information"),
                             gimp_standard_help_func,
                             "tools/transform_scale.html");

          gimp_transform_tool_info_dialog_connect (transform_tool,
                                                   GIMP_STOCK_TOOL_SCALE);

	  info_dialog_add_label (transform_tool->info_dialog,
                                 _("Original Width:"),
				 orig_width_buf);
	  info_dialog_add_label (transform_tool->info_dialog,
                                 _("Height:"),
				 orig_height_buf);

	  spinbutton =
	    info_dialog_add_spinbutton (transform_tool->info_dialog,
                                        _("Current Width:"),
					NULL, -1, 1, 1, 10, 1, 1, 2, NULL, NULL);
	  sizeentry =
	    info_dialog_add_sizeentry (transform_tool->info_dialog,
                                       _("Height:"),
				       size_vals, 1,
				       gdisp->gimage->unit, "%a",
				       TRUE, TRUE, FALSE,
				       GIMP_SIZE_ENTRY_UPDATE_SIZE,
				       G_CALLBACK (gimp_scale_tool_size_changed),
				       transform_tool);
	  g_signal_connect (G_OBJECT (sizeentry), "unit_changed",
			    G_CALLBACK (gimp_scale_tool_unit_changed),
			    transform_tool);

	  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (sizeentry),
				     GTK_SPIN_BUTTON (spinbutton), NULL);

	  info_dialog_add_label (transform_tool->info_dialog, 
                                 _("Scale Ratio X:"),
				 x_ratio_buf);
	  info_dialog_add_label (transform_tool->info_dialog,
                                 _("Y:"),
				 y_ratio_buf);

	  gtk_table_set_row_spacing
            (GTK_TABLE (transform_tool->info_dialog->info_table), 1, 4);
	  gtk_table_set_row_spacing
            (GTK_TABLE (transform_tool->info_dialog->info_table), 2, 0);
	}

      g_signal_handlers_block_by_func (G_OBJECT (sizeentry), 
                                       gimp_scale_tool_size_changed,
                                       transform_tool);
      g_signal_handlers_block_by_func (G_OBJECT (sizeentry), 
                                       gimp_scale_tool_unit_changed,
                                       transform_tool);

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

      g_signal_handlers_unblock_by_func (G_OBJECT (sizeentry), 
                                         gimp_scale_tool_size_changed,
                                         transform_tool);
      g_signal_handlers_unblock_by_func (G_OBJECT (sizeentry), 
                                         gimp_scale_tool_unit_changed,
                                         transform_tool);

      gtk_widget_set_sensitive (GTK_WIDGET (transform_tool->info_dialog->shell),
                                TRUE);

      transform_tool->trans_info[X0] = (gdouble) transform_tool->x1;
      transform_tool->trans_info[Y0] = (gdouble) transform_tool->y1;
      transform_tool->trans_info[X1] = (gdouble) transform_tool->x2;
      transform_tool->trans_info[Y1] = (gdouble) transform_tool->y2;
      break;

    case TRANSFORM_MOTION:
      gimp_scale_tool_motion (transform_tool, gdisp);
      gimp_scale_tool_recalc (transform_tool, gdisp);
      break;

    case TRANSFORM_RECALC:
      gimp_scale_tool_recalc (transform_tool, gdisp);
      break;

    case TRANSFORM_FINISH:
      return gimp_transform_tool_transform_tiles (transform_tool,
                                                  _("Scaling..."));
      break;
    }

  return NULL;
}

static void
gimp_scale_tool_info_update (GimpTransformTool *transform_tool)
{
  GimpTool          *tool;
  gdouble            ratio_x, ratio_y;
  gint               x1, y1, x2, y2, x3, y3, x4, y4;
  GimpUnit           unit;
  gdouble            unit_factor;
  gchar              format_buf[16];

  static GimpUnit  label_unit = GIMP_UNIT_PIXEL;

  tool = GIMP_TOOL (transform_tool);

  unit = gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (sizeentry));;

  /*  Find original sizes  */
  x1 = transform_tool->x1;
  y1 = transform_tool->y1;
  x2 = transform_tool->x2;
  y2 = transform_tool->y2;

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
  x3 = (gint) transform_tool->trans_info[X0];
  y3 = (gint) transform_tool->trans_info[Y0];
  x4 = (gint) transform_tool->trans_info[X1];
  y4 = (gint) transform_tool->trans_info[Y1];

  size_vals[0] = x4 - x3;
  size_vals[1] = y4 - y3;

  ratio_x = ratio_y = 0.0;

  if (x2 - x1)
    ratio_x = (double) (x4 - x3) / (double) (x2 - x1);
  if (y2 - y1)
    ratio_y = (double) (y4 - y3) / (double) (y2 - y1);

  g_snprintf (x_ratio_buf, sizeof (x_ratio_buf), "%0.2f", ratio_x);
  g_snprintf (y_ratio_buf, sizeof (y_ratio_buf), "%0.2f", ratio_y);

  info_dialog_update (transform_tool->info_dialog);
  info_dialog_popup (transform_tool->info_dialog);
}

static void
gimp_scale_tool_size_changed (GtkWidget *widget,
	         	      gpointer   data)
{
  GimpTransformTool *transform_tool;
  GimpTool          *tool;
  gint               width;
  gint               height;

  transform_tool = GIMP_TRANSFORM_TOOL (data);
  tool           = GIMP_TOOL (data);

  width  = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0));
  height = RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1));

  if ((width  != (transform_tool->trans_info[X1] -
                  transform_tool->trans_info[X0])) ||
      (height != (transform_tool->trans_info[Y1] -
                  transform_tool->trans_info[Y0])))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      transform_tool->trans_info[X1] = transform_tool->trans_info[X0] + width;
      transform_tool->trans_info[Y1] = transform_tool->trans_info[Y0] + height;

      gimp_scale_tool_recalc (transform_tool, tool->gdisp);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

static void
gimp_scale_tool_unit_changed (GtkWidget *widget,
	         	      gpointer   data)
{
  gimp_scale_tool_info_update (GIMP_TRANSFORM_TOOL (data));
}

static void
gimp_scale_tool_motion (GimpTransformTool *transform_tool,
		        GimpDisplay       *gdisp)
{
  TransformOptions *options;
  gdouble           ratio;
  gdouble          *x1;
  gdouble          *y1;
  gdouble          *x2;
  gdouble          *y2;
  gint              w, h;
  gint              dir_x, dir_y;
  gint              diff_x, diff_y;

  options = (TransformOptions *) GIMP_TOOL (transform_tool)->tool_info->tool_options;

  diff_x = transform_tool->curx - transform_tool->lastx;
  diff_y = transform_tool->cury - transform_tool->lasty;

  switch (transform_tool->function)
    {
    case TRANSFORM_HANDLE_1:
      x1 = &transform_tool->trans_info[X0];
      y1 = &transform_tool->trans_info[Y0];
      x2 = &transform_tool->trans_info[X1];
      y2 = &transform_tool->trans_info[Y1];
      dir_x = dir_y = 1;
      break;
    case TRANSFORM_HANDLE_2:
      x1 = &transform_tool->trans_info[X1];
      y1 = &transform_tool->trans_info[Y0];
      x2 = &transform_tool->trans_info[X0];
      y2 = &transform_tool->trans_info[Y1];
      dir_x = -1;
      dir_y = 1;
      break;
    case TRANSFORM_HANDLE_3:
      x1 = &transform_tool->trans_info[X0];
      y1 = &transform_tool->trans_info[Y1];
      x2 = &transform_tool->trans_info[X1];
      y2 = &transform_tool->trans_info[Y0];
      dir_x = 1;
      dir_y = -1;
      break;
    case TRANSFORM_HANDLE_4:
      x1 = &transform_tool->trans_info[X1];
      y1 = &transform_tool->trans_info[Y1];
      x2 = &transform_tool->trans_info[X0];
      y2 = &transform_tool->trans_info[Y0];
      dir_x = dir_y = -1;
      break;
    case TRANSFORM_HANDLE_CENTER:
      transform_tool->trans_info[X0] += diff_x;
      transform_tool->trans_info[Y0] += diff_y;
      transform_tool->trans_info[X1] += diff_x;
      transform_tool->trans_info[Y1] += diff_y;
      transform_tool->trans_info[X2] += diff_x;
      transform_tool->trans_info[Y2] += diff_y;
      transform_tool->trans_info[X3] += diff_x;
      transform_tool->trans_info[Y3] += diff_y;
      return;
    default:
      return;
    }

  /*  if just the mod1 key is down, affect only the height  */
  if (options->constrain_2 && ! options->constrain_1)
    {
      diff_x = 0;
    }
  /*  if just the control key is down, affect only the width  */
  else if (options->constrain_1 && ! options->constrain_2)
    {
      diff_y = 0;
    }

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
  if (options->constrain_1 && options->constrain_2)
    {
      ratio = ((gdouble) (transform_tool->x2 - transform_tool->x1) /
               (gdouble) (transform_tool->y2 - transform_tool->y1));

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
gimp_scale_tool_recalc (GimpTransformTool *transform_tool,
		        GimpDisplay       *gdisp)
{
  gimp_drawable_transform_matrix_scale (transform_tool->x1,
                                        transform_tool->y1,
                                        transform_tool->x2,
                                        transform_tool->y2,
                                        transform_tool->trans_info[X0],
                                        transform_tool->trans_info[Y0],
                                        transform_tool->trans_info[X1],
                                        transform_tool->trans_info[Y1],
                                        transform_tool->transform);

  /*  transform the bounding box  */
  gimp_transform_tool_transform_bounding_box (transform_tool);

  /*  update the information dialog  */
  gimp_scale_tool_info_update (transform_tool);
}
