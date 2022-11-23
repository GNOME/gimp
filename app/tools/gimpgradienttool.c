/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Major improvements for interactivity
 * Copyright (C) 2014 Michael Henning <drawoc@darkrefraction.com>
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

#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmaguiconfig.h"

#include "gegl/ligma-gegl-utils.h"

#include "operations/ligma-operation-config.h"

#include "operations/layer-modes/ligma-layer-modes.h"

#include "core/ligma.h"
#include "core/ligmadrawable.h"
#include "core/ligmadrawable-gradient.h"
#include "core/ligmadrawablefilter.h"
#include "core/ligmaerror.h"
#include "core/ligmagradient.h"
#include "core/ligmaimage.h"
#include "core/ligmaprogress.h"
#include "core/ligmaprojection.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmatoolline.h"

#include "ligmagradientoptions.h"
#include "ligmagradienttool.h"
#include "ligmagradienttool-editor.h"
#include "ligmatoolcontrol.h"
#include "ligmatools-utils.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   ligma_gradient_tool_dispose             (GObject               *object);

static gboolean ligma_gradient_tool_initialize        (LigmaTool              *tool,
                                                      LigmaDisplay           *display,
                                                      GError               **error);
static void   ligma_gradient_tool_control             (LigmaTool              *tool,
                                                      LigmaToolAction         action,
                                                      LigmaDisplay           *display);
static void   ligma_gradient_tool_button_press        (LigmaTool              *tool,
                                                      const LigmaCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      LigmaButtonPressType    press_type,
                                                      LigmaDisplay           *display);
static void   ligma_gradient_tool_button_release      (LigmaTool              *tool,
                                                      const LigmaCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      LigmaButtonReleaseType  release_type,
                                                      LigmaDisplay           *display);
static void   ligma_gradient_tool_motion              (LigmaTool              *tool,
                                                      const LigmaCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      LigmaDisplay           *display);
static gboolean ligma_gradient_tool_key_press         (LigmaTool              *tool,
                                                      GdkEventKey           *kevent,
                                                      LigmaDisplay           *display);
static void   ligma_gradient_tool_modifier_key        (LigmaTool              *tool,
                                                      GdkModifierType        key,
                                                      gboolean               press,
                                                      GdkModifierType        state,
                                                      LigmaDisplay           *display);
static void   ligma_gradient_tool_cursor_update       (LigmaTool              *tool,
                                                      const LigmaCoords      *coords,
                                                      GdkModifierType        state,
                                                      LigmaDisplay           *display);
static const gchar * ligma_gradient_tool_can_undo     (LigmaTool              *tool,
                                                      LigmaDisplay           *display);
static const gchar * ligma_gradient_tool_can_redo     (LigmaTool              *tool,
                                                      LigmaDisplay           *display);
static gboolean  ligma_gradient_tool_undo             (LigmaTool              *tool,
                                                      LigmaDisplay           *display);
static gboolean  ligma_gradient_tool_redo             (LigmaTool              *tool,
                                                      LigmaDisplay           *display);
static void   ligma_gradient_tool_options_notify      (LigmaTool              *tool,
                                                      LigmaToolOptions       *options,
                                                      const GParamSpec      *pspec);

static void   ligma_gradient_tool_start               (LigmaGradientTool         *gradient_tool,
                                                      const LigmaCoords      *coords,
                                                      LigmaDisplay           *display);
static void   ligma_gradient_tool_halt                (LigmaGradientTool         *gradient_tool);
static void   ligma_gradient_tool_commit              (LigmaGradientTool         *gradient_tool);

static void   ligma_gradient_tool_line_changed        (LigmaToolWidget        *widget,
                                                      LigmaGradientTool         *gradient_tool);
static void   ligma_gradient_tool_line_response       (LigmaToolWidget        *widget,
                                                      gint                   response_id,
                                                      LigmaGradientTool         *gradient_tool);

static void   ligma_gradient_tool_precalc_shapeburst  (LigmaGradientTool         *gradient_tool);

static void   ligma_gradient_tool_create_graph        (LigmaGradientTool         *gradient_tool);
static void   ligma_gradient_tool_update_graph        (LigmaGradientTool         *gradient_tool);

static void   ligma_gradient_tool_fg_bg_changed       (LigmaGradientTool         *gradient_tool);

static void   ligma_gradient_tool_gradient_dirty      (LigmaGradientTool         *gradient_tool);
static void   ligma_gradient_tool_set_gradient        (LigmaGradientTool         *gradient_tool,
                                                      LigmaGradient          *gradient);

static gboolean ligma_gradient_tool_is_shapeburst     (LigmaGradientTool         *gradient_tool);

static void   ligma_gradient_tool_create_filter       (LigmaGradientTool         *gradient_tool,
                                                      LigmaDrawable          *drawable);
static void   ligma_gradient_tool_filter_flush        (LigmaDrawableFilter    *filter,
                                                      LigmaTool              *tool);


G_DEFINE_TYPE (LigmaGradientTool, ligma_gradient_tool, LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_gradient_tool_parent_class


void
ligma_gradient_tool_register (LigmaToolRegisterCallback  callback,
                             gpointer                  data)
{
  (* callback) (LIGMA_TYPE_GRADIENT_TOOL,
                LIGMA_TYPE_GRADIENT_OPTIONS,
                ligma_gradient_options_gui,
                LIGMA_CONTEXT_PROP_MASK_FOREGROUND |
                LIGMA_CONTEXT_PROP_MASK_BACKGROUND |
                LIGMA_CONTEXT_PROP_MASK_OPACITY    |
                LIGMA_CONTEXT_PROP_MASK_PAINT_MODE |
                LIGMA_CONTEXT_PROP_MASK_GRADIENT,
                "ligma-gradient-tool",
                _("Gradient"),
                _("Gradient Tool: Fill selected area with a color gradient"),
                N_("Gra_dient"), "G",
                NULL, LIGMA_HELP_TOOL_GRADIENT,
                LIGMA_ICON_TOOL_GRADIENT,
                data);
}

static void
ligma_gradient_tool_class_init (LigmaGradientToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaToolClass *tool_class   = LIGMA_TOOL_CLASS (klass);

  object_class->dispose      = ligma_gradient_tool_dispose;

  tool_class->initialize     = ligma_gradient_tool_initialize;
  tool_class->control        = ligma_gradient_tool_control;
  tool_class->button_press   = ligma_gradient_tool_button_press;
  tool_class->button_release = ligma_gradient_tool_button_release;
  tool_class->motion         = ligma_gradient_tool_motion;
  tool_class->key_press      = ligma_gradient_tool_key_press;
  tool_class->modifier_key   = ligma_gradient_tool_modifier_key;
  tool_class->cursor_update  = ligma_gradient_tool_cursor_update;
  tool_class->can_undo       = ligma_gradient_tool_can_undo;
  tool_class->can_redo       = ligma_gradient_tool_can_redo;
  tool_class->undo           = ligma_gradient_tool_undo;
  tool_class->redo           = ligma_gradient_tool_redo;
  tool_class->options_notify = ligma_gradient_tool_options_notify;
}

static void
ligma_gradient_tool_init (LigmaGradientTool *gradient_tool)
{
  LigmaTool *tool = LIGMA_TOOL (gradient_tool);

  ligma_tool_control_set_scroll_lock        (tool->control, TRUE);
  ligma_tool_control_set_preserve           (tool->control, FALSE);
  ligma_tool_control_set_dirty_mask         (tool->control,
                                            LIGMA_DIRTY_IMAGE           |
                                            LIGMA_DIRTY_IMAGE_STRUCTURE |
                                            LIGMA_DIRTY_DRAWABLE        |
                                            LIGMA_DIRTY_ACTIVE_DRAWABLE);
  ligma_tool_control_set_dirty_action       (tool->control,
                                            LIGMA_TOOL_ACTION_COMMIT);
  ligma_tool_control_set_wants_click        (tool->control, TRUE);
  ligma_tool_control_set_wants_double_click (tool->control, TRUE);
  ligma_tool_control_set_active_modifiers   (tool->control,
                                            LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  ligma_tool_control_set_precision          (tool->control,
                                            LIGMA_CURSOR_PRECISION_SUBPIXEL);
  ligma_tool_control_set_tool_cursor        (tool->control,
                                            LIGMA_TOOL_CURSOR_GRADIENT);
  ligma_tool_control_set_action_opacity     (tool->control,
                                            "context/context-opacity-set");
  ligma_tool_control_set_action_object_1    (tool->control,
                                            "context/context-gradient-select-set");

  ligma_draw_tool_set_default_status (LIGMA_DRAW_TOOL (tool),
                                     _("Click-Drag to draw a gradient"));
}

static void
ligma_gradient_tool_dispose (GObject *object)
{
  LigmaGradientTool *gradient_tool = LIGMA_GRADIENT_TOOL (object);

  ligma_gradient_tool_set_gradient (gradient_tool, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
ligma_gradient_tool_initialize (LigmaTool     *tool,
                               LigmaDisplay  *display,
                               GError      **error)
{
  LigmaImage           *image       = ligma_display_get_image (display);
  LigmaGradientOptions *options     = LIGMA_GRADIENT_TOOL_GET_OPTIONS (tool);
  LigmaGuiConfig       *config      = LIGMA_GUI_CONFIG (display->ligma->config);
  LigmaItem            *locked_item = NULL;
  GList               *drawables;
  LigmaDrawable        *drawable;

  if (! LIGMA_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  drawables = ligma_image_get_selected_drawables (image);
  if (g_list_length (drawables) != 1)
    {
      if (g_list_length (drawables) > 1)
        ligma_tool_message_literal (tool, display,
                                   _("Cannot paint on multiple drawables. Select only one."));
      else
        ligma_tool_message_literal (tool, display, _("No active drawables."));

      g_list_free (drawables);

      return FALSE;
    }

  /* Only a single drawable at this point. */
  drawable = drawables->data;
  g_list_free (drawables);

  if (ligma_viewable_get_children (LIGMA_VIEWABLE (drawable)))
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("Cannot modify the pixels of layer groups."));
      return FALSE;
    }

  if (ligma_item_is_content_locked (LIGMA_ITEM (drawable), &locked_item))
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("The selected layer's pixels are locked."));
      if (error)
        ligma_tools_blink_lock_box (display->ligma, locked_item);
      return FALSE;
    }

  if (! ligma_item_is_visible (LIGMA_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("The selected item is not visible."));
      return FALSE;
    }

  if (! ligma_context_get_gradient (LIGMA_CONTEXT (options)))
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("No gradient available for use with this tool."));
      return FALSE;
    }

  return TRUE;
}

static void
ligma_gradient_tool_control (LigmaTool       *tool,
                            LigmaToolAction  action,
                            LigmaDisplay    *display)
{
  LigmaGradientTool *gradient_tool = LIGMA_GRADIENT_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_gradient_tool_halt (gradient_tool);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      ligma_gradient_tool_commit (gradient_tool);
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_gradient_tool_button_press (LigmaTool            *tool,
                                 const LigmaCoords    *coords,
                                 guint32              time,
                                 GdkModifierType      state,
                                 LigmaButtonPressType  press_type,
                                 LigmaDisplay         *display)
{
  LigmaGradientTool *gradient_tool = LIGMA_GRADIENT_TOOL (tool);

  if (tool->display && display != tool->display)
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);

  if (! gradient_tool->widget)
    {
      ligma_gradient_tool_start (gradient_tool, coords, display);

      ligma_tool_widget_hover (gradient_tool->widget, coords, state, TRUE);
    }

  /* call start_edit() before widget_button_press(), because we need to record
   * the undo state before widget_button_press() potentially changes it.  note
   * that if widget_button_press() return FALSE, nothing changes and no undo
   * step is created.
   */
  if (press_type == LIGMA_BUTTON_PRESS_NORMAL)
    ligma_gradient_tool_editor_start_edit (gradient_tool);

  if (ligma_tool_widget_button_press (gradient_tool->widget, coords, time, state,
                                     press_type))
    {
      gradient_tool->grab_widget = gradient_tool->widget;
    }

  if (press_type == LIGMA_BUTTON_PRESS_NORMAL)
    ligma_tool_control_activate (tool->control);
}

static void
ligma_gradient_tool_button_release (LigmaTool              *tool,
                                   const LigmaCoords      *coords,
                                   guint32                time,
                                   GdkModifierType        state,
                                   LigmaButtonReleaseType  release_type,
                                   LigmaDisplay           *display)
{
  LigmaGradientTool    *gradient_tool = LIGMA_GRADIENT_TOOL (tool);
  LigmaGradientOptions *options       = LIGMA_GRADIENT_TOOL_GET_OPTIONS (tool);

  ligma_tool_pop_status (tool, display);

  ligma_tool_control_halt (tool->control);

  if (gradient_tool->grab_widget)
    {
      ligma_tool_widget_button_release (gradient_tool->grab_widget,
                                       coords, time, state, release_type);
      gradient_tool->grab_widget = NULL;

      if (options->instant)
        {
          if (release_type == LIGMA_BUTTON_RELEASE_CANCEL)
            ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);
          else
            ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, display);
        }
    }

  if (! options->instant)
    {
      ligma_gradient_tool_editor_end_edit (gradient_tool,
                                          release_type ==
                                          LIGMA_BUTTON_RELEASE_CANCEL);
    }
}

static void
ligma_gradient_tool_motion (LigmaTool         *tool,
                           const LigmaCoords *coords,
                           guint32           time,
                           GdkModifierType   state,
                           LigmaDisplay      *display)
{
  LigmaGradientTool *gradient_tool = LIGMA_GRADIENT_TOOL (tool);

  if (gradient_tool->grab_widget)
    {
      ligma_tool_widget_motion (gradient_tool->grab_widget, coords, time, state);
    }
}

static gboolean
ligma_gradient_tool_key_press (LigmaTool    *tool,
                              GdkEventKey *kevent,
                              LigmaDisplay *display)
{
  LigmaGradientTool *gradient_tool = LIGMA_GRADIENT_TOOL (tool);
  LigmaDrawTool     *draw_tool     = LIGMA_DRAW_TOOL (tool);
  gboolean          result;

  /* call start_edit() before widget_key_press(), because we need to record the
   * undo state before widget_key_press() potentially changes it.  note that if
   * widget_key_press() return FALSE, nothing changes and no undo step is
   * created.
   */
  if (display == draw_tool->display)
    ligma_gradient_tool_editor_start_edit (gradient_tool);

  result = LIGMA_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);

  if (display == draw_tool->display)
    ligma_gradient_tool_editor_end_edit (gradient_tool, FALSE);

  return result;
}

static void
ligma_gradient_tool_modifier_key (LigmaTool        *tool,
                                 GdkModifierType  key,
                                 gboolean         press,
                                 GdkModifierType  state,
                                 LigmaDisplay     *display)
{
  LigmaGradientOptions *options = LIGMA_GRADIENT_TOOL_GET_OPTIONS (tool);

  if (key == ligma_get_extend_selection_mask ())
    {
      if (options->instant_toggle &&
          gtk_widget_get_sensitive (options->instant_toggle))
        {
          g_object_set (options,
                        "instant", ! options->instant,
                        NULL);
        }
    }
}

static void
ligma_gradient_tool_cursor_update (LigmaTool         *tool,
                                  const LigmaCoords *coords,
                                  GdkModifierType   state,
                                  LigmaDisplay      *display)
{
  LigmaGuiConfig *config    = LIGMA_GUI_CONFIG (display->ligma->config);
  LigmaImage     *image     = ligma_display_get_image (display);
  GList         *drawables = ligma_image_get_selected_drawables (image);

  if (g_list_length (drawables) != 1                      ||
      ligma_viewable_get_children (drawables->data)        ||
      ligma_item_is_content_locked (drawables->data, NULL) ||
      ! (ligma_item_is_visible (drawables->data) ||
         config->edit_non_visible))
    {
      ligma_tool_set_cursor (tool, display,
                            ligma_tool_control_get_cursor (tool->control),
                            ligma_tool_control_get_tool_cursor (tool->control),
                            LIGMA_CURSOR_MODIFIER_BAD);
      g_list_free (drawables);
      return;
    }
  g_list_free (drawables);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static const gchar *
ligma_gradient_tool_can_undo (LigmaTool    *tool,
                             LigmaDisplay *display)
{
  return ligma_gradient_tool_editor_can_undo (LIGMA_GRADIENT_TOOL (tool));
}

static const gchar *
ligma_gradient_tool_can_redo (LigmaTool    *tool,
                             LigmaDisplay *display)
{
  return ligma_gradient_tool_editor_can_redo (LIGMA_GRADIENT_TOOL (tool));
}

static gboolean
ligma_gradient_tool_undo (LigmaTool    *tool,
                         LigmaDisplay *display)
{
  return ligma_gradient_tool_editor_undo (LIGMA_GRADIENT_TOOL (tool));
}

static gboolean
ligma_gradient_tool_redo (LigmaTool    *tool,
                         LigmaDisplay *display)
{
  return ligma_gradient_tool_editor_redo (LIGMA_GRADIENT_TOOL (tool));
}

static void
ligma_gradient_tool_options_notify (LigmaTool         *tool,
                                   LigmaToolOptions  *options,
                                   const GParamSpec *pspec)
{
  LigmaContext   *context    = LIGMA_CONTEXT (options);
  LigmaGradientTool *gradient_tool = LIGMA_GRADIENT_TOOL (tool);

  if (! strcmp (pspec->name, "gradient"))
    {
      ligma_gradient_tool_set_gradient (gradient_tool, context->gradient);

      if (gradient_tool->filter)
        ligma_drawable_filter_apply (gradient_tool->filter, NULL);
    }
  else if (gradient_tool->render_node &&
           gegl_node_find_property (gradient_tool->render_node, pspec->name))
    {
      /* Sync any property changes on the config object that match the op */
      GValue value = G_VALUE_INIT;

      g_value_init (&value, pspec->value_type);

      g_object_get_property (G_OBJECT (options), pspec->name, &value);
      gegl_node_set_property (gradient_tool->render_node, pspec->name, &value);

      g_value_unset (&value);

      if (! strcmp (pspec->name, "gradient-type"))
        {
          LigmaRepeatMode   gradient_repeat;
          LigmaRepeatMode   node_repeat;
          LigmaGradientType gradient_type;

          gradient_repeat = LIGMA_PAINT_OPTIONS (options)->gradient_options->gradient_repeat;
          gradient_type   = LIGMA_GRADIENT_OPTIONS (options)->gradient_type;
          gegl_node_get (gradient_tool->render_node,
                         "gradient-repeat", &node_repeat,
                         NULL);

          if (gradient_type >= LIGMA_GRADIENT_SHAPEBURST_ANGULAR)
            {
              /* These gradient types are only meant to work with repeat
               * value of "none" so these are the only ones where we
               * don't keep the render node and the gradient options in
               * sync.
               * We could instead reset the "gradient-repeat" value on
               * LigmaGradientOptions, but I assume one would want to revert
               * back to the last set value if changing back the
               * gradient type. So instead we just make the option
               * insensitive (both in GUI and in render).
               */
              if (node_repeat != LIGMA_REPEAT_NONE)
                gegl_node_set (gradient_tool->render_node,
                               "gradient-repeat", LIGMA_REPEAT_NONE,
                               NULL);
            }
          else if (node_repeat != gradient_repeat)
            {
              gegl_node_set (gradient_tool->render_node,
                             "gradient-repeat", gradient_repeat,
                             NULL);
            }

          if (ligma_gradient_tool_is_shapeburst (gradient_tool))
            ligma_gradient_tool_precalc_shapeburst (gradient_tool);

          ligma_gradient_tool_update_graph (gradient_tool);
        }

      ligma_drawable_filter_apply (gradient_tool->filter, NULL);
    }
  else if (gradient_tool->render_node                    &&
           ligma_gradient_tool_is_shapeburst (gradient_tool) &&
           g_strcmp0 (pspec->name, "distance-metric") == 0)
    {
      g_clear_object (&gradient_tool->dist_buffer);
      ligma_gradient_tool_precalc_shapeburst (gradient_tool);
      ligma_gradient_tool_update_graph (gradient_tool);
      ligma_drawable_filter_apply (gradient_tool->filter, NULL);
    }
  else if (gradient_tool->filter &&
           ! strcmp (pspec->name, "opacity"))
    {
      ligma_drawable_filter_set_opacity (gradient_tool->filter,
                                        ligma_context_get_opacity (context));
    }
  else if (gradient_tool->filter &&
           ! strcmp (pspec->name, "paint-mode"))
    {
      ligma_drawable_filter_set_mode (gradient_tool->filter,
                                     ligma_context_get_paint_mode (context),
                                     LIGMA_LAYER_COLOR_SPACE_AUTO,
                                     LIGMA_LAYER_COLOR_SPACE_AUTO,
                                     ligma_layer_mode_get_paint_composite_mode (
                                       ligma_context_get_paint_mode (context)));
    }

  ligma_gradient_tool_editor_options_notify (gradient_tool, options, pspec);
}

static void
ligma_gradient_tool_start (LigmaGradientTool *gradient_tool,
                          const LigmaCoords *coords,
                          LigmaDisplay      *display)
{
  LigmaTool            *tool      = LIGMA_TOOL (gradient_tool);
  LigmaDisplayShell    *shell     = ligma_display_get_shell (display);
  LigmaImage           *image     = ligma_display_get_image (display);
  GList               *drawables = ligma_image_get_selected_drawables (image);
  LigmaGradientOptions *options   = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  LigmaContext         *context   = LIGMA_CONTEXT (options);

  g_return_if_fail (g_list_length (drawables) == 1);

  if (options->instant_toggle)
    gtk_widget_set_sensitive (options->instant_toggle, FALSE);

  tool->display   = display;
  g_list_free (tool->drawables);
  tool->drawables = drawables;

  gradient_tool->start_x = coords->x;
  gradient_tool->start_y = coords->y;
  gradient_tool->end_x   = coords->x;
  gradient_tool->end_y   = coords->y;

  gradient_tool->widget = ligma_tool_line_new (shell,
                                              gradient_tool->start_x,
                                              gradient_tool->start_y,
                                              gradient_tool->end_x,
                                              gradient_tool->end_y);

  g_object_set (gradient_tool->widget,
                "status-title", _("Gradient: "),
                NULL);

  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (tool), gradient_tool->widget);

  g_signal_connect (gradient_tool->widget, "changed",
                    G_CALLBACK (ligma_gradient_tool_line_changed),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "response",
                    G_CALLBACK (ligma_gradient_tool_line_response),
                    gradient_tool);

  g_signal_connect_swapped (context, "background-changed",
                            G_CALLBACK (ligma_gradient_tool_fg_bg_changed),
                            gradient_tool);
  g_signal_connect_swapped (context, "foreground-changed",
                            G_CALLBACK (ligma_gradient_tool_fg_bg_changed),
                            gradient_tool);

  ligma_gradient_tool_create_filter (gradient_tool, tool->drawables->data);

  /* Initially sync all of the properties */
  ligma_operation_config_sync_node (G_OBJECT (options),
                                   gradient_tool->render_node);

  /* We don't allow repeat values for some shapes. */
  if (options->gradient_type >= LIGMA_GRADIENT_SHAPEBURST_ANGULAR)
    gegl_node_set (gradient_tool->render_node,
                   "gradient-repeat", LIGMA_REPEAT_NONE,
                   NULL);

  /* Connect signal handlers for the gradient */
  ligma_gradient_tool_set_gradient (gradient_tool, context->gradient);

  if (ligma_gradient_tool_is_shapeburst (gradient_tool))
    ligma_gradient_tool_precalc_shapeburst (gradient_tool);

  ligma_draw_tool_start (LIGMA_DRAW_TOOL (gradient_tool), display);

  ligma_gradient_tool_editor_start (gradient_tool);
}

static void
ligma_gradient_tool_halt (LigmaGradientTool *gradient_tool)
{
  LigmaTool            *tool    = LIGMA_TOOL (gradient_tool);
  LigmaGradientOptions *options = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  LigmaContext         *context = LIGMA_CONTEXT (options);

  ligma_gradient_tool_editor_halt (gradient_tool);

  if (gradient_tool->graph)
    {
      g_clear_object (&gradient_tool->graph);
      gradient_tool->render_node    = NULL;
#if 0
      gradient_tool->subtract_node  = NULL;
      gradient_tool->divide_node    = NULL;
#endif
      gradient_tool->dist_node      = NULL;
    }

  g_clear_object (&gradient_tool->dist_buffer);

  if (gradient_tool->filter)
    {
      ligma_tool_control_push_preserve (tool->control, TRUE);

      ligma_drawable_filter_abort (gradient_tool->filter);
      g_object_unref (gradient_tool->filter);
      gradient_tool->filter = NULL;

      ligma_tool_control_pop_preserve (tool->control);

      ligma_image_flush (ligma_display_get_image (tool->display));
    }

  ligma_gradient_tool_set_tentative_gradient (gradient_tool, NULL);

  g_signal_handlers_disconnect_by_func (context,
                                        G_CALLBACK (ligma_gradient_tool_fg_bg_changed),
                                        gradient_tool);

  if (tool->display)
    ligma_tool_pop_status (tool, tool->display);

  if (ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (gradient_tool)))
    ligma_draw_tool_stop (LIGMA_DRAW_TOOL (gradient_tool));

  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (tool), NULL);
  g_clear_object (&gradient_tool->widget);

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;

  if (options->instant_toggle)
    gtk_widget_set_sensitive (options->instant_toggle, TRUE);
}

static void
ligma_gradient_tool_commit (LigmaGradientTool *gradient_tool)
{
  LigmaTool *tool = LIGMA_TOOL (gradient_tool);

  if (gradient_tool->filter)
    {
      /* halt the editor before committing the filter so that the image-flush
       * idle source is removed, to avoid flushing the image, and hence
       * restarting the projection rendering, while applying the filter.
       */
      ligma_gradient_tool_editor_halt (gradient_tool);

      ligma_tool_control_push_preserve (tool->control, TRUE);

      ligma_drawable_filter_commit (gradient_tool->filter,
                                   LIGMA_PROGRESS (tool), FALSE);
      g_clear_object (&gradient_tool->filter);

      ligma_tool_control_pop_preserve (tool->control);

      ligma_image_flush (ligma_display_get_image (tool->display));
    }
}

static void
ligma_gradient_tool_line_changed (LigmaToolWidget   *widget,
                                 LigmaGradientTool *gradient_tool)
{
  gdouble  start_x;
  gdouble  start_y;
  gdouble  end_x;
  gdouble  end_y;
  gboolean update = FALSE;

  g_object_get (widget,
                "x1", &start_x,
                "y1", &start_y,
                "x2", &end_x,
                "y2", &end_y,
                NULL);

  if (start_x != gradient_tool->start_x ||
      start_y != gradient_tool->start_y ||
      end_x   != gradient_tool->end_x   ||
      end_y   != gradient_tool->end_y)
    {
      gradient_tool->start_x = start_x;
      gradient_tool->start_y = start_y;
      gradient_tool->end_x   = end_x;
      gradient_tool->end_y   = end_y;

      update = TRUE;
    }

  if (ligma_gradient_tool_editor_line_changed (gradient_tool))
    update = TRUE;

  if (update)
    {
      ligma_gradient_tool_update_graph (gradient_tool);
      ligma_drawable_filter_apply (gradient_tool->filter, NULL);
    }
}

static void
ligma_gradient_tool_line_response (LigmaToolWidget *widget,
                                  gint            response_id,
                                  LigmaGradientTool  *gradient_tool)
{
  LigmaTool *tool = LIGMA_TOOL (gradient_tool);

  switch (response_id)
    {
    case LIGMA_TOOL_WIDGET_RESPONSE_CONFIRM:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, tool->display);
      break;

    case LIGMA_TOOL_WIDGET_RESPONSE_CANCEL:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
ligma_gradient_tool_precalc_shapeburst (LigmaGradientTool *gradient_tool)
{
  LigmaGradientOptions *options = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  LigmaTool            *tool    = LIGMA_TOOL (gradient_tool);
  gint                 x, y, width, height;

  if (gradient_tool->dist_buffer || ! tool->drawables)
    return;

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  if (! ligma_item_mask_intersect (LIGMA_ITEM (tool->drawables->data),
                                  &x, &y, &width, &height))
    return;

  gradient_tool->dist_buffer =
    ligma_drawable_gradient_shapeburst_distmap (tool->drawables->data,
                                               options->distance_metric,
                                               GEGL_RECTANGLE (x, y, width, height),
                                               LIGMA_PROGRESS (gradient_tool));

  if (gradient_tool->dist_node)
    gegl_node_set (gradient_tool->dist_node,
                   "buffer", gradient_tool->dist_buffer,
                   NULL);

  ligma_progress_end (LIGMA_PROGRESS (gradient_tool));
}


/* gegl graph stuff */

static void
ligma_gradient_tool_create_graph (LigmaGradientTool *gradient_tool)
{
  LigmaGradientOptions *options = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  LigmaContext         *context = LIGMA_CONTEXT (options);
  GeglNode            *output;

  /* render_node is not supposed to be recreated */
  g_return_if_fail (gradient_tool->graph == NULL);

  gradient_tool->graph = gegl_node_new ();

  gradient_tool->dist_node =
    gegl_node_new_child (gradient_tool->graph,
                         "operation", "gegl:buffer-source",
                         "buffer",    gradient_tool->dist_buffer,
                         NULL);

#if 0
  gradient_tool->subtract_node =
    gegl_node_new_child (gradient_tool->graph,
                         "operation", "gegl:subtract",
                         NULL);

  gradient_tool->divide_node =
    gegl_node_new_child (gradient_tool->graph,
                         "operation", "gegl:divide",
                         NULL);
#endif

  gradient_tool->render_node =
    gegl_node_new_child (gradient_tool->graph,
                         "operation", "ligma:gradient",
                         "context", context,
                         NULL);

  output = gegl_node_get_output_proxy (gradient_tool->graph, "output");

  gegl_node_link_many (gradient_tool->dist_node,
#if 0
                       gradient_tool->subtract_node,
                       gradient_tool->divide_node,
#endif
                       gradient_tool->render_node,
                       output,
                       NULL);

  ligma_gegl_node_set_underlying_operation (gradient_tool->graph,
                                           gradient_tool->render_node);

  ligma_gradient_tool_update_graph (gradient_tool);
}

static void
ligma_gradient_tool_update_graph (LigmaGradientTool *gradient_tool)
{
  LigmaTool            *tool    = LIGMA_TOOL (gradient_tool);
  LigmaGradientOptions *options = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  gint                 off_x, off_y;

  ligma_item_get_offset (LIGMA_ITEM (tool->drawables->data), &off_x, &off_y);

#if 0
  if (ligma_gradient_tool_is_shapeburst (gradient_tool))
    {
      gfloat start, end;

      gegl_buffer_get (gradient_tool->dist_buffer,
                       GEGL_RECTANGLE (gradient_tool->start_x - off_x,
                                       gradient_tool->start_y - off_y,
                                       1, 1),
                       1.0, babl_format("Y float"), &start,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      gegl_buffer_get (gradient_tool->dist_buffer,
                       GEGL_RECTANGLE (gradient_tool->end_x - off_x,
                                       gradient_tool->end_y - off_y,
                                       1, 1),
                       1.0, babl_format("Y float"), &end,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (start != end)
        {
          gegl_node_set (gradient_tool->subtract_node,
                         "value", (gdouble) start,
                         NULL);
          gegl_node_set (gradient_tool->divide_node,
                         "value", (gdouble) (end - start),
                         NULL);
        }
    }
  else
#endif
    {
      GeglRectangle roi;
      gdouble       start_x, start_y;
      gdouble       end_x,   end_y;

      ligma_item_mask_intersect (LIGMA_ITEM (tool->drawables->data),
                                &roi.x, &roi.y, &roi.width, &roi.height);

      start_x = gradient_tool->start_x - off_x;
      start_y = gradient_tool->start_y - off_y;
      end_x   = gradient_tool->end_x   - off_x;
      end_y   = gradient_tool->end_y   - off_y;

      ligma_drawable_gradient_adjust_coords (tool->drawables->data,
                                            options->gradient_type,
                                            &roi,
                                            &start_x, &start_y, &end_x, &end_y);

      gegl_node_set (gradient_tool->render_node,
                     "start-x", start_x,
                     "start-y", start_y,
                     "end-x",   end_x,
                     "end-y",   end_y,
                     NULL);
    }
}

static void
ligma_gradient_tool_fg_bg_changed (LigmaGradientTool *gradient_tool)
{
  if (! gradient_tool->filter || ! gradient_tool->gradient)
    return;

  if (ligma_gradient_has_fg_bg_segments (gradient_tool->gradient))
    {
      /* Set a property on the node. Otherwise it will cache and refuse to update */
      gegl_node_set (gradient_tool->render_node,
                     "gradient", gradient_tool->gradient,
                     NULL);

      /* Update the filter */
      ligma_drawable_filter_apply (gradient_tool->filter, NULL);

      ligma_gradient_tool_editor_fg_bg_changed (gradient_tool);
    }
}

static void
ligma_gradient_tool_gradient_dirty (LigmaGradientTool *gradient_tool)
{
  if (! gradient_tool->filter)
    return;

  if (! gradient_tool->tentative_gradient)
    {
      /* Set a property on the node. Otherwise it will cache and refuse to update */
      gegl_node_set (gradient_tool->render_node,
                     "gradient", gradient_tool->gradient,
                     NULL);

      /* Update the filter */
      ligma_drawable_filter_apply (gradient_tool->filter, NULL);
    }

  ligma_gradient_tool_editor_gradient_dirty (gradient_tool);
}

static void
ligma_gradient_tool_set_gradient (LigmaGradientTool *gradient_tool,
                                 LigmaGradient     *gradient)
{
  if (gradient_tool->gradient)
    g_signal_handlers_disconnect_by_func (gradient_tool->gradient,
                                          G_CALLBACK (ligma_gradient_tool_gradient_dirty),
                                          gradient_tool);


  g_set_object (&gradient_tool->gradient, gradient);

  if (gradient_tool->gradient)
    {
      g_signal_connect_swapped (gradient_tool->gradient, "dirty",
                                G_CALLBACK (ligma_gradient_tool_gradient_dirty),
                                gradient_tool);

      if (gradient_tool->render_node)
        gegl_node_set (gradient_tool->render_node,
                       "gradient", gradient_tool->gradient,
                       NULL);
    }

  ligma_gradient_tool_editor_gradient_changed (gradient_tool);
}

static gboolean
ligma_gradient_tool_is_shapeburst (LigmaGradientTool *gradient_tool)
{
  LigmaGradientOptions *options = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);

  return options->gradient_type >= LIGMA_GRADIENT_SHAPEBURST_ANGULAR &&
         options->gradient_type <= LIGMA_GRADIENT_SHAPEBURST_DIMPLED;
}


/* image map stuff */

static void
ligma_gradient_tool_create_filter (LigmaGradientTool *gradient_tool,
                                  LigmaDrawable     *drawable)
{
  LigmaGradientOptions *options = LIGMA_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  LigmaContext         *context = LIGMA_CONTEXT (options);

  if (! gradient_tool->graph)
    ligma_gradient_tool_create_graph (gradient_tool);

  gradient_tool->filter = ligma_drawable_filter_new (drawable,
                                                 C_("undo-type", "Gradient"),
                                                 gradient_tool->graph,
                                                 LIGMA_ICON_TOOL_GRADIENT);

  ligma_drawable_filter_set_region (gradient_tool->filter,
                                   LIGMA_FILTER_REGION_DRAWABLE);
  ligma_drawable_filter_set_opacity (gradient_tool->filter,
                                    ligma_context_get_opacity (context));
  ligma_drawable_filter_set_mode (gradient_tool->filter,
                                 ligma_context_get_paint_mode (context),
                                 LIGMA_LAYER_COLOR_SPACE_AUTO,
                                 LIGMA_LAYER_COLOR_SPACE_AUTO,
                                 ligma_layer_mode_get_paint_composite_mode (
                                   ligma_context_get_paint_mode (context)));

  g_signal_connect (gradient_tool->filter, "flush",
                    G_CALLBACK (ligma_gradient_tool_filter_flush),
                    gradient_tool);
}

static void
ligma_gradient_tool_filter_flush (LigmaDrawableFilter *filter,
                                 LigmaTool           *tool)
{
  LigmaImage *image = ligma_display_get_image (tool->display);

  ligma_projection_flush (ligma_image_get_projection (image));
}


/*  protected functions  */


void
ligma_gradient_tool_set_tentative_gradient (LigmaGradientTool *gradient_tool,
                                           LigmaGradient     *gradient)
{
  g_return_if_fail (LIGMA_IS_GRADIENT_TOOL (gradient_tool));
  g_return_if_fail (gradient == NULL || LIGMA_IS_GRADIENT (gradient));

  if (g_set_object (&gradient_tool->tentative_gradient, gradient))
    {
      if (gradient_tool->render_node)
        {
          gegl_node_set (gradient_tool->render_node,
                         "gradient", gradient ? gradient : gradient_tool->gradient,
                         NULL);

          ligma_drawable_filter_apply (gradient_tool->filter, NULL);
        }
    }
}
