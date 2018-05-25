/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpboundary.h"
#include "core/gimpdrawable-transform.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpitem-linked.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimpprogress.h"
#include "core/gimpprojection.h"
#include "core/gimptoolinfo.h"

#include "vectors/gimpvectors.h"
#include "vectors/gimpstroke.h"

#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasitem.h"
#include "display/gimpdisplay.h"
#include "display/gimptoolgui.h"
#include "display/gimptoolwidget.h"

#include "gimptoolcontrol.h"
#include "gimptransformoptions.h"
#include "gimptransformtool.h"
#include "gimptransformtoolundo.h"

#include "gimp-intl.h"


#define RESPONSE_RESET 1


static void      gimp_transform_tool_finalize            (GObject               *object);

static gboolean  gimp_transform_tool_initialize          (GimpTool              *tool,
                                                          GimpDisplay           *display,
                                                          GError               **error);
static void      gimp_transform_tool_control             (GimpTool              *tool,
                                                          GimpToolAction         action,
                                                          GimpDisplay           *display);
static void      gimp_transform_tool_button_press        (GimpTool              *tool,
                                                          const GimpCoords      *coords,
                                                          guint32                time,
                                                          GdkModifierType        state,
                                                          GimpButtonPressType    press_type,
                                                          GimpDisplay           *display);
static void      gimp_transform_tool_button_release      (GimpTool              *tool,
                                                          const GimpCoords      *coords,
                                                          guint32                time,
                                                          GdkModifierType        state,
                                                          GimpButtonReleaseType  release_type,
                                                          GimpDisplay           *display);
static void      gimp_transform_tool_motion              (GimpTool              *tool,
                                                          const GimpCoords      *coords,
                                                          guint32                time,
                                                          GdkModifierType        state,
                                                          GimpDisplay           *display);
static void      gimp_transform_tool_modifier_key        (GimpTool              *tool,
                                                          GdkModifierType        key,
                                                          gboolean               press,
                                                          GdkModifierType        state,
                                                          GimpDisplay           *display);
static void      gimp_transform_tool_cursor_update       (GimpTool              *tool,
                                                          const GimpCoords      *coords,
                                                          GdkModifierType        state,
                                                          GimpDisplay           *display);
static const gchar * gimp_transform_tool_can_undo        (GimpTool              *tool,
                                                          GimpDisplay           *display);
static const gchar * gimp_transform_tool_can_redo        (GimpTool              *tool,
                                                          GimpDisplay           *display);
static gboolean  gimp_transform_tool_undo                (GimpTool              *tool,
                                                          GimpDisplay           *display);
static gboolean  gimp_transform_tool_redo                (GimpTool              *tool,
                                                          GimpDisplay           *display);
static void      gimp_transform_tool_options_notify      (GimpTool              *tool,
                                                          GimpToolOptions       *options,
                                                          const GParamSpec      *pspec);

static void      gimp_transform_tool_draw                (GimpDrawTool          *draw_tool);

static GeglBuffer *
                 gimp_transform_tool_real_transform      (GimpTransformTool     *tr_tool,
                                                          GimpItem              *item,
                                                          GeglBuffer            *orig_buffer,
                                                          gint                   orig_offset_x,
                                                          gint                   orig_offset_y,
                                                          GimpColorProfile     **buffer_profile,
                                                          gint                  *new_offset_x,
                                                          gint                  *new_offset_y);

static void      gimp_transform_tool_widget_changed      (GimpToolWidget        *widget,
                                                          GimpTransformTool     *tr_tool);
static void      gimp_transform_tool_widget_response     (GimpToolWidget        *widget,
                                                          gint                   response_id,
                                                          GimpTransformTool     *tr_tool);

static void      gimp_transform_tool_halt                (GimpTransformTool     *tr_tool);
static void      gimp_transform_tool_commit              (GimpTransformTool     *tr_tool);

static gboolean  gimp_transform_tool_bounds              (GimpTransformTool     *tr_tool,
                                                          GimpDisplay           *display);
static void      gimp_transform_tool_dialog              (GimpTransformTool     *tr_tool);
static void      gimp_transform_tool_dialog_update       (GimpTransformTool     *tr_tool);
static void      gimp_transform_tool_prepare             (GimpTransformTool     *tr_tool,
                                                          GimpDisplay           *display);
static GimpToolWidget *
                 gimp_transform_tool_get_widget          (GimpTransformTool     *tr_tool);

static void      gimp_transform_tool_response            (GimpToolGui           *gui,
                                                          gint                   response_id,
                                                          GimpTransformTool     *tr_tool);

static void      gimp_transform_tool_update_sensitivity  (GimpTransformTool     *tr_tool);
static GimpItem *gimp_transform_tool_get_active_item     (GimpTransformTool     *tr_tool,
                                                          GimpImage             *image);
static GimpItem *gimp_transform_tool_check_active_item   (GimpTransformTool     *tr_tool,
                                                          GimpImage             *display,
                                                          gboolean               invisible_layer_ok,
                                                          GError               **error);
static void      gimp_transform_tool_hide_active_item    (GimpTransformTool     *tr_tool,
                                                          GimpItem              *item);
static void      gimp_transform_tool_show_active_item    (GimpTransformTool     *tr_tool);

static TransInfo * trans_info_new  (void);
static void        trans_info_free (TransInfo *info);


G_DEFINE_TYPE (GimpTransformTool, gimp_transform_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_transform_tool_parent_class


static void
gimp_transform_tool_class_init (GimpTransformToolClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class   = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_class   = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->finalize          = gimp_transform_tool_finalize;

  tool_class->initialize          = gimp_transform_tool_initialize;
  tool_class->control             = gimp_transform_tool_control;
  tool_class->button_press        = gimp_transform_tool_button_press;
  tool_class->button_release      = gimp_transform_tool_button_release;
  tool_class->motion              = gimp_transform_tool_motion;
  tool_class->modifier_key        = gimp_transform_tool_modifier_key;
  tool_class->cursor_update       = gimp_transform_tool_cursor_update;
  tool_class->can_undo            = gimp_transform_tool_can_undo;
  tool_class->can_redo            = gimp_transform_tool_can_redo;
  tool_class->undo                = gimp_transform_tool_undo;
  tool_class->redo                = gimp_transform_tool_redo;
  tool_class->options_notify      = gimp_transform_tool_options_notify;

  draw_class->draw                = gimp_transform_tool_draw;

  klass->dialog                   = NULL;
  klass->dialog_update            = NULL;
  klass->prepare                  = NULL;
  klass->recalc_matrix            = NULL;
  klass->get_undo_desc            = NULL;
  klass->transform                = gimp_transform_tool_real_transform;

  klass->ok_button_label          = _("_Transform");
}

static void
gimp_transform_tool_init (GimpTransformTool *tr_tool)
{
  GimpTool *tool = GIMP_TOOL (tr_tool);

  gimp_tool_control_set_scroll_lock      (tool->control, TRUE);
  gimp_tool_control_set_preserve         (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask       (tool->control,
                                          GIMP_DIRTY_IMAGE_SIZE |
                                          GIMP_DIRTY_DRAWABLE   |
                                          GIMP_DIRTY_SELECTION  |
                                          GIMP_DIRTY_ACTIVE_DRAWABLE);
  gimp_tool_control_set_active_modifiers (tool->control,
                                          GIMP_TOOL_ACTIVE_MODIFIERS_SAME);
  gimp_tool_control_set_precision        (tool->control,
                                          GIMP_CURSOR_PRECISION_SUBPIXEL);
  gimp_tool_control_set_cursor           (tool->control,
                                          GIMP_CURSOR_CROSSHAIR_SMALL);
  gimp_tool_control_set_action_opacity   (tool->control,
                                          "tools/tools-transform-preview-opacity-set");

  tr_tool->progress_text = _("Transforming");

  gimp_matrix3_identity (&tr_tool->transform);
  tr_tool->transform_valid = TRUE;

  tr_tool->strokes = g_ptr_array_new ();
}

static void
gimp_transform_tool_finalize (GObject *object)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (object);

  g_clear_object (&tr_tool->gui);
  g_clear_pointer (&tr_tool->strokes, g_ptr_array_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_transform_tool_initialize (GimpTool     *tool,
                                GimpDisplay  *display,
                                GError      **error)
{
  GimpTransformTool *tr_tool  = GIMP_TRANSFORM_TOOL (tool);
  GimpImage         *image    = gimp_display_get_image (display);
  GimpDrawable      *drawable = gimp_image_get_active_drawable (image);
  GimpItem          *item;

  item = gimp_transform_tool_check_active_item (tr_tool, image, FALSE, error);

  if (! item)
    return FALSE;

  /*  Find the transform bounds for some tools (like scale,
   *  perspective) that actually need the bounds for initializing
   */
  if (! gimp_transform_tool_bounds (tr_tool, display))
    {
      g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                   _("The selection does not intersect with the layer."));
      return FALSE;
    }

  tool->display  = display;
  tool->drawable = drawable;

  /*  Initialize the transform tool dialog  */
  if (! tr_tool->gui)
    gimp_transform_tool_dialog (tr_tool);

  /*  Initialize the tool-specific trans_info, and adjust the tool dialog  */
  gimp_transform_tool_prepare (tr_tool, display);

  /*  Recalculate the transform tool  */
  gimp_transform_tool_recalc_matrix (tr_tool, NULL);

  /*  Get the on-canvas gui  */
  tr_tool->widget = gimp_transform_tool_get_widget (tr_tool);

  gimp_transform_tool_hide_active_item (tr_tool, item);

  /*  start drawing the bounding box and handles...  */
  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);

  /* Initialize undo and redo lists */
  tr_tool->undo_list = g_list_prepend (NULL, trans_info_new ());
  tr_tool->redo_list = NULL;
  tr_tool->old_trans_info = g_list_last (tr_tool->undo_list)->data;
  tr_tool->prev_trans_info = g_list_first (tr_tool->undo_list)->data;
  gimp_transform_tool_update_sensitivity (tr_tool);

  /*  Save the current transformation info  */
  memcpy (tr_tool->prev_trans_info, tr_tool->trans_info,
          sizeof (TransInfo));

  return TRUE;
}

static void
gimp_transform_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      break;

    case GIMP_TOOL_ACTION_RESUME:
      gimp_transform_tool_bounds (tr_tool, display);
      gimp_transform_tool_recalc_matrix (tr_tool, tr_tool->widget);
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_transform_tool_halt (tr_tool);
     break;

    case GIMP_TOOL_ACTION_COMMIT:
      if (tool->display)
        gimp_transform_tool_commit (tr_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_transform_tool_button_press (GimpTool            *tool,
                                  const GimpCoords    *coords,
                                  guint32              time,
                                  GdkModifierType      state,
                                  GimpButtonPressType  press_type,
                                  GimpDisplay         *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  if (tr_tool->widget)
    {
      gimp_tool_widget_hover (tr_tool->widget, coords, state, TRUE);

      if (gimp_tool_widget_button_press (tr_tool->widget, coords, time, state,
                                         press_type))
        {
          tr_tool->grab_widget = tr_tool->widget;
        }
    }

  gimp_tool_control_activate (tool->control);
}

void
gimp_transform_tool_push_internal_undo (GimpTransformTool *tr_tool)
{
  g_return_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool));
  g_return_if_fail (tr_tool->prev_trans_info != NULL);

  /* push current state on the undo list and set this state as the
   * current state, but avoid doing this if there were no changes
   */
  if (memcmp (tr_tool->prev_trans_info, tr_tool->trans_info,
              sizeof (TransInfo)) != 0)
    {
      tr_tool->prev_trans_info = trans_info_new ();
      memcpy (tr_tool->prev_trans_info, tr_tool->trans_info,
              sizeof (TransInfo));

      tr_tool->undo_list = g_list_prepend (tr_tool->undo_list,
                                           tr_tool->prev_trans_info);

      /* If we undid anything and started interacting, we have to
       * discard the redo history
       */
      g_list_free_full (tr_tool->redo_list, (GDestroyNotify) trans_info_free);
      tr_tool->redo_list = NULL;

      gimp_transform_tool_update_sensitivity (tr_tool);

      /*  update the undo actions / menu items  */
      gimp_image_flush (gimp_display_get_image (GIMP_TOOL (tr_tool)->display));
    }
}

static void
gimp_transform_tool_button_release (GimpTool              *tool,
                                    const GimpCoords      *coords,
                                    guint32                time,
                                    GdkModifierType        state,
                                    GimpButtonReleaseType  release_type,
                                    GimpDisplay           *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  gimp_tool_control_halt (tool->control);

  if (tr_tool->grab_widget)
    {
      gimp_tool_widget_button_release (tr_tool->grab_widget,
                                       coords, time, state, release_type);
      tr_tool->grab_widget = NULL;
    }

  if (release_type != GIMP_BUTTON_RELEASE_CANCEL)
    {
      /* This hack is to perform the flip immediately with the flip tool */
      if (! tr_tool->widget)
        {
          gimp_transform_tool_response (NULL, GTK_RESPONSE_OK, tr_tool);
          return;
        }

      /* We're done with an interaction, save it on the undo list */
      gimp_transform_tool_push_internal_undo (tr_tool);
    }
  else
    {
      /*  Restore the last saved state  */
      memcpy (tr_tool->trans_info, tr_tool->prev_trans_info,
              sizeof (TransInfo));

      /*  reget the selection bounds  */
      gimp_transform_tool_bounds (tr_tool, display);

      /*  recalculate the tool's transformation matrix  */
      gimp_transform_tool_recalc_matrix (tr_tool, tr_tool->widget);
    }
}

static void
gimp_transform_tool_motion (GimpTool         *tool,
                            const GimpCoords *coords,
                            guint32           time,
                            GdkModifierType   state,
                            GimpDisplay      *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  if (tr_tool->grab_widget)
    {
      gimp_tool_widget_motion (tr_tool->grab_widget, coords, time, state);
    }
}

static void
gimp_transform_tool_modifier_key (GimpTool        *tool,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state,
                                  GimpDisplay     *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  if (tr_tool->widget)
    {
      GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press,
                                                    state, display);
    }
  else
    {
      GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);

      if (key == gimp_get_constrain_behavior_mask ())
        {
          g_object_set (options,
                        "frompivot-scale",       ! options->frompivot_scale,
                        "frompivot-shear",       ! options->frompivot_shear,
                        "frompivot-perspective", ! options->frompivot_perspective,
                        NULL);
        }
      else if (key == gimp_get_extend_selection_mask ())
        {
          g_object_set (options,
                        "cornersnap",            ! options->cornersnap,
                        "constrain-move",        ! options->constrain_move,
                        "constrain-scale",       ! options->constrain_scale,
                        "constrain-rotate",      ! options->constrain_rotate,
                        "constrain-shear",       ! options->constrain_shear,
                        "constrain-perspective", ! options->constrain_perspective,
                        NULL);
        }
    }
}

static void
gimp_transform_tool_cursor_update (GimpTool         *tool,
                                   const GimpCoords *coords,
                                   GdkModifierType   state,
                                   GimpDisplay      *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);
  GimpImage         *image   = gimp_display_get_image (display);

  if (! gimp_transform_tool_check_active_item (tr_tool, image, TRUE, NULL))
    {
      gimp_tool_set_cursor (tool, display,
                            gimp_tool_control_get_cursor (tool->control),
                            gimp_tool_control_get_tool_cursor (tool->control),
                            GIMP_CURSOR_MODIFIER_BAD);
      return;
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static const gchar *
gimp_transform_tool_can_undo (GimpTool    *tool,
                              GimpDisplay *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  if (! tr_tool->undo_list || ! tr_tool->undo_list->next)
    return NULL;

  return _("Transform Step");
}

static const gchar *
gimp_transform_tool_can_redo (GimpTool    *tool,
                              GimpDisplay *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  if (! tr_tool->redo_list)
    return NULL;

  return _("Transform Step");
}

static gboolean
gimp_transform_tool_undo (GimpTool    *tool,
                          GimpDisplay *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);
  GList             *item;

  item = g_list_next (tr_tool->undo_list);

  /* Move prev_trans_info from undo_list to redo_list */
  tr_tool->redo_list = g_list_prepend (tr_tool->redo_list,
                                       tr_tool->prev_trans_info);
  tr_tool->undo_list = g_list_remove (tr_tool->undo_list,
                                      tr_tool->prev_trans_info);

  tr_tool->prev_trans_info = item->data;

  /*  Restore the previous transformation info  */
  memcpy (tr_tool->trans_info, tr_tool->prev_trans_info,
          sizeof (TransInfo));

  /*  reget the selection bounds  */
  gimp_transform_tool_bounds (tr_tool, tool->display);

  /*  recalculate the tool's transformation matrix  */
  gimp_transform_tool_recalc_matrix (tr_tool, tr_tool->widget);

  return TRUE;
}

static gboolean
gimp_transform_tool_redo (GimpTool    *tool,
                          GimpDisplay *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);
  GList             *item;

  item = tr_tool->redo_list;

  /* Move prev_trans_info from redo_list to undo_list */
  tr_tool->prev_trans_info = item->data;

  tr_tool->undo_list = g_list_prepend (tr_tool->undo_list,
                                       tr_tool->prev_trans_info);
  tr_tool->redo_list = g_list_remove (tr_tool->redo_list,
                                      tr_tool->prev_trans_info);

  /*  Restore the previous transformation info  */
  memcpy (tr_tool->trans_info, tr_tool->prev_trans_info,
          sizeof (TransInfo));

  /*  reget the selection bounds  */
  gimp_transform_tool_bounds (tr_tool, tool->display);

  /*  recalculate the tool's transformation matrix  */
  gimp_transform_tool_recalc_matrix (tr_tool, tr_tool->widget);

  return TRUE;
}

static void
gimp_transform_tool_options_notify (GimpTool         *tool,
                                    GimpToolOptions  *options,
                                    const GParamSpec *pspec)
{
  GimpTransformTool    *tr_tool    = GIMP_TRANSFORM_TOOL (tool);
  GimpTransformOptions *tr_options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);

  GIMP_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "type"))
    {
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
      return;
    }

  if (! tr_tool->widget)
    return;

  if (! strcmp (pspec->name, "direction"))
    {
      /*  reget the selection bounds  */
      gimp_transform_tool_bounds (tr_tool, tool->display);

      /*  recalculate the tool's transformation matrix  */
      gimp_transform_tool_recalc_matrix (tr_tool, tr_tool->widget);
    }
  else if (! strcmp (pspec->name, "show-preview"))
    {
      if (tr_tool->preview)
        {
          GimpDisplay *display;
          GimpImage   *image;
          GimpItem    *item;
          gboolean     show_preview;

          show_preview = gimp_transform_options_show_preview (tr_options) &&
                         tr_tool->transform_valid;

          gimp_canvas_item_set_visible (tr_tool->preview, show_preview);

          display = tool->display;
          image   = gimp_display_get_image (display);
          item    = gimp_transform_tool_check_active_item (tr_tool, image, TRUE, NULL);
          if (item)
            {
              if (show_preview)
                gimp_transform_tool_hide_active_item (tr_tool, item);
              else
                gimp_transform_tool_show_active_item (tr_tool);
            }
        }
    }
  else if (g_str_has_prefix (pspec->name, "constrain-") ||
           g_str_has_prefix (pspec->name, "frompivot-") ||
           ! strcmp (pspec->name, "fixedpivot") ||
           ! strcmp (pspec->name, "cornersnap"))
    {
      gimp_transform_tool_dialog_update (tr_tool);
    }
}

static void
gimp_transform_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool             *tool    = GIMP_TOOL (draw_tool);
  GimpTransformTool    *tr_tool = GIMP_TRANSFORM_TOOL (draw_tool);
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);
  GimpImage            *image   = gimp_display_get_image (tool->display);
  GimpMatrix3           matrix  = tr_tool->transform;
  GimpCanvasItem       *item;

  if (options->direction == GIMP_TRANSFORM_BACKWARD)
    gimp_matrix3_invert (&matrix);

  if (tr_tool->widget)
    {
      gboolean show_preview = gimp_transform_options_show_preview (options) &&
                              tr_tool->transform_valid;

      tr_tool->preview =
        gimp_draw_tool_add_transform_preview (draw_tool,
                                              tool->drawable,
                                              &matrix,
                                              tr_tool->x1,
                                              tr_tool->y1,
                                              tr_tool->x2,
                                              tr_tool->y2);
      g_object_add_weak_pointer (G_OBJECT (tr_tool->preview),
                                 (gpointer) &tr_tool->preview);

      gimp_canvas_item_set_visible (tr_tool->preview, show_preview);

      g_object_bind_property (G_OBJECT (options),          "preview-opacity",
                              G_OBJECT (tr_tool->preview), "opacity",
                              G_BINDING_SYNC_CREATE |
                              G_BINDING_BIDIRECTIONAL);

      GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
    }

  if (options->type == GIMP_TRANSFORM_TYPE_SELECTION)
    {
      const GimpBoundSeg *segs_in;
      const GimpBoundSeg *segs_out;
      gint                n_segs_in;
      gint                n_segs_out;

      gimp_channel_boundary (gimp_image_get_mask (image),
                             &segs_in, &segs_out,
                             &n_segs_in, &n_segs_out,
                             0, 0, 0, 0);

      if (segs_in)
        {
          tr_tool->boundary_in =
            gimp_draw_tool_add_boundary (draw_tool,
                                         segs_in, n_segs_in,
                                         &matrix,
                                         0, 0);
          g_object_add_weak_pointer (G_OBJECT (tr_tool->boundary_in),
                                     (gpointer) &tr_tool->boundary_in);

          gimp_canvas_item_set_visible (tr_tool->boundary_in,
                                        tr_tool->transform_valid);
        }

      if (segs_out)
        {
          tr_tool->boundary_out =
            gimp_draw_tool_add_boundary (draw_tool,
                                         segs_out, n_segs_out,
                                         &matrix,
                                         0, 0);
          g_object_add_weak_pointer (G_OBJECT (tr_tool->boundary_in),
                                     (gpointer) &tr_tool->boundary_out);

          gimp_canvas_item_set_visible (tr_tool->boundary_out,
                                        tr_tool->transform_valid);
        }
    }
  else if (options->type == GIMP_TRANSFORM_TYPE_PATH)
    {
      GimpVectors *vectors = gimp_image_get_active_vectors (image);

      if (vectors)
        {
          GimpStroke *stroke = NULL;

          while ((stroke = gimp_vectors_stroke_get_next (vectors, stroke)))
            {
              GArray   *coords;
              gboolean  closed;

              coords = gimp_stroke_interpolate (stroke, 1.0, &closed);

              if (coords && coords->len)
                {
                  item =
                    gimp_draw_tool_add_strokes (draw_tool,
                                                &g_array_index (coords,
                                                                GimpCoords, 0),
                                                coords->len, &matrix, FALSE);

                  g_ptr_array_add (tr_tool->strokes, item);
                  g_object_weak_ref (G_OBJECT (item),
                                     (GWeakNotify) g_ptr_array_remove,
                                     tr_tool->strokes);

                  gimp_canvas_item_set_visible (item, tr_tool->transform_valid);
                }

              if (coords)
                g_array_free (coords, TRUE);
            }
        }
    }
}

static GeglBuffer *
gimp_transform_tool_real_transform (GimpTransformTool *tr_tool,
                                    GimpItem          *active_item,
                                    GeglBuffer        *orig_buffer,
                                    gint               orig_offset_x,
                                    gint               orig_offset_y,
                                    GimpColorProfile **buffer_profile,
                                    gint              *new_offset_x,
                                    gint              *new_offset_y)
{
  GimpTool             *tool    = GIMP_TOOL (tr_tool);
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);
  GimpContext          *context = GIMP_CONTEXT (options);
  GeglBuffer           *ret     = NULL;
  GimpTransformResize   clip    = options->clip;
  GimpProgress         *progress;

  progress = gimp_progress_start (GIMP_PROGRESS (tool), FALSE,
                                  "%s", tr_tool->progress_text);

  if (orig_buffer)
    {
      /*  this happens when transforming a selection cut out of a
       *  normal drawable, or the selection
       */

      /*  always clip the selection and unfloated channels
       *  so they keep their size
       */
      if (GIMP_IS_CHANNEL (active_item) &&
          ! babl_format_has_alpha (gegl_buffer_get_format (orig_buffer)))
        clip = GIMP_TRANSFORM_RESIZE_CLIP;

      ret = gimp_drawable_transform_buffer_affine (GIMP_DRAWABLE (active_item),
                                                   context,
                                                   orig_buffer,
                                                   orig_offset_x,
                                                   orig_offset_y,
                                                   &tr_tool->transform,
                                                   options->direction,
                                                   options->interpolation,
                                                   clip,
                                                   buffer_profile,
                                                   new_offset_x,
                                                   new_offset_y,
                                                   progress);
    }
  else
    {
      /*  this happens for entire drawables, paths and layer groups  */

      if (gimp_item_get_linked (active_item))
        {
          gimp_item_linked_transform (active_item, context,
                                      &tr_tool->transform,
                                      options->direction,
                                      options->interpolation,
                                      clip,
                                      progress);
        }
      else
        {
          /*  always clip layer masks so they keep their size
           */
          if (GIMP_IS_CHANNEL (active_item))
            clip = GIMP_TRANSFORM_RESIZE_CLIP;

          gimp_item_transform (active_item, context,
                               &tr_tool->transform,
                               options->direction,
                               options->interpolation,
                               clip,
                               progress);
        }
    }

  if (progress)
    gimp_progress_end (progress);

  return ret;
}

static void
gimp_transform_tool_widget_changed (GimpToolWidget    *widget,
                                    GimpTransformTool *tr_tool)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  GimpMatrix3           matrix  = tr_tool->transform;
  gint                  i;

  if (options->direction == GIMP_TRANSFORM_BACKWARD)
    gimp_matrix3_invert (&matrix);

  if (tr_tool->preview)
    {
      gboolean show_preview = gimp_transform_options_show_preview (options) &&
                              tr_tool->transform_valid;

      gimp_canvas_item_begin_change (tr_tool->preview);
      gimp_canvas_item_set_visible (tr_tool->preview, show_preview);
      g_object_set (tr_tool->preview,
                    "transform", &matrix,
                    NULL);
      gimp_canvas_item_end_change (tr_tool->preview);
    }

  if (tr_tool->boundary_in)
    {
      gimp_canvas_item_begin_change (tr_tool->boundary_in);
      gimp_canvas_item_set_visible (tr_tool->boundary_in,
                                    tr_tool->transform_valid);
      g_object_set (tr_tool->boundary_in,
                    "transform", &matrix,
                    NULL);
      gimp_canvas_item_end_change (tr_tool->boundary_in);
    }

  if (tr_tool->boundary_out)
    {
      gimp_canvas_item_begin_change (tr_tool->boundary_out);
      gimp_canvas_item_set_visible (tr_tool->boundary_out,
                                    tr_tool->transform_valid);
      g_object_set (tr_tool->boundary_out,
                    "transform", &matrix,
                    NULL);
      gimp_canvas_item_end_change (tr_tool->boundary_out);
    }

  for (i = 0; i < tr_tool->strokes->len; i++)
    {
      GimpCanvasItem *item = g_ptr_array_index (tr_tool->strokes, i);

      gimp_canvas_item_begin_change (item);
      gimp_canvas_item_set_visible (item, tr_tool->transform_valid);
      g_object_set (item,
                    "transform", &matrix,
                    NULL);
      gimp_canvas_item_end_change (item);
    }
}

static void
gimp_transform_tool_widget_response (GimpToolWidget    *widget,
                                     gint               response_id,
                                     GimpTransformTool *tr_tool)
{
  switch (response_id)
    {
    case GIMP_TOOL_WIDGET_RESPONSE_CONFIRM:
      gimp_transform_tool_response (NULL, GTK_RESPONSE_OK, tr_tool);
      break;

    case GIMP_TOOL_WIDGET_RESPONSE_CANCEL:
      gimp_transform_tool_response (NULL, GTK_RESPONSE_CANCEL, tr_tool);
      break;

    case GIMP_TOOL_WIDGET_RESPONSE_RESET:
      gimp_transform_tool_response (NULL, RESPONSE_RESET, tr_tool);
      break;
    }
}

static void
gimp_transform_tool_halt (GimpTransformTool *tr_tool)
{
  GimpTool *tool = GIMP_TOOL (tr_tool);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tr_tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (tr_tool));

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tr_tool), NULL);
  g_clear_object (&tr_tool->widget);

  if (tr_tool->gui)
    gimp_tool_gui_hide (tr_tool->gui);

  if (tr_tool->redo_list)
    {
      g_list_free_full (tr_tool->redo_list, (GDestroyNotify) trans_info_free);
      tr_tool->redo_list = NULL;
    }

  if (tr_tool->undo_list)
    {
      g_list_free_full (tr_tool->undo_list, (GDestroyNotify) trans_info_free);
      tr_tool->undo_list = NULL;
      tr_tool->prev_trans_info = NULL;
    }

  gimp_transform_tool_show_active_item (tr_tool);

  tool->display  = NULL;
  tool->drawable = NULL;
 }

static void
gimp_transform_tool_commit (GimpTransformTool *tr_tool)
{
  GimpTool             *tool           = GIMP_TOOL (tr_tool);
  GimpTransformOptions *options        = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);
  GimpContext          *context        = GIMP_CONTEXT (options);
  GimpDisplay          *display        = tool->display;
  GimpImage            *image          = gimp_display_get_image (display);
  GimpItem             *active_item;
  GeglBuffer           *orig_buffer    = NULL;
  gint                  orig_offset_x  = 0;
  gint                  orig_offset_y  = 0;
  GeglBuffer           *new_buffer;
  gint                  new_offset_x;
  gint                  new_offset_y;
  GimpColorProfile     *buffer_profile;
  gchar                *undo_desc      = NULL;
  gboolean              new_layer      = FALSE;
  GError               *error          = NULL;

  active_item = gimp_transform_tool_check_active_item (tr_tool, image,
                                                       TRUE, &error);

  if (! active_item)
    {
      gimp_tool_message_literal (tool, display, error->message);
      g_clear_error (&error);
      return;
    }

  if (! tr_tool->transform_valid)
    {
      gimp_tool_message_literal (tool, display,
                                 _("The current transform is invalid"));
      return;
    }

  if (tr_tool->gui)
    gimp_tool_gui_hide (tr_tool->gui);

  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix &&
      gimp_matrix3_is_identity (&tr_tool->transform))
    {
      /* No need to commit an identity transformation! */
      return;
    }

  gimp_set_busy (display->gimp);

  /* undraw the tool before we muck around with the transform matrix */
  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tr_tool));

  /*  We're going to dirty this image, but we want to keep the tool around  */
  gimp_tool_control_push_preserve (tool->control, TRUE);

  undo_desc = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->get_undo_desc (tr_tool);
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM, undo_desc);
  g_free (undo_desc);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      if (! gimp_viewable_get_children (GIMP_VIEWABLE (tool->drawable)) &&
          ! gimp_channel_is_empty (gimp_image_get_mask (image)))
        {
          orig_buffer = gimp_drawable_transform_cut (tool->drawable,
                                                     context,
                                                     &orig_offset_x,
                                                     &orig_offset_y,
                                                     &new_layer);
        }
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
      orig_buffer = g_object_ref (gimp_drawable_get_buffer (GIMP_DRAWABLE (active_item)));
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      break;
    }

  /*  Send the request for the transformation to the tool...
   */
  new_buffer = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->transform (tr_tool,
                                                                   active_item,
                                                                   orig_buffer,
                                                                   orig_offset_x,
                                                                   orig_offset_y,
                                                                   &buffer_profile,
                                                                   &new_offset_x,
                                                                   &new_offset_y);

  if (orig_buffer)
    g_object_unref (orig_buffer);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      if (new_buffer)
        {
          /*  paste the new transformed image to the image...also implement
           *  undo...
           */
          gimp_drawable_transform_paste (tool->drawable,
                                         new_buffer, buffer_profile,
                                         new_offset_x, new_offset_y,
                                         new_layer);
          g_object_unref (new_buffer);
        }
      break;

     case GIMP_TRANSFORM_TYPE_SELECTION:
      if (new_buffer)
        {
          gimp_channel_push_undo (GIMP_CHANNEL (active_item), NULL);

          gimp_drawable_set_buffer (GIMP_DRAWABLE (active_item),
                                    FALSE, NULL, new_buffer);
          g_object_unref (new_buffer);
        }
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      /*  Nothing to be done  */
      break;
    }

  gimp_image_undo_push (image, GIMP_TYPE_TRANSFORM_TOOL_UNDO,
                        GIMP_UNDO_TRANSFORM, NULL,
                        0,
                        "transform-tool", tr_tool,
                        NULL);

  gimp_image_undo_group_end (image);

  /*  We're done dirtying the image, and would like to be restarted if
   *  the image gets dirty while the tool exists
   */
  gimp_tool_control_pop_preserve (tool->control);

  gimp_unset_busy (display->gimp);

  gimp_image_flush (image);
}

static gboolean
gimp_transform_tool_bounds (GimpTransformTool *tr_tool,
                            GimpDisplay       *display)
{
  GimpTransformOptions *options   = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  GimpImage            *image     = gimp_display_get_image (display);
  gboolean              non_empty = TRUE;

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      {
        GimpDrawable *drawable;
        gint          offset_x;
        gint          offset_y;
        gint          x, y;
        gint          width, height;

        drawable = gimp_image_get_active_drawable (image);

        gimp_item_get_offset (GIMP_ITEM (drawable), &offset_x, &offset_y);

        non_empty = gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                              &x, &y, &width, &height);
        tr_tool->x1 = x + offset_x;
        tr_tool->y1 = y + offset_y;
        tr_tool->x2 = x + width  + offset_x;
        tr_tool->y2 = y + height + offset_y;
      }
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
      {
        gimp_item_bounds (GIMP_ITEM (gimp_image_get_mask (image)),
                          &tr_tool->x1, &tr_tool->y1,
                          &tr_tool->x2, &tr_tool->y2);
        tr_tool->x2 += tr_tool->x1;
        tr_tool->y2 += tr_tool->y1;
      }
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      {
        GimpChannel *selection = gimp_image_get_mask (image);

        /* if selection is not empty, use its bounds to perform the
         * transformation of the path
         */

        if (! gimp_channel_is_empty (selection))
          {
            gimp_item_bounds (GIMP_ITEM (selection),
                              &tr_tool->x1, &tr_tool->y1,
                              &tr_tool->x2, &tr_tool->y2);
          }
        else
          {
            /* without selection, test the emptiness of the path bounds :
             * if empty, use the canvas bounds
             * else use the path bounds
             */

            if (! gimp_item_bounds (GIMP_ITEM (gimp_image_get_active_vectors (image)),
                                    &tr_tool->x1, &tr_tool->y1,
                                    &tr_tool->x2, &tr_tool->y2))
              {
                tr_tool->x1 = 0;
                tr_tool->y1 = 0;
                tr_tool->x2 = gimp_image_get_width (image);
                tr_tool->y2 = gimp_image_get_height (image);
              }
          }

        tr_tool->x2 += tr_tool->x1;
        tr_tool->y2 += tr_tool->y1;
      }

      break;
    }

  return non_empty;
}

static void
gimp_transform_tool_dialog (GimpTransformTool *tr_tool)
{
  GimpTool         *tool      = GIMP_TOOL (tr_tool);
  GimpToolInfo     *tool_info = tool->tool_info;
  GimpDisplayShell *shell;
  const gchar      *ok_button_label;

  if (! GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->dialog)
    return;

  g_return_if_fail (tool->display != NULL);

  shell = gimp_display_get_shell (tool->display);

  ok_button_label = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->ok_button_label;

  tr_tool->gui = gimp_tool_gui_new (tool_info,
                                    NULL, NULL, NULL, NULL,
                                    gimp_widget_get_monitor (GTK_WIDGET (shell)),
                                    TRUE,

                                    _("_Reset"),     RESPONSE_RESET,
                                    _("_Cancel"),    GTK_RESPONSE_CANCEL,
                                    ok_button_label, GTK_RESPONSE_OK,

                                    NULL);

  gimp_tool_gui_set_auto_overlay (tr_tool->gui, TRUE);
  gimp_tool_gui_set_default_response (tr_tool->gui, GTK_RESPONSE_OK);

  gimp_tool_gui_set_alternative_button_order (tr_tool->gui,
                                              RESPONSE_RESET,
                                              GTK_RESPONSE_OK,
                                              GTK_RESPONSE_CANCEL,
                                              -1);

  g_signal_connect (tr_tool->gui, "response",
                    G_CALLBACK (gimp_transform_tool_response),
                    tr_tool);

  GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->dialog (tr_tool);
}

static void
gimp_transform_tool_dialog_update (GimpTransformTool *tr_tool)
{
  if (tr_tool->gui &&
      GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->dialog_update)
    {
      GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->dialog_update (tr_tool);
    }
}

static void
gimp_transform_tool_prepare (GimpTransformTool *tr_tool,
                             GimpDisplay       *display)
{
  if (tr_tool->gui)
    {
      GimpImage *image = gimp_display_get_image (display);
      GimpItem  *item  = gimp_transform_tool_get_active_item (tr_tool, image);

      gimp_tool_gui_set_shell (tr_tool->gui, gimp_display_get_shell (display));
      gimp_tool_gui_set_viewable (tr_tool->gui, GIMP_VIEWABLE (item));
    }

  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->prepare)
    GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->prepare (tr_tool);
}

static GimpToolWidget *
gimp_transform_tool_get_widget (GimpTransformTool *tr_tool)
{
  static const gchar *properties[] =
  {
    "constrain-move",
    "constrain-scale",
    "constrain-rotate",
    "constrain-shear",
    "constrain-perspective",
    "frompivot-scale",
    "frompivot-shear",
    "frompivot-perspective",
    "cornersnap",
    "fixedpivot"
  };

  GimpToolWidget *widget = NULL;

  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->get_widget)
    {
      GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
      gint                  i;

      widget = GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->get_widget (tr_tool);

      gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tr_tool), widget);

      g_object_bind_property (G_OBJECT (options), "grid-type",
                              G_OBJECT (widget),  "guide-type",
                              G_BINDING_SYNC_CREATE |
                              G_BINDING_BIDIRECTIONAL);
      g_object_bind_property (G_OBJECT (options), "grid-size",
                              G_OBJECT (widget),  "n-guides",
                              G_BINDING_SYNC_CREATE |
                              G_BINDING_BIDIRECTIONAL);

      for (i = 0; i < G_N_ELEMENTS (properties); i++)
        g_object_bind_property (G_OBJECT (options), properties[i],
                                G_OBJECT (widget),  properties[i],
                                G_BINDING_SYNC_CREATE |
                                G_BINDING_BIDIRECTIONAL);

      g_signal_connect (widget, "changed",
                        G_CALLBACK (gimp_transform_tool_widget_changed),
                        tr_tool);
      g_signal_connect (widget, "response",
                        G_CALLBACK (gimp_transform_tool_widget_response),
                        tr_tool);
    }

  return widget;
}

void
gimp_transform_tool_recalc_matrix (GimpTransformTool *tr_tool,
                                   GimpToolWidget    *widget)
{
  g_return_if_fail (GIMP_IS_TRANSFORM_TOOL (tr_tool));
  g_return_if_fail (widget == NULL || GIMP_IS_TOOL_WIDGET (widget));

  if (GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix)
    GIMP_TRANSFORM_TOOL_GET_CLASS (tr_tool)->recalc_matrix (tr_tool, widget);

  gimp_transform_tool_dialog_update (tr_tool);
  gimp_transform_tool_update_sensitivity (tr_tool);

  if (tr_tool->gui)
    gimp_tool_gui_show (tr_tool->gui);
}

static void
gimp_transform_tool_response (GimpToolGui       *gui,
                              gint               response_id,
                              GimpTransformTool *tr_tool)
{
  GimpTool    *tool    = GIMP_TOOL (tr_tool);
  GimpDisplay *display = tool->display;

  switch (response_id)
    {
    case RESPONSE_RESET:
      /* Move all undo events to redo, and pop off the first
       * one as that's the current one, which always sits on
       * the undo_list
       */
      tr_tool->redo_list =
        g_list_remove (g_list_concat (g_list_reverse (tr_tool->undo_list),
                                      tr_tool->redo_list),
                       tr_tool->old_trans_info);
      tr_tool->prev_trans_info = tr_tool->old_trans_info;
      tr_tool->undo_list = g_list_prepend (NULL,
                                           tr_tool->prev_trans_info);

      gimp_transform_tool_update_sensitivity (tr_tool);

      /*  Restore the previous transformation info  */
      memcpy (tr_tool->trans_info, tr_tool->prev_trans_info,
              sizeof (TransInfo));

      /*  reget the selection bounds  */
      gimp_transform_tool_bounds (tr_tool, display);

      /*  recalculate the tool's transformation matrix  */
      gimp_transform_tool_recalc_matrix (tr_tool, tr_tool->widget);

      /*  update the undo actions / menu items  */
      gimp_image_flush (gimp_display_get_image (display));
      break;

    case GTK_RESPONSE_OK:
      g_return_if_fail (display != NULL);
      gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);
      break;

    default:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);

      /*  update the undo actions / menu items  */
      if (display)
        gimp_image_flush (gimp_display_get_image (display));
      break;
    }
}

static void
gimp_transform_tool_update_sensitivity (GimpTransformTool *tr_tool)
{
  if (! tr_tool->gui)
    return;

  gimp_tool_gui_set_response_sensitive (tr_tool->gui, GTK_RESPONSE_OK,
                                        tr_tool->transform_valid);
  gimp_tool_gui_set_response_sensitive (tr_tool->gui, RESPONSE_RESET,
                                        g_list_next (tr_tool->undo_list) != NULL);
}

static GimpItem *
gimp_transform_tool_get_active_item (GimpTransformTool *tr_tool,
                                     GimpImage         *image)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      return GIMP_ITEM (gimp_image_get_active_drawable (image));

    case GIMP_TRANSFORM_TYPE_SELECTION:
      {
        GimpChannel *selection = gimp_image_get_mask (image);

        if (gimp_channel_is_empty (selection))
          return NULL;
        else
          return GIMP_ITEM (selection);
      }

    case GIMP_TRANSFORM_TYPE_PATH:
      return GIMP_ITEM (gimp_image_get_active_vectors (image));
    }

  return NULL;
}

static GimpItem *
gimp_transform_tool_check_active_item (GimpTransformTool  *tr_tool,
                                       GimpImage          *image,
                                       gboolean            invisible_layer_ok,
                                       GError            **error)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  GimpItem             *item;
  const gchar          *null_message   = NULL;
  const gchar          *locked_message = NULL;

  item = gimp_transform_tool_get_active_item (tr_tool, image);

  switch (options->type)
    {
    case GIMP_TRANSFORM_TYPE_LAYER:
      null_message = _("There is no layer to transform.");

      if (item)
        {
          if (gimp_item_is_content_locked (item))
            locked_message = _("The active layer's pixels are locked.");
          else if (gimp_item_is_position_locked (item))
            locked_message = _("The active layer's position and size are locked.");

          /*  invisible_layer_ok is such a hack, see bug #759194 */
          if (! invisible_layer_ok && ! gimp_item_is_visible (item))
            {
              g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                                   _("The active layer is not visible."));
              return NULL;
            }
        }
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
      null_message = _("There is no selection to transform.");

      if (item)
        {
          /* cannot happen, so don't translate these messages */
          if (gimp_item_is_content_locked (item))
            locked_message = "The selection's pixels are locked.";
          else if (gimp_item_is_position_locked (item))
            locked_message = "The selection's position and size are locked.";
        }
      break;

    case GIMP_TRANSFORM_TYPE_PATH:
      null_message = _("There is no path to transform.");

      if (item)
        {
          if (gimp_item_is_content_locked (item))
            locked_message = _("The active path's strokes are locked.");
          else if (gimp_item_is_position_locked (item))
            locked_message = _("The active path's position is locked.");
          else if (! gimp_vectors_get_n_strokes (GIMP_VECTORS (item)))
            locked_message = _("The active path has no strokes.");
        }
      break;
    }

  if (! item)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED, null_message);
      return NULL;
    }

  if (locked_message)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED, locked_message);
      return NULL;
    }

  return item;
}

static void
gimp_transform_tool_hide_active_item (GimpTransformTool *tr_tool,
                                      GimpItem          *item)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  GimpDisplay          *display = GIMP_TOOL (tr_tool)->display;
  GimpImage            *image   = gimp_display_get_image (display);

  /*  hide only complete layers and channels, not layer masks  */
  if (options->type == GIMP_TRANSFORM_TYPE_LAYER &&
      options->show_preview                      &&
      tr_tool->widget /* not for flip */         &&
      GIMP_IS_DRAWABLE (item)                    &&
      ! GIMP_IS_LAYER_MASK (item)                &&
      gimp_item_get_visible (item)               &&
      gimp_channel_is_empty (gimp_image_get_mask (image)))
    {
      tr_tool->hidden_item = item;
      gimp_item_set_visible (item, FALSE, FALSE);

      gimp_projection_flush (gimp_image_get_projection (image));
    }
}

static void
gimp_transform_tool_show_active_item (GimpTransformTool *tr_tool)
{
  if (tr_tool->hidden_item)
    {
      GimpDisplay *display = GIMP_TOOL (tr_tool)->display;
      GimpImage   *image   = gimp_display_get_image (display);

      gimp_item_set_visible (tr_tool->hidden_item, TRUE, FALSE);
      tr_tool->hidden_item = NULL;

      gimp_image_flush (image);
    }
}

static TransInfo *
trans_info_new (void)
{
  return g_slice_new0 (TransInfo);
}

static void
trans_info_free (TransInfo *info)
{
  g_slice_free (TransInfo, info);
}
