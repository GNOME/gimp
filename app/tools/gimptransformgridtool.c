/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "gegl/ligmaapplicator.h"
#include "gegl/ligma-gegl-nodes.h"
#include "gegl/ligma-gegl-utils.h"

#include "core/ligma.h"
#include "core/ligma-transform-resize.h"
#include "core/ligma-transform-utils.h"
#include "core/ligmaboundary.h"
#include "core/ligmacontainer.h"
#include "core/ligmadrawablefilter.h"
#include "core/ligmaerror.h"
#include "core/ligmafilter.h"
#include "core/ligmagrouplayer.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-item-list.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaimage-undo-push.h"
#include "core/ligmalayer.h"
#include "core/ligmalayermask.h"
#include "core/ligmapickable.h"
#include "core/ligmaprojection.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmaviewable.h"

#include "vectors/ligmavectors.h"
#include "vectors/ligmastroke.h"

#include "widgets/ligmawidgets-utils.h"

#include "display/ligmacanvasitem.h"
#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmatoolgui.h"
#include "display/ligmatoolwidget.h"

#include "ligmatoolcontrol.h"
#include "ligmatransformgridoptions.h"
#include "ligmatransformgridtool.h"
#include "ligmatransformgridtoolundo.h"
#include "ligmatransformoptions.h"

#include "ligma-intl.h"


#define EPSILON 1e-6


#define RESPONSE_RESET     1
#define RESPONSE_READJUST  2

#define UNDO_COMPRESS_TIME (0.5 * G_TIME_SPAN_SECOND)


typedef struct
{
  LigmaTransformGridTool *tg_tool;

  LigmaDrawable          *drawable;
  LigmaDrawableFilter    *filter;

  LigmaDrawable          *root_drawable;

  GeglNode              *transform_node;
  GeglNode              *crop_node;

  LigmaMatrix3            transform;
  GeglRectangle          bounds;
} Filter;

typedef struct
{
  LigmaTransformGridTool *tg_tool;
  LigmaDrawable          *root_drawable;
} AddFilterData;

typedef struct
{
  gint64                 time;
  LigmaTransformDirection direction;
  TransInfo              trans_infos[2];
} UndoInfo;


static void      ligma_transform_grid_tool_finalize           (GObject                *object);

static gboolean  ligma_transform_grid_tool_initialize         (LigmaTool               *tool,
                                                              LigmaDisplay            *display,
                                                              GError                **error);
static void      ligma_transform_grid_tool_control            (LigmaTool               *tool,
                                                              LigmaToolAction          action,
                                                              LigmaDisplay            *display);
static void      ligma_transform_grid_tool_button_press       (LigmaTool               *tool,
                                                              const LigmaCoords       *coords,
                                                              guint32                 time,
                                                              GdkModifierType         state,
                                                              LigmaButtonPressType     press_type,
                                                              LigmaDisplay            *display);
static void      ligma_transform_grid_tool_button_release     (LigmaTool               *tool,
                                                              const LigmaCoords       *coords,
                                                              guint32                 time,
                                                              GdkModifierType         state,
                                                              LigmaButtonReleaseType   release_type,
                                                              LigmaDisplay            *display);
static void      ligma_transform_grid_tool_motion             (LigmaTool               *tool,
                                                              const LigmaCoords       *coords,
                                                              guint32                 time,
                                                              GdkModifierType         state,
                                                              LigmaDisplay            *display);
static void      ligma_transform_grid_tool_modifier_key       (LigmaTool               *tool,
                                                              GdkModifierType         key,
                                                              gboolean                press,
                                                              GdkModifierType         state,
                                                              LigmaDisplay            *display);
static void      ligma_transform_grid_tool_cursor_update      (LigmaTool               *tool,
                                                              const LigmaCoords       *coords,
                                                              GdkModifierType         state,
                                                              LigmaDisplay            *display);
static const gchar * ligma_transform_grid_tool_can_undo       (LigmaTool               *tool,
                                                              LigmaDisplay            *display);
static const gchar * ligma_transform_grid_tool_can_redo       (LigmaTool               *tool,
                                                              LigmaDisplay            *display);
static gboolean  ligma_transform_grid_tool_undo               (LigmaTool               *tool,
                                                              LigmaDisplay            *display);
static gboolean  ligma_transform_grid_tool_redo               (LigmaTool               *tool,
                                                              LigmaDisplay            *display);
static void      ligma_transform_grid_tool_options_notify     (LigmaTool               *tool,
                                                              LigmaToolOptions        *options,
                                                              const GParamSpec       *pspec);

static void      ligma_transform_grid_tool_draw               (LigmaDrawTool           *draw_tool);

static void      ligma_transform_grid_tool_recalc_matrix      (LigmaTransformTool      *tr_tool);
static gchar   * ligma_transform_grid_tool_get_undo_desc      (LigmaTransformTool      *tr_tool);
static LigmaTransformDirection ligma_transform_grid_tool_get_direction
                                                             (LigmaTransformTool      *tr_tool);
static GeglBuffer * ligma_transform_grid_tool_transform       (LigmaTransformTool      *tr_tool,
                                                              GList                  *objects,
                                                              GeglBuffer             *orig_buffer,
                                                              gint                    orig_offset_x,
                                                              gint                    orig_offset_y,
                                                              LigmaColorProfile      **buffer_profile,
                                                              gint                   *new_offset_x,
                                                              gint                   *new_offset_y);

static void     ligma_transform_grid_tool_real_apply_info     (LigmaTransformGridTool  *tg_tool,
                                                              const TransInfo         info);
static gchar  * ligma_transform_grid_tool_real_get_undo_desc  (LigmaTransformGridTool  *tg_tool);
static void     ligma_transform_grid_tool_real_update_widget  (LigmaTransformGridTool  *tg_tool);
static void     ligma_transform_grid_tool_real_widget_changed (LigmaTransformGridTool  *tg_tool);
static GeglBuffer * ligma_transform_grid_tool_real_transform  (LigmaTransformGridTool  *tg_tool,
                                                              GList                  *objects,
                                                              GeglBuffer             *orig_buffer,
                                                              gint                    orig_offset_x,
                                                              gint                    orig_offset_y,
                                                              LigmaColorProfile      **buffer_profile,
                                                              gint                   *new_offset_x,
                                                              gint                   *new_offset_y);

static void      ligma_transform_grid_tool_widget_changed     (LigmaToolWidget         *widget,
                                                              LigmaTransformGridTool  *tg_tool);
static void      ligma_transform_grid_tool_widget_response    (LigmaToolWidget         *widget,
                                                              gint                    response_id,
                                                              LigmaTransformGridTool  *tg_tool);

static void      ligma_transform_grid_tool_filter_flush       (LigmaDrawableFilter     *filter,
                                                              LigmaTransformGridTool  *tg_tool);

static void      ligma_transform_grid_tool_halt               (LigmaTransformGridTool  *tg_tool);
static void      ligma_transform_grid_tool_commit             (LigmaTransformGridTool  *tg_tool);

static void      ligma_transform_grid_tool_dialog             (LigmaTransformGridTool  *tg_tool);
static void      ligma_transform_grid_tool_dialog_update      (LigmaTransformGridTool  *tg_tool);
static void      ligma_transform_grid_tool_prepare            (LigmaTransformGridTool  *tg_tool,
                                                              LigmaDisplay            *display);
static LigmaToolWidget * ligma_transform_grid_tool_get_widget  (LigmaTransformGridTool  *tg_tool);
static void      ligma_transform_grid_tool_update_widget      (LigmaTransformGridTool  *tg_tool);

static void      ligma_transform_grid_tool_response           (LigmaToolGui            *gui,
                                                              gint                    response_id,
                                                              LigmaTransformGridTool  *tg_tool);

static gboolean  ligma_transform_grid_tool_composited_preview (LigmaTransformGridTool  *tg_tool);
static void      ligma_transform_grid_tool_update_sensitivity (LigmaTransformGridTool  *tg_tool);
static void      ligma_transform_grid_tool_update_preview     (LigmaTransformGridTool  *tg_tool);
static void      ligma_transform_grid_tool_update_filters     (LigmaTransformGridTool  *tg_tool);
static void   ligma_transform_grid_tool_hide_selected_objects (LigmaTransformGridTool  *tg_tool,
                                                              GList                  *objects);
static void   ligma_transform_grid_tool_show_selected_objects (LigmaTransformGridTool  *tg_tool);

static void      ligma_transform_grid_tool_add_filter         (LigmaDrawable           *drawable,
                                                              AddFilterData          *data);
static void      ligma_transform_grid_tool_remove_filter      (LigmaDrawable           *drawable,
                                                              LigmaTransformGridTool  *tg_tool);

static void      ligma_transform_grid_tool_effective_mode_changed
                                                             (LigmaLayer              *layer,
                                                              LigmaTransformGridTool  *tg_tool);

static Filter  * filter_new                                  (LigmaTransformGridTool  *tg_tool,
                                                              LigmaDrawable           *drawable,
                                                              LigmaDrawable           *root_drawable,
                                                              gboolean                add_filter);
static void      filter_free                                 (Filter                 *filter);

static UndoInfo * undo_info_new  (void);
static void       undo_info_free (UndoInfo *info);

static gboolean   trans_info_equal  (const TransInfo  trans_info1,
                                     const TransInfo  trans_info2);
static gboolean   trans_infos_equal (const TransInfo *trans_infos1,
                                     const TransInfo *trans_infos2);


G_DEFINE_TYPE (LigmaTransformGridTool, ligma_transform_grid_tool, LIGMA_TYPE_TRANSFORM_TOOL)

#define parent_class ligma_transform_grid_tool_parent_class


static void
ligma_transform_grid_tool_class_init (LigmaTransformGridToolClass *klass)
{
  GObjectClass           *object_class = G_OBJECT_CLASS (klass);
  LigmaToolClass          *tool_class   = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass      *draw_class   = LIGMA_DRAW_TOOL_CLASS (klass);
  LigmaTransformToolClass *tr_class     = LIGMA_TRANSFORM_TOOL_CLASS (klass);

  object_class->finalize     = ligma_transform_grid_tool_finalize;

  tool_class->initialize     = ligma_transform_grid_tool_initialize;
  tool_class->control        = ligma_transform_grid_tool_control;
  tool_class->button_press   = ligma_transform_grid_tool_button_press;
  tool_class->button_release = ligma_transform_grid_tool_button_release;
  tool_class->motion         = ligma_transform_grid_tool_motion;
  tool_class->modifier_key   = ligma_transform_grid_tool_modifier_key;
  tool_class->cursor_update  = ligma_transform_grid_tool_cursor_update;
  tool_class->can_undo       = ligma_transform_grid_tool_can_undo;
  tool_class->can_redo       = ligma_transform_grid_tool_can_redo;
  tool_class->undo           = ligma_transform_grid_tool_undo;
  tool_class->redo           = ligma_transform_grid_tool_redo;
  tool_class->options_notify = ligma_transform_grid_tool_options_notify;

  draw_class->draw           = ligma_transform_grid_tool_draw;

  tr_class->recalc_matrix    = ligma_transform_grid_tool_recalc_matrix;
  tr_class->get_undo_desc    = ligma_transform_grid_tool_get_undo_desc;
  tr_class->get_direction    = ligma_transform_grid_tool_get_direction;
  tr_class->transform        = ligma_transform_grid_tool_transform;

  klass->info_to_matrix      = NULL;
  klass->matrix_to_info      = NULL;
  klass->apply_info          = ligma_transform_grid_tool_real_apply_info;
  klass->get_undo_desc       = ligma_transform_grid_tool_real_get_undo_desc;
  klass->dialog              = NULL;
  klass->dialog_update       = NULL;
  klass->prepare             = NULL;
  klass->readjust            = NULL;
  klass->get_widget          = NULL;
  klass->update_widget       = ligma_transform_grid_tool_real_update_widget;
  klass->widget_changed      = ligma_transform_grid_tool_real_widget_changed;
  klass->transform           = ligma_transform_grid_tool_real_transform;

  klass->ok_button_label     = _("_Transform");
}

static void
ligma_transform_grid_tool_init (LigmaTransformGridTool *tg_tool)
{
  LigmaTool *tool = LIGMA_TOOL (tg_tool);

  ligma_tool_control_set_scroll_lock      (tool->control, TRUE);
  ligma_tool_control_set_preserve         (tool->control, FALSE);
  ligma_tool_control_set_dirty_mask       (tool->control,
                                          LIGMA_DIRTY_IMAGE_SIZE      |
                                          LIGMA_DIRTY_IMAGE_STRUCTURE |
                                          LIGMA_DIRTY_DRAWABLE        |
                                          LIGMA_DIRTY_SELECTION       |
                                          LIGMA_DIRTY_ACTIVE_DRAWABLE);
  ligma_tool_control_set_active_modifiers (tool->control,
                                          LIGMA_TOOL_ACTIVE_MODIFIERS_SAME);
  ligma_tool_control_set_precision        (tool->control,
                                          LIGMA_CURSOR_PRECISION_SUBPIXEL);
  ligma_tool_control_set_cursor           (tool->control,
                                          LIGMA_CURSOR_CROSSHAIR_SMALL);
  ligma_tool_control_set_action_opacity   (tool->control,
                                          "tools/tools-transform-preview-opacity-set");

  tg_tool->strokes = g_ptr_array_new ();
}

static void
ligma_transform_grid_tool_finalize (GObject *object)
{
  LigmaTransformGridTool *tg_tool = LIGMA_TRANSFORM_GRID_TOOL (object);

  g_clear_object (&tg_tool->gui);
  g_clear_pointer (&tg_tool->strokes, g_ptr_array_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
ligma_transform_grid_tool_initialize (LigmaTool     *tool,
                                     LigmaDisplay  *display,
                                     GError      **error)
{
  LigmaTransformTool        *tr_tool    = LIGMA_TRANSFORM_TOOL (tool);
  LigmaTransformGridTool    *tg_tool    = LIGMA_TRANSFORM_GRID_TOOL (tool);
  LigmaTransformGridOptions *tg_options = LIGMA_TRANSFORM_GRID_TOOL_GET_OPTIONS (tool);
  LigmaImage                *image      = ligma_display_get_image (display);
  GList                    *drawables;
  GList                    *objects;
  GList                    *iter;
  UndoInfo                 *undo_info;

  objects = ligma_transform_tool_check_selected_objects (tr_tool, display, error);

  if (! objects)
    return FALSE;

  drawables = ligma_image_get_selected_drawables (image);
  if (g_list_length (drawables) < 1)
    {
      ligma_tool_message_literal (tool, display, _("No selected drawables."));
      g_list_free (drawables);
      g_list_free (objects);
      return FALSE;
    }

  tool->display    = display;
  tool->drawables  = drawables;

  tr_tool->objects = objects;

  for (iter = objects; iter; iter = iter->next)
    if (LIGMA_IS_DRAWABLE (iter->data))
      ligma_viewable_preview_freeze (iter->data);

  /*  Initialize the transform_grid tool dialog  */
  if (! tg_tool->gui)
    ligma_transform_grid_tool_dialog (tg_tool);

  /*  Find the transform bounds for some tools (like scale,
   *  perspective) that actually need the bounds for initializing
   */
  ligma_transform_tool_bounds (tr_tool, display);

  /*  Initialize the tool-specific trans_info, and adjust the tool dialog  */
  ligma_transform_grid_tool_prepare (tg_tool, display);

  /*  Recalculate the tool's transformation matrix  */
  ligma_transform_tool_recalc_matrix (tr_tool, display);

  /*  Get the on-canvas gui  */
  tg_tool->widget = ligma_transform_grid_tool_get_widget (tg_tool);

  ligma_transform_grid_tool_hide_selected_objects (tg_tool, objects);

  /*  start drawing the bounding box and handles...  */
  ligma_draw_tool_start (LIGMA_DRAW_TOOL (tool), display);

  /* Initialize undo and redo lists */
  undo_info = undo_info_new ();
  tg_tool->undo_list = g_list_prepend (NULL, undo_info);
  tg_tool->redo_list = NULL;

  /*  Save the current transformation info  */
  memcpy (undo_info->trans_infos, tg_tool->trans_infos,
          sizeof (tg_tool->trans_infos));

  if (tg_options->direction_chain_button)
    gtk_widget_set_sensitive (tg_options->direction_chain_button, TRUE);

  return TRUE;
}

static void
ligma_transform_grid_tool_control (LigmaTool       *tool,
                                  LigmaToolAction  action,
                                  LigmaDisplay    *display)
{
  LigmaTransformGridTool *tg_tool = LIGMA_TRANSFORM_GRID_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_transform_grid_tool_halt (tg_tool);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      if (tool->display)
        ligma_transform_grid_tool_commit (tg_tool);
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_transform_grid_tool_button_press (LigmaTool            *tool,
                                       const LigmaCoords    *coords,
                                       guint32              time,
                                       GdkModifierType      state,
                                       LigmaButtonPressType  press_type,
                                       LigmaDisplay         *display)
{
  LigmaTransformGridTool *tg_tool = LIGMA_TRANSFORM_GRID_TOOL (tool);

  if (tg_tool->widget)
    {
      ligma_tool_widget_hover (tg_tool->widget, coords, state, TRUE);

      if (ligma_tool_widget_button_press (tg_tool->widget, coords, time, state,
                                         press_type))
        {
          tg_tool->grab_widget = tg_tool->widget;
        }
    }

  ligma_tool_control_activate (tool->control);
}

static void
ligma_transform_grid_tool_button_release (LigmaTool              *tool,
                                         const LigmaCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         LigmaButtonReleaseType  release_type,
                                         LigmaDisplay           *display)
{
  LigmaTransformTool     *tr_tool = LIGMA_TRANSFORM_TOOL (tool);
  LigmaTransformGridTool *tg_tool = LIGMA_TRANSFORM_GRID_TOOL (tool);

  ligma_tool_control_halt (tool->control);

  if (tg_tool->grab_widget)
    {
      ligma_tool_widget_button_release (tg_tool->grab_widget,
                                       coords, time, state, release_type);
      tg_tool->grab_widget = NULL;
    }

  if (release_type != LIGMA_BUTTON_RELEASE_CANCEL)
    {
      /* We're done with an interaction, save it on the undo list */
      ligma_transform_grid_tool_push_internal_undo (tg_tool, FALSE);
    }
  else
    {
      UndoInfo *undo_info = tg_tool->undo_list->data;

      /*  Restore the last saved state  */
      memcpy (tg_tool->trans_infos, undo_info->trans_infos,
              sizeof (tg_tool->trans_infos));

      /*  recalculate the tool's transformation matrix  */
      ligma_transform_tool_recalc_matrix (tr_tool, display);
    }
}

static void
ligma_transform_grid_tool_motion (LigmaTool         *tool,
                                 const LigmaCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state,
                                 LigmaDisplay      *display)
{
  LigmaTransformGridTool *tg_tool = LIGMA_TRANSFORM_GRID_TOOL (tool);

  if (tg_tool->grab_widget)
    {
      ligma_tool_widget_motion (tg_tool->grab_widget, coords, time, state);
    }
}

static void
ligma_transform_grid_tool_modifier_key (LigmaTool        *tool,
                                       GdkModifierType  key,
                                       gboolean         press,
                                       GdkModifierType  state,
                                       LigmaDisplay     *display)
{
  LigmaTransformGridTool *tg_tool = LIGMA_TRANSFORM_GRID_TOOL (tool);

  if (tg_tool->widget)
    {
      LIGMA_TOOL_CLASS (parent_class)->modifier_key (tool, key, press,
                                                    state, display);
    }
  else
    {
      LigmaTransformGridOptions *options = LIGMA_TRANSFORM_GRID_TOOL_GET_OPTIONS (tool);

      if (key == ligma_get_constrain_behavior_mask ())
        {
          g_object_set (options,
                        "frompivot-scale",       ! options->frompivot_scale,
                        "frompivot-shear",       ! options->frompivot_shear,
                        "frompivot-perspective", ! options->frompivot_perspective,
                        NULL);
        }
      else if (key == ligma_get_extend_selection_mask ())
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
ligma_transform_grid_tool_cursor_update (LigmaTool         *tool,
                                        const LigmaCoords *coords,
                                        GdkModifierType   state,
                                        LigmaDisplay      *display)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tool);
  GList             *objects;

  objects = ligma_transform_tool_check_selected_objects (tr_tool, display, NULL);

  if (display != tool->display && ! objects)
    {
      ligma_tool_set_cursor (tool, display,
                            ligma_tool_control_get_cursor (tool->control),
                            ligma_tool_control_get_tool_cursor (tool->control),
                            LIGMA_CURSOR_MODIFIER_BAD);
      return;
    }
  g_list_free (objects);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static const gchar *
ligma_transform_grid_tool_can_undo (LigmaTool    *tool,
                                   LigmaDisplay *display)
{
  LigmaTransformGridTool *tg_tool = LIGMA_TRANSFORM_GRID_TOOL (tool);

  if (! tg_tool->undo_list || ! tg_tool->undo_list->next)
    return NULL;

  return _("Transform Step");
}

static const gchar *
ligma_transform_grid_tool_can_redo (LigmaTool    *tool,
                                   LigmaDisplay *display)
{
  LigmaTransformGridTool *tg_tool = LIGMA_TRANSFORM_GRID_TOOL (tool);

  if (! tg_tool->redo_list)
    return NULL;

  return _("Transform Step");
}

static gboolean
ligma_transform_grid_tool_undo (LigmaTool    *tool,
                               LigmaDisplay *display)
{
  LigmaTransformTool      *tr_tool    = LIGMA_TRANSFORM_TOOL (tool);
  LigmaTransformGridTool  *tg_tool    = LIGMA_TRANSFORM_GRID_TOOL (tool);
  LigmaTransformOptions   *tr_options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tool);
  UndoInfo               *undo_info;
  LigmaTransformDirection  direction;

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
  ligma_transform_tool_recalc_matrix (tr_tool, display);

  return TRUE;
}

static gboolean
ligma_transform_grid_tool_redo (LigmaTool    *tool,
                               LigmaDisplay *display)
{
  LigmaTransformTool      *tr_tool    = LIGMA_TRANSFORM_TOOL (tool);
  LigmaTransformGridTool  *tg_tool    = LIGMA_TRANSFORM_GRID_TOOL (tool);
  LigmaTransformOptions   *tr_options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tool);
  UndoInfo               *undo_info;
  LigmaTransformDirection  direction;

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
  ligma_transform_tool_recalc_matrix (tr_tool, display);

  return TRUE;
}

static void
ligma_transform_grid_tool_options_notify (LigmaTool         *tool,
                                         LigmaToolOptions  *options,
                                         const GParamSpec *pspec)
{
  LigmaTransformTool        *tr_tool    = LIGMA_TRANSFORM_TOOL (tool);
  LigmaTransformGridTool    *tg_tool    = LIGMA_TRANSFORM_GRID_TOOL (tool);
  LigmaTransformGridOptions *tg_options = LIGMA_TRANSFORM_GRID_OPTIONS (options);

  LIGMA_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "type"))
    {
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);
      return;
    }

  if (! tg_tool->widget)
    return;

  if (! strcmp (pspec->name, "direction"))
    {
      /*  recalculate the tool's transformation matrix  */
      ligma_transform_tool_recalc_matrix (tr_tool, tool->display);
    }
  else if (! strcmp (pspec->name, "show-preview") ||
           ! strcmp (pspec->name, "composited-preview"))
    {
      if (tg_tool->previews)
        {
          LigmaDisplay *display;
          GList       *objects;

          display = tool->display;
          objects = ligma_transform_tool_get_selected_objects (tr_tool, display);

          if (objects)
            {
              if (tg_options->show_preview &&
                  ! ligma_transform_grid_tool_composited_preview (tg_tool))
                {
                  ligma_transform_grid_tool_hide_selected_objects (tg_tool, objects);
                }
              else
                {
                  ligma_transform_grid_tool_show_selected_objects (tg_tool);
                }

              g_list_free (objects);
            }

          ligma_transform_grid_tool_update_preview (tg_tool);
        }
    }
  else if (! strcmp (pspec->name, "interpolation") ||
           ! strcmp (pspec->name, "clip")          ||
           ! strcmp (pspec->name, "preview-opacity"))
    {
      ligma_transform_grid_tool_update_preview (tg_tool);
    }
  else if (g_str_has_prefix (pspec->name, "constrain-") ||
           g_str_has_prefix (pspec->name, "frompivot-") ||
           ! strcmp (pspec->name, "fixedpivot") ||
           ! strcmp (pspec->name, "cornersnap"))
    {
      ligma_transform_grid_tool_dialog_update (tg_tool);
    }
}

static void
ligma_transform_grid_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaTool                 *tool       = LIGMA_TOOL (draw_tool);
  LigmaTransformTool        *tr_tool    = LIGMA_TRANSFORM_TOOL (draw_tool);
  LigmaTransformGridTool    *tg_tool    = LIGMA_TRANSFORM_GRID_TOOL (draw_tool);
  LigmaTransformGridOptions *options    = LIGMA_TRANSFORM_GRID_TOOL_GET_OPTIONS (tool);
  LigmaTransformOptions     *tr_options = LIGMA_TRANSFORM_OPTIONS (options);
  LigmaDisplayShell         *shell      = ligma_display_get_shell (tool->display);
  LigmaImage                *image      = ligma_display_get_image (tool->display);
  LigmaMatrix3               matrix     = tr_tool->transform;
  LigmaCanvasItem           *item;

  if (tr_options->direction == LIGMA_TRANSFORM_BACKWARD)
    ligma_matrix3_invert (&matrix);

  if (tr_options->type == LIGMA_TRANSFORM_TYPE_LAYER ||
      tr_options->type == LIGMA_TRANSFORM_TYPE_IMAGE)
    {
      GList *pickables = NULL;
      GList *iter;

      if (tr_options->type == LIGMA_TRANSFORM_TYPE_IMAGE)
        {
          if (! shell->show_all)
            pickables = g_list_prepend (pickables, image);
          else
            pickables = g_list_prepend (pickables, ligma_image_get_projection (image));
        }
      else
        {
          for (iter = tool->drawables; iter; iter = iter->next)
            pickables = g_list_prepend (pickables, iter->data);
        }

      for (iter = pickables; iter; iter = iter->next)
        {
          LigmaCanvasItem *preview;

          preview = ligma_draw_tool_add_transform_preview (draw_tool,
                                                          LIGMA_PICKABLE (iter->data),
                                                          &matrix,
                                                          tr_tool->x1,
                                                          tr_tool->y1,
                                                          tr_tool->x2,
                                                          tr_tool->y2);
          tg_tool->previews = g_list_prepend (tg_tool->previews, preview);
          g_object_add_weak_pointer (G_OBJECT (tg_tool->previews->data),
                                     (gpointer) &tg_tool->previews->data);
        }
      g_list_free (pickables);
    }

  if (tr_options->type == LIGMA_TRANSFORM_TYPE_SELECTION)
    {
      const LigmaBoundSeg *segs_in;
      const LigmaBoundSeg *segs_out;
      gint                n_segs_in;
      gint                n_segs_out;

      ligma_channel_boundary (ligma_image_get_mask (image),
                             &segs_in, &segs_out,
                             &n_segs_in, &n_segs_out,
                             0, 0, 0, 0);

      if (segs_in)
        {
          tg_tool->boundary_in =
            ligma_draw_tool_add_boundary (draw_tool,
                                         segs_in, n_segs_in,
                                         &matrix,
                                         0, 0);
          g_object_add_weak_pointer (G_OBJECT (tg_tool->boundary_in),
                                     (gpointer) &tg_tool->boundary_in);

          ligma_canvas_item_set_visible (tg_tool->boundary_in,
                                        tr_tool->transform_valid);
        }

      if (segs_out)
        {
          tg_tool->boundary_out =
            ligma_draw_tool_add_boundary (draw_tool,
                                         segs_out, n_segs_out,
                                         &matrix,
                                         0, 0);
          g_object_add_weak_pointer (G_OBJECT (tg_tool->boundary_out),
                                     (gpointer) &tg_tool->boundary_out);

          ligma_canvas_item_set_visible (tg_tool->boundary_out,
                                        tr_tool->transform_valid);
        }
    }
  else if (tr_options->type == LIGMA_TRANSFORM_TYPE_PATH)
    {
      GList *iter;

      for (iter = ligma_image_get_selected_vectors (image); iter; iter = iter->next)
        {
          LigmaVectors *vectors = iter->data;
          LigmaStroke  *stroke  = NULL;

          while ((stroke = ligma_vectors_stroke_get_next (vectors, stroke)))
            {
              GArray   *coords;
              gboolean  closed;

              coords = ligma_stroke_interpolate (stroke, 1.0, &closed);

              if (coords && coords->len)
                {
                  item =
                    ligma_draw_tool_add_strokes (draw_tool,
                                                &g_array_index (coords,
                                                                LigmaCoords, 0),
                                                coords->len, &matrix, FALSE);

                  g_ptr_array_add (tg_tool->strokes, item);
                  g_object_weak_ref (G_OBJECT (item),
                                     (GWeakNotify) g_ptr_array_remove,
                                     tg_tool->strokes);

                  ligma_canvas_item_set_visible (item, tr_tool->transform_valid);
                }

              if (coords)
                g_array_free (coords, TRUE);
            }
        }
    }

  LIGMA_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  ligma_transform_grid_tool_update_preview (tg_tool);
}

static void
ligma_transform_grid_tool_recalc_matrix (LigmaTransformTool *tr_tool)
{
  LigmaTransformGridTool    *tg_tool    = LIGMA_TRANSFORM_GRID_TOOL (tr_tool);
  LigmaTransformOptions     *tr_options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  LigmaTransformGridOptions *tg_options = LIGMA_TRANSFORM_GRID_TOOL_GET_OPTIONS (tr_tool);

  if (LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->info_to_matrix)
    {
      LigmaMatrix3 forward_transform;
      LigmaMatrix3 backward_transform;
      gboolean    forward_transform_valid;
      gboolean    backward_transform_valid;

      tg_tool->trans_info      = tg_tool->trans_infos[LIGMA_TRANSFORM_FORWARD];
      forward_transform_valid  = ligma_transform_grid_tool_info_to_matrix (
                                   tg_tool, &forward_transform);

      tg_tool->trans_info      = tg_tool->trans_infos[LIGMA_TRANSFORM_BACKWARD];
      backward_transform_valid = ligma_transform_grid_tool_info_to_matrix (
                                   tg_tool, &backward_transform);

      if (LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info &&
          tg_options->direction_linked)
        {
          LigmaMatrix3 transform = tr_tool->transform;

          switch (tr_options->direction)
            {
            case LIGMA_TRANSFORM_FORWARD:
              if (forward_transform_valid)
                {
                  ligma_matrix3_invert (&transform);

                  backward_transform = forward_transform;
                  ligma_matrix3_mult (&transform, &backward_transform);

                  tg_tool->trans_info =
                    tg_tool->trans_infos[LIGMA_TRANSFORM_BACKWARD];
                  ligma_transform_grid_tool_matrix_to_info (tg_tool,
                                                           &backward_transform);
                  backward_transform_valid =
                    ligma_transform_grid_tool_info_to_matrix (
                      tg_tool, &backward_transform);
                }
              break;

            case LIGMA_TRANSFORM_BACKWARD:
              if (backward_transform_valid)
                {
                  forward_transform = backward_transform;
                  ligma_matrix3_mult (&transform, &forward_transform);

                  tg_tool->trans_info =
                    tg_tool->trans_infos[LIGMA_TRANSFORM_FORWARD];
                  ligma_transform_grid_tool_matrix_to_info (tg_tool,
                                                           &forward_transform);
                  forward_transform_valid =
                    ligma_transform_grid_tool_info_to_matrix (
                      tg_tool, &forward_transform);
                }
              break;
            }
        }
      else if (forward_transform_valid && backward_transform_valid)
        {
          tr_tool->transform = backward_transform;
          ligma_matrix3_invert (&tr_tool->transform);
          ligma_matrix3_mult (&forward_transform, &tr_tool->transform);
        }

      tr_tool->transform_valid = forward_transform_valid &&
                                 backward_transform_valid;
    }

  tg_tool->trans_info = tg_tool->trans_infos[tr_options->direction];

  ligma_transform_grid_tool_dialog_update (tg_tool);
  ligma_transform_grid_tool_update_sensitivity (tg_tool);
  ligma_transform_grid_tool_update_widget (tg_tool);
  ligma_transform_grid_tool_update_preview (tg_tool);

  if (tg_tool->gui)
    ligma_tool_gui_show (tg_tool->gui);
}

static gchar *
ligma_transform_grid_tool_get_undo_desc (LigmaTransformTool *tr_tool)
{
  LigmaTransformGridTool *tg_tool    = LIGMA_TRANSFORM_GRID_TOOL (tr_tool);
  LigmaTransformOptions  *tr_options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  gchar                 *result;

  if (LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info)
    {
      TransInfo trans_info;

      memcpy (&trans_info, &tg_tool->init_trans_info, sizeof (TransInfo));

      tg_tool->trans_info = trans_info;
      ligma_transform_grid_tool_matrix_to_info (tg_tool, &tr_tool->transform);
      result = LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->get_undo_desc (
        tg_tool);
    }
  else if (trans_info_equal (tg_tool->trans_infos[LIGMA_TRANSFORM_BACKWARD],
                             tg_tool->init_trans_info))
    {
      tg_tool->trans_info = tg_tool->trans_infos[LIGMA_TRANSFORM_FORWARD];
      result = LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->get_undo_desc (
        tg_tool);
    }
  else if (trans_info_equal (tg_tool->trans_infos[LIGMA_TRANSFORM_FORWARD],
                             tg_tool->init_trans_info))
    {
      gchar *desc;

      tg_tool->trans_info = tg_tool->trans_infos[LIGMA_TRANSFORM_BACKWARD];
      desc = LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->get_undo_desc (
        tg_tool);

      result = g_strdup_printf (_("%s (Corrective)"), desc);

      g_free (desc);
    }
  else
    {
      result = LIGMA_TRANSFORM_TOOL_CLASS (parent_class)->get_undo_desc (
        tr_tool);
    }

  tg_tool->trans_info = tg_tool->trans_infos[tr_options->direction];

  return result;
}

static LigmaTransformDirection
ligma_transform_grid_tool_get_direction (LigmaTransformTool *tr_tool)
{
  return LIGMA_TRANSFORM_FORWARD;
}

static GeglBuffer *
ligma_transform_grid_tool_transform (LigmaTransformTool  *tr_tool,
                                    GList              *objects,
                                    GeglBuffer         *orig_buffer,
                                    gint                orig_offset_x,
                                    gint                orig_offset_y,
                                    LigmaColorProfile  **buffer_profile,
                                    gint               *new_offset_x,
                                    gint               *new_offset_y)
{
  LigmaTool              *tool    = LIGMA_TOOL (tr_tool);
  LigmaTransformGridTool *tg_tool = LIGMA_TRANSFORM_GRID_TOOL (tr_tool);
  LigmaDisplay           *display = tool->display;
  LigmaImage             *image   = ligma_display_get_image (display);
  GeglBuffer            *new_buffer;

  /*  Send the request for the transformation to the tool...
   */
  new_buffer =
    LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->transform (tg_tool,
                                                             objects,
                                                             orig_buffer,
                                                             orig_offset_x,
                                                             orig_offset_y,
                                                             buffer_profile,
                                                             new_offset_x,
                                                             new_offset_y);

  ligma_image_undo_push (image, LIGMA_TYPE_TRANSFORM_GRID_TOOL_UNDO,
                        LIGMA_UNDO_TRANSFORM_GRID, NULL,
                        0,
                        "transform-tool", tg_tool,
                        NULL);

  return new_buffer;
}

static void
ligma_transform_grid_tool_real_apply_info (LigmaTransformGridTool *tg_tool,
                                          const TransInfo        info)
{
  memcpy (tg_tool->trans_info, info, sizeof (TransInfo));
}

static gchar *
ligma_transform_grid_tool_real_get_undo_desc (LigmaTransformGridTool *tg_tool)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);

  return LIGMA_TRANSFORM_TOOL_CLASS (parent_class)->get_undo_desc (tr_tool);
}

static void
ligma_transform_grid_tool_real_update_widget (LigmaTransformGridTool *tg_tool)
{
  if (LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->info_to_matrix)
    {
      LigmaMatrix3 transform;

      ligma_transform_grid_tool_info_to_matrix (tg_tool, &transform);

      g_object_set (tg_tool->widget,
                    "transform", &transform,
                    NULL);
    }
}

static void
ligma_transform_grid_tool_real_widget_changed (LigmaTransformGridTool *tg_tool)
{
  LigmaTool          *tool    = LIGMA_TOOL (tg_tool);
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);
  LigmaToolWidget    *widget  = tg_tool->widget;

  /* suppress the call to LigmaTransformGridTool::update_widget() when
   * recalculating the matrix
   */
  tg_tool->widget = NULL;

  ligma_transform_tool_recalc_matrix (tr_tool, tool->display);

  tg_tool->widget = widget;
}

static GeglBuffer *
ligma_transform_grid_tool_real_transform (LigmaTransformGridTool  *tg_tool,
                                         GList                  *objects,
                                         GeglBuffer             *orig_buffer,
                                         gint                    orig_offset_x,
                                         gint                    orig_offset_y,
                                         LigmaColorProfile      **buffer_profile,
                                         gint                   *new_offset_x,
                                         gint                   *new_offset_y)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);

  return LIGMA_TRANSFORM_TOOL_CLASS (parent_class)->transform (tr_tool,
                                                              objects,
                                                              orig_buffer,
                                                              orig_offset_x,
                                                              orig_offset_y,
                                                              buffer_profile,
                                                              new_offset_x,
                                                              new_offset_y);
}

static void
ligma_transform_grid_tool_widget_changed (LigmaToolWidget        *widget,
                                         LigmaTransformGridTool *tg_tool)
{
  if (LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->widget_changed)
    LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->widget_changed (tg_tool);
}

static void
ligma_transform_grid_tool_widget_response (LigmaToolWidget        *widget,
                                          gint                   response_id,
                                          LigmaTransformGridTool *tg_tool)
{
  switch (response_id)
    {
    case LIGMA_TOOL_WIDGET_RESPONSE_CONFIRM:
      ligma_transform_grid_tool_response (NULL, GTK_RESPONSE_OK, tg_tool);
      break;

    case LIGMA_TOOL_WIDGET_RESPONSE_CANCEL:
      ligma_transform_grid_tool_response (NULL, GTK_RESPONSE_CANCEL, tg_tool);
      break;

    case LIGMA_TOOL_WIDGET_RESPONSE_RESET:
      ligma_transform_grid_tool_response (NULL, RESPONSE_RESET, tg_tool);
      break;
    }
}

static void
ligma_transform_grid_tool_filter_flush (LigmaDrawableFilter    *filter,
                                       LigmaTransformGridTool *tg_tool)
{
  LigmaTool  *tool = LIGMA_TOOL (tg_tool);
  LigmaImage *image = ligma_display_get_image (tool->display);

  ligma_projection_flush (ligma_image_get_projection (image));
}

static void
ligma_transform_grid_tool_halt (LigmaTransformGridTool *tg_tool)
{
  LigmaTool                 *tool       = LIGMA_TOOL (tg_tool);
  LigmaTransformTool        *tr_tool    = LIGMA_TRANSFORM_TOOL (tg_tool);
  LigmaTransformGridOptions *tg_options = LIGMA_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  GList                    *iter;

  if (ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (tg_tool)))
    ligma_draw_tool_stop (LIGMA_DRAW_TOOL (tg_tool));

  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (tg_tool), NULL);
  g_clear_object (&tg_tool->widget);

  g_clear_pointer (&tg_tool->filters, g_hash_table_unref);
  g_clear_pointer (&tg_tool->preview_drawables, g_list_free);

  for (iter = tg_tool->previews; iter; iter = iter->next)
    {
      if (iter->data)
        g_object_unref (iter->data);
    }
  g_clear_pointer (&tg_tool->previews, g_list_free);

  if (tg_tool->gui)
    ligma_tool_gui_hide (tg_tool->gui);

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

  ligma_transform_grid_tool_show_selected_objects (tg_tool);

  if (tg_options->direction_chain_button)
    {
      g_object_set (tg_options,
                    "direction-linked", FALSE,
                    NULL);

      gtk_widget_set_sensitive (tg_options->direction_chain_button, FALSE);
    }

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;

  if (tr_tool->objects)
    {
      GList *iter;

      for (iter = tr_tool->objects; iter; iter = iter->next)
        if (LIGMA_IS_DRAWABLE (iter->data))
          ligma_viewable_preview_thaw (iter->data);

      g_list_free (tr_tool->objects);
      tr_tool->objects = NULL;
    }
}

static void
ligma_transform_grid_tool_commit (LigmaTransformGridTool *tg_tool)
{
  LigmaTool          *tool    = LIGMA_TOOL (tg_tool);
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);
  LigmaDisplay       *display = tool->display;

  /* undraw the tool before we muck around with the transform matrix */
  ligma_draw_tool_stop (LIGMA_DRAW_TOOL (tg_tool));

  ligma_transform_tool_transform (tr_tool, display);
}

static void
ligma_transform_grid_tool_dialog (LigmaTransformGridTool *tg_tool)
{
  LigmaTool         *tool      = LIGMA_TOOL (tg_tool);
  LigmaToolInfo     *tool_info = tool->tool_info;
  LigmaDisplayShell *shell;
  const gchar      *ok_button_label;

  if (! LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->dialog)
    return;

  g_return_if_fail (tool->display != NULL);

  shell = ligma_display_get_shell (tool->display);

  ok_button_label = LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->ok_button_label;

  tg_tool->gui = ligma_tool_gui_new (tool_info,
                                    NULL, NULL, NULL, NULL,
                                    ligma_widget_get_monitor (GTK_WIDGET (shell)),
                                    TRUE,
                                    NULL);

  ligma_tool_gui_add_button   (tg_tool->gui, _("_Reset"),     RESPONSE_RESET);
  if (LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->readjust)
    ligma_tool_gui_add_button (tg_tool->gui, _("Re_adjust"),  RESPONSE_READJUST);
  ligma_tool_gui_add_button   (tg_tool->gui, _("_Cancel"),    GTK_RESPONSE_CANCEL);
  ligma_tool_gui_add_button   (tg_tool->gui, ok_button_label, GTK_RESPONSE_OK);

  ligma_tool_gui_set_auto_overlay (tg_tool->gui, TRUE);
  ligma_tool_gui_set_default_response (tg_tool->gui, GTK_RESPONSE_OK);

  ligma_tool_gui_set_alternative_button_order (tg_tool->gui,
                                              RESPONSE_RESET,
                                              RESPONSE_READJUST,
                                              GTK_RESPONSE_OK,
                                              GTK_RESPONSE_CANCEL,
                                              -1);

  g_signal_connect (tg_tool->gui, "response",
                    G_CALLBACK (ligma_transform_grid_tool_response),
                    tg_tool);

  LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->dialog (tg_tool);
}

static void
ligma_transform_grid_tool_dialog_update (LigmaTransformGridTool *tg_tool)
{
  if (tg_tool->gui &&
      LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->dialog_update)
    {
      LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->dialog_update (tg_tool);
    }
}

static void
ligma_transform_grid_tool_prepare (LigmaTransformGridTool *tg_tool,
                                  LigmaDisplay           *display)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);

  if (tg_tool->gui)
    {
      GList *objects = ligma_transform_tool_get_selected_objects (tr_tool, display);

      ligma_tool_gui_set_shell (tg_tool->gui, ligma_display_get_shell (display));
      ligma_tool_gui_set_viewables (tg_tool->gui, objects);
      g_list_free (objects);
    }

  if (LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->prepare)
    {
      tg_tool->trans_info = tg_tool->init_trans_info;
      LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->prepare (tg_tool);

      memcpy (tg_tool->trans_infos[LIGMA_TRANSFORM_FORWARD],
              tg_tool->init_trans_info, sizeof (TransInfo));
      memcpy (tg_tool->trans_infos[LIGMA_TRANSFORM_BACKWARD],
              tg_tool->init_trans_info, sizeof (TransInfo));
    }

  ligma_matrix3_identity (&tr_tool->transform);
  tr_tool->transform_valid = TRUE;
}

static LigmaToolWidget *
ligma_transform_grid_tool_get_widget (LigmaTransformGridTool *tg_tool)
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

  LigmaToolWidget *widget = NULL;

  if (LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->get_widget)
    {
      LigmaTransformGridOptions *options = LIGMA_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
      gint                      i;

      widget = LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->get_widget (tg_tool);

      ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (tg_tool), widget);

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
                        G_CALLBACK (ligma_transform_grid_tool_widget_changed),
                        tg_tool);
      g_signal_connect (widget, "response",
                        G_CALLBACK (ligma_transform_grid_tool_widget_response),
                        tg_tool);
    }

  return widget;
}

static void
ligma_transform_grid_tool_update_widget (LigmaTransformGridTool *tg_tool)
{
  if (tg_tool->widget &&
      LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->update_widget)
    {
      g_signal_handlers_block_by_func (
        tg_tool->widget,
        G_CALLBACK (ligma_transform_grid_tool_widget_changed),
        tg_tool);

      LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->update_widget (tg_tool);

      g_signal_handlers_unblock_by_func (
        tg_tool->widget,
        G_CALLBACK (ligma_transform_grid_tool_widget_changed),
        tg_tool);
    }
}

static void
ligma_transform_grid_tool_response (LigmaToolGui           *gui,
                                   gint                   response_id,
                                   LigmaTransformGridTool *tg_tool)
{
  LigmaTool                 *tool       = LIGMA_TOOL (tg_tool);
  LigmaTransformTool        *tr_tool    = LIGMA_TRANSFORM_TOOL (tg_tool);
  LigmaTransformOptions     *tr_options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tg_tool);
  LigmaTransformGridOptions *tg_options = LIGMA_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  LigmaDisplay              *display    = tool->display;

  /* we can get here while already committing a transformation.  just return in
   * this case.  see issue #4734.
   */
  if (! ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (tg_tool)))
    return;

  switch (response_id)
    {
    case RESPONSE_RESET:
      {
        gboolean direction_linked;

        /*  restore the initial transformation info  */
        memcpy (tg_tool->trans_infos[LIGMA_TRANSFORM_FORWARD],
                tg_tool->init_trans_info,
                sizeof (TransInfo));
        memcpy (tg_tool->trans_infos[LIGMA_TRANSFORM_BACKWARD],
                tg_tool->init_trans_info,
                sizeof (TransInfo));

        /*  recalculate the tool's transformation matrix  */
        direction_linked             = tg_options->direction_linked;
        tg_options->direction_linked = FALSE;
        ligma_transform_tool_recalc_matrix (tr_tool, display);
        tg_options->direction_linked = direction_linked;

        /*  push the restored info to the undo stack  */
        ligma_transform_grid_tool_push_internal_undo (tg_tool, FALSE);
      }
      break;

    case RESPONSE_READJUST:
      if (LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->readjust       &&
          LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info &&
          tr_tool->transform_valid)
        {
          TransInfo old_trans_infos[2];
          gboolean  direction_linked;
          gboolean  transform_valid;

          /*  save the current transformation info  */
          memcpy (old_trans_infos, tg_tool->trans_infos,
                  sizeof (old_trans_infos));

          /*  readjust the transformation info to view  */
          LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->readjust (tg_tool);

          /*  recalculate the tool's transformation matrix, preserving the
           *  overall transformation
           */
          direction_linked             = tg_options->direction_linked;
          tg_options->direction_linked = TRUE;
          ligma_transform_tool_recalc_matrix (tr_tool, display);
          tg_options->direction_linked = direction_linked;

          transform_valid = tr_tool->transform_valid;

          /*  if the resulting transformation is invalid, or if the
           *  transformation info is already adjusted to view ...
           */
          if (! transform_valid ||
              trans_infos_equal (old_trans_infos, tg_tool->trans_infos))
            {
              /*  ... readjust the transformation info to the item bounds  */
              LigmaMatrix3 transform = tr_tool->transform;

              if (tr_options->direction == LIGMA_TRANSFORM_BACKWARD)
                ligma_matrix3_invert (&transform);

              LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->apply_info (
                tg_tool, tg_tool->init_trans_info);
              LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info (
                tg_tool, &transform);

              /*  recalculate the tool's transformation matrix, preserving the
               *  overall transformation
               */
              direction_linked             = tg_options->direction_linked;
              tg_options->direction_linked = TRUE;
              ligma_transform_tool_recalc_matrix (tr_tool, display);
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
              ligma_transform_grid_tool_push_internal_undo (tg_tool, FALSE);
            }
          else
            {
              /*  restore the old transformation info  */
              memcpy (tg_tool->trans_infos, old_trans_infos,
                      sizeof (old_trans_infos));

              /*  recalculate the tool's transformation matrix  */
              direction_linked             = tg_options->direction_linked;
              tg_options->direction_linked = FALSE;
              ligma_transform_tool_recalc_matrix (tr_tool, display);
              tg_options->direction_linked = direction_linked;

              ligma_tool_message_literal (tool, tool->display,
                                         _("Cannot readjust the transformation"));
            }
        }
      break;

    case GTK_RESPONSE_OK:
      g_return_if_fail (display != NULL);
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, display);
      break;

    default:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);

      /*  update the undo actions / menu items  */
      if (display)
        ligma_image_flush (ligma_display_get_image (display));
      break;
    }
}

static gboolean
ligma_transform_grid_tool_composited_preview (LigmaTransformGridTool *tg_tool)
{
  LigmaTool                 *tool       = LIGMA_TOOL (tg_tool);
  LigmaTransformOptions     *tr_options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tg_tool);
  LigmaTransformGridOptions *tg_options = LIGMA_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  LigmaImage                *image      = ligma_display_get_image (tool->display);

  return tg_options->composited_preview                &&
         tr_options->type == LIGMA_TRANSFORM_TYPE_LAYER &&
         ligma_channel_is_empty (ligma_image_get_mask (image));
}

static void
ligma_transform_grid_tool_update_sensitivity (LigmaTransformGridTool *tg_tool)
{
  LigmaTransformTool *tr_tool = LIGMA_TRANSFORM_TOOL (tg_tool);

  if (! tg_tool->gui)
    return;

  ligma_tool_gui_set_response_sensitive (
    tg_tool->gui, GTK_RESPONSE_OK,
    tr_tool->transform_valid);

  ligma_tool_gui_set_response_sensitive (
    tg_tool->gui, RESPONSE_RESET,
    ! (trans_info_equal (tg_tool->trans_infos[LIGMA_TRANSFORM_FORWARD],
                         tg_tool->init_trans_info) &&
       trans_info_equal (tg_tool->trans_infos[LIGMA_TRANSFORM_BACKWARD],
                         tg_tool->init_trans_info)));

  ligma_tool_gui_set_response_sensitive (
    tg_tool->gui, RESPONSE_READJUST,
    tr_tool->transform_valid);
}

static void
ligma_transform_grid_tool_update_preview (LigmaTransformGridTool *tg_tool)
{
  LigmaTool                 *tool       = LIGMA_TOOL (tg_tool);
  LigmaTransformTool        *tr_tool    = LIGMA_TRANSFORM_TOOL (tg_tool);
  LigmaTransformOptions     *tr_options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tg_tool);
  LigmaTransformGridOptions *tg_options = LIGMA_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  GList                    *iter;
  gint                      i;

  if (! tool->display)
    return;

  if (tg_options->show_preview                              &&
      ligma_transform_grid_tool_composited_preview (tg_tool) &&
      tr_tool->transform_valid)
    {
      GHashTableIter  iter;
      LigmaDrawable   *drawable;
      Filter         *filter;
      gboolean        flush = FALSE;

      if (! tg_tool->filters)
        {
          tg_tool->filters = g_hash_table_new_full (
            g_direct_hash, g_direct_equal,
            NULL, (GDestroyNotify) filter_free);

          ligma_transform_grid_tool_update_filters (tg_tool);
        }

      g_hash_table_iter_init (&iter, tg_tool->filters);

      while (g_hash_table_iter_next (&iter,
                                     (gpointer *) &drawable,
                                     (gpointer *) &filter))
        {
          LigmaMatrix3   transform;
          GeglRectangle bounds;
          gint          offset_x;
          gint          offset_y;
          gint          width;
          gint          height;
          gint          x1, y1;
          gint          x2, y2;
          gboolean      update = FALSE;

          if (! filter->filter)
            continue;

          ligma_item_get_offset (LIGMA_ITEM (drawable), &offset_x, &offset_y);

          width  = ligma_item_get_width  (LIGMA_ITEM (drawable));
          height = ligma_item_get_height (LIGMA_ITEM (drawable));

          ligma_matrix3_identity (&transform);
          ligma_matrix3_translate (&transform, +offset_x, +offset_y);
          ligma_matrix3_mult (&tr_tool->transform, &transform);
          ligma_matrix3_translate (&transform, -offset_x, -offset_y);

          ligma_transform_resize_boundary (&tr_tool->transform,
                                          ligma_item_get_clip (
                                            LIGMA_ITEM (filter->root_drawable),
                                            tr_options->clip),
                                          offset_x,         offset_y,
                                          offset_x + width, offset_y + height,
                                          &x1,              &y1,
                                          &x2,              &y2);

          bounds.x      = x1 - offset_x;
          bounds.y      = y1 - offset_y;
          bounds.width  = x2 - x1;
          bounds.height = y2 - y1;

          if (! ligma_matrix3_equal (&transform, &filter->transform))
            {
              filter->transform = transform;

              ligma_gegl_node_set_matrix (filter->transform_node, &transform);

              update = TRUE;
            }

          if (! gegl_rectangle_equal (&bounds, &filter->bounds))
            {
              filter->bounds = bounds;

              gegl_node_set (filter->crop_node,
                             "x",      (gdouble) bounds.x,
                             "y",      (gdouble) bounds.y,
                             "width",  (gdouble) bounds.width,
                             "height", (gdouble) bounds.height,
                             NULL);

              update = TRUE;
            }

          if (LIGMA_IS_LAYER (drawable))
            {
              ligma_drawable_filter_set_add_alpha (
                filter->filter,
                tr_options->interpolation != LIGMA_INTERPOLATION_NONE);
            }

          if (update)
            {
              if (tg_options->synchronous_preview)
                {
                  g_signal_handlers_block_by_func (
                    filter->filter,
                    G_CALLBACK (ligma_transform_grid_tool_filter_flush),
                    tg_tool);
                }

              ligma_drawable_filter_apply (filter->filter, NULL);

              if (tg_options->synchronous_preview)
                {
                  g_signal_handlers_unblock_by_func (
                    filter->filter,
                    G_CALLBACK (ligma_transform_grid_tool_filter_flush),
                    tg_tool);

                  flush = TRUE;
                }
            }
        }

      if (flush)
        {
          LigmaImage *image = ligma_display_get_image (tool->display);

          ligma_projection_flush_now (ligma_image_get_projection (image), TRUE);
          ligma_display_flush_now (tool->display);
        }
    }
  else
    {
      g_clear_pointer (&tg_tool->filters, g_hash_table_unref);
      g_clear_pointer (&tg_tool->preview_drawables, g_list_free);
    }

  for (iter = tg_tool->previews; iter; iter = iter->next)
    {
      LigmaCanvasItem *preview = iter->data;

      if (preview == NULL)
        continue;

      if (tg_options->show_preview                                &&
          ! ligma_transform_grid_tool_composited_preview (tg_tool) &&
          tr_tool->transform_valid)
        {
          ligma_canvas_item_begin_change (preview);
          ligma_canvas_item_set_visible (preview, TRUE);
          g_object_set (
            preview,
            "transform", &tr_tool->transform,
            "clip",      ligma_item_get_clip (LIGMA_ITEM (tool->drawables->data),
                                             tr_options->clip),
            "opacity",   tg_options->preview_opacity,
            NULL);
          ligma_canvas_item_end_change (preview);
        }
      else
        {
          ligma_canvas_item_set_visible (preview, FALSE);
        }
    }

  if (tg_tool->boundary_in)
    {
      ligma_canvas_item_begin_change (tg_tool->boundary_in);
      ligma_canvas_item_set_visible (tg_tool->boundary_in,
                                    tr_tool->transform_valid);
      g_object_set (tg_tool->boundary_in,
                    "transform", &tr_tool->transform,
                    NULL);
      ligma_canvas_item_end_change (tg_tool->boundary_in);
    }

  if (tg_tool->boundary_out)
    {
      ligma_canvas_item_begin_change (tg_tool->boundary_out);
      ligma_canvas_item_set_visible (tg_tool->boundary_out,
                                    tr_tool->transform_valid);
      g_object_set (tg_tool->boundary_out,
                    "transform", &tr_tool->transform,
                    NULL);
      ligma_canvas_item_end_change (tg_tool->boundary_out);
    }

  for (i = 0; i < tg_tool->strokes->len; i++)
    {
      LigmaCanvasItem *item = g_ptr_array_index (tg_tool->strokes, i);

      ligma_canvas_item_begin_change (item);
      ligma_canvas_item_set_visible (item, tr_tool->transform_valid);
      g_object_set (item,
                    "transform", &tr_tool->transform,
                    NULL);
      ligma_canvas_item_end_change (item);
    }
}

static void
ligma_transform_grid_tool_update_filters (LigmaTransformGridTool *tg_tool)
{
  LigmaTool                 *tool = LIGMA_TOOL (tg_tool);
  GHashTable               *new_drawables;
  GList                    *drawables;
  GList                    *iter;
  LigmaDrawable             *drawable;
  GHashTableIter            hash_iter;

  if (! tg_tool->filters)
    return;

  drawables = ligma_image_item_list_filter (g_list_copy (tool->drawables));

  new_drawables = g_hash_table_new (g_direct_hash, g_direct_equal);

  for (iter = drawables; iter; iter = g_list_next (iter))
    g_hash_table_add (new_drawables, iter->data);

  for (iter = tg_tool->preview_drawables; iter; iter = g_list_next (iter))
    {
      drawable = iter->data;

      if (! g_hash_table_remove (new_drawables, drawable))
        ligma_transform_grid_tool_remove_filter (drawable, tg_tool);
    }

  g_hash_table_iter_init (&hash_iter, new_drawables);

  while (g_hash_table_iter_next (&hash_iter, (gpointer *) &drawable, NULL))
    {
      AddFilterData data;

      data.tg_tool       = tg_tool;
      data.root_drawable = drawable;

      ligma_transform_grid_tool_add_filter (drawable, &data);
    }

  g_hash_table_unref (new_drawables);

  g_list_free (tg_tool->preview_drawables);
  tg_tool->preview_drawables = drawables;
}

static void
ligma_transform_grid_tool_hide_selected_objects (LigmaTransformGridTool *tg_tool,
                                                GList                 *objects)
{
  LigmaTransformGridOptions *options    = LIGMA_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  LigmaTransformOptions     *tr_options = LIGMA_TRANSFORM_OPTIONS (options);
  LigmaDisplay              *display    = LIGMA_TOOL (tg_tool)->display;
  LigmaImage                *image      = ligma_display_get_image (display);

  g_return_if_fail (tr_options->type != LIGMA_TRANSFORM_TYPE_IMAGE ||
                    (g_list_length (objects) == 1 && LIGMA_IS_IMAGE (objects->data)));

  g_list_free (tg_tool->hidden_objects);
  tg_tool->hidden_objects = NULL;

  if (options->show_preview)
    {
      if (tr_options->type == LIGMA_TRANSFORM_TYPE_IMAGE)
        {
          tg_tool->hidden_objects = g_list_copy (objects);
          ligma_display_shell_set_show_image (ligma_display_get_shell (display),
                                             FALSE);
        }
      else
        {
          /*  hide only complete layers and channels, not layer masks  */
          GList *iter;

          for (iter = objects; iter; iter = iter->next)
            {
              if (tr_options->type == LIGMA_TRANSFORM_TYPE_LAYER &&
                  ! options->composited_preview                 &&
                  LIGMA_IS_DRAWABLE (iter->data)                 &&
                  ! LIGMA_IS_LAYER_MASK (iter->data)             &&
                  ligma_item_get_visible (iter->data)            &&
                  ligma_channel_is_empty (ligma_image_get_mask (image)))
                {
                  tg_tool->hidden_objects = g_list_prepend (tg_tool->hidden_objects, iter->data);

                  ligma_item_set_visible (iter->data, FALSE, FALSE);
                }
            }

          ligma_projection_flush (ligma_image_get_projection (image));
        }
    }
}

static void
ligma_transform_grid_tool_show_selected_objects (LigmaTransformGridTool *tg_tool)
{
  if (tg_tool->hidden_objects)
    {
      LigmaDisplay *display = LIGMA_TOOL (tg_tool)->display;
      LigmaImage   *image   = ligma_display_get_image (display);
      GList       *iter;

      for (iter = tg_tool->hidden_objects; iter; iter = iter->next)
        {
          if (LIGMA_IS_ITEM (iter->data))
            {
              ligma_item_set_visible (LIGMA_ITEM (iter->data), TRUE,
                                     FALSE);
            }
          else
            {
              g_return_if_fail (LIGMA_IS_IMAGE (iter->data));

              ligma_display_shell_set_show_image (ligma_display_get_shell (display),
                                                 TRUE);
            }
        }

      g_list_free (tg_tool->hidden_objects);
      tg_tool->hidden_objects = NULL;

      ligma_image_flush (image);
    }
}

static void
ligma_transform_grid_tool_add_filter (LigmaDrawable  *drawable,
                                     AddFilterData *data)
{
  Filter        *filter;
  LigmaLayerMode  mode = LIGMA_LAYER_MODE_NORMAL;

  if (LIGMA_IS_LAYER (drawable))
    {
      ligma_layer_get_effective_mode (LIGMA_LAYER (drawable),
                                     &mode, NULL, NULL, NULL);
    }

  if (mode != LIGMA_LAYER_MODE_PASS_THROUGH)
    {
      filter = filter_new (data->tg_tool, drawable, data->root_drawable, TRUE);
    }
  else
    {
      LigmaContainer *container;

      filter = filter_new (data->tg_tool, drawable, data->root_drawable, FALSE);

      container = ligma_viewable_get_children (LIGMA_VIEWABLE (drawable));

      ligma_container_foreach (container,
                              (GFunc) ligma_transform_grid_tool_add_filter,
                              data);
    }

  g_hash_table_insert (data->tg_tool->filters, drawable, filter);

  if (LIGMA_IS_LAYER (drawable))
    {
      LigmaLayerMask *mask = ligma_layer_get_mask (LIGMA_LAYER (drawable));

      if (mask)
        ligma_transform_grid_tool_add_filter (LIGMA_DRAWABLE (mask), data);
    }
}

static void
ligma_transform_grid_tool_remove_filter (LigmaDrawable          *drawable,
                                        LigmaTransformGridTool *tg_tool)
{
  Filter *filter = g_hash_table_lookup (tg_tool->filters, drawable);

  if (LIGMA_IS_LAYER (drawable))
    {
      LigmaLayerMask *mask = ligma_layer_get_mask (LIGMA_LAYER (drawable));

      if (mask)
        ligma_transform_grid_tool_remove_filter (LIGMA_DRAWABLE (mask), tg_tool);
    }

  if (! filter->filter)
    {
      LigmaContainer *container;

      container = ligma_viewable_get_children (LIGMA_VIEWABLE (drawable));

      ligma_container_foreach (container,
                              (GFunc) ligma_transform_grid_tool_remove_filter,
                              tg_tool);
    }

  g_hash_table_remove (tg_tool->filters, drawable);
}

static void
ligma_transform_grid_tool_effective_mode_changed (LigmaLayer             *layer,
                                                 LigmaTransformGridTool *tg_tool)
{
  Filter        *filter = g_hash_table_lookup (tg_tool->filters, layer);
  LigmaLayerMode  mode;
  gboolean       old_pass_through;
  gboolean       new_pass_through;

  ligma_layer_get_effective_mode (layer, &mode, NULL, NULL, NULL);

  old_pass_through = ! filter->filter;
  new_pass_through = mode == LIGMA_LAYER_MODE_PASS_THROUGH;

  if (old_pass_through != new_pass_through)
    {
      AddFilterData data;

      data.tg_tool       = tg_tool;
      data.root_drawable = filter->root_drawable;

      ligma_transform_grid_tool_remove_filter (LIGMA_DRAWABLE (layer), tg_tool);
      ligma_transform_grid_tool_add_filter    (LIGMA_DRAWABLE (layer), &data);

      ligma_transform_grid_tool_update_preview (tg_tool);
    }
}

static Filter *
filter_new (LigmaTransformGridTool *tg_tool,
            LigmaDrawable          *drawable,
            LigmaDrawable          *root_drawable,
            gboolean               add_filter)
{
  Filter   *filter = g_slice_new0 (Filter);
  GeglNode *node;
  GeglNode *input_node;
  GeglNode *output_node;

  filter->tg_tool       = tg_tool;
  filter->drawable      = drawable;
  filter->root_drawable = root_drawable;

  if (add_filter)
    {
      node = gegl_node_new ();

      input_node  = gegl_node_get_input_proxy (node, "input");
      output_node = gegl_node_get_input_proxy (node, "output");

      filter->transform_node = gegl_node_new_child (
        node,
        "operation", "gegl:transform",
        "near-z",    LIGMA_TRANSFORM_NEAR_Z,
        "sampler",   GEGL_SAMPLER_NEAREST,
        NULL);

      filter->crop_node = gegl_node_new_child (
        node,
        "operation", "gegl:crop",
        NULL);

      gegl_node_link_many (input_node,
                           filter->transform_node,
                           filter->crop_node,
                           output_node,
                           NULL);

      ligma_gegl_node_set_underlying_operation (node, filter->transform_node);

      filter->filter = ligma_drawable_filter_new (
        drawable,
        LIGMA_TRANSFORM_TOOL_GET_CLASS (tg_tool)->undo_desc,
        node,
        ligma_tool_get_icon_name (LIGMA_TOOL (tg_tool)));

      ligma_drawable_filter_set_clip (filter->filter, FALSE);
      ligma_drawable_filter_set_override_constraints (filter->filter, TRUE);

      g_signal_connect (
        filter->filter, "flush",
        G_CALLBACK (ligma_transform_grid_tool_filter_flush),
        tg_tool);

      g_object_unref (node);
    }

  if (LIGMA_IS_GROUP_LAYER (drawable))
    {
      g_signal_connect (
        drawable, "effective-mode-changed",
        G_CALLBACK (ligma_transform_grid_tool_effective_mode_changed),
        tg_tool);
    }

  return filter;
}

static void
filter_free (Filter *filter)
{
  if (filter->filter)
    {
      ligma_drawable_filter_abort (filter->filter);

      g_object_unref (filter->filter);
    }

  if (LIGMA_IS_GROUP_LAYER (filter->drawable))
    {
      g_signal_handlers_disconnect_by_func (
        filter->drawable,
        ligma_transform_grid_tool_effective_mode_changed,
        filter->tg_tool);
    }

  g_slice_free (Filter, filter);
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
  return trans_info_equal (trans_infos1[LIGMA_TRANSFORM_FORWARD],
                           trans_infos2[LIGMA_TRANSFORM_FORWARD]) &&
         trans_info_equal (trans_infos1[LIGMA_TRANSFORM_BACKWARD],
                           trans_infos2[LIGMA_TRANSFORM_BACKWARD]);
}

gboolean
ligma_transform_grid_tool_info_to_matrix (LigmaTransformGridTool *tg_tool,
                                         LigmaMatrix3           *transform)
{
  g_return_val_if_fail (LIGMA_IS_TRANSFORM_GRID_TOOL (tg_tool), FALSE);
  g_return_val_if_fail (transform != NULL, FALSE);

  if (LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->info_to_matrix)
    {
      return LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->info_to_matrix (
        tg_tool, transform);
    }

  return FALSE;
}

void
ligma_transform_grid_tool_matrix_to_info (LigmaTransformGridTool *tg_tool,
                                         const LigmaMatrix3     *transform)
{
  g_return_if_fail (LIGMA_IS_TRANSFORM_GRID_TOOL (tg_tool));
  g_return_if_fail (transform != NULL);

  if (LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info)
    {
      return LIGMA_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info (
        tg_tool, transform);
    }
}

void
ligma_transform_grid_tool_push_internal_undo (LigmaTransformGridTool *tg_tool,
                                             gboolean               compress)
{
  UndoInfo *undo_info;

  g_return_if_fail (LIGMA_IS_TRANSFORM_GRID_TOOL (tg_tool));
  g_return_if_fail (tg_tool->undo_list != NULL);

  undo_info = tg_tool->undo_list->data;

  /* push current state on the undo list and set this state as the
   * current state, but avoid doing this if there were no changes
   */
  if (! trans_infos_equal (undo_info->trans_infos, tg_tool->trans_infos))
    {
      LigmaTransformOptions *tr_options = LIGMA_TRANSFORM_TOOL_GET_OPTIONS (tg_tool);
      gint64                time       = 0;
      gboolean              flush = FALSE;

      if (tg_tool->undo_list->next == NULL)
        flush = TRUE;

      if (compress)
        time = g_get_monotonic_time ();

      if (! compress || time - undo_info->time >= UNDO_COMPRESS_TIME)
        {
          undo_info = undo_info_new ();

          tg_tool->undo_list = g_list_prepend (tg_tool->undo_list, undo_info);
        }

      undo_info->time      = time;
      undo_info->direction = tr_options->direction;
      memcpy (undo_info->trans_infos, tg_tool->trans_infos,
              sizeof (tg_tool->trans_infos));

      /* If we undid anything and started interacting, we have to
       * discard the redo history
       */
      if (tg_tool->redo_list)
        {
          g_list_free_full (tg_tool->redo_list,
                            (GDestroyNotify) undo_info_free);
          tg_tool->redo_list = NULL;

          flush = TRUE;
        }

      ligma_transform_grid_tool_update_sensitivity (tg_tool);

      /*  update the undo actions / menu items  */
      if (flush)
        {
          ligma_image_flush (
            ligma_display_get_image (LIGMA_TOOL (tg_tool)->display));
        }
    }
}
