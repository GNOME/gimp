/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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
#include "gui/gui-types.h"

#include "base/tile-manager.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-transform.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimplayer.h"
#include "core/gimpmarshal.h"
#include "core/gimptoolinfo.h"

#include "gui/info-dialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"

#include "tool_manager.h"
#include "transform_options.h"
#include "gimptransformtool.h"
#include "gimpperspectivetool.h"
#include "gimprotatetool.h"
#include "gimpscaletool.h"
#include "gimpsheartool.h"
#include "gimpfliptool.h"

#include "floating_sel.h"
#include "gimpprogress.h"
#include "undo.h"
#include "path_transform.h"

#include "libgimp/gimpintl.h"


#define HANDLE 10

enum
{
  TRANSFORM,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   gimp_transform_tool_init        (GimpTransformTool      *tool);
static void   gimp_transform_tool_class_init  (GimpTransformToolClass *tool);

static void   gimp_transform_tool_finalize         (GObject           *object);

static void   gimp_transform_tool_control          (GimpTool          *tool,
                                                    ToolAction         action,
                                                    GimpDisplay       *gdisp);
static void   gimp_transform_tool_button_press     (GimpTool          *tool,
                                                    GimpCoords        *coords,
                                                    guint32            time,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *gdisp);
static void   gimp_transform_tool_button_release   (GimpTool          *tool,
                                                    GimpCoords        *coords,
                                                    guint32            time,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *gdisp);
static void   gimp_transform_tool_motion           (GimpTool          *tool,
                                                    GimpCoords        *coords,
                                                    guint32            time,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *gdisp);
static void   gimp_transform_tool_cursor_update    (GimpTool          *tool,
                                                    GimpCoords        *coords,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *gdisp);

static void   gimp_transform_tool_draw             (GimpDrawTool      *draw_tool);

static TileManager * gimp_transform_tool_transform (GimpTransformTool *tr_tool,
                                                    GimpDisplay       *gdisp,
                                                    TransformState     state);
static void   gimp_transform_tool_reset            (GimpTransformTool *tr_tool,
                                                    GimpDisplay       *gdisp);
static void   gimp_transform_tool_bounds           (GimpTransformTool *tr_tool,
                                                    GimpDisplay       *gdisp);
static void   gimp_transform_tool_recalc           (GimpTransformTool *tr_tool,
                                                    GimpDisplay       *gdisp);
static void   gimp_transform_tool_doit             (GimpTransformTool *tr_tool,
                                                    GimpDisplay       *gdisp);
static void   gimp_transform_tool_setup_grid       (GimpTransformTool *tr_tool,
                                                    TransformOptions  *options);
static void   gimp_transform_tool_grid_recalc      (GimpTransformTool *tr_tool);

static void   transform_ok_callback                (GtkWidget         *widget,
                                                    gpointer           data);
static void   transform_reset_callback             (GtkWidget         *widget,
                                                    gpointer           data);


/*  variables  */
static TransInfo          old_trans_info;
InfoDialog               *transform_info        = NULL;
static gboolean           transform_info_inited = FALSE;

static GimpDrawToolClass *parent_class = NULL;

static guint   gimp_transform_tool_signals[LAST_SIGNAL] = { 0 };


GType
gimp_transform_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpTransformToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_transform_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpTransformTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_transform_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_DRAW_TOOL,
					  "GimpTransformTool", 
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_transform_tool_class_init (GimpTransformToolClass *klass)
{
  GObjectClass      *object_class;
  GimpToolClass     *tool_class;
  GimpDrawToolClass *draw_class;

  object_class = G_OBJECT_CLASS (klass);
  tool_class   = GIMP_TOOL_CLASS (klass);
  draw_class   = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gimp_transform_tool_signals[TRANSFORM] =
    g_signal_new ("transform",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GimpTransformToolClass, transform),
		  NULL, NULL,
		  gimp_cclosure_marshal_POINTER__POINTER_INT,
		  G_TYPE_POINTER, 2,
		  G_TYPE_POINTER,
		  G_TYPE_INT);

  object_class->finalize     = gimp_transform_tool_finalize;

  tool_class->control        = gimp_transform_tool_control;
  tool_class->button_press   = gimp_transform_tool_button_press;
  tool_class->button_release = gimp_transform_tool_button_release;
  tool_class->motion         = gimp_transform_tool_motion;
  tool_class->cursor_update  = gimp_transform_tool_cursor_update;

  draw_class->draw           = gimp_transform_tool_draw;
}

static void
gimp_transform_tool_init (GimpTransformTool *transform_tool)
{
  GimpTool *tool;
  gint      i;

  tool = GIMP_TOOL (transform_tool);

  transform_tool->function = TRANSFORM_CREATING;
  transform_tool->original = NULL;

  for (i = 0; i < TRAN_INFO_SIZE; i++)
    transform_tool->trans_info[i] = 0.0;

  gimp_matrix3_identity (transform_tool->transform);

  transform_tool->use_grid     = TRUE;
  transform_tool->ngx          = 0;
  transform_tool->ngy          = 0;
  transform_tool->grid_coords  = NULL;
  transform_tool->tgrid_coords = NULL;

  tool->scroll_lock = TRUE;   /*  Disallow scrolling  */
  tool->preserve    = FALSE;  /*  Don't preserve on drawable change  */

}

static void
gimp_transform_tool_finalize (GObject *object)
{
  GimpTransformTool *tr_tool;

  tr_tool = GIMP_TRANSFORM_TOOL (object);

  if (tr_tool->original)
    {
      tile_manager_destroy (tr_tool->original);
      tr_tool->original = NULL;
    }

  if (transform_info)
    {
      info_dialog_free (transform_info);
      transform_info        = NULL;
      transform_info_inited = FALSE;
    }

  if (tr_tool->grid_coords)
    {
      g_free (tr_tool->grid_coords);
      tr_tool->grid_coords = NULL;
    }

  if (tr_tool->tgrid_coords)
    {
      g_free (tr_tool->tgrid_coords);
      tr_tool->tgrid_coords = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_transform_tool_control (GimpTool    *tool,
			     ToolAction   action,
			     GimpDisplay *gdisp)
{
  GimpTransformTool *transform_tool;

  transform_tool = GIMP_TRANSFORM_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      gimp_transform_tool_recalc (transform_tool, gdisp);
      break;

    case HALT:
      gimp_transform_tool_reset (transform_tool, gdisp);
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_transform_tool_button_press (GimpTool        *tool,
                                  GimpCoords      *coords,
                                  guint32          time,
                                  GdkModifierType  state,
			          GimpDisplay     *gdisp)
{
  GimpTransformTool *tr_tool;
  GimpDrawTool      *draw_tool;
  GimpDisplayShell  *shell;
  GimpDrawable      *drawable;
  gdouble            dist;
  gdouble            closest_dist;
  gint               i;
  gint               off_x, off_y;

  tr_tool   = GIMP_TRANSFORM_TOOL (tool);
  draw_tool = GIMP_DRAW_TOOL (tool);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (tr_tool->function == TRANSFORM_CREATING && tool->state == ACTIVE)
    {
      /*  Save the current transformation info  */
      for (i = 0; i < TRAN_INFO_SIZE; i++)
	old_trans_info[i] = tr_tool->trans_info[i];
    }

  /*  if we have already displayed the bounding box and handles,
   *  check to make sure that the display which currently owns the
   *  tool is the one which just received the button pressed event
   */
  if (gdisp == tool->gdisp)
    {
      /*  start drawing the bounding box and handles...  */
      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), shell->canvas->window);

      closest_dist = gimp_draw_tool_calc_distance (draw_tool, gdisp,
                                                   coords->x, coords->y,
                                                   tr_tool->tx1, tr_tool->ty1);
      tr_tool->function = TRANSFORM_HANDLE_1;

      dist = gimp_draw_tool_calc_distance (draw_tool, gdisp,
                                           coords->x, coords->y,
                                           tr_tool->tx2, tr_tool->ty2);
      if (dist < closest_dist)
	{
	  closest_dist = dist;
	  tr_tool->function = TRANSFORM_HANDLE_2;
	}

      dist = gimp_draw_tool_calc_distance (draw_tool, gdisp,
                                           coords->x, coords->y,
                                           tr_tool->tx3, tr_tool->ty3);
      if (dist < closest_dist)
	{
	  closest_dist = dist;
	  tr_tool->function = TRANSFORM_HANDLE_3;
	}

      dist = gimp_draw_tool_calc_distance (draw_tool, gdisp,
                                           coords->x, coords->y,
                                           tr_tool->tx4, tr_tool->ty4);
      if (dist < closest_dist)
	{
	  closest_dist = dist;
	  tr_tool->function = TRANSFORM_HANDLE_4;
	}

      if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                    coords->x, coords->y,
                                    GIMP_HANDLE_CIRCLE,
                                    tr_tool->tcx, tr_tool->tcy,
                                    HANDLE, HANDLE,
                                    GIMP_HANDLE_CIRCLE,
                                    FALSE))
	{
	  tr_tool->function = TRANSFORM_HANDLE_CENTER;
	}

      /*  Save the current pointer position  */
      tr_tool->lastx = tr_tool->startx = coords->x;
      tr_tool->lasty = tr_tool->starty = coords->y;

      gdk_pointer_grab (shell->canvas->window, FALSE,
			GDK_POINTER_MOTION_HINT_MASK |
			GDK_BUTTON1_MOTION_MASK |
			GDK_BUTTON_RELEASE_MASK,
			NULL, NULL, time);

      tool->state = ACTIVE;
      return;
    }

  /*  Initialisation stuff: if the cursor is clicked inside the current
   *  selection, show the bounding box and handles...
   */
  gimp_drawable_offsets (drawable, &off_x, &off_y);

  if (coords->x >= off_x &&
      coords->y >= off_y &&
      coords->x < (off_x + gimp_drawable_width (drawable)) &&
      coords->y < (off_y + gimp_drawable_height (drawable)))
    {
      if (gimage_mask_is_empty (gdisp->gimage) ||
	  gimage_mask_value (gdisp->gimage, coords->x, coords->y))
	{
	  if (GIMP_IS_LAYER (drawable) &&
	      gimp_layer_get_mask (GIMP_LAYER (drawable)))
	    {
	      g_message (_("Transformations do not work on\n"
			   "layers that contain layer masks."));
	      tool->state = INACTIVE;
	      return;
	    }

	  /*  If the tool is already active, clear the current state
	   *  and reset
	   */
	  if (tool->state == ACTIVE)
	    gimp_transform_tool_reset (tr_tool, gdisp);

	  /*  Set the pointer to the active display  */
	  tool->gdisp    = gdisp;
	  tool->drawable = drawable;
	  tool->state    = ACTIVE;

          gdk_pointer_grab (shell->canvas->window, FALSE,
                            GDK_POINTER_MOTION_HINT_MASK |
                            GDK_BUTTON1_MOTION_MASK |
                            GDK_BUTTON_RELEASE_MASK,
                            NULL, NULL, time);

	  /*  Find the transform bounds for some tools (like scale,
	   *  perspective) that actually need the bounds for
	   *  initializing
	   */
	  gimp_transform_tool_bounds (tr_tool, gdisp);

	  /*  Initialize the transform tool */
	  gimp_transform_tool_transform (tr_tool, gdisp, TRANSFORM_INIT);

	  if (transform_info && ! transform_info_inited)
	    {
	      GType tool_type;

	      tool_type =
		gimp_context_get_tool (gimp_get_user_context (gdisp->gimage->gimp))->tool_type;

	      gimp_dialog_create_action_area
		(GIMP_DIALOG (transform_info->shell),

		 /* FIXME: this does not belong here */
		 (tool_type == GIMP_TYPE_ROTATE_TOOL)      ? _("Rotate")    :
		 (tool_type == GIMP_TYPE_SCALE_TOOL)       ? _("Scale")     :
		 (tool_type == GIMP_TYPE_SHEAR_TOOL)       ? _("Shear")     :
		 (tool_type == GIMP_TYPE_PERSPECTIVE_TOOL) ? _("Transform") :
		 "EEK",
		 transform_ok_callback,
		 tool, NULL, NULL, TRUE, FALSE,

		 _("Reset"), transform_reset_callback,
		 tool, NULL, NULL, FALSE, FALSE,

		 NULL);

	      transform_info_inited = TRUE;
	    }

	  /*  Recalculate the transform tool  */
	  gimp_transform_tool_recalc (tr_tool, gdisp);

	  /*  recall this function to find which handle we're dragging  */
          gimp_transform_tool_button_press (tool, coords, time, state, gdisp);
	}
    }
}

static void
gimp_transform_tool_button_release (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
			            GdkModifierType  state,
			            GimpDisplay     *gdisp)
{
  GimpTransformTool *tr_tool;
  gint               i;

  tr_tool = GIMP_TRANSFORM_TOOL (tool);

  /*  if we are creating, there is nothing to be done...exit  */
  if (tr_tool->function == TRANSFORM_CREATING)
    return;

  /*  release of the pointer grab  */
  gdk_pointer_ungrab (time);
  gdk_flush ();

  /*  if the 3rd button isn't pressed, transform the selected mask  */
  if (! (state & GDK_BUTTON3_MASK))
    {
      /* Shift-clicking is another way to approve the transform  */
      if ((state & GDK_SHIFT_MASK) || GIMP_IS_FLIP_TOOL (tool))
	{
	  gimp_transform_tool_doit (tr_tool, gdisp);
	}
      else
	{
	  /*  Only update the paths preview */
	  path_transform_current_path (gdisp->gimage,
				       tr_tool->transform, TRUE);
	}
    }
  else
    {
      /*  stop the current tool drawing process  */
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      /*  Restore the previous transformation info  */
      for (i = 0; i < TRAN_INFO_SIZE; i++)
	tr_tool->trans_info[i] = old_trans_info[i];

      /*  recalculate the tool's transformation matrix  */
      gimp_transform_tool_recalc (tr_tool, gdisp);

      /*  resume drawing the current tool  */
      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

      /* Update the paths preview */
      path_transform_current_path (gdisp->gimage,
				   tr_tool->transform, TRUE);
    }

  tool->state = INACTIVE;
}

static void
gimp_transform_tool_motion (GimpTool        *tool,
                            GimpCoords      *coords,
                            guint32          time,
		            GdkModifierType  state,
		            GimpDisplay     *gdisp)
{
  GimpTransformTool *transform_tool;

  transform_tool = GIMP_TRANSFORM_TOOL (tool);

  /*  if we are creating or this tool is non-interactive, there is
   *  nothing to be done so exit.
   */
  if (transform_tool->function == TRANSFORM_CREATING)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  transform_tool->curx  = coords->x;
  transform_tool->cury  = coords->y;
  transform_tool->state = state;

  /*  recalculate the tool's transformation matrix  */
  gimp_transform_tool_transform (transform_tool, gdisp, TRANSFORM_MOTION);

  transform_tool->lastx = transform_tool->curx;
  transform_tool->lasty = transform_tool->cury;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_transform_tool_cursor_update (GimpTool        *tool,
                                   GimpCoords      *coords,
			           GdkModifierType  state,
			           GimpDisplay     *gdisp)
{
  GimpDrawable  *drawable;
  GdkCursorType  ctype = GDK_TOP_LEFT_ARROW;

  if ((drawable = gimp_image_active_drawable (gdisp->gimage)))
    {
      gint off_x, off_y;

      gimp_drawable_offsets (drawable, &off_x, &off_y);

      if (GIMP_IS_LAYER (drawable) &&
          gimp_layer_get_mask (GIMP_LAYER (drawable)))
	{
	  ctype = GIMP_BAD_CURSOR;
	}
      else if (coords->x >= off_x &&
	       coords->y >= off_y &&
	       coords->x < (off_x + drawable->width) &&
	       coords->y < (off_y + drawable->height))
	{
	  if (gimage_mask_is_empty (gdisp->gimage) ||
	      gimage_mask_value (gdisp->gimage, coords->x, coords->y))
	    {
	      ctype = GIMP_MOUSE_CURSOR;
	    }
	}
    }

  gimp_display_shell_install_tool_cursor (GIMP_DISPLAY_SHELL (gdisp->shell),
                                          ctype,
                                          tool->tool_cursor,
                                          GIMP_CURSOR_MODIFIER_NONE);
}

static void
gimp_transform_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTransformTool *tr_tool;
  TransformOptions  *options;
  gint               i, k, gci;

  tr_tool = GIMP_TRANSFORM_TOOL (draw_tool);

  options = (TransformOptions *) GIMP_TOOL (draw_tool)->tool_info->tool_options;

  /*  draw the bounding box  */
  gimp_draw_tool_draw_line (draw_tool,
                            tr_tool->tx1, tr_tool->ty1,
                            tr_tool->tx2, tr_tool->ty2,
                            FALSE);
  gimp_draw_tool_draw_line (draw_tool,
                            tr_tool->tx2, tr_tool->ty2,
                            tr_tool->tx4, tr_tool->ty4,
                            FALSE);
  gimp_draw_tool_draw_line (draw_tool,
                            tr_tool->tx3, tr_tool->ty3,
                            tr_tool->tx4, tr_tool->ty4,
                            FALSE);
  gimp_draw_tool_draw_line (draw_tool,
                            tr_tool->tx3, tr_tool->ty3,
                            tr_tool->tx1, tr_tool->ty1,
                            FALSE);

  /*  Draw the grid */

  if ((tr_tool->grid_coords != NULL) &&
      (tr_tool->tgrid_coords != NULL) /* FIXME!!! this doesn't belong here &&
      ((tool->type != PERSPECTIVE)  ||
       ((tr_tool->transform[0][0] >=0.0) &&
	(tr_tool->transform[1][1] >=0.0)) */ ) 
    {
      gci = 0;
      k = tr_tool->ngx + tr_tool->ngy;

      for (i = 0; i < k; i++)
	{
          gimp_draw_tool_draw_line (draw_tool,
                                    tr_tool->tgrid_coords[gci],
                                    tr_tool->tgrid_coords[gci + 1],
                                    tr_tool->tgrid_coords[gci + 2],
                                    tr_tool->tgrid_coords[gci + 3],
                                    FALSE);
	  gci += 4;
	}
    }

  /*  draw the tool handles  */
  gimp_draw_tool_draw_handle (draw_tool,
                              GIMP_HANDLE_SQUARE,
                              tr_tool->tx1, tr_tool->ty1,
                              HANDLE, HANDLE,
                              GTK_ANCHOR_CENTER,
                              FALSE);
  gimp_draw_tool_draw_handle (draw_tool,
                              GIMP_HANDLE_SQUARE,
                              tr_tool->tx2, tr_tool->ty2,
                              HANDLE, HANDLE,
                              GTK_ANCHOR_CENTER,
                              FALSE);
  gimp_draw_tool_draw_handle (draw_tool,
                              GIMP_HANDLE_SQUARE,
                              tr_tool->tx3, tr_tool->ty3,
                              HANDLE, HANDLE,
                              GTK_ANCHOR_CENTER,
                              FALSE);
  gimp_draw_tool_draw_handle (draw_tool,
                              GIMP_HANDLE_SQUARE,
                              tr_tool->tx4, tr_tool->ty4,
                              HANDLE, HANDLE,
                              GTK_ANCHOR_CENTER,
                              FALSE);

  /*  draw the center  */
  gimp_draw_tool_draw_handle (draw_tool,
                              GIMP_HANDLE_FILLED_CIRCLE,
                              tr_tool->tcx, tr_tool->tcy,
                              HANDLE, HANDLE,
                              GTK_ANCHOR_CENTER,
                              FALSE);

  if (options->show_path)
    {
      GimpMatrix3 tmp_matrix;

      if (options->direction == GIMP_TRANSFORM_BACKWARD)
	{
	  gimp_matrix3_invert (tr_tool->transform, tmp_matrix);
	}
      else
	{
	  gimp_matrix3_duplicate (tr_tool->transform, tmp_matrix);
	}

      path_transform_draw_current (GIMP_TOOL (draw_tool)->gdisp,
                                   draw_tool, tmp_matrix);
    }
}

static TileManager *
gimp_transform_tool_transform (GimpTransformTool   *tool,
                               GimpDisplay         *gdisp,
			       TransformState       state)
{
  TileManager *retval;

  g_return_val_if_fail (GIMP_IS_TRANSFORM_TOOL (tool), NULL);

  g_signal_emit (G_OBJECT (tool), gimp_transform_tool_signals[TRANSFORM], 0,
                 gdisp, state, &retval);

  return retval;
}

static void
gimp_transform_tool_doit (GimpTransformTool  *tr_tool,
		          GimpDisplay        *gdisp)
{
  GimpDisplayShell *shell;
  GimpTool         *tool;
  TileManager      *new_tiles;
  TransformUndo    *tu;
  PathUndo         *pundo;
  gboolean          new_layer;
  gint              i;

  gimp_set_busy (gdisp->gimage->gimp);

  tool = GIMP_TOOL (tr_tool);

  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  /* undraw the tool before we muck around with the transform matrix */
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

  /*  We're going to dirty this image, but we want to keep the tool
   *  around
   */
  tool->preserve = TRUE;

  /*  Start a transform undo group  */
  undo_push_group_start (gdisp->gimage, TRANSFORM_CORE_UNDO);

  /* With the old UI, if original is NULL, then this is the
   * first transformation. In the new UI, it is always so, right?
   */
  g_assert (tr_tool->original == NULL);

  /*  Copy the current selection to the transform tool's private
   *  selection pointer, so that the original source can be repeatedly
   *  modified.
   */
  tool->drawable = gimp_image_active_drawable (gdisp->gimage);

  tr_tool->original = gimp_drawable_transform_cut (tool->drawable,
                                                   &new_layer);

  pundo = path_transform_start_undo (gdisp->gimage);

  /*  Send the request for the transformation to the tool...
   */
  new_tiles = gimp_transform_tool_transform (tr_tool, gdisp, TRANSFORM_FINISH);

  gimp_transform_tool_transform (tr_tool, gdisp, TRANSFORM_INIT);

  gimp_transform_tool_recalc (tr_tool, gdisp);

  if (new_tiles)
    {
      /*  paste the new transformed image to the gimage...also implement
       *  undo...
       */
      /*  FIXME: we should check if the drawable is still valid  */
      gimp_drawable_transform_paste (tool->drawable,
                                     new_tiles,
                                     new_layer);

      /*  create and initialize the transform_undo structure  */
      tu = g_new0 (TransformUndo, 1);
      tu->tool_ID   = tool->ID;
      tu->tool_type = G_TYPE_FROM_INSTANCE (tool);

      for (i = 0; i < TRAN_INFO_SIZE; i++)
	tu->trans_info[i] = old_trans_info[i];

      tu->original  = NULL;
      tu->path_undo = pundo;

      /*  Make a note of the new current drawable (since we may have
       *  a floating selection, etc now.
       */
      tool->drawable = gimp_image_active_drawable (gdisp->gimage);

      undo_push_transform (gdisp->gimage, tu);
    }

  /*  push the undo group end  */
  undo_push_group_end (gdisp->gimage);

  /*  We're done dirtying the image, and would like to be restarted
   *  if the image gets dirty while the tool exists
   */
  tool->preserve = FALSE;

  gimp_unset_busy (gdisp->gimage->gimp);

  gdisplays_flush ();

  gimp_transform_tool_reset (tr_tool, gdisp);

  tool->state = INACTIVE;
}

TileManager *
gimp_transform_tool_transform_tiles (GimpTransformTool *transform_tool,
                                     const gchar       *progress_text)
{
  GimpTool         *tool;
  TransformOptions *options;
  GimpProgress     *progress;
  TileManager      *ret;

  g_return_val_if_fail (GIMP_IS_TRANSFORM_TOOL (transform_tool), NULL);
  g_return_val_if_fail (progress_text != NULL, NULL);

  gtk_widget_set_sensitive (GTK_WIDGET (transform_info->shell), FALSE);

  tool = GIMP_TOOL (transform_tool);

  options = (TransformOptions *) tool->tool_info->tool_options;

  progress = progress_start (tool->gdisp, progress_text, FALSE, NULL, NULL);

  ret = gimp_drawable_transform_tiles_affine (gimp_image_active_drawable (tool->gdisp->gimage),
                                              transform_tool->original,
                                              options->smoothing,
                                              options->clip,
                                              transform_tool->transform,
                                              options->direction,
                                              progress ? progress_update_and_flush :
                                              (GimpProgressFunc) NULL,
                                              progress);

  if (progress)
    progress_end (progress);

  return ret;
}

void
gimp_transform_tool_transform_bounding_box (GimpTransformTool *tr_tool)
{
  GimpTool  *tool;
  gint       i, k;
  gint       gci;

  tool = GIMP_TOOL (tr_tool);

  gimp_matrix3_transform_point (tr_tool->transform,
				tr_tool->x1, tr_tool->y1,
				&tr_tool->tx1, &tr_tool->ty1);
  gimp_matrix3_transform_point (tr_tool->transform,
				tr_tool->x2, tr_tool->y1,
				&tr_tool->tx2, &tr_tool->ty2);
  gimp_matrix3_transform_point (tr_tool->transform,
				tr_tool->x1, tr_tool->y2,
				&tr_tool->tx3, &tr_tool->ty3);
  gimp_matrix3_transform_point (tr_tool->transform,
				tr_tool->x2, tr_tool->y2,
				&tr_tool->tx4, &tr_tool->ty4);

  gimp_matrix3_transform_point (tr_tool->transform,
				tr_tool->cx, tr_tool->cy,
				&tr_tool->tcx, &tr_tool->tcy);

  if (tr_tool->grid_coords != NULL &&
      tr_tool->tgrid_coords != NULL)
    {
      gci = 0;
      k  = (tr_tool->ngx + tr_tool->ngy) * 2;

      for (i = 0; i < k; i++)
	{
	  gimp_matrix3_transform_point (tr_tool->transform,
					tr_tool->grid_coords[gci],
					tr_tool->grid_coords[gci+1],
					&(tr_tool->tgrid_coords[gci]),
					&(tr_tool->tgrid_coords[gci+1]));
	  gci += 2;
	}
    }
}

void
gimp_transform_tool_reset (GimpTransformTool *tr_tool,
		           GimpDisplay       *gdisp)
{
  GimpTool *tool;

  tool = GIMP_TOOL (tr_tool);

  if (tr_tool->original)
    {
      tile_manager_destroy (tr_tool->original);
      tr_tool->original = NULL;
    }

  /*  inactivate the tool  */
  tr_tool->function = TRANSFORM_CREATING;
  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tr_tool));
  info_dialog_popdown (transform_info);

  tool->state    = INACTIVE;
  tool->gdisp    = NULL;
  tool->drawable = NULL;
}

static void
gimp_transform_tool_bounds (GimpTransformTool *tr_tool,
		            GimpDisplay       *gdisp)
{
  TileManager  *tiles;
  GimpDrawable *drawable;
  gint          offset_x, offset_y;

  tiles    = tr_tool->original;
  drawable = gimp_image_active_drawable (gdisp->gimage);

  /*  find the boundaries  */
  if (tiles)
    {
      tile_manager_get_offsets (tiles,
				&tr_tool->x1, &tr_tool->y1);
				
      tr_tool->x2 = tr_tool->x1 + tile_manager_width (tiles);
      tr_tool->y2 = tr_tool->y1 + tile_manager_height (tiles);
    }
  else
    {
      gimp_drawable_offsets (drawable, &offset_x, &offset_y);
      gimp_drawable_mask_bounds (drawable,
				 &tr_tool->x1, &tr_tool->y1,
				 &tr_tool->x2, &tr_tool->y2);
      tr_tool->x1 += offset_x;
      tr_tool->y1 += offset_y;
      tr_tool->x2 += offset_x;
      tr_tool->y2 += offset_y;
    }

  tr_tool->cx = (tr_tool->x1 + tr_tool->x2) / 2;
  tr_tool->cy = (tr_tool->y1 + tr_tool->y2) / 2;

  if (tr_tool->use_grid)
    {
      /*  changing the bounds invalidates any grid we may have  */
      gimp_transform_tool_grid_recalc (tr_tool);
    }
}

void
gimp_transform_tool_grid_density_changed (GimpTransformTool *transform_tool)
{
  if (transform_tool->function == TRANSFORM_CREATING)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (transform_tool));

  gimp_transform_tool_grid_recalc (transform_tool);
  gimp_transform_tool_transform_bounding_box (transform_tool);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (transform_tool));
}

void
gimp_transform_tool_show_path_changed (GimpTransformTool *transform_tool,
                                       gint               type /* a truly undescriptive name */)
{
  if (transform_tool->function == TRANSFORM_CREATING)
    return;

  if (type)
    gimp_draw_tool_pause (GIMP_DRAW_TOOL (transform_tool));
  else
    gimp_draw_tool_resume (GIMP_DRAW_TOOL (transform_tool));
}

static void
gimp_transform_tool_grid_recalc (GimpTransformTool *tr_tool)
{
  TransformOptions *options;

  options = (TransformOptions *) GIMP_TOOL (tr_tool)->tool_info->tool_options;

  if (tr_tool->grid_coords != NULL)
    {
      g_free (tr_tool->grid_coords);
      tr_tool->grid_coords = NULL;
    }

  if (tr_tool->tgrid_coords != NULL)
    {
      g_free (tr_tool->tgrid_coords);
      tr_tool->tgrid_coords = NULL;
    }

  if (options->show_grid)
    {
      gimp_transform_tool_setup_grid (tr_tool, options);
    }
}

static void
gimp_transform_tool_setup_grid (GimpTransformTool *tr_tool,
                                TransformOptions  *options)
{
  GimpTool *tool;
  gint      i, gci;
  gdouble  *coords;

  tool = GIMP_TOOL (tr_tool);

  /*  We use the gimp_transform_tool_grid_size function only here, even
   *  if the user changes the grid size in the middle of an
   *  operation, nothing happens.
   */
  tr_tool->ngx = (tr_tool->x2 - tr_tool->x1) / options->grid_size;
  if (tr_tool->ngx > 0)
    tr_tool->ngx--;

  tr_tool->ngy = (tr_tool->y2 - tr_tool->y1) / options->grid_size;
  if (tr_tool->ngy > 0)
    tr_tool->ngy--;

  tr_tool->grid_coords = coords =
    g_new (gdouble, (tr_tool->ngx + tr_tool->ngy) * 4);

  tr_tool->tgrid_coords =
    g_new (gdouble, (tr_tool->ngx + tr_tool->ngy) * 4);

  gci = 0;

  for (i = 1; i <= tr_tool->ngx; i++)
    {
      coords[gci] = tr_tool->x1 +
	((gdouble) i) / (tr_tool->ngx + 1) *
	(tr_tool->x2 - tr_tool->x1);
      coords[gci+1] = tr_tool->y1;
      coords[gci+2] = coords[gci];
      coords[gci+3] = tr_tool->y2;
      gci += 4;
    }

  for (i = 1; i <= tr_tool->ngy; i++)
    {
      coords[gci] = tr_tool->x1;
      coords[gci+1] = tr_tool->y1 +
	((gdouble) i) / (tr_tool->ngy + 1) *
	(tr_tool->y2 - tr_tool->y1);
      coords[gci+2] = tr_tool->x2;
      coords[gci+3] = coords[gci+1];
      gci += 4;
    }
}

static void
gimp_transform_tool_recalc (GimpTransformTool *tr_tool,
		            GimpDisplay       *gdisp)
{
  gimp_transform_tool_bounds (tr_tool, gdisp);

  gimp_transform_tool_transform (tr_tool, gdisp, TRANSFORM_RECALC);
}

static void
transform_ok_callback (GtkWidget *widget,
		       gpointer   data)
{
  GimpTool *tool;

  tool = GIMP_TOOL(data);
  gimp_transform_tool_doit (GIMP_TRANSFORM_TOOL(tool), tool->gdisp);
}

static void
transform_reset_callback (GtkWidget *widget,
			  gpointer   data)
{
  GimpTransformTool *transform_tool;
  GimpTool          *tool;
  gint               i;

  transform_tool = GIMP_TRANSFORM_TOOL (data);
  tool           = GIMP_TOOL (data);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /*  Restore the previous transformation info  */
  for (i = 0; i < TRAN_INFO_SIZE; i++)
    transform_tool->trans_info [i] = old_trans_info [i];

  /*  recalculate the tool's transformation matrix  */
  gimp_transform_tool_recalc (transform_tool, tool->gdisp);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}
