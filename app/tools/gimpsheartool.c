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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-transform-utils.h"
#include "core/gimpimage.h"
#include "core/gimpdrawable-transform.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#ifdef __GNUC__
#warning FIXME #include "gui/gui-types.h"
#endif
#include "gui/gui-types.h"
#include "gui/info-dialog.h"

#include "gimpsheartool.h"
#include "gimptoolcontrol.h"
#include "gimptransformoptions.h"

#include "gimp-intl.h"


/*  index into trans_info array  */
#define HORZ_OR_VERT 0
#define XSHEAR       1
#define YSHEAR       2

/*  the minimum movement before direction of shear can be determined (pixels) */
#define MIN_MOVE     5


/*  forward function declarations  */

static void   gimp_shear_tool_class_init (GimpShearToolClass *klass);
static void   gimp_shear_tool_init       (GimpShearTool      *shear_tool);

static void   gimp_shear_tool_dialog     (GimpTransformTool  *tr_tool);
static void   gimp_shear_tool_prepare    (GimpTransformTool  *tr_tool,
                                          GimpDisplay        *gdisp);
static void   gimp_shear_tool_motion     (GimpTransformTool  *tr_tool,
                                          GimpDisplay        *gdisp);
static void   gimp_shear_tool_recalc     (GimpTransformTool  *tr_tool,
                                          GimpDisplay        *gdisp);

static void   shear_info_update          (GimpTransformTool  *tr_tool);

static void   shear_x_mag_changed        (GtkWidget          *widget,
                                          GimpTransformTool  *tr_tool);
static void   shear_y_mag_changed        (GtkWidget          *widget,
                                          GimpTransformTool  *tr_tool);


/*  variables local to this file  */
static gdouble  xshear_val;
static gdouble  yshear_val;

static GimpTransformToolClass *parent_class = NULL;


/* Public functions */

void
gimp_shear_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_SHEAR_TOOL,
                GIMP_TYPE_TRANSFORM_OPTIONS,
                gimp_transform_options_gui,
                0,
                "gimp-shear-tool",
                _("Shear"),
                _("Shear the layer or selection"),
                N_("S_hear"), "<shift>S",
                NULL, GIMP_HELP_TOOL_SHEAR,
                GIMP_STOCK_TOOL_SHEAR,
                data);
}

GType
gimp_shear_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpShearToolClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_shear_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpShearTool),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_shear_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_TRANSFORM_TOOL,
                                          "GimpShearTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_shear_tool_class_init (GimpShearToolClass *klass)
{
  GimpTransformToolClass *trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  trans_class->dialog  = gimp_shear_tool_dialog;
  trans_class->prepare = gimp_shear_tool_prepare;
  trans_class->motion  = gimp_shear_tool_motion;
  trans_class->recalc  = gimp_shear_tool_recalc;
}

static void
gimp_shear_tool_init (GimpShearTool *shear_tool)
{
  GimpTool          *tool    = GIMP_TOOL (shear_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (shear_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_SHEAR);

  tr_tool->use_center    = FALSE;
  tr_tool->shell_desc    = _("Shearing Information");
  tr_tool->progress_text = _("Shearing...");
}

static void
gimp_shear_tool_dialog (GimpTransformTool *tr_tool)
{
  info_dialog_add_spinbutton (tr_tool->info_dialog,
                              _("Shear Magnitude X:"),
                              &xshear_val,
                              -65536, 65536, 1, 15, 1, 1, 0,
                              G_CALLBACK (shear_x_mag_changed),
                              tr_tool);

  info_dialog_add_spinbutton (tr_tool->info_dialog,
                              _("Shear Magnitude Y:"),
                              &yshear_val,
                              -65536, 65536, 1, 15, 1, 1, 0,
                              G_CALLBACK (shear_y_mag_changed),
                              tr_tool);
}

static void
gimp_shear_tool_prepare (GimpTransformTool *tr_tool,
                         GimpDisplay       *gdisp)
{
  tr_tool->trans_info[HORZ_OR_VERT] = GIMP_ORIENTATION_UNKNOWN;
  tr_tool->trans_info[XSHEAR]       = 0.0;
  tr_tool->trans_info[YSHEAR]       = 0.0;
}

static void
gimp_shear_tool_motion (GimpTransformTool *tr_tool,
                        GimpDisplay       *gdisp)
{
  gdouble diffx, diffy;
  gint    dir;

  diffx = tr_tool->curx - tr_tool->lastx;
  diffy = tr_tool->cury - tr_tool->lasty;

  /*  If we haven't yet decided on which way to control shearing
   *  decide using the maximum differential
   */

  if (tr_tool->trans_info[HORZ_OR_VERT] == GIMP_ORIENTATION_UNKNOWN)
    {
      if (abs (diffx) > MIN_MOVE || abs (diffy) > MIN_MOVE)
	{
	  if (abs (diffx) > abs (diffy))
	    {
	      tr_tool->trans_info[HORZ_OR_VERT] = GIMP_ORIENTATION_HORIZONTAL;
	      tr_tool->trans_info[XSHEAR] = 0.0;
	    }
	  else
	    {
	      tr_tool->trans_info[HORZ_OR_VERT] = GIMP_ORIENTATION_VERTICAL;
	      tr_tool->trans_info[XSHEAR] = 0.0;
	    }
	}
      /*  set the current coords to the last ones  */
      else
	{
	  tr_tool->curx = tr_tool->lastx;
	  tr_tool->cury = tr_tool->lasty;
	}
    }

  /*  if the direction is known, keep track of the magnitude  */
  if (tr_tool->trans_info[HORZ_OR_VERT] != GIMP_ORIENTATION_UNKNOWN)
    {
      dir = tr_tool->trans_info[HORZ_OR_VERT];

      switch (tr_tool->function)
	{
	case TRANSFORM_HANDLE_1:
	  if (dir == GIMP_ORIENTATION_HORIZONTAL)
	    tr_tool->trans_info[XSHEAR] -= diffx;
	  else
	    tr_tool->trans_info[YSHEAR] -= diffy;
	  break;

	case TRANSFORM_HANDLE_2:
	  if (dir == GIMP_ORIENTATION_HORIZONTAL)
	    tr_tool->trans_info[XSHEAR] -= diffx;
	  else
	    tr_tool->trans_info[YSHEAR] += diffy;
	  break;

	case TRANSFORM_HANDLE_3:
	  if (dir == GIMP_ORIENTATION_HORIZONTAL)
	    tr_tool->trans_info[XSHEAR] += diffx;
	  else
	    tr_tool->trans_info[YSHEAR] -= diffy;
	  break;

	case TRANSFORM_HANDLE_4:
	  if (dir == GIMP_ORIENTATION_HORIZONTAL)
	    tr_tool->trans_info[XSHEAR] += diffx;
	  else
	    tr_tool->trans_info[YSHEAR] += diffy;
	  break;

	default:
	  break;
	}
    }

  gimp_transform_tool_expose_preview (tr_tool);
}

static void
gimp_shear_tool_recalc (GimpTransformTool *tr_tool,
                        GimpDisplay       *gdisp)
{
  gdouble amount;

  if (tr_tool->trans_info[HORZ_OR_VERT] == GIMP_ORIENTATION_HORIZONTAL)
    amount = tr_tool->trans_info[XSHEAR];
  else
    amount = tr_tool->trans_info[YSHEAR];

  gimp_transform_matrix_shear (tr_tool->x1,
                               tr_tool->y1,
                               tr_tool->x2,
                               tr_tool->y2,
                               tr_tool->trans_info[HORZ_OR_VERT],
                               amount,
                               &tr_tool->transform);

  /*  transform the bounding box  */
  gimp_transform_tool_transform_bounding_box (tr_tool);

  /*  update the information dialog  */
  shear_info_update (tr_tool);
}

static void
shear_info_update (GimpTransformTool *tr_tool)
{
  xshear_val = tr_tool->trans_info[XSHEAR];
  yshear_val = tr_tool->trans_info[YSHEAR];

  info_dialog_update (tr_tool->info_dialog);
  info_dialog_show (tr_tool->info_dialog);
}

static void
shear_x_mag_changed (GtkWidget         *widget,
                     GimpTransformTool *tr_tool)
{
  gdouble value;

  value = GTK_ADJUSTMENT (widget)->value;

  if (value != tr_tool->trans_info[XSHEAR])
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

      tr_tool->trans_info[XSHEAR] = value;

      gimp_shear_tool_recalc (tr_tool, GIMP_TOOL (tr_tool)->gdisp);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
    }

  gimp_transform_tool_expose_preview (tr_tool);
}

static void
shear_y_mag_changed (GtkWidget         *widget,
                     GimpTransformTool *tr_tool)
{
  gdouble value;

  value = GTK_ADJUSTMENT (widget)->value;

  if (value != tr_tool->trans_info[YSHEAR])
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

      tr_tool->trans_info[YSHEAR] = value;

      gimp_shear_tool_recalc (tr_tool, GIMP_TOOL (tr_tool)->gdisp);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
    }
  
  gimp_transform_tool_expose_preview (tr_tool);
}
