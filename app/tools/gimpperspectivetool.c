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

#include "apptypes.h"

#include "core/gimpimage.h"

#include "gdisplay.h"
#include "gimpprogress.h"
#include "gui/info-dialog.h"
#include "selection.h"
#include "tile_manager.h"
#include "undo.h"

#include "gimpperspectivetool.h"
#include "tool_manager.h"
#include "tool_options.h"
#include "transform_options.h"

#include "libgimp/gimpintl.h"

#include "pixmaps2.h"

/*  forward function declarations  */
static void          gimp_perspective_tool_class_init  (GimpPerspectiveToolClass *klass);
static void          gimp_perspective_tool_init        (GimpPerspectiveTool      *perspective_tool);

static void          gimp_perspective_tool_destroy     (GtkObject         *object);

static TileManager * gimp_perspective_tool_transform   (GimpTransformTool *transform_tool,
							GDisplay       *gdisp,
							TransformState  state);
static void          perspective_tool_recalc           (GimpTool       *tool,
							GDisplay       *gdisp);
static void          perspective_tool_motion           (GimpTool       *tool,
							GDisplay       *gdisp);
static void          perspective_info_update           (GimpTool       *tool);


/*  storage for information dialog fields  */
static gchar  matrix_row_buf [3][MAX_INFO_BUF];

static GimpTransformToolClass *parent_class = NULL;

static TransformOptions *perspective_options = NULL;

/*  public functions  */

void 
gimp_perspective_tool_register (void)
{
  tool_manager_register_tool (GIMP_TYPE_PERSPECTIVE_TOOL,
                              FALSE,
			      "gimp:perspective_tool",
			      _("Perspective Tool"),
			      _("Change perspective of the layer or selection"),
			      N_("/Tools/Transform Tools/Perspective"), "<shift>P",
			      NULL, "tools/perspective.html",
			      (const gchar **) perspective_bits);
}

GtkType
gimp_perspective_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpPerspectiveTool",
        sizeof (GimpPerspectiveTool),
        sizeof (GimpPerspectiveToolClass),
        (GtkClassInitFunc) gimp_perspective_tool_class_init,
        (GtkObjectInitFunc) gimp_perspective_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      tool_type = gtk_type_unique (GIMP_TYPE_TRANSFORM_TOOL, &tool_info);
    }

  return tool_type;
}

TileManager *
gimp_perspective_tool_perspective (GimpImage    *gimage,
				   GimpDrawable *drawable,
				   GDisplay     *gdisp,
				   TileManager  *float_tiles,
				   gboolean      interpolation,
				   GimpMatrix3   matrix)
{
  GimpProgress *progress;
  TileManager  *ret;

  progress = progress_start (gdisp, _("Perspective..."), FALSE, NULL, NULL);

  ret = gimp_transform_tool_do (gimage, drawable, float_tiles,
				interpolation, matrix,
				progress ? progress_update_and_flush :
				(GimpProgressFunc) NULL,
				progress);

  if (progress)
    progress_end (progress);

  return ret;
}

void
gimp_perspective_tool_find_transform (gdouble     *coords,
				      GimpMatrix3  matrix)
{
  gdouble dx1, dx2, dx3, dy1, dy2, dy3;
  gdouble det1, det2;

  dx1 = coords[X1] - coords[X3];
  dx2 = coords[X2] - coords[X3];
  dx3 = coords[X0] - coords[X1] + coords[X3] - coords[X2];

  dy1 = coords[Y1] - coords[Y3];
  dy2 = coords[Y2] - coords[Y3];
  dy3 = coords[Y0] - coords[Y1] + coords[Y3] - coords[Y2];

  /*  Is the mapping affine?  */
  if ((dx3 == 0.0) && (dy3 == 0.0))
    {
      matrix[0][0] = coords[X1] - coords[X0];
      matrix[0][1] = coords[X3] - coords[X1];
      matrix[0][2] = coords[X0];
      matrix[1][0] = coords[Y1] - coords[Y0];
      matrix[1][1] = coords[Y3] - coords[Y1];
      matrix[1][2] = coords[Y0];
      matrix[2][0] = 0.0;
      matrix[2][1] = 0.0;
    }
  else
    {
      det1 = dx3 * dy2 - dy3 * dx2;
      det2 = dx1 * dy2 - dy1 * dx2;
      matrix[2][0] = det1 / det2;
      det1 = dx1 * dy3 - dy1 * dx3;
      det2 = dx1 * dy2 - dy1 * dx2;
      matrix[2][1] = det1 / det2;

      matrix[0][0] = coords[X1] - coords[X0] + matrix[2][0] * coords[X1];
      matrix[0][1] = coords[X2] - coords[X0] + matrix[2][1] * coords[X2];
      matrix[0][2] = coords[X0];

      matrix[1][0] = coords[Y1] - coords[Y0] + matrix[2][0] * coords[Y1];
      matrix[1][1] = coords[Y2] - coords[Y0] + matrix[2][1] * coords[Y2];
      matrix[1][2] = coords[Y0];
    }

  matrix[2][2] = 1.0;
}

/* private function definitions */

static void
gimp_perspective_tool_class_init (GimpPerspectiveToolClass *klass)
{
  GtkObjectClass         *object_class;
  GimpTransformToolClass *trans_class;

  object_class = (GtkObjectClass *) klass;
  trans_class  = (GimpTransformToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_TRANSFORM_TOOL);

  object_class->destroy     = gimp_perspective_tool_destroy;

  trans_class->transform    = gimp_perspective_tool_transform;
}

static void
gimp_perspective_tool_init (GimpPerspectiveTool *perspective_tool)
{
  GimpTool          *tool;
  GimpTransformTool *tr_tool;

  tool    = GIMP_TOOL (perspective_tool);
  tr_tool = GIMP_TRANSFORM_TOOL (perspective_tool);

  if (! perspective_options)
    {
      perspective_options = transform_options_new (GIMP_TYPE_PERSPECTIVE_TOOL,
                                            transform_options_reset);

      tool_manager_register_tool_options (GIMP_TYPE_PERSPECTIVE_TOOL,
                                          (ToolOptions *) perspective_options);
    }

  tool->tool_cursor   = GIMP_PERSPECTIVE_TOOL_CURSOR;

  tr_tool->trans_info[X0] = 0;
  tr_tool->trans_info[Y0] = 0;
  tr_tool->trans_info[X1] = 0;
  tr_tool->trans_info[Y1] = 0;
  tr_tool->trans_info[X2] = 0;
  tr_tool->trans_info[Y2] = 0;
  tr_tool->trans_info[X3] = 0;
  tr_tool->trans_info[Y3] = 0;

  /*  assemble the transformation matrix  */
  gimp_matrix3_identity (tr_tool->transform);

}

static void
gimp_perspective_tool_destroy (GtkObject *object)
{
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static TileManager *
gimp_perspective_tool_transform (GimpTransformTool  *transform_tool,
				 GDisplay           *gdisp,
				 TransformState      state)
{
  GimpTool *tool;

  tool = GIMP_TOOL (transform_tool);

  switch (state)
    {
    case TRANSFORM_INIT:
      if (!transform_info)
	{
	  transform_info =
	    info_dialog_new (_("Perspective Transform Information"),
			     gimp_standard_help_func,
			     "tools/transform_perspective.html");
	  info_dialog_add_label (transform_info, _("Matrix:"),
				 matrix_row_buf[0]);
	  info_dialog_add_label (transform_info, "", matrix_row_buf[1]);
	  info_dialog_add_label (transform_info, "", matrix_row_buf[2]);
	}
      gtk_widget_set_sensitive (GTK_WIDGET (transform_info->shell), TRUE);

      transform_tool->trans_info [X0] = (double) transform_tool->x1;
      transform_tool->trans_info [Y0] = (double) transform_tool->y1;
      transform_tool->trans_info [X1] = (double) transform_tool->x2;
      transform_tool->trans_info [Y1] = (double) transform_tool->y1;
      transform_tool->trans_info [X2] = (double) transform_tool->x1;
      transform_tool->trans_info [Y2] = (double) transform_tool->y2;
      transform_tool->trans_info [X3] = (double) transform_tool->x2;
      transform_tool->trans_info [Y3] = (double) transform_tool->y2;

      return NULL;
      break;

    case TRANSFORM_MOTION:
      perspective_tool_motion (tool, gdisp);
      perspective_tool_recalc (tool, gdisp);
      break;

    case TRANSFORM_RECALC:
      perspective_tool_recalc (tool, gdisp);
      break;

    case TRANSFORM_FINISH:
      /*  Let the transform tool handle the inverse mapping  */
      gtk_widget_set_sensitive (GTK_WIDGET (transform_info->shell), FALSE);
      return
	gimp_perspective_tool_perspective (gdisp->gimage,
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
perspective_info_update (GimpTool *tool)
{
  GimpTransformTool *transform_tool;
  gint               i;
  
  transform_tool = GIMP_TRANSFORM_TOOL (tool);

  for (i = 0; i < 3; i++)
    {
      gchar *p = matrix_row_buf[i];
      gint   j;
      
      for (j = 0; j < 3; j++)
	{
	  p += g_snprintf (p, MAX_INFO_BUF - (p - matrix_row_buf[i]),
			   "%10.3g", transform_tool->transform[i][j]);
	}
    }

  info_dialog_update (transform_info);
  info_dialog_popup (transform_info);

  return;
}

static void
perspective_tool_motion (GimpTool *tool,
			 GDisplay *gdisp)
{
  GimpTransformTool *transform_tool;
  gint               diff_x, diff_y;

  transform_tool = GIMP_TRANSFORM_TOOL (tool);

  diff_x = transform_tool->curx - transform_tool->lastx;
  diff_y = transform_tool->cury - transform_tool->lasty;

  switch (transform_tool->function)
    {
    case TRANSFORM_HANDLE_1:
      transform_tool->trans_info [X0] += diff_x;
      transform_tool->trans_info [Y0] += diff_y;
      break;
    case TRANSFORM_HANDLE_2:
      transform_tool->trans_info [X1] += diff_x;
      transform_tool->trans_info [Y1] += diff_y;
      break;
    case TRANSFORM_HANDLE_3:
      transform_tool->trans_info [X2] += diff_x;
      transform_tool->trans_info [Y2] += diff_y;
      break;
    case TRANSFORM_HANDLE_4:
      transform_tool->trans_info [X3] += diff_x;
      transform_tool->trans_info [Y3] += diff_y;
      break;
    default:
      break;
    }
}

static void
perspective_tool_recalc (GimpTool *tool,
			 GDisplay *gdisp)
{
  GimpTransformTool *transform_tool;
  GimpMatrix3        m;
  gdouble            cx, cy;
  gdouble            scalex, scaley;

  transform_tool = GIMP_TRANSFORM_TOOL (tool);

  /*  determine the perspective transform that maps from
   *  the unit cube to the trans_info coordinates
   */
  gimp_perspective_tool_find_transform (transform_tool->trans_info, m);

  cx     = transform_tool->x1;
  cy     = transform_tool->y1;
  scalex = 1.0;
  scaley = 1.0;

  if (transform_tool->x2 - transform_tool->x1)
    scalex = 1.0 / (transform_tool->x2 - transform_tool->x1);
  if (transform_tool->y2 - transform_tool->y1)
    scaley = 1.0 / (transform_tool->y2 - transform_tool->y1);

  /*  assemble the transformation matrix  */
  gimp_matrix3_identity  (transform_tool->transform);
  gimp_matrix3_translate (transform_tool->transform, -cx, -cy);
  gimp_matrix3_scale     (transform_tool->transform, scalex, scaley);
  gimp_matrix3_mult      (m, transform_tool->transform);

  /*  transform the bounding box  */
  gimp_transform_tool_transform_bounding_box (transform_tool);

  /*  update the information dialog  */
  perspective_info_update (tool);
}

