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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
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
#include "gimptransformgridoptions.h"
#include "gimptransformgridtool.h"
#include "gimptransformgridtoolundo.h"
#include "gimptransformoptions.h"

#include "gimp-intl.h"


#define EPSILON 1e-6


#define RESPONSE_RESET    1
#define RESPONSE_READJUST 2


typedef struct
{
  GimpTransformDirection direction;
  TransInfo              trans_infos[2];
} UndoInfo;


static void      gimp_transform_grid_tool_finalize           (GObject                *object);

static gboolean  gimp_transform_grid_tool_initialize         (GimpTool               *tool,
                                                              GimpDisplay            *display,
                                                              GError                **error);
static void      gimp_transform_grid_tool_control            (GimpTool               *tool,
                                                              GimpToolAction          action,
                                                              GimpDisplay            *display);
static void      gimp_transform_grid_tool_button_press       (GimpTool               *tool,
                                                              const GimpCoords       *coords,
                                                              guint32                 time,
                                                              GdkModifierType         state,
                                                              GimpButtonPressType     press_type,
                                                              GimpDisplay            *display);
static void      gimp_transform_grid_tool_button_release     (GimpTool               *tool,
                                                              const GimpCoords       *coords,
                                                              guint32                 time,
                                                              GdkModifierType         state,
                                                              GimpButtonReleaseType   release_type,
                                                              GimpDisplay            *display);
static void      gimp_transform_grid_tool_motion             (GimpTool               *tool,
                                                              const GimpCoords       *coords,
                                                              guint32                 time,
                                                              GdkModifierType         state,
                                                              GimpDisplay            *display);
static void      gimp_transform_grid_tool_modifier_key       (GimpTool               *tool,
                                                              GdkModifierType         key,
                                                              gboolean                press,
                                                              GdkModifierType         state,
                                                              GimpDisplay            *display);
static void      gimp_transform_grid_tool_cursor_update      (GimpTool               *tool,
                                                              const GimpCoords       *coords,
                                                              GdkModifierType         state,
                                                              GimpDisplay            *display);
static const gchar * gimp_transform_grid_tool_can_undo       (GimpTool               *tool,
                                                              GimpDisplay            *display);
static const gchar * gimp_transform_grid_tool_can_redo       (GimpTool               *tool,
                                                              GimpDisplay            *display);
static gboolean  gimp_transform_grid_tool_undo               (GimpTool               *tool,
                                                              GimpDisplay            *display);
static gboolean  gimp_transform_grid_tool_redo               (GimpTool               *tool,
                                                              GimpDisplay            *display);
static void      gimp_transform_grid_tool_options_notify     (GimpTool               *tool,
                                                              GimpToolOptions        *options,
                                                              const GParamSpec       *pspec);

static void      gimp_transform_grid_tool_draw               (GimpDrawTool           *draw_tool);

static void      gimp_transform_grid_tool_recalc_matrix      (GimpTransformTool      *tr_tool);
static gchar   * gimp_transform_grid_tool_get_undo_desc      (GimpTransformTool      *tr_tool);
static GimpTransformDirection gimp_transform_grid_tool_get_direction
                                                             (GimpTransformTool      *tr_tool);
static GeglBuffer * gimp_transform_grid_tool_transform       (GimpTransformTool      *tr_tool,
                                                              GimpItem               *item,
                                                              GeglBuffer             *orig_buffer,
                                                              gint                    orig_offset_x,
                                                              gint                    orig_offset_y,
                                                              GimpColorProfile      **buffer_profile,
                                                              gint                   *new_offset_x,
                                                              gint                   *new_offset_y);

static gchar  * gimp_transform_grid_tool_real_get_undo_desc  (GimpTransformGridTool  *tg_tool);
static void     gimp_transform_grid_tool_real_update_widget  (GimpTransformGridTool  *tg_tool);
static void     gimp_transform_grid_tool_real_widget_changed (GimpTransformGridTool  *tg_tool);
static GeglBuffer * gimp_transform_grid_tool_real_transform  (GimpTransformGridTool  *tg_tool,
                                                              GimpItem               *item,
                                                              GeglBuffer             *orig_buffer,
                                                              gint                    orig_offset_x,
                                                              gint                    orig_offset_y,
                                                              GimpColorProfile      **buffer_profile,
                                                              gint                   *new_offset_x,
                                                              gint                   *new_offset_y);

static void      gimp_transform_grid_tool_widget_changed     (GimpToolWidget         *widget,
                                                              GimpTransformGridTool  *tg_tool);
static void      gimp_transform_grid_tool_widget_response    (GimpToolWidget         *widget,
                                                              gint                    response_id,
                                                              GimpTransformGridTool  *tg_tool);

static void      gimp_transform_grid_tool_halt               (GimpTransformGridTool  *tg_tool);
static void      gimp_transform_grid_tool_commit             (GimpTransformGridTool  *tg_tool);

static void      gimp_transform_grid_tool_dialog             (GimpTransformGridTool  *tg_tool);
static void      gimp_transform_grid_tool_dialog_update      (GimpTransformGridTool  *tg_tool);
static void      gimp_transform_grid_tool_prepare            (GimpTransformGridTool  *tg_tool,
                                                              GimpDisplay            *display);
static GimpToolWidget * gimp_transform_grid_tool_get_widget  (GimpTransformGridTool  *tg_tool);
static void      gimp_transform_grid_tool_update_widget      (GimpTransformGridTool  *tg_tool);

static void      gimp_transform_grid_tool_response           (GimpToolGui            *gui,
                                                              gint                    response_id,
                                                              GimpTransformGridTool  *tg_tool);

static void      gimp_transform_grid_tool_update_sensitivity (GimpTransformGridTool  *tg_tool);
static void      gimp_transform_grid_tool_update_preview     (GimpTransformGridTool  *tg_tool);
static void      gimp_transform_grid_tool_hide_active_item   (GimpTransformGridTool  *tg_tool,
                                                              GimpItem               *item);
static void      gimp_transform_grid_tool_show_active_item   (GimpTransformGridTool  *tg_tool);

static UndoInfo * undo_info_new  (void);
static void       undo_info_free (UndoInfo *info);

static gboolean   trans_info_equal  (const TransInfo  trans_info1,
                                     const TransInfo  trans_info2);
static gboolean   trans_infos_equal (const TransInfo *trans_infos1,
                                     const TransInfo *trans_infos2);


G_DEFINE_TYPE (GimpTransformGridTool, gimp_transform_grid_tool, GIMP_TYPE_TRANSFORM_TOOL)

#define parent_class gimp_transform_grid_tool_parent_class


static void
gimp_transform_grid_tool_class_init (GimpTransformGridToolClass *klass)
{
  GObjectClass           *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass          *tool_class   = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass      *draw_class   = GIMP_DRAW_TOOL_CLASS (klass);
  GimpTransformToolClass *tr_class     = GIMP_TRANSFORM_TOOL_CLASS (klass);

  object_class->finalize     = gimp_transform_grid_tool_finalize;

  tool_class->initialize     = gimp_transform_grid_tool_initialize;
  tool_class->control        = gimp_transform_grid_tool_control;
  tool_class->button_press   = gimp_transform_grid_tool_button_press;
  tool_class->button_release = gimp_transform_grid_tool_button_release;
  tool_class->motion         = gimp_transform_grid_tool_motion;
  tool_class->modifier_key   = gimp_transform_grid_tool_modifier_key;
  tool_class->cursor_update  = gimp_transform_grid_tool_cursor_update;
  tool_class->can_undo       = gimp_transform_grid_tool_can_undo;
  tool_class->can_redo       = gimp_transform_grid_tool_can_redo;
  tool_class->undo           = gimp_transform_grid_tool_undo;
  tool_class->redo           = gimp_transform_grid_tool_redo;
  tool_class->options_notify = gimp_transform_grid_tool_options_notify;

  draw_class->draw           = gimp_transform_grid_tool_draw;

  tr_class->recalc_matrix    = gimp_transform_grid_tool_recalc_matrix;
  tr_class->get_undo_desc    = gimp_transform_grid_tool_get_undo_desc;
  tr_class->get_direction    = gimp_transform_grid_tool_get_direction;
  tr_class->transform        = gimp_transform_grid_tool_transform;

  klass->info_to_matrix      = NULL;
  klass->matrix_to_info      = NULL;
  klass->get_undo_desc       = gimp_transform_grid_tool_real_get_undo_desc;
  klass->dialog              = NULL;
  klass->dialog_update       = NULL;
  klass->prepare             = NULL;
  klass->readjust            = NULL;
  klass->get_widget          = NULL;
  klass->update_widget       = gimp_transform_grid_tool_real_update_widget;
  klass->widget_changed      = gimp_transform_grid_tool_real_widget_changed;
  klass->transform           = gimp_transform_grid_tool_real_transform;

  klass->ok_button_label     = _("_Transform");
}

static void
gimp_transform_grid_tool_init (GimpTransformGridTool *tg_tool)
{
  GimpTool *tool = GIMP_TOOL (tg_tool);

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

  tg_tool->strokes = g_ptr_array_new ();
}

static void
gimp_transform_grid_tool_finalize (GObject *object)
{
  GimpTransformGridTool *tg_tool = GIMP_TRANSFORM_GRID_TOOL (object);

  g_clear_object (&tg_tool->gui);
  g_clear_pointer (&tg_tool->strokes, g_ptr_array_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_transform_grid_tool_initialize (GimpTool     *tool,
                                     GimpDisplay  *display,
                                     GError      **error)
{
  GimpTransformTool     *tr_tool  = GIMP_TRANSFORM_TOOL (tool);
  GimpTransformGridTool *tg_tool  = GIMP_TRANSFORM_GRID_TOOL (tool);
  GimpImage             *image    = gimp_display_get_image (display);
  GimpDrawable          *drawable = gimp_image_get_active_drawable (image);
  GimpItem              *item;
  UndoInfo              *undo_info;

  item = gimp_transform_tool_check_active_item (tr_tool, display, error);

  if (! item)
    return FALSE;

  tool->display  = display;
  tool->drawable = drawable;

  tr_tool->item  = item;

  /*  Initialize the transform_grid tool dialog  */
  if (! tg_tool->gui)
    gimp_transform_grid_tool_dialog (tg_tool);

  /*  Find the transform bounds for some tools (like scale,
   *  perspective) that actually need the bounds for initializing
   */
  gimp_transform_tool_bounds (tr_tool, display);

  /*  Initialize the tool-specific trans_info, and adjust the tool dialog  */
  gimp_transform_grid_tool_prepare (tg_tool, display);

  /*  Recalculate the tool's transformation matrix  */
  gimp_transform_tool_recalc_matrix (tr_tool, display);

  /*  Get the on-canvas gui  */
  tg_tool->widget = gimp_transform_grid_tool_get_widget (tg_tool);

  gimp_transform_grid_tool_hide_active_item (tg_tool, item);

  /*  start drawing the bounding box and handles...  */
  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);

  /* Initialize undo and redo lists */
  undo_info = undo_info_new ();
  tg_tool->undo_list = g_list_prepend (NULL, undo_info);
  tg_tool->redo_list = NULL;

  /*  Save the current transformation info  */
  memcpy (undo_info->trans_infos, tg_tool->trans_infos,
          sizeof (tg_tool->trans_infos));

  return TRUE;
}

static void
gimp_transform_grid_tool_control (GimpTool       *tool,
                                  GimpToolAction  action,
                                  GimpDisplay    *display)
{
  GimpTransformTool     *tr_tool = GIMP_TRANSFORM_TOOL (tool);
  GimpTransformGridTool *tg_tool = GIMP_TRANSFORM_GRID_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      break;

    case GIMP_TOOL_ACTION_RESUME:
      gimp_transform_tool_recalc_matrix (tr_tool, display);
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_transform_grid_tool_halt (tg_tool);
     break;

    case GIMP_TOOL_ACTION_COMMIT:
      if (tool->display)
        gimp_transform_grid_tool_commit (tg_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_transform_grid_tool_button_press (GimpTool            *tool,
                                       const GimpCoords    *coords,
                                       guint32              time,
                                       GdkModifierType      state,
                                       GimpButtonPressType  press_type,
                                       GimpDisplay         *display)
{
  GimpTransformGridTool *tg_tool = GIMP_TRANSFORM_GRID_TOOL (tool);

  if (tg_tool->widget)
    {
      gimp_tool_widget_hover (tg_tool->widget, coords, state, TRUE);

      if (gimp_tool_widget_button_press (tg_tool->widget, coords, time, state,
                                         press_type))
        {
          tg_tool->grab_widget = tg_tool->widget;
        }
    }

  gimp_tool_control_activate (tool->control);
}

static void
gimp_transform_grid_tool_button_release (GimpTool              *tool,
                                         const GimpCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         GimpButtonReleaseType  release_type,
                                         GimpDisplay           *display)
{
  GimpTransformTool     *tr_tool = GIMP_TRANSFORM_TOOL (tool);
  GimpTransformGridTool *tg_tool = GIMP_TRANSFORM_GRID_TOOL (tool);

  gimp_tool_control_halt (tool->control);

  if (tg_tool->grab_widget)
    {
      gimp_tool_widget_button_release (tg_tool->grab_widget,
                                       coords, time, state, release_type);
      tg_tool->grab_widget = NULL;
    }

  if (release_type != GIMP_BUTTON_RELEASE_CANCEL)
    {
      /* We're done with an interaction, save it on the undo list */
      gimp_transform_grid_tool_push_internal_undo (tg_tool);
    }
  else
    {
      UndoInfo *undo_info = tg_tool->undo_list->data;

      /*  Restore the last saved state  */
      memcpy (tg_tool->trans_infos, undo_info->trans_infos,
              sizeof (tg_tool->trans_infos));

      /*  recalculate the tool's transformation matrix  */
      gimp_transform_tool_recalc_matrix (tr_tool, display);
    }
}

static void
gimp_transform_grid_tool_motion (GimpTool         *tool,
                                 const GimpCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state,
                                 GimpDisplay      *display)
{
  GimpTransformGridTool *tg_tool = GIMP_TRANSFORM_GRID_TOOL (tool);

  if (tg_tool->grab_widget)
    {
      gimp_tool_widget_motion (tg_tool->grab_widget, coords, time, state);
    }
}

static void
gimp_transform_grid_tool_modifier_key (GimpTool        *tool,
                                       GdkModifierType  key,
                                       gboolean         press,
                                       GdkModifierType  state,
                                       GimpDisplay     *display)
{
  GimpTransformGridTool *tg_tool = GIMP_TRANSFORM_GRID_TOOL (tool);

  if (tg_tool->widget)
    {
      GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press,
                                                    state, display);
    }
  else
    {
      GimpTransformGridOptions *options = GIMP_TRANSFORM_GRID_TOOL_GET_OPTIONS (tool);

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
gimp_transform_grid_tool_cursor_update (GimpTool         *tool,
                                        const GimpCoords *coords,
                                        GdkModifierType   state,
                                        GimpDisplay      *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tool);

  if (display != tool->display &&
      ! gimp_transform_tool_check_active_item (tr_tool, display, NULL))
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
gimp_transform_grid_tool_can_undo (GimpTool    *tool,
                                   GimpDisplay *display)
{
  GimpTransformGridTool *tg_tool = GIMP_TRANSFORM_GRID_TOOL (tool);

  if (! tg_tool->undo_list || ! tg_tool->undo_list->next)
    return NULL;

  return _("Transform Step");
}

static const gchar *
gimp_transform_grid_tool_can_redo (GimpTool    *tool,
                                   GimpDisplay *display)
{
  GimpTransformGridTool *tg_tool = GIMP_TRANSFORM_GRID_TOOL (tool);

  if (! tg_tool->redo_list)
    return NULL;

  return _("Transform Step");
}

static gboolean
gimp_transform_grid_tool_undo (GimpTool    *tool,
                               GimpDisplay *display)
{
  GimpTransformTool      *tr_tool    = GIMP_TRANSFORM_TOOL (tool);
  GimpTransformGridTool  *tg_tool    = GIMP_TRANSFORM_GRID_TOOL (tool);
  GimpTransformOptions   *tr_options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);
  UndoInfo               *undo_info;
  GimpTransformDirection  direction;

  undo_info = tg_tool->undo_list->data;
  direction = undo_info->direction;

  /* Move undo_info from undo_list to redo_list */
  tg_tool->redo_list = g_list_prepend (tg_tool->redo_list, undo_info);
  tg_tool->undo_list = g_list_remove (tg_tool->undo_list, undo_info);

  undo_info = tg_tool->undo_list->data;

  /*  Restore the previous transformation info  */
  memcpy (tg_tool->trans_infos, undo_info->trans_infos,
          sizeof (tg_tool->trans_infos));

  /*  Restore the previous transformation direction  */
  if (direction != tr_options->direction)
    {
      g_object_set (tr_options,
                    "direction", direction,
                    NULL);
    }

  /*  recalculate the tool's transformation matrix  */
  gimp_transform_tool_recalc_matrix (tr_tool, display);

  return TRUE;
}

static gboolean
gimp_transform_grid_tool_redo (GimpTool    *tool,
                               GimpDisplay *display)
{
  GimpTransformTool      *tr_tool    = GIMP_TRANSFORM_TOOL (tool);
  GimpTransformGridTool  *tg_tool    = GIMP_TRANSFORM_GRID_TOOL (tool);
  GimpTransformOptions   *tr_options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);
  UndoInfo               *undo_info;
  GimpTransformDirection  direction;

  undo_info = tg_tool->redo_list->data;
  direction = undo_info->direction;

  /* Move undo_info from redo_list to undo_list */
  tg_tool->undo_list = g_list_prepend (tg_tool->undo_list, undo_info);
  tg_tool->redo_list = g_list_remove (tg_tool->redo_list, undo_info);

  /*  Restore the previous transformation info  */
  memcpy (tg_tool->trans_infos, undo_info->trans_infos,
          sizeof (tg_tool->trans_infos));

  /*  Restore the previous transformation direction  */
  if (direction != tr_options->direction)
    {
      g_object_set (tr_options,
                    "direction", direction,
                    NULL);
    }

  /*  recalculate the tool's transformation matrix  */
  gimp_transform_tool_recalc_matrix (tr_tool, display);

  return TRUE;
}

static void
gimp_transform_grid_tool_options_notify (GimpTool         *tool,
                                         GimpToolOptions  *options,
                                         const GParamSpec *pspec)
{
  GimpTransformTool        *tr_tool    = GIMP_TRANSFORM_TOOL (tool);
  GimpTransformGridTool    *tg_tool    = GIMP_TRANSFORM_GRID_TOOL (tool);
  GimpTransformGridOptions *tg_options = GIMP_TRANSFORM_GRID_OPTIONS (options);

  GIMP_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "type"))
    {
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
      return;
    }

  if (! tg_tool->widget)
    return;

  if (! strcmp (pspec->name, "direction"))
    {
      /*  recalculate the tool's transformation matrix  */
      gimp_transform_tool_recalc_matrix (tr_tool, tool->display);
    }
  else if (! strcmp (pspec->name, "show-preview"))
    {
      if (tg_tool->preview)
        {
          GimpDisplay *display;
          GimpItem    *item;
          gboolean     show_preview;

          show_preview = gimp_transform_grid_options_show_preview (tg_options) &&
                         tr_tool->transform_valid;

          gimp_canvas_item_set_visible (tg_tool->preview, show_preview);

          display = tool->display;
          item    = gimp_transform_tool_get_active_item (tr_tool, display);
          if (item)
            {
              if (gimp_transform_grid_options_show_preview (tg_options))
                gimp_transform_grid_tool_hide_active_item (tg_tool, item);
              else
                gimp_transform_grid_tool_show_active_item (tg_tool);
            }
        }
    }
  else if (g_str_has_prefix (pspec->name, "constrain-") ||
           g_str_has_prefix (pspec->name, "frompivot-") ||
           ! strcmp (pspec->name, "fixedpivot") ||
           ! strcmp (pspec->name, "cornersnap"))
    {
      gimp_transform_grid_tool_dialog_update (tg_tool);
    }
}

static void
gimp_transform_grid_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool                 *tool       = GIMP_TOOL (draw_tool);
  GimpTransformTool        *tr_tool    = GIMP_TRANSFORM_TOOL (draw_tool);
  GimpTransformGridTool    *tg_tool    = GIMP_TRANSFORM_GRID_TOOL (draw_tool);
  GimpTransformGridOptions *options    = GIMP_TRANSFORM_GRID_TOOL_GET_OPTIONS (tool);
  GimpTransformOptions     *tr_options = GIMP_TRANSFORM_OPTIONS (options);
  GimpImage                *image      = gimp_display_get_image (tool->display);
  GimpMatrix3               matrix     = tr_tool->transform;
  GimpCanvasItem           *item;

  if (tr_options->direction == GIMP_TRANSFORM_BACKWARD)
    gimp_matrix3_invert (&matrix);

  if (tg_tool->widget)
    {
      gboolean show_preview = gimp_transform_grid_options_show_preview (options) &&
                              tr_tool->transform_valid;

      tg_tool->preview =
        gimp_draw_tool_add_transform_preview (draw_tool,
                                              tool->drawable,
                                              &matrix,
                                              tr_tool->x1,
                                              tr_tool->y1,
                                              tr_tool->x2,
                                              tr_tool->y2);
      g_object_add_weak_pointer (G_OBJECT (tg_tool->preview),
                                 (gpointer) &tg_tool->preview);

      gimp_canvas_item_set_visible (tg_tool->preview, show_preview);

      g_object_bind_property (G_OBJECT (options),          "preview-opacity",
                              G_OBJECT (tg_tool->preview), "opacity",
                              G_BINDING_SYNC_CREATE |
                              G_BINDING_BIDIRECTIONAL);

      GIMP_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
    }

  if (tr_options->type == GIMP_TRANSFORM_TYPE_SELECTION)
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
          tg_tool->boundary_in =
            gimp_draw_tool_add_boundary (draw_tool,
                                         segs_in, n_segs_in,
                                         &matrix,
                                         0, 0);
          g_object_add_weak_pointer (G_OBJECT (tg_tool->boundary_in),
                                     (gpointer) &tg_tool->boundary_in);

          gimp_canvas_item_set_visible (tg_tool->boundary_in,
                                        tr_tool->transform_valid);
        }

      if (segs_out)
        {
          tg_tool->boundary_out =
            gimp_draw_tool_add_boundary (draw_tool,
                                         segs_out, n_segs_out,
                                         &matrix,
                                         0, 0);
          g_object_add_weak_pointer (G_OBJECT (tg_tool->boundary_out),
                                     (gpointer) &tg_tool->boundary_out);

          gimp_canvas_item_set_visible (tg_tool->boundary_out,
                                        tr_tool->transform_valid);
        }
    }
  else if (tr_options->type == GIMP_TRANSFORM_TYPE_PATH)
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

                  g_ptr_array_add (tg_tool->strokes, item);
                  g_object_weak_ref (G_OBJECT (item),
                                     (GWeakNotify) g_ptr_array_remove,
                                     tg_tool->strokes);

                  gimp_canvas_item_set_visible (item, tr_tool->transform_valid);
                }

              if (coords)
                g_array_free (coords, TRUE);
            }
        }
    }
}

static void
gimp_transform_grid_tool_recalc_matrix (GimpTransformTool *tr_tool)
{
  GimpTransformGridTool    *tg_tool    = GIMP_TRANSFORM_GRID_TOOL (tr_tool);
  GimpTransformOptions     *tr_options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  GimpTransformGridOptions *tg_options = GIMP_TRANSFORM_GRID_TOOL_GET_OPTIONS (tr_tool);

  if (GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->info_to_matrix)
    {
      GimpMatrix3 forward_transform;
      GimpMatrix3 backward_transform;
      gboolean    forward_transform_valid;
      gboolean    backward_transform_valid;

      tg_tool->trans_info      = tg_tool->trans_infos[GIMP_TRANSFORM_FORWARD];
      forward_transform_valid  = gimp_transform_grid_tool_info_to_matrix (
                                   tg_tool, &forward_transform);

      tg_tool->trans_info      = tg_tool->trans_infos[GIMP_TRANSFORM_BACKWARD];
      backward_transform_valid = gimp_transform_grid_tool_info_to_matrix (
                                   tg_tool, &backward_transform);

      if (GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info &&
          tg_options->direction_linked)
        {
          GimpMatrix3 transform = tr_tool->transform;

          switch (tr_options->direction)
            {
            case GIMP_TRANSFORM_FORWARD:
              if (forward_transform_valid)
                {
                  gimp_matrix3_invert (&transform);

                  backward_transform = forward_transform;
                  gimp_matrix3_mult (&transform, &backward_transform);

                  tg_tool->trans_info =
                    tg_tool->trans_infos[GIMP_TRANSFORM_BACKWARD];
                  gimp_transform_grid_tool_matrix_to_info (tg_tool,
                                                           &backward_transform);
                  backward_transform_valid =
                    gimp_transform_grid_tool_info_to_matrix (
                      tg_tool, &backward_transform);
                }
              break;

            case GIMP_TRANSFORM_BACKWARD:
              if (backward_transform_valid)
                {
                  forward_transform = backward_transform;
                  gimp_matrix3_mult (&transform, &forward_transform);

                  tg_tool->trans_info =
                    tg_tool->trans_infos[GIMP_TRANSFORM_FORWARD];
                  gimp_transform_grid_tool_matrix_to_info (tg_tool,
                                                           &forward_transform);
                  forward_transform_valid =
                    gimp_transform_grid_tool_info_to_matrix (
                      tg_tool, &forward_transform);
                }
              break;
            }
        }
      else if (forward_transform_valid && backward_transform_valid)
        {
          tr_tool->transform = backward_transform;
          gimp_matrix3_invert (&tr_tool->transform);
          gimp_matrix3_mult (&forward_transform, &tr_tool->transform);
        }

      tr_tool->transform_valid = forward_transform_valid &&
                                 backward_transform_valid;
    }

  tg_tool->trans_info = tg_tool->trans_infos[tr_options->direction];

  gimp_transform_grid_tool_dialog_update (tg_tool);
  gimp_transform_grid_tool_update_sensitivity (tg_tool);
  gimp_transform_grid_tool_update_widget (tg_tool);
  gimp_transform_grid_tool_update_preview (tg_tool);

  if (tg_tool->gui)
    gimp_tool_gui_show (tg_tool->gui);
}

static gchar *
gimp_transform_grid_tool_get_undo_desc (GimpTransformTool *tr_tool)
{
  GimpTransformGridTool *tg_tool    = GIMP_TRANSFORM_GRID_TOOL (tr_tool);
  GimpTransformOptions  *tr_options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  gchar                 *result;

  if (GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info)
    {
      TransInfo trans_info;

      memcpy (&trans_info, &tg_tool->init_trans_info, sizeof (TransInfo));

      tg_tool->trans_info = trans_info;
      gimp_transform_grid_tool_matrix_to_info (tg_tool, &tr_tool->transform);
      result = GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->get_undo_desc (
        tg_tool);
    }
  else if (trans_info_equal (tg_tool->trans_infos[GIMP_TRANSFORM_BACKWARD],
                             tg_tool->init_trans_info))
    {
      tg_tool->trans_info = tg_tool->trans_infos[GIMP_TRANSFORM_FORWARD];
      result = GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->get_undo_desc (
        tg_tool);
    }
  else if (trans_info_equal (tg_tool->trans_infos[GIMP_TRANSFORM_FORWARD],
                             tg_tool->init_trans_info))
    {
      gchar *desc;

      tg_tool->trans_info = tg_tool->trans_infos[GIMP_TRANSFORM_BACKWARD];
      desc = GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->get_undo_desc (
        tg_tool);

      result = g_strdup_printf (_("%s (Corrective)"), desc);

      g_free (desc);
    }
  else
    {
      result = GIMP_TRANSFORM_TOOL_CLASS (parent_class)->get_undo_desc (
        tr_tool);
    }

  tg_tool->trans_info = tg_tool->trans_infos[tr_options->direction];

  return result;
}

static GimpTransformDirection
gimp_transform_grid_tool_get_direction (GimpTransformTool *tr_tool)
{
  return GIMP_TRANSFORM_FORWARD;
}

static GeglBuffer *
gimp_transform_grid_tool_transform (GimpTransformTool  *tr_tool,
                                    GimpItem           *item,
                                    GeglBuffer         *orig_buffer,
                                    gint                orig_offset_x,
                                    gint                orig_offset_y,
                                    GimpColorProfile  **buffer_profile,
                                    gint               *new_offset_x,
                                    gint               *new_offset_y)
{
  GimpTool              *tool    = GIMP_TOOL (tr_tool);
  GimpTransformGridTool *tg_tool = GIMP_TRANSFORM_GRID_TOOL (tr_tool);
  GimpDisplay           *display = tool->display;
  GimpImage             *image   = gimp_display_get_image (display);
  GeglBuffer            *new_buffer;

  /*  Send the request for the transformation to the tool...
   */
  new_buffer =
    GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->transform (tg_tool,
                                                             item,
                                                             orig_buffer,
                                                             orig_offset_x,
                                                             orig_offset_y,
                                                             buffer_profile,
                                                             new_offset_x,
                                                             new_offset_y);

  gimp_image_undo_push (image, GIMP_TYPE_TRANSFORM_GRID_TOOL_UNDO,
                        GIMP_UNDO_TRANSFORM_GRID, NULL,
                        0,
                        "transform-tool", tg_tool,
                        NULL);

  return new_buffer;
}

static gchar *
gimp_transform_grid_tool_real_get_undo_desc (GimpTransformGridTool *tg_tool)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

  return GIMP_TRANSFORM_TOOL_CLASS (parent_class)->get_undo_desc (tr_tool);
}

static void
gimp_transform_grid_tool_real_update_widget (GimpTransformGridTool *tg_tool)
{
  if (GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->info_to_matrix)
    {
      GimpMatrix3 transform;

      gimp_transform_grid_tool_info_to_matrix (tg_tool, &transform);

      g_object_set (tg_tool->widget,
                    "transform", &transform,
                    NULL);
    }
}

static void
gimp_transform_grid_tool_real_widget_changed (GimpTransformGridTool *tg_tool)
{
  GimpTool          *tool    = GIMP_TOOL (tg_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  GimpToolWidget    *widget  = tg_tool->widget;

  /* suppress the call to GimpTransformGridTool::update_widget() when
   * recalculating the matrix
   */
  tg_tool->widget = NULL;

  gimp_transform_tool_recalc_matrix (tr_tool, tool->display);

  tg_tool->widget = widget;
}

static GeglBuffer *
gimp_transform_grid_tool_real_transform (GimpTransformGridTool  *tg_tool,
                                         GimpItem               *item,
                                         GeglBuffer             *orig_buffer,
                                         gint                    orig_offset_x,
                                         gint                    orig_offset_y,
                                         GimpColorProfile      **buffer_profile,
                                         gint                   *new_offset_x,
                                         gint                   *new_offset_y)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

  return GIMP_TRANSFORM_TOOL_CLASS (parent_class)->transform (tr_tool,
                                                              item,
                                                              orig_buffer,
                                                              orig_offset_x,
                                                              orig_offset_y,
                                                              buffer_profile,
                                                              new_offset_x,
                                                              new_offset_y);
}

static void
gimp_transform_grid_tool_widget_changed (GimpToolWidget        *widget,
                                         GimpTransformGridTool *tg_tool)
{
  if (GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->widget_changed)
    GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->widget_changed (tg_tool);
}

static void
gimp_transform_grid_tool_widget_response (GimpToolWidget        *widget,
                                          gint                   response_id,
                                          GimpTransformGridTool *tg_tool)
{
  switch (response_id)
    {
    case GIMP_TOOL_WIDGET_RESPONSE_CONFIRM:
      gimp_transform_grid_tool_response (NULL, GTK_RESPONSE_OK, tg_tool);
      break;

    case GIMP_TOOL_WIDGET_RESPONSE_CANCEL:
      gimp_transform_grid_tool_response (NULL, GTK_RESPONSE_CANCEL, tg_tool);
      break;

    case GIMP_TOOL_WIDGET_RESPONSE_RESET:
      gimp_transform_grid_tool_response (NULL, RESPONSE_RESET, tg_tool);
      break;
    }
}

static void
gimp_transform_grid_tool_halt (GimpTransformGridTool *tg_tool)
{
  GimpTool          *tool    = GIMP_TOOL (tg_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tg_tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (tg_tool));

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tg_tool), NULL);
  g_clear_object (&tg_tool->widget);

  if (tg_tool->gui)
    gimp_tool_gui_hide (tg_tool->gui);

  if (tg_tool->redo_list)
    {
      g_list_free_full (tg_tool->redo_list, (GDestroyNotify) undo_info_free);
      tg_tool->redo_list = NULL;
    }

  if (tg_tool->undo_list)
    {
      g_list_free_full (tg_tool->undo_list, (GDestroyNotify) undo_info_free);
      tg_tool->undo_list = NULL;
    }

  gimp_transform_grid_tool_show_active_item (tg_tool);

  tool->display  = NULL;
  tool->drawable = NULL;

  tr_tool->item  = NULL;
}

static void
gimp_transform_grid_tool_commit (GimpTransformGridTool *tg_tool)
{
  GimpTool          *tool    = GIMP_TOOL (tg_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  GimpDisplay       *display = tool->display;

  /* undraw the tool before we muck around with the transform matrix */
  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tg_tool));

  gimp_transform_tool_transform (tr_tool, display);
}

static void
gimp_transform_grid_tool_dialog (GimpTransformGridTool *tg_tool)
{
  GimpTool         *tool      = GIMP_TOOL (tg_tool);
  GimpToolInfo     *tool_info = tool->tool_info;
  GimpDisplayShell *shell;
  const gchar      *ok_button_label;

  if (! GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->dialog)
    return;

  g_return_if_fail (tool->display != NULL);

  shell = gimp_display_get_shell (tool->display);

  ok_button_label = GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->ok_button_label;

  tg_tool->gui = gimp_tool_gui_new (tool_info,
                                    NULL, NULL, NULL, NULL,
                                    gtk_widget_get_screen (GTK_WIDGET (shell)),
                                    gimp_widget_get_monitor (GTK_WIDGET (shell)),
                                    TRUE,
                                    NULL);

  gimp_tool_gui_add_button   (tg_tool->gui, _("_Reset"),     RESPONSE_RESET);
  if (GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->readjust)
    gimp_tool_gui_add_button (tg_tool->gui, _("Re_adjust"),  RESPONSE_READJUST);
  gimp_tool_gui_add_button   (tg_tool->gui, _("_Cancel"),    GTK_RESPONSE_CANCEL);
  gimp_tool_gui_add_button   (tg_tool->gui, ok_button_label, GTK_RESPONSE_OK);

  gimp_tool_gui_set_auto_overlay (tg_tool->gui, TRUE);
  gimp_tool_gui_set_default_response (tg_tool->gui, GTK_RESPONSE_OK);

  gimp_tool_gui_set_alternative_button_order (tg_tool->gui,
                                              RESPONSE_RESET,
                                              RESPONSE_READJUST,
                                              GTK_RESPONSE_OK,
                                              GTK_RESPONSE_CANCEL,
                                              -1);

  g_signal_connect (tg_tool->gui, "response",
                    G_CALLBACK (gimp_transform_grid_tool_response),
                    tg_tool);

  GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->dialog (tg_tool);
}

static void
gimp_transform_grid_tool_dialog_update (GimpTransformGridTool *tg_tool)
{
  if (tg_tool->gui &&
      GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->dialog_update)
    {
      GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->dialog_update (tg_tool);
    }
}

static void
gimp_transform_grid_tool_prepare (GimpTransformGridTool *tg_tool,
                                  GimpDisplay           *display)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

  if (tg_tool->gui)
    {
      GimpItem *item = gimp_transform_tool_get_active_item (tr_tool, display);

      gimp_tool_gui_set_shell (tg_tool->gui, gimp_display_get_shell (display));
      gimp_tool_gui_set_viewable (tg_tool->gui, GIMP_VIEWABLE (item));
    }

  if (GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->prepare)
    {
      tg_tool->trans_info = tg_tool->init_trans_info;
      GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->prepare (tg_tool);

      memcpy (tg_tool->trans_infos[GIMP_TRANSFORM_FORWARD],
              tg_tool->init_trans_info, sizeof (TransInfo));
      memcpy (tg_tool->trans_infos[GIMP_TRANSFORM_BACKWARD],
              tg_tool->init_trans_info, sizeof (TransInfo));
    }

  gimp_matrix3_identity (&tr_tool->transform);
  tr_tool->transform_valid = TRUE;
}

static GimpToolWidget *
gimp_transform_grid_tool_get_widget (GimpTransformGridTool *tg_tool)
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

  if (GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->get_widget)
    {
      GimpTransformGridOptions *options = GIMP_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
      gint                      i;

      widget = GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->get_widget (tg_tool);

      gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tg_tool), widget);

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
                        G_CALLBACK (gimp_transform_grid_tool_widget_changed),
                        tg_tool);
      g_signal_connect (widget, "response",
                        G_CALLBACK (gimp_transform_grid_tool_widget_response),
                        tg_tool);
    }

  return widget;
}

static void
gimp_transform_grid_tool_update_widget (GimpTransformGridTool *tg_tool)
{
  if (tg_tool->widget &&
      GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->update_widget)
    {
      g_signal_handlers_block_by_func (
        tg_tool->widget,
        G_CALLBACK (gimp_transform_grid_tool_widget_changed),
        tg_tool);

      GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->update_widget (tg_tool);

      g_signal_handlers_unblock_by_func (
        tg_tool->widget,
        G_CALLBACK (gimp_transform_grid_tool_widget_changed),
        tg_tool);
    }
}

static void
gimp_transform_grid_tool_response (GimpToolGui           *gui,
                                   gint                   response_id,
                                   GimpTransformGridTool *tg_tool)
{
  GimpTool                 *tool       = GIMP_TOOL (tg_tool);
  GimpTransformTool        *tr_tool    = GIMP_TRANSFORM_TOOL (tg_tool);
  GimpTransformOptions     *tr_options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tg_tool);
  GimpTransformGridOptions *tg_options = GIMP_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  GimpDisplay              *display    = tool->display;

  switch (response_id)
    {
    case RESPONSE_RESET:
      {
        gboolean direction_linked;

        /*  restore the initial transformation info  */
        memcpy (tg_tool->trans_infos[GIMP_TRANSFORM_FORWARD],
                tg_tool->init_trans_info,
                sizeof (TransInfo));
        memcpy (tg_tool->trans_infos[GIMP_TRANSFORM_BACKWARD],
                tg_tool->init_trans_info,
                sizeof (TransInfo));

        /*  recalculate the tool's transformation matrix  */
        direction_linked             = tg_options->direction_linked;
        tg_options->direction_linked = FALSE;
        gimp_transform_tool_recalc_matrix (tr_tool, display);
        tg_options->direction_linked = direction_linked;

        /*  push the restored info to the undo stack  */
        gimp_transform_grid_tool_push_internal_undo (tg_tool);
      }
      break;

    case RESPONSE_READJUST:
      if (GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->readjust       &&
          GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info &&
          tr_tool->transform_valid)
        {
          TransInfo old_trans_infos[2];
          gboolean  direction_linked;
          gboolean  transform_valid;

          /*  save the current transformation info  */
          memcpy (old_trans_infos, tg_tool->trans_infos,
                  sizeof (old_trans_infos));

          /*  readjust the transformation info to view  */
          GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->readjust (tg_tool);

          /*  recalculate the tool's transformation matrix, preserving the
           *  overall transformation
           */
          direction_linked             = tg_options->direction_linked;
          tg_options->direction_linked = TRUE;
          gimp_transform_tool_recalc_matrix (tr_tool, display);
          tg_options->direction_linked = direction_linked;

          transform_valid = tr_tool->transform_valid;

          /*  if the resulting transformation is invalid, or if the
           *  transformation info is already adjusted to view ...
           */
          if (! transform_valid ||
              trans_infos_equal (old_trans_infos, tg_tool->trans_infos))
            {
              /*  ... readjust the transformation info to the item bounds  */
              GimpMatrix3 transform = tr_tool->transform;

              if (tr_options->direction == GIMP_TRANSFORM_BACKWARD)
                gimp_matrix3_invert (&transform);

              memcpy (tg_tool->trans_info, tg_tool->init_trans_info,
                      sizeof (TransInfo));

              GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info (
                tg_tool, &transform);

              /*  recalculate the tool's transformation matrix, preserving the
               *  overall transformation
               */
              direction_linked             = tg_options->direction_linked;
              tg_options->direction_linked = TRUE;
              gimp_transform_tool_recalc_matrix (tr_tool, display);
              tg_options->direction_linked = direction_linked;

              if (! tr_tool->transform_valid ||
                  ! trans_infos_equal (old_trans_infos, tg_tool->trans_infos))
                {
                  transform_valid = tr_tool->transform_valid;
                }
            }

          if (transform_valid)
            {
              /*  push the new info to the undo stack  */
              gimp_transform_grid_tool_push_internal_undo (tg_tool);
            }
          else
            {
              /*  restore the old transformation info  */
              memcpy (tg_tool->trans_infos, old_trans_infos,
                      sizeof (old_trans_infos));

              /*  recalculate the tool's transformation matrix  */
              direction_linked             = tg_options->direction_linked;
              tg_options->direction_linked = FALSE;
              gimp_transform_tool_recalc_matrix (tr_tool, display);
              tg_options->direction_linked = direction_linked;

              gimp_tool_message_literal (tool, tool->display,
                                         _("Cannot readjust the transformation"));
            }
        }
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
gimp_transform_grid_tool_update_sensitivity (GimpTransformGridTool *tg_tool)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);

  if (! tg_tool->gui)
    return;

  gimp_tool_gui_set_response_sensitive (
    tg_tool->gui, GTK_RESPONSE_OK,
    tr_tool->transform_valid);

  gimp_tool_gui_set_response_sensitive (
    tg_tool->gui, RESPONSE_RESET,
    ! (trans_info_equal (tg_tool->trans_infos[GIMP_TRANSFORM_FORWARD],
                         tg_tool->init_trans_info) &&
       trans_info_equal (tg_tool->trans_infos[GIMP_TRANSFORM_BACKWARD],
                         tg_tool->init_trans_info)));

  gimp_tool_gui_set_response_sensitive (
    tg_tool->gui, RESPONSE_READJUST,
    tr_tool->transform_valid);
}

static void
gimp_transform_grid_tool_update_preview (GimpTransformGridTool *tg_tool)
{
  GimpTransformTool        *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  GimpTransformGridOptions *options = GIMP_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  gint                      i;

  if (tg_tool->preview)
    {
      gboolean show_preview = gimp_transform_grid_options_show_preview (options) &&
                              tr_tool->transform_valid;

      gimp_canvas_item_begin_change (tg_tool->preview);
      gimp_canvas_item_set_visible (tg_tool->preview, show_preview);
      g_object_set (tg_tool->preview,
                    "transform", &tr_tool->transform,
                    NULL);
      gimp_canvas_item_end_change (tg_tool->preview);
    }

  if (tg_tool->boundary_in)
    {
      gimp_canvas_item_begin_change (tg_tool->boundary_in);
      gimp_canvas_item_set_visible (tg_tool->boundary_in,
                                    tr_tool->transform_valid);
      g_object_set (tg_tool->boundary_in,
                    "transform", &tr_tool->transform,
                    NULL);
      gimp_canvas_item_end_change (tg_tool->boundary_in);
    }

  if (tg_tool->boundary_out)
    {
      gimp_canvas_item_begin_change (tg_tool->boundary_out);
      gimp_canvas_item_set_visible (tg_tool->boundary_out,
                                    tr_tool->transform_valid);
      g_object_set (tg_tool->boundary_out,
                    "transform", &tr_tool->transform,
                    NULL);
      gimp_canvas_item_end_change (tg_tool->boundary_out);
    }

  for (i = 0; i < tg_tool->strokes->len; i++)
    {
      GimpCanvasItem *item = g_ptr_array_index (tg_tool->strokes, i);

      gimp_canvas_item_begin_change (item);
      gimp_canvas_item_set_visible (item, tr_tool->transform_valid);
      g_object_set (item,
                    "transform", &tr_tool->transform,
                    NULL);
      gimp_canvas_item_end_change (item);
    }
}

static void
gimp_transform_grid_tool_hide_active_item (GimpTransformGridTool *tg_tool,
                                           GimpItem          *item)
{
  GimpTransformGridOptions *options    = GIMP_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  GimpTransformOptions     *tr_options = GIMP_TRANSFORM_OPTIONS (options);
  GimpDisplay              *display    = GIMP_TOOL (tg_tool)->display;
  GimpImage                *image      = gimp_display_get_image (display);

  /*  hide only complete layers and channels, not layer masks  */
  if (tr_options->type == GIMP_TRANSFORM_TYPE_LAYER &&
      options->show_preview                         &&
      GIMP_IS_DRAWABLE (item)                       &&
      ! GIMP_IS_LAYER_MASK (item)                   &&
      gimp_item_get_visible (item)                  &&
      gimp_channel_is_empty (gimp_image_get_mask (image)))
    {
      tg_tool->hidden_item = item;
      gimp_item_set_visible (item, FALSE, FALSE);

      gimp_projection_flush (gimp_image_get_projection (image));
    }
}

static void
gimp_transform_grid_tool_show_active_item (GimpTransformGridTool *tg_tool)
{
  if (tg_tool->hidden_item)
    {
      GimpDisplay *display = GIMP_TOOL (tg_tool)->display;
      GimpImage   *image   = gimp_display_get_image (display);

      gimp_item_set_visible (tg_tool->hidden_item, TRUE, FALSE);
      tg_tool->hidden_item = NULL;

      gimp_image_flush (image);
    }
}

static UndoInfo *
undo_info_new (void)
{
  return g_slice_new0 (UndoInfo);
}

static void
undo_info_free (UndoInfo *info)
{
  g_slice_free (UndoInfo, info);
}

static gboolean
trans_info_equal (const TransInfo trans_info1,
                  const TransInfo trans_info2)
{
  gint i;

  for (i = 0; i < TRANS_INFO_SIZE; i++)
    {
      if (fabs (trans_info1[i] - trans_info2[i]) > EPSILON)
        return FALSE;
    }

  return TRUE;
}

static gboolean
trans_infos_equal (const TransInfo *trans_infos1,
                   const TransInfo *trans_infos2)
{
  return trans_info_equal (trans_infos1[GIMP_TRANSFORM_FORWARD],
                           trans_infos2[GIMP_TRANSFORM_FORWARD]) &&
         trans_info_equal (trans_infos1[GIMP_TRANSFORM_BACKWARD],
                           trans_infos2[GIMP_TRANSFORM_BACKWARD]);
}

gboolean
gimp_transform_grid_tool_info_to_matrix (GimpTransformGridTool *tg_tool,
                                         GimpMatrix3           *transform)
{
  g_return_val_if_fail (GIMP_IS_TRANSFORM_GRID_TOOL (tg_tool), FALSE);
  g_return_val_if_fail (transform != NULL, FALSE);

  if (GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->info_to_matrix)
    {
      return GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->info_to_matrix (
        tg_tool, transform);
    }

  return FALSE;
}

void
gimp_transform_grid_tool_matrix_to_info (GimpTransformGridTool *tg_tool,
                                         const GimpMatrix3     *transform)
{
  g_return_if_fail (GIMP_IS_TRANSFORM_GRID_TOOL (tg_tool));
  g_return_if_fail (transform != NULL);

  if (GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info)
    {
      return GIMP_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info (
        tg_tool, transform);
    }
}

void
gimp_transform_grid_tool_push_internal_undo (GimpTransformGridTool *tg_tool)
{
  UndoInfo *undo_info;

  g_return_if_fail (GIMP_IS_TRANSFORM_GRID_TOOL (tg_tool));
  g_return_if_fail (tg_tool->undo_list != NULL);

  undo_info = tg_tool->undo_list->data;

  /* push current state on the undo list and set this state as the
   * current state, but avoid doing this if there were no changes
   */
  if (! trans_infos_equal (undo_info->trans_infos, tg_tool->trans_infos))
    {
      GimpTransformOptions *tr_options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tg_tool);

      undo_info            = undo_info_new ();
      undo_info->direction = tr_options->direction;
      memcpy (undo_info->trans_infos, tg_tool->trans_infos,
              sizeof (tg_tool->trans_infos));

      tg_tool->undo_list = g_list_prepend (tg_tool->undo_list, undo_info);

      /* If we undid anything and started interacting, we have to
       * discard the redo history
       */
      g_list_free_full (tg_tool->redo_list, (GDestroyNotify) undo_info_free);
      tg_tool->redo_list = NULL;

      gimp_transform_grid_tool_update_sensitivity (tg_tool);

      /*  update the undo actions / menu items  */
      gimp_image_flush (gimp_display_get_image (GIMP_TOOL (tg_tool)->display));
    }
}
