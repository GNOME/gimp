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

#include "core/gimpimage.h"

#include "gui/info-dialog.h"

#include "floating_sel.h"
#include "gdisplay.h"
#include "gdisplay_ops.h"
#include "gimpprogress.h"
#include "undo.h"

#include "gimpsheartool.h"
#include "tool_manager.h"
#include "tool_options.h"
#include "transform_options.h"

#include "libgimp/gimpintl.h"

#define WANT_SHEAR_BITS
#include "icons.h"


/*  index into trans_info array  */
#define HORZ_OR_VERT 0
#define XSHEAR       1
#define YSHEAR       2

/*  the minimum movement before direction of shear can be determined (pixels) */
#define MIN_MOVE     5


/*  forward function declarations  */

static void          gimp_shear_tool_class_init (GimpShearToolClass *klass);
static void          gimp_shear_tool_init      (GimpShearTool       *shear_tool);

static void          gimp_shear_tool_destroy   (GtkObject      *object);

static TileManager * gimp_shear_tool_transform (GimpTransformTool *transform_tool,
						GDisplay       *gdisp,
						TransformState  state);

static void          shear_tool_recalc         (GimpTool       *tool,
						GDisplay       *gdisp);
static void          shear_tool_motion         (GimpTool       *tool,
						GDisplay       *gdisp);
static void          shear_info_update         (GimpTool       *tool);

static void          shear_x_mag_changed       (GtkWidget      *widget,
						gpointer        data);
static void          shear_y_mag_changed       (GtkWidget      *widget,
						gpointer        data);


/*  variables local to this file  */
static gdouble  xshear_val;
static gdouble  yshear_val;

static GimpTransformToolClass *parent_class = NULL;

static TransformOptions *shear_options = NULL;


/* Public functions */

void 
gimp_shear_tool_register (Gimp *gimp)
{
  tool_manager_register_tool (gimp,
			      GIMP_TYPE_SHEAR_TOOL,
                              FALSE,
			      "gimp:shear_tool",
			      _("Shear Tool"),
			      _("Shear the layer or selection"),
			      N_("/Tools/Transform Tools/Shear"), "<shift>F",
			      NULL, "tools/shear.html",
			      (const gchar **) shear_bits);
}

GtkType
gimp_shear_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpShearTool",
        sizeof (GimpShearTool),
        sizeof (GimpShearToolClass),
        (GtkClassInitFunc) gimp_shear_tool_class_init,
        (GtkObjectInitFunc) gimp_shear_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      tool_type = gtk_type_unique (GIMP_TYPE_TRANSFORM_TOOL, &tool_info);
    }

  return tool_type;
}

TileManager *
gimp_shear_tool_shear (GimpImage    *gimage,
		       GimpDrawable *drawable,
		       GDisplay     *gdisp,
		       TileManager  *float_tiles,
		       gboolean      interpolation,
		       GimpMatrix3   matrix)
{
  GimpProgress *progress;
  TileManager  *ret;

  progress = progress_start (gdisp, _("Shearing..."), FALSE, NULL, NULL);

  ret = gimp_transform_tool_do (gimage, drawable, float_tiles,
				interpolation, matrix,
				progress ? progress_update_and_flush :
				(GimpProgressFunc) NULL,
				progress);

  if (progress)
    progress_end (progress);

  return ret;
}

/* private functions */

static void
gimp_shear_tool_class_init (GimpShearToolClass *klass)
{
  GtkObjectClass         *object_class;
  GimpTransformToolClass *trans_class;

  object_class = (GtkObjectClass *) klass;
  trans_class  = (GimpTransformToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_TRANSFORM_TOOL);

  object_class->destroy     = gimp_shear_tool_destroy;

  trans_class->transform    = gimp_shear_tool_transform;
}

static void
gimp_shear_tool_init (GimpShearTool *shear_tool)
{
  GimpTool          *tool;
  GimpTransformTool *tr_tool;

  tool    = GIMP_TOOL (shear_tool);
  tr_tool = GIMP_TRANSFORM_TOOL (shear_tool);

  if (! shear_options)
    {
      shear_options = transform_options_new (GIMP_TYPE_SHEAR_TOOL,
                                            transform_options_reset);

      tool_manager_register_tool_options (GIMP_TYPE_SHEAR_TOOL,
                                          (ToolOptions *) shear_options);
    }

  tool->tool_cursor   = GIMP_SHEAR_TOOL_CURSOR;

  /*  assemble the transformation matrix  */
  gimp_matrix3_identity (tr_tool->transform);

}

static void
gimp_shear_tool_destroy (GtkObject *object)
{
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static TileManager *
gimp_shear_tool_transform (GimpTransformTool       *transform_tool,
			   GDisplay                *gdisp,
			   TransformState          state)
{
  GimpTool *tool;
  tool = GIMP_TOOL (transform_tool);

  switch (state)
    {
    case TRANSFORM_INIT:
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
      transform_tool->trans_info[HORZ_OR_VERT] = ORIENTATION_UNKNOWN;
      transform_tool->trans_info[XSHEAR] = 0.0;
      transform_tool->trans_info[YSHEAR] = 0.0;

      return NULL;
      break;

    case TRANSFORM_MOTION:
      shear_tool_motion (tool, gdisp);
      shear_tool_recalc (tool, gdisp);
      break;

    case TRANSFORM_RECALC:
      shear_tool_recalc (tool, gdisp);
      break;

    case TRANSFORM_FINISH:
      gtk_widget_set_sensitive (GTK_WIDGET (transform_info->shell), FALSE);
      return gimp_shear_tool_shear (gdisp->gimage,
				    gimp_image_active_drawable (gdisp->gimage),
				    gdisp,
				    transform_tool->original,
				    gimp_transform_tool_smoothing (),
				    transform_tool->transform);
      break;
    }

  return NULL;
}

static void
shear_info_update (GimpTool *tool)
{
  GimpTransformTool *transform_tool;

  transform_tool = GIMP_TRANSFORM_TOOL (tool);

  xshear_val = transform_tool->trans_info[XSHEAR];
  yshear_val = transform_tool->trans_info[YSHEAR];

  info_dialog_update (transform_info);
  info_dialog_popup (transform_info);
}

static void
shear_x_mag_changed (GtkWidget *widget,
		     gpointer   data)
{
  GimpTool          *tool;
  GimpDrawTool      *draw_tool;
  GimpTransformTool *transform_tool;
  gint               value;

  tool = (GimpTool *) data;

  if (tool)
    {
      draw_tool      = GIMP_DRAW_TOOL (tool);
      transform_tool = GIMP_TRANSFORM_TOOL (tool);

      value = GTK_ADJUSTMENT (widget)->value;

      if (value != transform_tool->trans_info[XSHEAR])
	{
	  gimp_draw_tool_pause (draw_tool);
	  transform_tool->trans_info[XSHEAR] = value;
	  shear_tool_recalc (tool, tool->gdisp);
	  gimp_draw_tool_resume (draw_tool);
	}
    }
}

static void
shear_y_mag_changed (GtkWidget *widget,
		     gpointer   data)
{
  GimpTool          *tool;
  GimpDrawTool      *draw_tool;
  GimpTransformTool *transform_tool;
  gint               value;

  tool = (GimpTool *) data;

  if (tool)
    {
      draw_tool      = GIMP_DRAW_TOOL (tool);
      transform_tool = GIMP_TRANSFORM_TOOL (tool);

      value = GTK_ADJUSTMENT (widget)->value;

      if (value != transform_tool->trans_info[YSHEAR])
	{
	  gimp_draw_tool_pause (draw_tool);
	  transform_tool->trans_info[YSHEAR] = value;
	  shear_tool_recalc (tool, tool->gdisp);
	  gimp_draw_tool_resume (draw_tool);
	}
    }
}

static void
shear_tool_motion (GimpTool     *tool,
		   GDisplay *gdisp)
{
  GimpTransformTool *transform_tool;
  gint               diffx, diffy;
  gint               dir;

  transform_tool = GIMP_TRANSFORM_TOOL (tool);

  diffx = transform_tool->curx - transform_tool->lastx;
  diffy = transform_tool->cury - transform_tool->lasty;

  /*  If we haven't yet decided on which way to control shearing
   *  decide using the maximum differential
   */

  if (transform_tool->trans_info[HORZ_OR_VERT] == ORIENTATION_UNKNOWN)
    {
      if (abs (diffx) > MIN_MOVE || abs (diffy) > MIN_MOVE)
	{
	  if (abs (diffx) > abs (diffy))
	    {
	      transform_tool->trans_info[HORZ_OR_VERT] = ORIENTATION_HORIZONTAL;
	      transform_tool->trans_info[ORIENTATION_VERTICAL] = 0.0;
	    }
	  else
	    {
	      transform_tool->trans_info[HORZ_OR_VERT] = ORIENTATION_VERTICAL;
	      transform_tool->trans_info[ORIENTATION_HORIZONTAL] = 0.0;
	    }
	}
      /*  set the current coords to the last ones  */
      else
	{
	  transform_tool->curx = transform_tool->lastx;
	  transform_tool->cury = transform_tool->lasty;
	}
    }

  /*  if the direction is known, keep track of the magnitude  */
  if (transform_tool->trans_info[HORZ_OR_VERT] != ORIENTATION_UNKNOWN)
    {
      dir = transform_tool->trans_info[HORZ_OR_VERT];
      switch (transform_tool->function)
	{
	case TRANSFORM_HANDLE_1:
	  if (dir == ORIENTATION_HORIZONTAL)
	    transform_tool->trans_info[XSHEAR] -= diffx;
	  else
	    transform_tool->trans_info[YSHEAR] -= diffy;
	  break;
	case TRANSFORM_HANDLE_2:
	  if (dir == ORIENTATION_HORIZONTAL)
	    transform_tool->trans_info[XSHEAR] -= diffx;
	  else
	    transform_tool->trans_info[YSHEAR] += diffy;
	  break;
	case TRANSFORM_HANDLE_3:
	  if (dir == ORIENTATION_HORIZONTAL)
	    transform_tool->trans_info[XSHEAR] += diffx;
	  else
	    transform_tool->trans_info[YSHEAR] -= diffy;
	  break;
	case TRANSFORM_HANDLE_4:
	  if (dir == ORIENTATION_HORIZONTAL)
	    transform_tool->trans_info[XSHEAR] += diffx;
	  else
	    transform_tool->trans_info[YSHEAR] += diffy;
	  break;
	default:
	  break;
	}
    }
}

static void
shear_tool_recalc (GimpTool     *tool,
		   GDisplay *gdisp)
{
  GimpTransformTool *transform_tool;
  gfloat             width, height;
  gfloat             cx, cy;

  transform_tool = GIMP_TRANSFORM_TOOL (tool);

  cx = (transform_tool->x1 + transform_tool->x2) / 2.0;
  cy = (transform_tool->y1 + transform_tool->y2) / 2.0;

  width = transform_tool->x2 - transform_tool->x1;
  height = transform_tool->y2 - transform_tool->y1;

  if (width == 0)
    width = 1;
  if (height == 0)
    height = 1;

  /*  assemble the transformation matrix  */
  gimp_matrix3_identity  (transform_tool->transform);
  gimp_matrix3_translate (transform_tool->transform, -cx, -cy);

  /*  shear matrix  */
  if (transform_tool->trans_info[HORZ_OR_VERT] == ORIENTATION_HORIZONTAL)
    gimp_matrix3_xshear (transform_tool->transform,
			 (float) transform_tool->trans_info [XSHEAR] / height);
  else
    gimp_matrix3_yshear (transform_tool->transform,
			 (float) transform_tool->trans_info [YSHEAR] / width);

  gimp_matrix3_translate (transform_tool->transform, +cx, +cy);

  /*  transform the bounding box  */
  gimp_transform_tool_transform_bounding_box (transform_tool);

  /*  update the information dialog  */
  shear_info_update (tool);
}
