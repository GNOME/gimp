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

#ifdef __GNUC__
#warning FIXME #include "gui/gui-types.h"
#endif
#include "gui/gui-types.h"

#include "base/tile-manager.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-transform.h"
#include "core/gimpimage.h"
#include "core/gimpimage-mask.h"
#include "core/gimplayer.h"
#include "core/gimpmarshal.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpviewabledialog.h"

#include "gui/info-dialog.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpprogress.h"

#include "gimptransformtool.h"
#include "gimptransformoptions.h"
#include "tool_manager.h"

#include "undo.h"
#include "path_transform.h"

#include "libgimp/gimpintl.h"


#define HANDLE_SIZE 10


/*  local function prototypes  */

static void   gimp_transform_tool_init        (GimpTransformTool      *tool);
static void   gimp_transform_tool_class_init  (GimpTransformToolClass *tool);

static void   gimp_transform_tool_finalize         (GObject           *object);

static void   gimp_transform_tool_control          (GimpTool          *tool,
                                                    GimpToolAction     action,
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
static void   gimp_transform_tool_modifier_key     (GimpTool          *tool,
                                                    GdkModifierType    key,
                                                    gboolean           press,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *gdisp);
static void   gimp_transform_tool_oper_update      (GimpTool          *tool,
                                                    GimpCoords        *coords,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *gdisp);
static void   gimp_transform_tool_cursor_update    (GimpTool          *tool,
                                                    GimpCoords        *coords,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *gdisp);

static void   gimp_transform_tool_draw             (GimpDrawTool      *draw_tool);

static TileManager *
              gimp_transform_tool_real_transform   (GimpTransformTool *tr_tool,
                                                    GimpDisplay       *gdisp);

static void   gimp_transform_tool_reset            (GimpTransformTool *tr_tool);
static void   gimp_transform_tool_bounds           (GimpTransformTool *tr_tool,
                                                    GimpDisplay       *gdisp);
static void   gimp_transform_tool_dialog           (GimpTransformTool *tr_tool);
static void   gimp_transform_tool_prepare          (GimpTransformTool *tr_tool,
                                                    GimpDisplay       *gdisp);
static void   gimp_transform_tool_recalc           (GimpTransformTool *tr_tool,
                                                    GimpDisplay       *gdisp);
static void   gimp_transform_tool_doit             (GimpTransformTool *tr_tool,
                                                    GimpDisplay       *gdisp);
static void   gimp_transform_tool_grid_recalc      (GimpTransformTool *tr_tool);

static void   transform_reset_callback             (GtkWidget         *widget,
                                                    GimpTransformTool *tr_tool);
static void   transform_cancel_callback            (GtkWidget         *widget,
                                                    GimpTransformTool *tr_tool);
static void   transform_ok_callback                (GtkWidget         *widget,
                                                    GimpTransformTool *tr_tool);

static void   gimp_transform_tool_notify_grid   (GimpTransformOptions *options,
                                                 GParamSpec           *pspec,
                                                 GimpTransformTool    *tr_tool);
static void   gimp_transform_tool_notify_path   (GimpTransformOptions *options,
                                                 GParamSpec           *pspec,
                                                 GimpTransformTool    *tr_tool);


static GimpDrawToolClass *parent_class = NULL;


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

  object_class->finalize     = gimp_transform_tool_finalize;

  tool_class->control        = gimp_transform_tool_control;
  tool_class->button_press   = gimp_transform_tool_button_press;
  tool_class->button_release = gimp_transform_tool_button_release;
  tool_class->motion         = gimp_transform_tool_motion;
  tool_class->modifier_key   = gimp_transform_tool_modifier_key;
  tool_class->oper_update    = gimp_transform_tool_oper_update;
  tool_class->cursor_update  = gimp_transform_tool_cursor_update;

  draw_class->draw           = gimp_transform_tool_draw;

  klass->dialog              = NULL;
  klass->prepare             = NULL;
  klass->motion              = NULL;
  klass->recalc              = NULL;
  klass->transform           = gimp_transform_tool_real_transform;
}

static void
gimp_transform_tool_init (GimpTransformTool *tr_tool)
{
  GimpTool *tool;
  gint      i;

  tool = GIMP_TOOL (tr_tool);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);

  tr_tool->function = TRANSFORM_CREATING;
  tr_tool->original = NULL;

  for (i = 0; i < TRAN_INFO_SIZE; i++)
    {
      tr_tool->trans_info[i]     = 0.0;
      tr_tool->old_trans_info[i] = 0.0;
    }

  gimp_matrix3_identity (tr_tool->transform);

  tr_tool->use_grid         = TRUE;
  tr_tool->use_center       = TRUE;
  tr_tool->ngx              = 0;
  tr_tool->ngy              = 0;
  tr_tool->grid_coords      = NULL;
  tr_tool->tgrid_coords     = NULL;

  tr_tool->notify_connected = FALSE;
  tr_tool->show_path        = FALSE;

  tr_tool->shell_desc       = NULL;
  tr_tool->progress_text    = _("Transforming...");
  tr_tool->info_dialog      = NULL;
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

  if (tr_tool->info_dialog)
    {
      info_dialog_free (tr_tool->info_dialog);
      tr_tool->info_dialog = NULL;
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
gimp_transform_tool_control (GimpTool       *tool,
			     GimpToolAction  action,
			     GimpDisplay    *gdisp)
{
  GimpTransformTool *tr_tool;

  tr_tool = GIMP_TRANSFORM_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      gimp_transform_tool_recalc (tr_tool, gdisp);
      break;

    case HALT:
      gimp_transform_tool_reset (tr_tool);
      return; /* don't upchain */
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
  GimpDrawable      *drawable;
  gint               off_x, off_y;

  tr_tool   = GIMP_TRANSFORM_TOOL (tool);
  draw_tool = GIMP_DRAW_TOOL (tool);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (! tr_tool->notify_connected)
    {
      tr_tool->show_path =
        GIMP_TRANSFORM_OPTIONS (tool->tool_info->tool_options)->show_path;

      g_signal_connect_object (tool->tool_info->tool_options,
                               "notify::grid-type",
                               G_CALLBACK (gimp_transform_tool_notify_grid),
                               tr_tool, 0);
      g_signal_connect_object (tool->tool_info->tool_options,
                               "notify::grid-size",
                               G_CALLBACK (gimp_transform_tool_notify_grid),
                               tr_tool, 0);
      g_signal_connect_object (tool->tool_info->tool_options,
                               "notify::show-path",
                               G_CALLBACK (gimp_transform_tool_notify_path),
                               tr_tool, 0);

      tr_tool->notify_connected = TRUE;
    }

  if (gdisp != tool->gdisp)
    {
      /*  Initialisation stuff: if the cursor is clicked inside the current
       *  selection, show the bounding box and handles...
       */
      gimp_drawable_offsets (drawable, &off_x, &off_y);

      if (coords->x >= off_x &&
          coords->y >= off_y &&
          coords->x < (off_x + gimp_drawable_width (drawable))  &&
          coords->y < (off_y + gimp_drawable_height (drawable)) &&

          (gimp_image_mask_is_empty (gdisp->gimage) ||
           gimp_image_mask_value (gdisp->gimage, coords->x, coords->y)))
        {
          if (GIMP_IS_LAYER (drawable) &&
              gimp_layer_get_mask (GIMP_LAYER (drawable)))
            {
              g_message (_("Transformations do not work on\n"
                           "layers that contain layer masks."));
              gimp_tool_control_halt (tool->control);
              return;
            }

          /*  If the tool is already active, clear the current state
           *  and reset
           */
          if (gimp_tool_control_is_active (tool->control))
            {
              g_warning ("%s: tool_already ACTIVE", G_GNUC_FUNCTION);

              gimp_transform_tool_reset (tr_tool);
            }

          /*  Set the pointer to the active display  */
          tool->gdisp    = gdisp;
          tool->drawable = drawable;
          gimp_tool_control_activate (tool->control);

          /*  Find the transform bounds for some tools (like scale,
           *  perspective) that actually need the bounds for
           *  initializing
           */
          gimp_transform_tool_bounds (tr_tool, gdisp);

          /*  Initialize the transform tool */
          if (! tr_tool->info_dialog)
            gimp_transform_tool_dialog (tr_tool);

          gimp_transform_tool_prepare (tr_tool, gdisp);

          /*  Recalculate the transform tool  */
          gimp_transform_tool_recalc (tr_tool, gdisp);

          /*  start drawing the bounding box and handles...  */
          gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);

          /*  find which handle we're dragging  */
          gimp_transform_tool_oper_update (tool, coords, state, gdisp);

          tr_tool->function = TRANSFORM_CREATING;
        }
    }

  if (tr_tool->function == TRANSFORM_CREATING &&
      gimp_tool_control_is_active (tool->control))
    {
      gint i;

      /*  Save the current transformation info  */
      for (i = 0; i < TRAN_INFO_SIZE; i++)
	tr_tool->old_trans_info[i] = tr_tool->trans_info[i];
    }

  /*  if we have already displayed the bounding box and handles,
   *  check to make sure that the display which currently owns the
   *  tool is the one which just received the button pressed event
   */
  if (gdisp == tool->gdisp)
    {
      /*  Save the current pointer position  */
      tr_tool->lastx = tr_tool->startx = coords->x;
      tr_tool->lasty = tr_tool->starty = coords->y;

      gimp_tool_control_activate (tool->control);
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
  if (tr_tool->function == TRANSFORM_CREATING && tr_tool->use_grid)
    return;

  /*  if the 3rd button isn't pressed, transform the selected mask  */
  if (! (state & GDK_BUTTON3_MASK))
    {
      /* Shift-clicking is another way to approve the transform  */
      if ((state & GDK_SHIFT_MASK) || ! tr_tool->use_grid)
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
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      /*  Restore the previous transformation info  */
      for (i = 0; i < TRAN_INFO_SIZE; i++)
	tr_tool->trans_info[i] = tr_tool->old_trans_info[i];

      /*  recalculate the tool's transformation matrix  */
      gimp_transform_tool_recalc (tr_tool, gdisp);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

      /* Update the paths preview */
      path_transform_current_path (gdisp->gimage,
				   tr_tool->transform, TRUE);
    }
}

static void
gimp_transform_tool_motion (GimpTool        *tool,
                            GimpCoords      *coords,
                            guint32          time,
		            GdkModifierType  state,
		            GimpDisplay     *gdisp)
{
  GimpTransformToolClass *tr_tool_class;
  GimpTransformTool      *tr_tool;

  tr_tool = GIMP_TRANSFORM_TOOL (tool);

  /*  if we are creating, there is nothing to be done so exit.  */
  if (tr_tool->function == TRANSFORM_CREATING || ! tr_tool->use_grid)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  tr_tool->curx  = coords->x;
  tr_tool->cury  = coords->y;
  tr_tool->state = state;

  /*  recalculate the tool's transformation matrix  */
  tr_tool_class = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool);

  if (tr_tool_class->motion)
    {
      tr_tool_class->motion (tr_tool, gdisp);

      if (tr_tool_class->recalc)
        tr_tool_class->recalc (tr_tool, gdisp);
    }

  tr_tool->lastx = tr_tool->curx;
  tr_tool->lasty = tr_tool->cury;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_transform_tool_modifier_key (GimpTool        *tool,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state,
                                  GimpDisplay     *gdisp)
{
  GimpTransformOptions *options;

  options = GIMP_TRANSFORM_OPTIONS (tool->tool_info->tool_options);

  if (key == GDK_CONTROL_MASK)
    {
      g_object_set (G_OBJECT (options),
                    "constrain-1", ! options->constrain_1,
                    NULL);
    }
  else if (key == GDK_MOD1_MASK)
    {
      g_object_set (G_OBJECT (options),
                    "constrain-2", ! options->constrain_2,
                    NULL);
    }
}

static void
gimp_transform_tool_oper_update (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 GdkModifierType  state,
                                 GimpDisplay     *gdisp)
{
  GimpTransformTool *tr_tool;
  GimpDrawTool      *draw_tool;

  tr_tool   = GIMP_TRANSFORM_TOOL (tool);
  draw_tool = GIMP_DRAW_TOOL (tool);

  if (! tr_tool->use_grid)
    return;

  if (gdisp == tool->gdisp)
    {
      gdouble closest_dist;
      gdouble dist;

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
                                    HANDLE_SIZE, HANDLE_SIZE,
                                    GTK_ANCHOR_CENTER,
                                    FALSE))
	{
	  tr_tool->function = TRANSFORM_HANDLE_CENTER;
	}
    }
}

static void
gimp_transform_tool_cursor_update (GimpTool        *tool,
                                   GimpCoords      *coords,
			           GdkModifierType  state,
			           GimpDisplay     *gdisp)
{
  GimpTransformTool  *tr_tool;
  GimpDrawable       *drawable;

  tr_tool = GIMP_TRANSFORM_TOOL (tool);

  if (tr_tool->use_grid)
    {
      GdkCursorType      ctype     = GDK_TOP_LEFT_ARROW;
      GimpCursorModifier cmodifier = GIMP_CURSOR_MODIFIER_NONE;

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
              if (gimp_image_mask_is_empty (gdisp->gimage) ||
                  gimp_image_mask_value (gdisp->gimage, coords->x, coords->y))
                {
                  ctype = GIMP_MOUSE_CURSOR;
                }
            }
        }

      if (tr_tool->use_center && tr_tool->function == TRANSFORM_HANDLE_CENTER)
        {
          cmodifier = GIMP_CURSOR_MODIFIER_MOVE;
        }

      gimp_tool_control_set_cursor          (tool->control, ctype);
      gimp_tool_control_set_cursor_modifier (tool->control, cmodifier);
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_transform_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTransformTool    *tr_tool;
  GimpTransformOptions *options;
  gint                  i, k, gci;

  tr_tool = GIMP_TRANSFORM_TOOL (draw_tool);
  options = GIMP_TRANSFORM_OPTIONS (GIMP_TOOL (draw_tool)->tool_info->tool_options);

  if (! tr_tool->use_grid)
    return;

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

  if ((tr_tool->grid_coords != NULL) && (tr_tool->tgrid_coords != NULL))
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
                              HANDLE_SIZE, HANDLE_SIZE,
                              GTK_ANCHOR_CENTER,
                              FALSE);
  gimp_draw_tool_draw_handle (draw_tool,
                              GIMP_HANDLE_SQUARE,
                              tr_tool->tx2, tr_tool->ty2,
                              HANDLE_SIZE, HANDLE_SIZE,
                              GTK_ANCHOR_CENTER,
                              FALSE);
  gimp_draw_tool_draw_handle (draw_tool,
                              GIMP_HANDLE_SQUARE,
                              tr_tool->tx3, tr_tool->ty3,
                              HANDLE_SIZE, HANDLE_SIZE,
                              GTK_ANCHOR_CENTER,
                              FALSE);
  gimp_draw_tool_draw_handle (draw_tool,
                              GIMP_HANDLE_SQUARE,
                              tr_tool->tx4, tr_tool->ty4,
                              HANDLE_SIZE, HANDLE_SIZE,
                              GTK_ANCHOR_CENTER,
                              FALSE);

  /*  draw the center  */
  if (tr_tool->use_center)
    {
      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_FILLED_CIRCLE,
                                  tr_tool->tcx, tr_tool->tcy,
                                  HANDLE_SIZE, HANDLE_SIZE,
                                  GTK_ANCHOR_CENTER,
                                  FALSE);
    }

  if (tr_tool->show_path)
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
gimp_transform_tool_real_transform (GimpTransformTool *tr_tool,
                                    GimpDisplay       *gdisp)
{
  GimpTool             *tool;
  GimpDrawable         *drawable;
  GimpTransformOptions *options;
  GimpProgress         *progress;
  TileManager          *ret;

  tool    = GIMP_TOOL (tr_tool);
  options = GIMP_TRANSFORM_OPTIONS (tool->tool_info->tool_options);

  if (tr_tool->info_dialog)
    gtk_widget_set_sensitive (GTK_WIDGET (tr_tool->info_dialog->shell), FALSE);

  progress = gimp_progress_start (gdisp, tr_tool->progress_text, FALSE,
                                  NULL, NULL);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  ret = gimp_drawable_transform_tiles_affine (drawable,
                                              tr_tool->original,
                                              options->interpolation,
                                              options->clip,
                                              tr_tool->transform,
                                              options->direction,
                                              progress ?
                                              gimp_progress_update_and_flush : 
                                              NULL,
                                              progress);

  if (progress)
    gimp_progress_end (progress);

  return ret;
}

static void
gimp_transform_tool_doit (GimpTransformTool  *tr_tool,
		          GimpDisplay        *gdisp)
{
  GimpTool    *tool;
  TileManager *new_tiles;
  GSList      *path_undo;
  gboolean     new_layer;

  gimp_set_busy (gdisp->gimage->gimp);

  tool = GIMP_TOOL (tr_tool);

  /* undraw the tool before we muck around with the transform matrix */
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

  /*  We're going to dirty this image, but we want to keep the tool
   *  around
   */
  gimp_tool_control_set_preserve (tool->control, TRUE);

  /*  Start a transform undo group  */
  undo_push_group_start (gdisp->gimage, TRANSFORM_UNDO_GROUP);

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

  path_undo = path_transform_start_undo (gdisp->gimage);

  /*  Send the request for the transformation to the tool...
   */
  new_tiles = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->transform (tr_tool,
                                                                  gdisp);

  gimp_transform_tool_prepare (tr_tool, gdisp);
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

      /*  Make a note of the new current drawable (since we may have
       *  a floating selection, etc now.
       */
      tool->drawable = gimp_image_active_drawable (gdisp->gimage);

      undo_push_transform (gdisp->gimage,
                           tool->ID,
                           G_TYPE_FROM_INSTANCE (tool),
                           tr_tool->old_trans_info,
                           NULL,
                           path_undo);
    }

  /*  push the undo group end  */
  undo_push_group_end (gdisp->gimage);

  /*  We're done dirtying the image, and would like to be restarted
   *  if the image gets dirty while the tool exists
   */
  gimp_tool_control_set_preserve (tool->control, FALSE);

  gimp_unset_busy (gdisp->gimage->gimp);

  gimp_image_flush (gdisp->gimage);

  gimp_transform_tool_reset (tr_tool);
}

void
gimp_transform_tool_transform_bounding_box (GimpTransformTool *tr_tool)
{
  g_return_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool));

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

  if (tr_tool->grid_coords != NULL && tr_tool->tgrid_coords != NULL)
    {
      gint i, k;
      gint gci;

      gci = 0;
      k   = (tr_tool->ngx + tr_tool->ngy) * 2;

      for (i = 0; i < k; i++)
	{
	  gimp_matrix3_transform_point (tr_tool->transform,
					tr_tool->grid_coords[gci],
					tr_tool->grid_coords[gci + 1],
					&tr_tool->tgrid_coords[gci],
					&tr_tool->tgrid_coords[gci + 1]);
	  gci += 2;
	}
    }
}

static void
gimp_transform_tool_reset (GimpTransformTool *tr_tool)
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

  if (tr_tool->info_dialog)
    info_dialog_popdown (tr_tool->info_dialog);

  gimp_tool_control_halt (tool->control);
  tool->gdisp    = NULL;
  tool->drawable = NULL;
}

static void
gimp_transform_tool_bounds (GimpTransformTool *tr_tool,
		            GimpDisplay       *gdisp)
{
  TileManager  *tiles;
  GimpDrawable *drawable;

  tiles    = tr_tool->original;
  drawable = gimp_image_active_drawable (gdisp->gimage);

  /*  find the boundaries  */
  if (tiles)
    {
      tile_manager_get_offsets (tiles, &tr_tool->x1, &tr_tool->y1);

      tr_tool->x2 = tr_tool->x1 + tile_manager_width (tiles);
      tr_tool->y2 = tr_tool->y1 + tile_manager_height (tiles);
    }
  else
    {
      gint offset_x, offset_y;

      gimp_drawable_offsets (drawable, &offset_x, &offset_y);

      gimp_drawable_mask_bounds (drawable,
				 &tr_tool->x1, &tr_tool->y1,
				 &tr_tool->x2, &tr_tool->y2);
      tr_tool->x1 += offset_x;
      tr_tool->y1 += offset_y;
      tr_tool->x2 += offset_x;
      tr_tool->y2 += offset_y;
    }

  tr_tool->cx = (gdouble) (tr_tool->x1 + tr_tool->x2) / 2.0;
  tr_tool->cy = (gdouble) (tr_tool->y1 + tr_tool->y2) / 2.0;

  if (tr_tool->use_grid)
    {
      /*  changing the bounds invalidates any grid we may have  */
      gimp_transform_tool_grid_recalc (tr_tool);
    }
}

static void
gimp_transform_tool_grid_recalc (GimpTransformTool *tr_tool)
{
  GimpTransformOptions *options;

  options = GIMP_TRANSFORM_OPTIONS (GIMP_TOOL (tr_tool)->tool_info->tool_options);

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

  switch (options->grid_type)
    {
    case GIMP_TRANSFORM_GRID_TYPE_N_LINES:
    case GIMP_TRANSFORM_GRID_TYPE_SPACING:
      {
        GimpTool *tool;
        gint      i, gci;
        gdouble  *coords;
        gint      width, height;

        width  = MAX (1, tr_tool->x2 - tr_tool->x1);
        height = MAX (1, tr_tool->y2 - tr_tool->y1);

        tool = GIMP_TOOL (tr_tool);

        if (options->grid_type == GIMP_TRANSFORM_GRID_TYPE_N_LINES)
          {
            if (width <= height)
              {
                tr_tool->ngx = options->grid_size;
                tr_tool->ngy = tr_tool->ngx * MAX (1, height / width);
              }
            else
              {
                tr_tool->ngy = options->grid_size;
                tr_tool->ngx = tr_tool->ngy * MAX (1, width / height);
              }
          }
        else /* GIMP_TRANSFORM_GRID_TYPE_SPACING */
          {
            gint grid_size = MAX (2, options->grid_size);

            tr_tool->ngx = width  / grid_size;
            tr_tool->ngy = height / grid_size;
          }

        tr_tool->grid_coords = coords =
          g_new (gdouble, (tr_tool->ngx + tr_tool->ngy) * 4);

        tr_tool->tgrid_coords =
          g_new (gdouble, (tr_tool->ngx + tr_tool->ngy) * 4);

        gci = 0;

        for (i = 1; i <= tr_tool->ngx; i++)
          {
            coords[gci]     = tr_tool->x1 + (((gdouble) i) / (tr_tool->ngx + 1) *
                                             (tr_tool->x2 - tr_tool->x1));
            coords[gci + 1] = tr_tool->y1;
            coords[gci + 2] = coords[gci];
            coords[gci + 3] = tr_tool->y2;

            gci += 4;
          }

        for (i = 1; i <= tr_tool->ngy; i++)
          {
            coords[gci]     = tr_tool->x1;
            coords[gci + 1] = tr_tool->y1 + (((gdouble) i) / (tr_tool->ngy + 1) *
                                             (tr_tool->y2 - tr_tool->y1));
            coords[gci + 2] = tr_tool->x2;
            coords[gci + 3] = coords[gci + 1];

            gci += 4;
          }
      }

    default:
      break;
    }
}

static void
gimp_transform_tool_dialog (GimpTransformTool *tr_tool)
{
  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->dialog)
    {
      GimpToolInfo *tool_info;

      tool_info = GIMP_TOOL (tr_tool)->tool_info;

      tr_tool->info_dialog =
        info_dialog_new (NULL,
                         tool_info->blurb,
                         GIMP_OBJECT (tool_info)->name,
                         tool_info->stock_id,
                         tr_tool->shell_desc,
                         tool_manager_help_func, NULL);

      gimp_dialog_create_action_area (GIMP_DIALOG (tr_tool->info_dialog->shell),

                                      GIMP_STOCK_RESET,
                                      transform_reset_callback,
                                      tr_tool, NULL, NULL, FALSE, FALSE,

                                      GTK_STOCK_CANCEL,
                                      transform_cancel_callback,
                                      tr_tool, NULL, NULL, FALSE, TRUE,

                                      tool_info->stock_id,
                                      transform_ok_callback,
                                      tr_tool, NULL, NULL, TRUE, FALSE,

                                      NULL);

      GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->dialog (tr_tool);
    }
}

static void
gimp_transform_tool_prepare (GimpTransformTool *tr_tool,
                             GimpDisplay       *gdisp)
{
  if (tr_tool->info_dialog)
    {
      gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (tr_tool->info_dialog->shell),
                                         GIMP_VIEWABLE (gimp_image_active_drawable (gdisp->gimage)));

      gtk_widget_set_sensitive (GTK_WIDGET (tr_tool->info_dialog->shell), TRUE);
    }

  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->prepare)
    GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->prepare (tr_tool, gdisp);
}

static void
gimp_transform_tool_recalc (GimpTransformTool *tr_tool,
		            GimpDisplay       *gdisp)
{
  gimp_transform_tool_bounds (tr_tool, gdisp);

  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc)
    GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc (tr_tool, gdisp);
}

static void
transform_reset_callback (GtkWidget         *widget,
			  GimpTransformTool *tr_tool)
{
  GimpTool *tool;
  gint      i;

  tool = GIMP_TOOL (tr_tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /*  Restore the previous transformation info  */
  for (i = 0; i < TRAN_INFO_SIZE; i++)
    tr_tool->trans_info[i] = tr_tool->old_trans_info[i];

  /*  recalculate the tool's transformation matrix  */
  gimp_transform_tool_recalc (tr_tool, tool->gdisp);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
transform_cancel_callback (GtkWidget         *widget,
                           GimpTransformTool *tr_tool)
{
  gimp_transform_tool_reset (tr_tool);
}

static void
transform_ok_callback (GtkWidget         *widget,
		       GimpTransformTool *tr_tool)
{
  gimp_transform_tool_doit (tr_tool, GIMP_TOOL (tr_tool)->gdisp);
}

static void
gimp_transform_tool_notify_grid (GimpTransformOptions *options,
                                 GParamSpec           *pspec,
                                 GimpTransformTool    *tr_tool)
{
  if (tr_tool->function == TRANSFORM_CREATING)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

  gimp_transform_tool_grid_recalc (tr_tool);
  gimp_transform_tool_transform_bounding_box (tr_tool);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
}

static void
gimp_transform_tool_notify_path (GimpTransformOptions *options,
                                 GParamSpec           *pspec,
                                 GimpTransformTool    *tr_tool)
{
  if (tr_tool->function == TRANSFORM_CREATING)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

  tr_tool->show_path = options->show_path;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
}
