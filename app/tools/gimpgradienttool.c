/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpguiconfig.h"

#include "gegl/gimp-gegl-utils.h"

#include "operations/gimp-operation-config.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable.h"
#include "core/gimpdrawable-filters.h"
#include "core/gimpdrawable-gradient.h"
#include "core/gimpdrawablefilter.h"
#include "core/gimperror.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"
#include "core/gimpprojection.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolline.h"

#include "gimpgradientoptions.h"
#include "gimpgradienttool.h"
#include "gimpgradienttool-editor.h"
#include "gimptoolcontrol.h"
#include "gimptools-utils.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_gradient_tool_dispose             (GObject               *object);

static gboolean gimp_gradient_tool_initialize        (GimpTool              *tool,
                                                      GimpDisplay           *display,
                                                      GError               **error);
static void   gimp_gradient_tool_control             (GimpTool              *tool,
                                                      GimpToolAction         action,
                                                      GimpDisplay           *display);
static void   gimp_gradient_tool_button_press        (GimpTool              *tool,
                                                      const GimpCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      GimpButtonPressType    press_type,
                                                      GimpDisplay           *display);
static void   gimp_gradient_tool_button_release      (GimpTool              *tool,
                                                      const GimpCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      GimpButtonReleaseType  release_type,
                                                      GimpDisplay           *display);
static void   gimp_gradient_tool_motion              (GimpTool              *tool,
                                                      const GimpCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      GimpDisplay           *display);
static gboolean gimp_gradient_tool_key_press         (GimpTool              *tool,
                                                      GdkEventKey           *kevent,
                                                      GimpDisplay           *display);
static void   gimp_gradient_tool_modifier_key        (GimpTool              *tool,
                                                      GdkModifierType        key,
                                                      gboolean               press,
                                                      GdkModifierType        state,
                                                      GimpDisplay           *display);
static void   gimp_gradient_tool_cursor_update       (GimpTool              *tool,
                                                      const GimpCoords      *coords,
                                                      GdkModifierType        state,
                                                      GimpDisplay           *display);
static const gchar * gimp_gradient_tool_can_undo     (GimpTool              *tool,
                                                      GimpDisplay           *display);
static const gchar * gimp_gradient_tool_can_redo     (GimpTool              *tool,
                                                      GimpDisplay           *display);
static gboolean  gimp_gradient_tool_undo             (GimpTool              *tool,
                                                      GimpDisplay           *display);
static gboolean  gimp_gradient_tool_redo             (GimpTool              *tool,
                                                      GimpDisplay           *display);
static void   gimp_gradient_tool_options_notify      (GimpTool              *tool,
                                                      GimpToolOptions       *options,
                                                      const GParamSpec      *pspec);

static void   gimp_gradient_tool_start               (GimpGradientTool         *gradient_tool,
                                                      const GimpCoords      *coords,
                                                      GimpDisplay           *display);
static void   gimp_gradient_tool_halt                (GimpGradientTool         *gradient_tool);
static void   gimp_gradient_tool_commit              (GimpGradientTool         *gradient_tool);

static void   gimp_gradient_tool_line_changed        (GimpToolWidget        *widget,
                                                      GimpGradientTool         *gradient_tool);
static void   gimp_gradient_tool_line_response       (GimpToolWidget        *widget,
                                                      gint                   response_id,
                                                      GimpGradientTool         *gradient_tool);

static void   gimp_gradient_tool_precalc_shapeburst  (GimpGradientTool         *gradient_tool);

static void   gimp_gradient_tool_create_graph        (GimpGradientTool         *gradient_tool);
static void   gimp_gradient_tool_update_graph        (GimpGradientTool         *gradient_tool);

static void   gimp_gradient_tool_fg_bg_changed       (GimpGradientTool         *gradient_tool);

static void   gimp_gradient_tool_gradient_dirty      (GimpGradientTool         *gradient_tool);
static void   gimp_gradient_tool_set_gradient        (GimpGradientTool         *gradient_tool,
                                                      GimpGradient          *gradient);

static gboolean gimp_gradient_tool_is_shapeburst     (GimpGradientTool         *gradient_tool);

static void   gimp_gradient_tool_create_filter       (GimpGradientTool         *gradient_tool,
                                                      GimpDrawable          *drawable);
static void   gimp_gradient_tool_filter_flush        (GimpDrawableFilter    *filter,
                                                      GimpTool              *tool);


G_DEFINE_TYPE (GimpGradientTool, gimp_gradient_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_gradient_tool_parent_class


void
gimp_gradient_tool_register (GimpToolRegisterCallback  callback,
                             gpointer                  data)
{
  (* callback) (GIMP_TYPE_GRADIENT_TOOL,
                GIMP_TYPE_GRADIENT_OPTIONS,
                gimp_gradient_options_gui,
                GIMP_CONTEXT_PROP_MASK_FOREGROUND |
                GIMP_CONTEXT_PROP_MASK_BACKGROUND |
                GIMP_CONTEXT_PROP_MASK_OPACITY    |
                GIMP_CONTEXT_PROP_MASK_PAINT_MODE |
                GIMP_CONTEXT_PROP_MASK_GRADIENT,
                "gimp-gradient-tool",
                _("Gradient"),
                _("Gradient Tool: Fill selected area with a color gradient"),
                N_("Gra_dient"), "G",
                NULL, GIMP_HELP_TOOL_GRADIENT,
                GIMP_ICON_TOOL_GRADIENT,
                data);
}

static void
gimp_gradient_tool_class_init (GimpGradientToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass *tool_class   = GIMP_TOOL_CLASS (klass);

  object_class->dispose      = gimp_gradient_tool_dispose;

  tool_class->initialize     = gimp_gradient_tool_initialize;
  tool_class->control        = gimp_gradient_tool_control;
  tool_class->button_press   = gimp_gradient_tool_button_press;
  tool_class->button_release = gimp_gradient_tool_button_release;
  tool_class->motion         = gimp_gradient_tool_motion;
  tool_class->key_press      = gimp_gradient_tool_key_press;
  tool_class->modifier_key   = gimp_gradient_tool_modifier_key;
  tool_class->cursor_update  = gimp_gradient_tool_cursor_update;
  tool_class->can_undo       = gimp_gradient_tool_can_undo;
  tool_class->can_redo       = gimp_gradient_tool_can_redo;
  tool_class->undo           = gimp_gradient_tool_undo;
  tool_class->redo           = gimp_gradient_tool_redo;
  tool_class->options_notify = gimp_gradient_tool_options_notify;
}

static void
gimp_gradient_tool_init (GimpGradientTool *gradient_tool)
{
  GimpTool *tool = GIMP_TOOL (gradient_tool);

  gimp_tool_control_set_scroll_lock        (tool->control, TRUE);
  gimp_tool_control_set_preserve           (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask         (tool->control,
                                            GIMP_DIRTY_IMAGE           |
                                            GIMP_DIRTY_IMAGE_STRUCTURE |
                                            GIMP_DIRTY_DRAWABLE        |
                                            GIMP_DIRTY_ACTIVE_DRAWABLE);
  gimp_tool_control_set_dirty_action       (tool->control,
                                            GIMP_TOOL_ACTION_COMMIT);
  gimp_tool_control_set_wants_click        (tool->control, TRUE);
  gimp_tool_control_set_wants_double_click (tool->control, TRUE);
  gimp_tool_control_set_active_modifiers   (tool->control,
                                            GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  gimp_tool_control_set_precision          (tool->control,
                                            GIMP_CURSOR_PRECISION_SUBPIXEL);
  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_GRADIENT);
  gimp_tool_control_set_action_opacity     (tool->control,
                                            "context-opacity-set");
  gimp_tool_control_set_action_object_1    (tool->control,
                                            "context-gradient-select-set");

  gimp_draw_tool_set_default_status (GIMP_DRAW_TOOL (tool),
                                     _("Click-Drag to draw a gradient"));
}

static void
gimp_gradient_tool_dispose (GObject *object)
{
  GimpGradientTool *gradient_tool = GIMP_GRADIENT_TOOL (object);

  gimp_gradient_tool_set_gradient (gradient_tool, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
gimp_gradient_tool_initialize (GimpTool     *tool,
                               GimpDisplay  *display,
                               GError      **error)
{
  GimpImage           *image       = gimp_display_get_image (display);
  GimpGradientOptions *options     = GIMP_GRADIENT_TOOL_GET_OPTIONS (tool);
  GimpGuiConfig       *config      = GIMP_GUI_CONFIG (display->gimp->config);
  GimpItem            *locked_item = NULL;
  GList               *drawables;
  GimpDrawable        *drawable;

  if (! GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  drawables = gimp_image_get_selected_drawables (image);
  if (g_list_length (drawables) != 1)
    {
      if (g_list_length (drawables) > 1)
        gimp_tool_message_literal (tool, display,
                                   _("Cannot paint on multiple drawables. Select only one."));
      else
        gimp_tool_message_literal (tool, display, _("No active drawables."));

      g_list_free (drawables);

      return FALSE;
    }

  /* Only a single drawable at this point. */
  drawable = drawables->data;
  g_list_free (drawables);

  if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Cannot modify the pixels of layer groups."));
      return FALSE;
    }

  if (gimp_item_is_content_locked (GIMP_ITEM (drawable), &locked_item))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("The selected layer's pixels are locked."));
      if (error)
        gimp_tools_blink_lock_box (display->gimp, locked_item);
      return FALSE;
    }

  if (! gimp_item_is_visible (GIMP_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("The selected item is not visible."));
      return FALSE;
    }

  if (! gimp_context_get_gradient (GIMP_CONTEXT (options)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("No gradient available for use with this tool."));
      return FALSE;
    }

  return TRUE;
}

static void
gimp_gradient_tool_control (GimpTool       *tool,
                            GimpToolAction  action,
                            GimpDisplay    *display)
{
  GimpGradientTool *gradient_tool = GIMP_GRADIENT_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_gradient_tool_halt (gradient_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_gradient_tool_commit (gradient_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_gradient_tool_button_press (GimpTool            *tool,
                                 const GimpCoords    *coords,
                                 guint32              time,
                                 GdkModifierType      state,
                                 GimpButtonPressType  press_type,
                                 GimpDisplay         *display)
{
  GimpGradientTool *gradient_tool = GIMP_GRADIENT_TOOL (tool);

  if (tool->display && display != tool->display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);

  if (! gradient_tool->widget)
    {
      gimp_gradient_tool_start (gradient_tool, coords, display);

      gimp_tool_widget_hover (gradient_tool->widget, coords, state, TRUE);
    }

  /* call start_edit() before widget_button_press(), because we need to record
   * the undo state before widget_button_press() potentially changes it.  note
   * that if widget_button_press() return FALSE, nothing changes and no undo
   * step is created.
   */
  if (press_type == GIMP_BUTTON_PRESS_NORMAL)
    gimp_gradient_tool_editor_start_edit (gradient_tool);

  if (gimp_tool_widget_button_press (gradient_tool->widget, coords, time, state,
                                     press_type))
    {
      gradient_tool->grab_widget = gradient_tool->widget;
    }

  if (press_type == GIMP_BUTTON_PRESS_NORMAL)
    gimp_tool_control_activate (tool->control);
}

static void
gimp_gradient_tool_button_release (GimpTool              *tool,
                                   const GimpCoords      *coords,
                                   guint32                time,
                                   GdkModifierType        state,
                                   GimpButtonReleaseType  release_type,
                                   GimpDisplay           *display)
{
  GimpGradientTool    *gradient_tool = GIMP_GRADIENT_TOOL (tool);
  GimpGradientOptions *options       = GIMP_GRADIENT_TOOL_GET_OPTIONS (tool);

  gimp_tool_pop_status (tool, display);

  gimp_tool_control_halt (tool->control);

  if (gradient_tool->grab_widget)
    {
      gimp_tool_widget_button_release (gradient_tool->grab_widget,
                                       coords, time, state, release_type);
      gradient_tool->grab_widget = NULL;

      if (options->instant)
        {
          if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
            gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
          else
            gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);
        }
    }

  if (! options->instant)
    {
      gimp_gradient_tool_editor_end_edit (gradient_tool,
                                          release_type ==
                                          GIMP_BUTTON_RELEASE_CANCEL);
    }
}

static void
gimp_gradient_tool_motion (GimpTool         *tool,
                           const GimpCoords *coords,
                           guint32           time,
                           GdkModifierType   state,
                           GimpDisplay      *display)
{
  GimpGradientTool *gradient_tool = GIMP_GRADIENT_TOOL (tool);

  if (gradient_tool->grab_widget)
    {
      gimp_tool_widget_motion (gradient_tool->grab_widget, coords, time, state);
    }
}

static gboolean
gimp_gradient_tool_key_press (GimpTool    *tool,
                              GdkEventKey *kevent,
                              GimpDisplay *display)
{
  GimpGradientTool *gradient_tool = GIMP_GRADIENT_TOOL (tool);
  GimpDrawTool     *draw_tool     = GIMP_DRAW_TOOL (tool);
  gboolean          result;

  /* call start_edit() before widget_key_press(), because we need to record the
   * undo state before widget_key_press() potentially changes it.  note that if
   * widget_key_press() return FALSE, nothing changes and no undo step is
   * created.
   */
  if (display == draw_tool->display)
    gimp_gradient_tool_editor_start_edit (gradient_tool);

  result = GIMP_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);

  if (display == draw_tool->display)
    gimp_gradient_tool_editor_end_edit (gradient_tool, FALSE);

  return result;
}

static void
gimp_gradient_tool_modifier_key (GimpTool        *tool,
                                 GdkModifierType  key,
                                 gboolean         press,
                                 GdkModifierType  state,
                                 GimpDisplay     *display)
{
  GimpGradientOptions *options = GIMP_GRADIENT_TOOL_GET_OPTIONS (tool);

  if (key == gimp_get_extend_selection_mask ())
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
gimp_gradient_tool_cursor_update (GimpTool         *tool,
                                  const GimpCoords *coords,
                                  GdkModifierType   state,
                                  GimpDisplay      *display)
{
  GimpGuiConfig *config    = GIMP_GUI_CONFIG (display->gimp->config);
  GimpImage     *image     = gimp_display_get_image (display);
  GList         *drawables = gimp_image_get_selected_drawables (image);

  if (g_list_length (drawables) != 1                      ||
      gimp_viewable_get_children (drawables->data)        ||
      gimp_item_is_content_locked (drawables->data, NULL) ||
      ! (gimp_item_is_visible (drawables->data) ||
         config->edit_non_visible))
    {
      gimp_tool_set_cursor (tool, display,
                            gimp_tool_control_get_cursor (tool->control),
                            gimp_tool_control_get_tool_cursor (tool->control),
                            GIMP_CURSOR_MODIFIER_BAD);
      g_list_free (drawables);
      return;
    }
  g_list_free (drawables);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static const gchar *
gimp_gradient_tool_can_undo (GimpTool    *tool,
                             GimpDisplay *display)
{
  return gimp_gradient_tool_editor_can_undo (GIMP_GRADIENT_TOOL (tool));
}

static const gchar *
gimp_gradient_tool_can_redo (GimpTool    *tool,
                             GimpDisplay *display)
{
  return gimp_gradient_tool_editor_can_redo (GIMP_GRADIENT_TOOL (tool));
}

static gboolean
gimp_gradient_tool_undo (GimpTool    *tool,
                         GimpDisplay *display)
{
  return gimp_gradient_tool_editor_undo (GIMP_GRADIENT_TOOL (tool));
}

static gboolean
gimp_gradient_tool_redo (GimpTool    *tool,
                         GimpDisplay *display)
{
  return gimp_gradient_tool_editor_redo (GIMP_GRADIENT_TOOL (tool));
}

static void
gimp_gradient_tool_options_notify (GimpTool         *tool,
                                   GimpToolOptions  *options,
                                   const GParamSpec *pspec)
{
  GimpContext      *context       = GIMP_CONTEXT (options);
  GimpGradientTool *gradient_tool = GIMP_GRADIENT_TOOL (tool);

  if (! strcmp (pspec->name, "gradient"))
    {
      gimp_gradient_tool_set_gradient (gradient_tool, context->gradient);

      if (gradient_tool->filter)
        gimp_drawable_filter_apply (gradient_tool->filter, NULL);
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
          GimpRepeatMode   gradient_repeat;
          GimpRepeatMode   node_repeat;
          GimpGradientType gradient_type;

          gradient_repeat = GIMP_PAINT_OPTIONS (options)->gradient_options->gradient_repeat;
          gradient_type   = GIMP_GRADIENT_OPTIONS (options)->gradient_type;
          gegl_node_get (gradient_tool->render_node,
                         "gradient-repeat", &node_repeat,
                         NULL);

          if (gradient_type >= GIMP_GRADIENT_SHAPEBURST_ANGULAR)
            {
              /* These gradient types are only meant to work with repeat
               * value of "none" so these are the only ones where we
               * don't keep the render node and the gradient options in
               * sync.
               * We could instead reset the "gradient-repeat" value on
               * GimpGradientOptions, but I assume one would want to revert
               * back to the last set value if changing back the
               * gradient type. So instead we just make the option
               * insensitive (both in GUI and in render).
               */
              if (node_repeat != GIMP_REPEAT_NONE)
                gegl_node_set (gradient_tool->render_node,
                               "gradient-repeat", GIMP_REPEAT_NONE,
                               NULL);
            }
          else if (node_repeat != gradient_repeat)
            {
              gegl_node_set (gradient_tool->render_node,
                             "gradient-repeat", gradient_repeat,
                             NULL);
            }

          if (gimp_gradient_tool_is_shapeburst (gradient_tool))
            gimp_gradient_tool_precalc_shapeburst (gradient_tool);

          gimp_gradient_tool_update_graph (gradient_tool);
        }

      gimp_drawable_filter_apply (gradient_tool->filter, NULL);
    }
  else if (gradient_tool->render_node                    &&
           gimp_gradient_tool_is_shapeburst (gradient_tool) &&
           g_strcmp0 (pspec->name, "distance-metric") == 0)
    {
      g_clear_object (&gradient_tool->dist_buffer);
      gimp_gradient_tool_precalc_shapeburst (gradient_tool);
      gimp_gradient_tool_update_graph (gradient_tool);
      gimp_drawable_filter_apply (gradient_tool->filter, NULL);
    }
  else if (gradient_tool->filter &&
           ! strcmp (pspec->name, "opacity"))
    {
      gimp_drawable_filter_set_opacity (gradient_tool->filter,
                                        gimp_context_get_opacity (context));
    }
  else if (gradient_tool->filter &&
           ! strcmp (pspec->name, "paint-mode"))
    {
      gimp_drawable_filter_set_mode (gradient_tool->filter,
                                     gimp_context_get_paint_mode (context),
                                     GIMP_LAYER_COLOR_SPACE_AUTO,
                                     GIMP_LAYER_COLOR_SPACE_AUTO,
                                     gimp_layer_mode_get_paint_composite_mode (
                                       gimp_context_get_paint_mode (context)));
    }

  gimp_gradient_tool_editor_options_notify (gradient_tool, options, pspec);
}

static void
gimp_gradient_tool_start (GimpGradientTool *gradient_tool,
                          const GimpCoords *coords,
                          GimpDisplay      *display)
{
  GimpTool            *tool      = GIMP_TOOL (gradient_tool);
  GimpDisplayShell    *shell     = gimp_display_get_shell (display);
  GimpImage           *image     = gimp_display_get_image (display);
  GList               *drawables = gimp_image_get_selected_drawables (image);
  GimpGradientOptions *options   = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  GimpContext         *context   = GIMP_CONTEXT (options);
  GimpContainer       *filters;

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

  gradient_tool->widget = gimp_tool_line_new (shell,
                                              gradient_tool->start_x,
                                              gradient_tool->start_y,
                                              gradient_tool->end_x,
                                              gradient_tool->end_y);

  g_object_set (gradient_tool->widget,
                "status-title", _("Gradient: "),
                NULL);

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), gradient_tool->widget);

  g_signal_connect (gradient_tool->widget, "changed",
                    G_CALLBACK (gimp_gradient_tool_line_changed),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "response",
                    G_CALLBACK (gimp_gradient_tool_line_response),
                    gradient_tool);

  g_signal_connect_swapped (context, "background-changed",
                            G_CALLBACK (gimp_gradient_tool_fg_bg_changed),
                            gradient_tool);
  g_signal_connect_swapped (context, "foreground-changed",
                            G_CALLBACK (gimp_gradient_tool_fg_bg_changed),
                            gradient_tool);

  gimp_gradient_tool_create_filter (gradient_tool, tool->drawables->data);

  /* Initially sync all of the properties */
  gimp_operation_config_sync_node (G_OBJECT (options),
                                   gradient_tool->render_node);

  /* We don't allow repeat values for some shapes. */
  if (options->gradient_type >= GIMP_GRADIENT_SHAPEBURST_ANGULAR)
    gegl_node_set (gradient_tool->render_node,
                   "gradient-repeat", GIMP_REPEAT_NONE,
                   NULL);

  /* Connect signal handlers for the gradient */
  gimp_gradient_tool_set_gradient (gradient_tool, context->gradient);

  if (gimp_gradient_tool_is_shapeburst (gradient_tool))
    gimp_gradient_tool_precalc_shapeburst (gradient_tool);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (gradient_tool), display);

  gimp_gradient_tool_editor_start (gradient_tool);

  gimp_drawable_filter_apply (gradient_tool->filter, NULL);

  /* Move this operation below any non-destructive filters that
   * may be active, so that it's directly affect the raw pixels. */
  filters =
    gimp_drawable_get_filters (gimp_drawable_filter_get_drawable (gradient_tool->filter));

  if (gimp_container_have (filters, GIMP_OBJECT (gradient_tool->filter)))
  {
    gint end_index = gimp_container_get_n_children (filters) - 1;
    gint index     = gimp_container_get_child_index (filters,
                                                     GIMP_OBJECT (gradient_tool->filter));

    if (end_index > 0 && index != end_index)
      gimp_container_reorder (filters, GIMP_OBJECT (gradient_tool->filter),
                              end_index);
  }
}

static void
gimp_gradient_tool_halt (GimpGradientTool *gradient_tool)
{
  GimpTool            *tool    = GIMP_TOOL (gradient_tool);
  GimpGradientOptions *options = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  GimpContext         *context = GIMP_CONTEXT (options);

  gimp_gradient_tool_editor_halt (gradient_tool);

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
      gimp_tool_control_push_preserve (tool->control, TRUE);

      gimp_drawable_filter_abort (gradient_tool->filter);
      g_object_unref (gradient_tool->filter);
      gradient_tool->filter = NULL;

      gimp_tool_control_pop_preserve (tool->control);

      gimp_image_flush (gimp_display_get_image (tool->display));
    }

  gimp_gradient_tool_set_tentative_gradient (gradient_tool, NULL);

  g_signal_handlers_disconnect_by_func (context,
                                        G_CALLBACK (gimp_gradient_tool_fg_bg_changed),
                                        gradient_tool);

  if (tool->display)
    gimp_tool_pop_status (tool, tool->display);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (gradient_tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (gradient_tool));

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), NULL);
  g_clear_object (&gradient_tool->widget);

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;

  if (options->instant_toggle)
    gtk_widget_set_sensitive (options->instant_toggle, TRUE);
}

static void
gimp_gradient_tool_commit (GimpGradientTool *gradient_tool)
{
  GimpTool *tool = GIMP_TOOL (gradient_tool);

  if (gradient_tool->filter)
    {
      /* halt the editor before committing the filter so that the image-flush
       * idle source is removed, to avoid flushing the image, and hence
       * restarting the projection rendering, while applying the filter.
       */
      gimp_gradient_tool_editor_halt (gradient_tool);

      gimp_tool_control_push_preserve (tool->control, TRUE);

      gimp_drawable_filter_commit (gradient_tool->filter, FALSE,
                                   GIMP_PROGRESS (tool), FALSE);
      g_clear_object (&gradient_tool->filter);

      gimp_tool_control_pop_preserve (tool->control);

      gimp_image_flush (gimp_display_get_image (tool->display));
    }
}

static void
gimp_gradient_tool_line_changed (GimpToolWidget   *widget,
                                 GimpGradientTool *gradient_tool)
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

  if (gimp_gradient_tool_editor_line_changed (gradient_tool))
    update = TRUE;

  if (update)
    {
      gimp_gradient_tool_update_graph (gradient_tool);
      gimp_drawable_filter_apply (gradient_tool->filter, NULL);
    }
}

static void
gimp_gradient_tool_line_response (GimpToolWidget *widget,
                                  gint            response_id,
                                  GimpGradientTool  *gradient_tool)
{
  GimpTool *tool = GIMP_TOOL (gradient_tool);

  switch (response_id)
    {
    case GIMP_TOOL_WIDGET_RESPONSE_CONFIRM:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, tool->display);
      break;

    case GIMP_TOOL_WIDGET_RESPONSE_CANCEL:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
gimp_gradient_tool_precalc_shapeburst (GimpGradientTool *gradient_tool)
{
  GimpGradientOptions *options = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  GimpTool            *tool    = GIMP_TOOL (gradient_tool);
  gint                 x, y, width, height;

  if (gradient_tool->dist_buffer || ! tool->drawables)
    return;

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  if (! gimp_item_mask_intersect (GIMP_ITEM (tool->drawables->data),
                                  &x, &y, &width, &height))
    return;

  gradient_tool->dist_buffer =
    gimp_drawable_gradient_shapeburst_distmap (tool->drawables->data,
                                               options->distance_metric,
                                               GEGL_RECTANGLE (x, y, width, height),
                                               GIMP_PROGRESS (gradient_tool));

  if (gradient_tool->dist_node)
    gegl_node_set (gradient_tool->dist_node,
                   "buffer", gradient_tool->dist_buffer,
                   NULL);

  gimp_progress_end (GIMP_PROGRESS (gradient_tool));
}


/* gegl graph stuff */

static void
gimp_gradient_tool_create_graph (GimpGradientTool *gradient_tool)
{
  GimpGradientOptions *options = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  GimpContext         *context = GIMP_CONTEXT (options);
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
                         "operation", "gimp:gradient",
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

  gimp_gegl_node_set_underlying_operation (gradient_tool->graph,
                                           gradient_tool->render_node);

  gimp_gradient_tool_update_graph (gradient_tool);
}

static void
gimp_gradient_tool_update_graph (GimpGradientTool *gradient_tool)
{
  GimpTool            *tool    = GIMP_TOOL (gradient_tool);
  GimpGradientOptions *options = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  gint                 off_x, off_y;

  gimp_item_get_offset (GIMP_ITEM (tool->drawables->data), &off_x, &off_y);

#if 0
  if (gimp_gradient_tool_is_shapeburst (gradient_tool))
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

      gimp_item_mask_intersect (GIMP_ITEM (tool->drawables->data),
                                &roi.x, &roi.y, &roi.width, &roi.height);

      start_x = gradient_tool->start_x - off_x;
      start_y = gradient_tool->start_y - off_y;
      end_x   = gradient_tool->end_x   - off_x;
      end_y   = gradient_tool->end_y   - off_y;

      gimp_drawable_gradient_adjust_coords (tool->drawables->data,
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
gimp_gradient_tool_fg_bg_changed (GimpGradientTool *gradient_tool)
{
  if (! gradient_tool->filter || ! gradient_tool->gradient)
    return;

  if (gimp_gradient_has_fg_bg_segments (gradient_tool->gradient))
    {
      /* Set a property on the node. Otherwise it will cache and refuse to update */
      gegl_node_set (gradient_tool->render_node,
                     "gradient", gradient_tool->gradient,
                     NULL);

      /* Update the filter */
      gimp_drawable_filter_apply (gradient_tool->filter, NULL);

      gimp_gradient_tool_editor_fg_bg_changed (gradient_tool);
    }
}

static void
gimp_gradient_tool_gradient_dirty (GimpGradientTool *gradient_tool)
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
      gimp_drawable_filter_apply (gradient_tool->filter, NULL);
    }

  gimp_gradient_tool_editor_gradient_dirty (gradient_tool);
}

static void
gimp_gradient_tool_set_gradient (GimpGradientTool *gradient_tool,
                                 GimpGradient     *gradient)
{
  if (gradient_tool->gradient)
    g_signal_handlers_disconnect_by_func (gradient_tool->gradient,
                                          G_CALLBACK (gimp_gradient_tool_gradient_dirty),
                                          gradient_tool);


  g_set_object (&gradient_tool->gradient, gradient);

  if (gradient_tool->gradient)
    {
      g_signal_connect_swapped (gradient_tool->gradient, "dirty",
                                G_CALLBACK (gimp_gradient_tool_gradient_dirty),
                                gradient_tool);

      if (gradient_tool->render_node)
        gegl_node_set (gradient_tool->render_node,
                       "gradient", gradient_tool->gradient,
                       NULL);
    }

  gimp_gradient_tool_editor_gradient_changed (gradient_tool);
}

static gboolean
gimp_gradient_tool_is_shapeburst (GimpGradientTool *gradient_tool)
{
  GimpGradientOptions *options = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);

  return options->gradient_type >= GIMP_GRADIENT_SHAPEBURST_ANGULAR &&
         options->gradient_type <= GIMP_GRADIENT_SHAPEBURST_DIMPLED;
}


/* image map stuff */

static void
gimp_gradient_tool_create_filter (GimpGradientTool *gradient_tool,
                                  GimpDrawable     *drawable)
{
  GimpGradientOptions *options = GIMP_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  GimpContext         *context = GIMP_CONTEXT (options);

  if (! gradient_tool->graph)
    gimp_gradient_tool_create_graph (gradient_tool);

  gradient_tool->filter = gimp_drawable_filter_new (drawable,
                                                 C_("undo-type", "Gradient"),
                                                 gradient_tool->graph,
                                                 GIMP_ICON_TOOL_GRADIENT);

  gimp_drawable_filter_set_region (gradient_tool->filter,
                                   GIMP_FILTER_REGION_DRAWABLE);
  gimp_drawable_filter_set_opacity (gradient_tool->filter,
                                    gimp_context_get_opacity (context));
  gimp_drawable_filter_set_mode (gradient_tool->filter,
                                 gimp_context_get_paint_mode (context),
                                 GIMP_LAYER_COLOR_SPACE_AUTO,
                                 GIMP_LAYER_COLOR_SPACE_AUTO,
                                 gimp_layer_mode_get_paint_composite_mode (
                                   gimp_context_get_paint_mode (context)));

  g_signal_connect (gradient_tool->filter, "flush",
                    G_CALLBACK (gimp_gradient_tool_filter_flush),
                    gradient_tool);
}

static void
gimp_gradient_tool_filter_flush (GimpDrawableFilter *filter,
                                 GimpTool           *tool)
{
  GimpImage *image = gimp_display_get_image (tool->display);

  gimp_projection_flush (gimp_image_get_projection (image));
}


/*  protected functions  */


void
gimp_gradient_tool_set_tentative_gradient (GimpGradientTool *gradient_tool,
                                           GimpGradient     *gradient)
{
  g_return_if_fail (GIMP_IS_GRADIENT_TOOL (gradient_tool));
  g_return_if_fail (gradient == NULL || GIMP_IS_GRADIENT (gradient));

  if (g_set_object (&gradient_tool->tentative_gradient, gradient))
    {
      if (gradient_tool->render_node)
        {
          gegl_node_set (gradient_tool->render_node,
                         "gradient", gradient ? gradient : gradient_tool->gradient,
                         NULL);

          gimp_drawable_filter_apply (gradient_tool->filter, NULL);
        }
    }
}
