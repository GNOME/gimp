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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "operations/gimp-operation-config.h"

#include "core/gimpdrawable.h"
#include "core/gimpdrawable-blend.h"
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

#include "gimpblendoptions.h"
#include "gimpblendtool.h"
#include "gimpblendtool-editor.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_blend_tool_dispose             (GObject               *object);

static gboolean gimp_blend_tool_initialize        (GimpTool              *tool,
                                                   GimpDisplay           *display,
                                                   GError               **error);
static void   gimp_blend_tool_control             (GimpTool              *tool,
                                                   GimpToolAction         action,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_button_press        (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpButtonPressType    press_type,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_button_release      (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpButtonReleaseType  release_type,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_motion              (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);
static gboolean gimp_blend_tool_key_press         (GimpTool              *tool,
                                                   GdkEventKey           *kevent,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_modifier_key        (GimpTool              *tool,
                                                   GdkModifierType        key,
                                                   gboolean               press,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_cursor_update       (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);
static const gchar * gimp_blend_tool_can_undo     (GimpTool              *tool,
                                                   GimpDisplay           *display);
static const gchar * gimp_blend_tool_can_redo     (GimpTool              *tool,
                                                   GimpDisplay           *display);
static gboolean  gimp_blend_tool_undo             (GimpTool              *tool,
                                                   GimpDisplay           *display);
static gboolean  gimp_blend_tool_redo             (GimpTool              *tool,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_options_notify      (GimpTool              *tool,
                                                   GimpToolOptions       *options,
                                                   const GParamSpec      *pspec);

static void   gimp_blend_tool_start               (GimpBlendTool         *blend_tool,
                                                   const GimpCoords      *coords,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_halt                (GimpBlendTool         *blend_tool);
static void   gimp_blend_tool_commit              (GimpBlendTool         *blend_tool);

static void   gimp_blend_tool_line_changed        (GimpToolWidget        *widget,
                                                   GimpBlendTool         *blend_tool);
static void   gimp_blend_tool_line_response       (GimpToolWidget        *widget,
                                                   gint                   response_id,
                                                   GimpBlendTool         *blend_tool);

static void   gimp_blend_tool_precalc_shapeburst  (GimpBlendTool         *blend_tool);

static void   gimp_blend_tool_create_graph        (GimpBlendTool         *blend_tool);
static void   gimp_blend_tool_update_graph        (GimpBlendTool         *blend_tool);

static void   gimp_blend_tool_fg_bg_changed       (GimpBlendTool         *blend_tool);

static void   gimp_blend_tool_gradient_dirty      (GimpBlendTool         *blend_tool);
static void   gimp_blend_tool_set_gradient        (GimpBlendTool         *blend_tool,
                                                   GimpGradient          *gradient);

static gboolean gimp_blend_tool_is_shapeburst     (GimpBlendTool         *blend_tool);

static void   gimp_blend_tool_create_filter       (GimpBlendTool         *blend_tool,
                                                   GimpDrawable          *drawable);
static void   gimp_blend_tool_filter_flush        (GimpDrawableFilter    *filter,
                                                   GimpTool              *tool);


G_DEFINE_TYPE (GimpBlendTool, gimp_blend_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_blend_tool_parent_class


void
gimp_blend_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_BLEND_TOOL,
                GIMP_TYPE_BLEND_OPTIONS,
                gimp_blend_options_gui,
                GIMP_CONTEXT_PROP_MASK_FOREGROUND |
                GIMP_CONTEXT_PROP_MASK_BACKGROUND |
                GIMP_CONTEXT_PROP_MASK_OPACITY    |
                GIMP_CONTEXT_PROP_MASK_PAINT_MODE |
                GIMP_CONTEXT_PROP_MASK_GRADIENT,
                "gimp-blend-tool",
                _("Blend"),
                _("Blend Tool: Fill selected area with a color gradient"),
                N_("Blen_d"), "L",
                NULL, GIMP_HELP_TOOL_BLEND,
                GIMP_ICON_TOOL_GRADIENT,
                data);
}

static void
gimp_blend_tool_class_init (GimpBlendToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass *tool_class   = GIMP_TOOL_CLASS (klass);

  object_class->dispose      = gimp_blend_tool_dispose;

  tool_class->initialize     = gimp_blend_tool_initialize;
  tool_class->control        = gimp_blend_tool_control;
  tool_class->button_press   = gimp_blend_tool_button_press;
  tool_class->button_release = gimp_blend_tool_button_release;
  tool_class->motion         = gimp_blend_tool_motion;
  tool_class->key_press      = gimp_blend_tool_key_press;
  tool_class->modifier_key   = gimp_blend_tool_modifier_key;
  tool_class->cursor_update  = gimp_blend_tool_cursor_update;
  tool_class->can_undo       = gimp_blend_tool_can_undo;
  tool_class->can_redo       = gimp_blend_tool_can_redo;
  tool_class->undo           = gimp_blend_tool_undo;
  tool_class->redo           = gimp_blend_tool_redo;
  tool_class->options_notify = gimp_blend_tool_options_notify;
}

static void
gimp_blend_tool_init (GimpBlendTool *blend_tool)
{
  GimpTool *tool = GIMP_TOOL (blend_tool);

  gimp_tool_control_set_scroll_lock        (tool->control, TRUE);
  gimp_tool_control_set_preserve           (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask         (tool->control,
                                            GIMP_DIRTY_IMAGE           |
                                            GIMP_DIRTY_IMAGE_STRUCTURE |
                                            GIMP_DIRTY_DRAWABLE        |
                                            GIMP_DIRTY_ACTIVE_DRAWABLE);
  gimp_tool_control_set_wants_click        (tool->control, TRUE);
  gimp_tool_control_set_wants_double_click (tool->control, TRUE);
  gimp_tool_control_set_active_modifiers   (tool->control,
                                            GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  gimp_tool_control_set_precision          (tool->control,
                                            GIMP_CURSOR_PRECISION_SUBPIXEL);
  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_BLEND);
  gimp_tool_control_set_action_opacity     (tool->control,
                                            "context/context-opacity-set");
  gimp_tool_control_set_action_object_1    (tool->control,
                                            "context/context-gradient-select-set");

  gimp_draw_tool_set_default_status (GIMP_DRAW_TOOL (tool),
                                     _("Click-Drag to draw a gradient"));
}

static void
gimp_blend_tool_dispose (GObject *object)
{
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (object);

  gimp_blend_tool_set_gradient (blend_tool, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
gimp_blend_tool_initialize (GimpTool     *tool,
                            GimpDisplay  *display,
                            GError      **error)
{
  GimpImage        *image    = gimp_display_get_image (display);
  GimpDrawable     *drawable = gimp_image_get_active_drawable (image);
  GimpBlendOptions *options  = GIMP_BLEND_TOOL_GET_OPTIONS (tool);

  if (! GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Cannot modify the pixels of layer groups."));
      return FALSE;
    }

  if (gimp_item_is_content_locked (GIMP_ITEM (drawable)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("The active layer's pixels are locked."));
      return FALSE;
    }

  if (! gimp_item_is_visible (GIMP_ITEM (drawable)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("The active layer is not visible."));
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
gimp_blend_tool_control (GimpTool       *tool,
                         GimpToolAction  action,
                         GimpDisplay    *display)
{
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_blend_tool_halt (blend_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_blend_tool_commit (blend_tool);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_blend_tool_button_press (GimpTool            *tool,
                              const GimpCoords    *coords,
                              guint32              time,
                              GdkModifierType      state,
                              GimpButtonPressType  press_type,
                              GimpDisplay         *display)
{
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (tool);

  if (tool->display && display != tool->display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);

  if (! blend_tool->widget)
    {
      gimp_blend_tool_start (blend_tool, coords, display);

      gimp_tool_widget_hover (blend_tool->widget, coords, state, TRUE);
    }

  /* call start_edit() before widget_button_press(), because we need to record
   * the undo state before widget_button_press() potentially changes it.  note
   * that if widget_button_press() return FALSE, nothing changes and no undo
   * step is created.
   */
  if (press_type == GIMP_BUTTON_PRESS_NORMAL)
    gimp_blend_tool_editor_start_edit (blend_tool);

  if (gimp_tool_widget_button_press (blend_tool->widget, coords, time, state,
                                     press_type))
    {
      blend_tool->grab_widget = blend_tool->widget;
    }

  if (press_type == GIMP_BUTTON_PRESS_NORMAL)
    gimp_tool_control_activate (tool->control);
}

static void
gimp_blend_tool_button_release (GimpTool              *tool,
                                const GimpCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                GimpButtonReleaseType  release_type,
                                GimpDisplay           *display)
{
  GimpBlendTool    *blend_tool = GIMP_BLEND_TOOL (tool);
  GimpBlendOptions *options    = GIMP_BLEND_TOOL_GET_OPTIONS (tool);

  gimp_tool_pop_status (tool, display);

  gimp_tool_control_halt (tool->control);

  if (blend_tool->grab_widget)
    {
      gimp_tool_widget_button_release (blend_tool->grab_widget,
                                       coords, time, state, release_type);
      blend_tool->grab_widget = NULL;

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
      gimp_blend_tool_editor_end_edit (blend_tool,
                                       release_type ==
                                       GIMP_BUTTON_RELEASE_CANCEL);
    }
}

static void
gimp_blend_tool_motion (GimpTool         *tool,
                        const GimpCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        GimpDisplay      *display)
{
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (tool);

  if (blend_tool->grab_widget)
    {
      gimp_tool_widget_motion (blend_tool->grab_widget, coords, time, state);
    }
}

static gboolean
gimp_blend_tool_key_press (GimpTool    *tool,
                           GdkEventKey *kevent,
                           GimpDisplay *display)
{
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (tool);
  GimpDrawTool  *draw_tool  = GIMP_DRAW_TOOL (tool);
  gboolean       result;

  /* call start_edit() before widget_key_press(), because we need to record the
   * undo state before widget_key_press() potentially changes it.  note that if
   * widget_key_press() return FALSE, nothing changes and no undo step is
   * created.
   */
  if (display == draw_tool->display)
    gimp_blend_tool_editor_start_edit (blend_tool);

  result = GIMP_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);

  if (display == draw_tool->display)
    gimp_blend_tool_editor_end_edit (blend_tool, FALSE);

  return result;
}

static void
gimp_blend_tool_modifier_key (GimpTool        *tool,
                              GdkModifierType  key,
                              gboolean         press,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (tool);

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
gimp_blend_tool_cursor_update (GimpTool         *tool,
                               const GimpCoords *coords,
                               GdkModifierType   state,
                               GimpDisplay      *display)
{
  GimpImage    *image    = gimp_display_get_image (display);
  GimpDrawable *drawable = gimp_image_get_active_drawable (image);

  if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)) ||
      gimp_item_is_content_locked (GIMP_ITEM (drawable))    ||
      ! gimp_item_is_visible (GIMP_ITEM (drawable)))
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
gimp_blend_tool_can_undo (GimpTool    *tool,
                          GimpDisplay *display)
{
  return gimp_blend_tool_editor_can_undo (GIMP_BLEND_TOOL (tool));
}

static const gchar *
gimp_blend_tool_can_redo (GimpTool    *tool,
                          GimpDisplay *display)
{
  return gimp_blend_tool_editor_can_redo (GIMP_BLEND_TOOL (tool));
}

static gboolean
gimp_blend_tool_undo (GimpTool    *tool,
                      GimpDisplay *display)
{
  return gimp_blend_tool_editor_undo (GIMP_BLEND_TOOL (tool));
}

static gboolean
gimp_blend_tool_redo (GimpTool    *tool,
                      GimpDisplay *display)
{
  return gimp_blend_tool_editor_redo (GIMP_BLEND_TOOL (tool));
}

static void
gimp_blend_tool_options_notify (GimpTool         *tool,
                                GimpToolOptions  *options,
                                const GParamSpec *pspec)
{
  GimpContext   *context    = GIMP_CONTEXT (options);
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (tool);

  if (! strcmp (pspec->name, "gradient"))
    {
      gimp_blend_tool_set_gradient (blend_tool, context->gradient);

      if (blend_tool->filter)
        gimp_drawable_filter_apply (blend_tool->filter, NULL);
    }
  else if (blend_tool->render_node &&
           gegl_node_find_property (blend_tool->render_node, pspec->name))
    {
      /* Sync any property changes on the config object that match the op */
      GValue value = G_VALUE_INIT;

      g_value_init (&value, pspec->value_type);

      g_object_get_property (G_OBJECT (options), pspec->name, &value);
      gegl_node_set_property (blend_tool->render_node, pspec->name, &value);

      g_value_unset (&value);

      if (! strcmp (pspec->name, "gradient-type"))
        {
          GimpRepeatMode   gradient_repeat;
          GimpRepeatMode   node_repeat;
          GimpGradientType gradient_type;

          gradient_repeat = GIMP_PAINT_OPTIONS (options)->gradient_options->gradient_repeat;
          gradient_type   = GIMP_BLEND_OPTIONS (options)->gradient_type;
          gegl_node_get (blend_tool->render_node,
                         "gradient-repeat", &node_repeat,
                         NULL);

          if (gradient_type >= GIMP_GRADIENT_SHAPEBURST_ANGULAR)
            {
              /* These gradient types are only meant to work with repeat
               * value of "none" so these are the only ones where we
               * don't keep the render node and the blend options in
               * sync.
               * We could instead reset the "gradient-repeat" value on
               * GimpBlendOptions, but I assume one would want to revert
               * back to the last set value if changing back the
               * gradient type. So instead we just make the option
               * insensitive (both in GUI and in render).
               */
              if (node_repeat != GIMP_REPEAT_NONE)
                gegl_node_set (blend_tool->render_node,
                               "gradient-repeat", GIMP_REPEAT_NONE,
                               NULL);
            }
          else if (node_repeat != gradient_repeat)
            {
              gegl_node_set (blend_tool->render_node,
                             "gradient-repeat", gradient_repeat,
                             NULL);
            }

          if (gimp_blend_tool_is_shapeburst (blend_tool))
            gimp_blend_tool_precalc_shapeburst (blend_tool);

          gimp_blend_tool_update_graph (blend_tool);
        }

      gimp_drawable_filter_apply (blend_tool->filter, NULL);
    }
  else if (blend_tool->render_node                    &&
           gimp_blend_tool_is_shapeburst (blend_tool) &&
           g_strcmp0 (pspec->name, "distance-metric") == 0)
    {
      g_clear_object (&blend_tool->dist_buffer);
      gimp_blend_tool_precalc_shapeburst (blend_tool);
      gimp_blend_tool_update_graph (blend_tool);
      gimp_drawable_filter_apply (blend_tool->filter, NULL);
    }
  else if (blend_tool->filter &&
           ! strcmp (pspec->name, "opacity"))
    {
      gimp_drawable_filter_set_opacity (blend_tool->filter,
                                        gimp_context_get_opacity (context));
    }
  else if (blend_tool->filter &&
           ! strcmp (pspec->name, "paint-mode"))
    {
      gimp_drawable_filter_set_mode (blend_tool->filter,
                                     gimp_context_get_paint_mode (context),
                                     GIMP_LAYER_COLOR_SPACE_AUTO,
                                     GIMP_LAYER_COLOR_SPACE_AUTO,
                                     GIMP_LAYER_COMPOSITE_AUTO);
    }

  gimp_blend_tool_editor_options_notify (blend_tool, options, pspec);
}

static void
gimp_blend_tool_start (GimpBlendTool    *blend_tool,
                       const GimpCoords *coords,
                       GimpDisplay      *display)
{
  GimpTool         *tool     = GIMP_TOOL (blend_tool);
  GimpDisplayShell *shell    = gimp_display_get_shell (display);
  GimpImage        *image    = gimp_display_get_image (display);
  GimpDrawable     *drawable = gimp_image_get_active_drawable (image);
  GimpBlendOptions *options  = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpContext      *context  = GIMP_CONTEXT (options);

  if (options->instant_toggle)
    gtk_widget_set_sensitive (options->instant_toggle, FALSE);

  tool->display  = display;
  tool->drawable = drawable;

  blend_tool->start_x = coords->x;
  blend_tool->start_y = coords->y;
  blend_tool->end_x   = coords->x;
  blend_tool->end_y   = coords->y;

  blend_tool->widget = gimp_tool_line_new (shell,
                                           blend_tool->start_x,
                                           blend_tool->start_y,
                                           blend_tool->end_x,
                                           blend_tool->end_y);

  g_object_set (blend_tool->widget,
                "status-title", _("Blend: "),
                NULL);

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), blend_tool->widget);

  g_signal_connect (blend_tool->widget, "changed",
                    G_CALLBACK (gimp_blend_tool_line_changed),
                    blend_tool);
  g_signal_connect (blend_tool->widget, "response",
                    G_CALLBACK (gimp_blend_tool_line_response),
                    blend_tool);

  g_signal_connect_swapped (context, "background-changed",
                            G_CALLBACK (gimp_blend_tool_fg_bg_changed),
                            blend_tool);
  g_signal_connect_swapped (context, "foreground-changed",
                            G_CALLBACK (gimp_blend_tool_fg_bg_changed),
                            blend_tool);

  gimp_blend_tool_create_filter (blend_tool, drawable);

  /* Initially sync all of the properties */
  gimp_operation_config_sync_node (G_OBJECT (options),
                                   blend_tool->render_node);

  /* We don't allow repeat values for some shapes. */
  if (options->gradient_type >= GIMP_GRADIENT_SHAPEBURST_ANGULAR)
    gegl_node_set (blend_tool->render_node,
                   "gradient-repeat", GIMP_REPEAT_NONE,
                   NULL);

  /* Connect signal handlers for the gradient */
  gimp_blend_tool_set_gradient (blend_tool, context->gradient);

  if (gimp_blend_tool_is_shapeburst (blend_tool))
    gimp_blend_tool_precalc_shapeburst (blend_tool);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (blend_tool), display);

  gimp_blend_tool_editor_start (blend_tool);
}

static void
gimp_blend_tool_halt (GimpBlendTool *blend_tool)
{
  GimpTool         *tool    = GIMP_TOOL (blend_tool);
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpContext      *context = GIMP_CONTEXT (options);

  gimp_blend_tool_editor_halt (blend_tool);

  if (blend_tool->graph)
    {
      g_clear_object (&blend_tool->graph);
      blend_tool->render_node    = NULL;
#if 0
      blend_tool->subtract_node  = NULL;
      blend_tool->divide_node    = NULL;
#endif
      blend_tool->dist_node      = NULL;
    }

  g_clear_object (&blend_tool->dist_buffer);

  if (blend_tool->filter)
    {
      gimp_tool_control_push_preserve (tool->control, TRUE);

      gimp_drawable_filter_abort (blend_tool->filter);
      g_object_unref (blend_tool->filter);
      blend_tool->filter = NULL;

      gimp_tool_control_pop_preserve (tool->control);

      gimp_image_flush (gimp_display_get_image (tool->display));
    }

  gimp_blend_tool_set_tentative_gradient (blend_tool, NULL);

  g_signal_handlers_disconnect_by_func (context,
                                        G_CALLBACK (gimp_blend_tool_fg_bg_changed),
                                        blend_tool);

  if (tool->display)
    gimp_tool_pop_status (tool, tool->display);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (blend_tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (blend_tool));

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), NULL);
  g_clear_object (&blend_tool->widget);

  tool->display  = NULL;
  tool->drawable = NULL;

  if (options->instant_toggle)
    gtk_widget_set_sensitive (options->instant_toggle, TRUE);
}

static void
gimp_blend_tool_commit (GimpBlendTool *blend_tool)
{
  GimpTool *tool = GIMP_TOOL (blend_tool);

  if (blend_tool->filter)
    {
      gimp_tool_control_push_preserve (tool->control, TRUE);

      gimp_drawable_filter_commit (blend_tool->filter,
                                   GIMP_PROGRESS (tool), FALSE);
      g_clear_object (&blend_tool->filter);

      gimp_tool_control_pop_preserve (tool->control);

      gimp_image_flush (gimp_display_get_image (tool->display));
    }
}

static void
gimp_blend_tool_line_changed (GimpToolWidget *widget,
                              GimpBlendTool  *blend_tool)
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

  if (start_x != blend_tool->start_x ||
      start_y != blend_tool->start_y ||
      end_x   != blend_tool->end_x   ||
      end_y   != blend_tool->end_y)
    {
      blend_tool->start_x = start_x;
      blend_tool->start_y = start_y;
      blend_tool->end_x   = end_x;
      blend_tool->end_y   = end_y;

      update = TRUE;
    }

  if (gimp_blend_tool_editor_line_changed (blend_tool))
    update = TRUE;

  if (update)
    {
      gimp_blend_tool_update_graph (blend_tool);
      gimp_drawable_filter_apply (blend_tool->filter, NULL);
    }
}

static void
gimp_blend_tool_line_response (GimpToolWidget *widget,
                               gint            response_id,
                               GimpBlendTool  *blend_tool)
{
  GimpTool *tool = GIMP_TOOL (blend_tool);

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
gimp_blend_tool_precalc_shapeburst (GimpBlendTool *blend_tool)
{
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpTool         *tool    = GIMP_TOOL (blend_tool);
  gint              x, y, width, height;

  if (blend_tool->dist_buffer || ! tool->drawable)
    return;

  if (! gimp_item_mask_intersect (GIMP_ITEM (tool->drawable),
                                  &x, &y, &width, &height))
    return;

  blend_tool->dist_buffer =
    gimp_drawable_blend_shapeburst_distmap (tool->drawable, options->distance_metric,
                                            GEGL_RECTANGLE (x, y, width, height),
                                            GIMP_PROGRESS (blend_tool));

  if (blend_tool->dist_node)
    gegl_node_set (blend_tool->dist_node,
                   "buffer", blend_tool->dist_buffer,
                   NULL);

  gimp_progress_end (GIMP_PROGRESS (blend_tool));
}


/* gegl graph stuff */

static void
gimp_blend_tool_create_graph (GimpBlendTool *blend_tool)
{
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpContext      *context = GIMP_CONTEXT (options);
  GeglNode         *output;

  /* render_node is not supposed to be recreated */
  g_return_if_fail (blend_tool->graph == NULL);

  blend_tool->graph = gegl_node_new ();

  blend_tool->dist_node =
    gegl_node_new_child (blend_tool->graph,
                         "operation", "gegl:buffer-source",
                         "buffer",    blend_tool->dist_buffer,
                         NULL);

#if 0
  blend_tool->subtract_node =
    gegl_node_new_child (blend_tool->graph,
                         "operation", "gegl:subtract",
                         NULL);

  blend_tool->divide_node =
    gegl_node_new_child (blend_tool->graph,
                         "operation", "gegl:divide",
                         NULL);
#endif

  blend_tool->render_node =
    gegl_node_new_child (blend_tool->graph,
                         "operation", "gimp:blend",
                         "context", context,
                         NULL);

  output = gegl_node_get_output_proxy (blend_tool->graph, "output");

  gegl_node_link_many (blend_tool->dist_node,
#if 0
                       blend_tool->subtract_node,
                       blend_tool->divide_node,
#endif
                       blend_tool->render_node,
                       output,
                       NULL);

  gimp_blend_tool_update_graph (blend_tool);
}

static void
gimp_blend_tool_update_graph (GimpBlendTool *blend_tool)
{
  GimpTool *tool = GIMP_TOOL (blend_tool);
  gint      off_x, off_y;

  gimp_item_get_offset (GIMP_ITEM (tool->drawable), &off_x, &off_y);

#if 0
  if (gimp_blend_tool_is_shapeburst (blend_tool))
    {
      gfloat start, end;

      gegl_buffer_get (blend_tool->dist_buffer,
                       GEGL_RECTANGLE (blend_tool->start_x - off_x,
                                       blend_tool->start_y - off_y,
                                       1, 1),
                       1.0, babl_format("Y float"), &start,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      gegl_buffer_get (blend_tool->dist_buffer,
                       GEGL_RECTANGLE (blend_tool->end_x - off_x,
                                       blend_tool->end_y - off_y,
                                       1, 1),
                       1.0, babl_format("Y float"), &end,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (start != end)
        {
          gegl_node_set (blend_tool->subtract_node,
                         "value", (gdouble) start,
                         NULL);
          gegl_node_set (blend_tool->divide_node,
                         "value", (gdouble) (end - start),
                         NULL);
        }
    }
  else
#endif
    {
      gegl_node_set (blend_tool->render_node,
                     "start_x", blend_tool->start_x - off_x,
                     "start_y", blend_tool->start_y - off_y,
                     "end_x",   blend_tool->end_x - off_x,
                     "end_y",   blend_tool->end_y - off_y,
                     NULL);
    }
}

static void
gimp_blend_tool_fg_bg_changed (GimpBlendTool *blend_tool)
{
  if (! blend_tool->filter || ! blend_tool->gradient)
    return;

  if (gimp_gradient_has_fg_bg_segments (blend_tool->gradient))
    {
      /* Set a property on the node. Otherwise it will cache and refuse to update */
      gegl_node_set (blend_tool->render_node,
                     "gradient", blend_tool->gradient,
                     NULL);

      /* Update the filter */
      gimp_drawable_filter_apply (blend_tool->filter, NULL);

      gimp_blend_tool_editor_fg_bg_changed (blend_tool);
    }
}

static void
gimp_blend_tool_gradient_dirty (GimpBlendTool *blend_tool)
{
  if (! blend_tool->filter)
    return;

  if (! blend_tool->tentative_gradient)
    {
      /* Set a property on the node. Otherwise it will cache and refuse to update */
      gegl_node_set (blend_tool->render_node,
                     "gradient", blend_tool->gradient,
                     NULL);

      /* Update the filter */
      gimp_drawable_filter_apply (blend_tool->filter, NULL);
    }

  gimp_blend_tool_editor_gradient_dirty (blend_tool);
}

static void
gimp_blend_tool_set_gradient (GimpBlendTool *blend_tool,
                              GimpGradient  *gradient)
{
  if (blend_tool->gradient)
    {
      g_signal_handlers_disconnect_by_func (blend_tool->gradient,
                                            G_CALLBACK (gimp_blend_tool_gradient_dirty),
                                            blend_tool);

      g_object_unref (blend_tool->gradient);
    }

  blend_tool->gradient = gradient;

  if (blend_tool->gradient)
    {
      g_object_ref (gradient);

      g_signal_connect_swapped (blend_tool->gradient, "dirty",
                                G_CALLBACK (gimp_blend_tool_gradient_dirty),
                                blend_tool);

      if (blend_tool->render_node)
        gegl_node_set (blend_tool->render_node,
                       "gradient", blend_tool->gradient,
                       NULL);
    }

  gimp_blend_tool_editor_gradient_changed (blend_tool);
}

static gboolean
gimp_blend_tool_is_shapeburst (GimpBlendTool *blend_tool)
{
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);

  return options->gradient_type >= GIMP_GRADIENT_SHAPEBURST_ANGULAR &&
         options->gradient_type <= GIMP_GRADIENT_SHAPEBURST_DIMPLED;
}


/* image map stuff */

static void
gimp_blend_tool_create_filter (GimpBlendTool *blend_tool,
                               GimpDrawable  *drawable)
{
  GimpBlendOptions *options = GIMP_BLEND_TOOL_GET_OPTIONS (blend_tool);
  GimpContext      *context = GIMP_CONTEXT (options);

  if (! blend_tool->graph)
    gimp_blend_tool_create_graph (blend_tool);

  blend_tool->filter = gimp_drawable_filter_new (drawable,
                                                 C_("undo-type", "Blend"),
                                                 blend_tool->graph,
                                                 GIMP_ICON_TOOL_GRADIENT);

  gimp_drawable_filter_set_region (blend_tool->filter,
                                   GIMP_FILTER_REGION_DRAWABLE);
  gimp_drawable_filter_set_opacity (blend_tool->filter,
                                    gimp_context_get_opacity (context));
  gimp_drawable_filter_set_mode (blend_tool->filter,
                                 gimp_context_get_paint_mode (context),
                                 GIMP_LAYER_COLOR_SPACE_AUTO,
                                 GIMP_LAYER_COLOR_SPACE_AUTO,
                                 GIMP_LAYER_COMPOSITE_AUTO);

  g_signal_connect (blend_tool->filter, "flush",
                    G_CALLBACK (gimp_blend_tool_filter_flush),
                    blend_tool);
}

static void
gimp_blend_tool_filter_flush (GimpDrawableFilter *filter,
                              GimpTool           *tool)
{
  GimpImage *image = gimp_display_get_image (tool->display);

  gimp_projection_flush (gimp_image_get_projection (image));
}


/*  protected functions  */


void
gimp_blend_tool_set_tentative_gradient (GimpBlendTool *blend_tool,
                                        GimpGradient  *gradient)
{
  g_return_if_fail (GIMP_IS_BLEND_TOOL (blend_tool));
  g_return_if_fail (gradient == NULL || GIMP_IS_GRADIENT (gradient));

  if (gradient != blend_tool->tentative_gradient)
    {
      g_clear_object (&blend_tool->tentative_gradient);

      blend_tool->tentative_gradient = gradient;

      if (gradient)
        g_object_ref (gradient);

      if (blend_tool->render_node)
        {
          gegl_node_set (blend_tool->render_node,
                         "gradient", gradient ? gradient : blend_tool->gradient,
                         NULL);

          gimp_drawable_filter_apply (blend_tool->filter, NULL);
        }
    }
}
