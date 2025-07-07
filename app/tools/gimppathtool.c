/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Path tool
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpdialogconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-item.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimptoolinfo.h"
#include "core/gimpundostack.h"

#include "paint/gimppaintoptions.h" /* GIMP_PAINT_OPTIONS_CONTEXT_MASK */

#include "path/gimppath.h"
#include "path/gimpvectorlayer.h"
#include "path/gimpvectorlayeroptions.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockcontainer.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimptoolpath.h"

#include "gimppathoptions.h"
#include "gimppathtool.h"
#include "gimptoolcontrol.h"

#include "dialogs/fill-dialog.h"
#include "dialogs/stroke-dialog.h"

#include "gimp-intl.h"


#define TOGGLE_MASK  gimp_get_extend_selection_mask ()
#define MOVE_MASK    GDK_MOD1_MASK
#define INSDEL_MASK  gimp_get_toggle_behavior_mask ()


/*  local function prototypes  */

static void     gimp_path_tool_constructed        (GObject               *object);
static void     gimp_path_tool_dispose            (GObject               *object);

static void     gimp_path_tool_control            (GimpTool              *tool,
                                                   GimpToolAction         action,
                                                   GimpDisplay           *display);
static void     gimp_path_tool_button_press       (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpButtonPressType    press_type,
                                                   GimpDisplay           *display);
static void     gimp_path_tool_button_release     (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpButtonReleaseType  release_type,
                                                   GimpDisplay           *display);
static void     gimp_path_tool_motion             (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);
static void     gimp_path_tool_modifier_key       (GimpTool              *tool,
                                                   GdkModifierType        key,
                                                   gboolean               press,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);
static void     gimp_path_tool_cursor_update      (GimpTool              *tool,
                                                   const GimpCoords      *coords,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);

static void     gimp_path_tool_start              (GimpPathTool          *path_tool,
                                                   GimpDisplay           *display);
static void     gimp_path_tool_halt               (GimpPathTool          *path_tool);

static void     gimp_path_tool_image_changed      (GimpPathTool          *path_tool,
                                                   GimpImage             *image,
                                                   GimpContext           *context);
static void     gimp_path_tool_image_selected_layers_changed
                                                  (GimpPathTool          *path_tool);

static void     gimp_path_tool_tool_path_changed  (GimpToolWidget        *tool_path,
                                                   GimpPathTool          *path_tool);
static void     gimp_path_tool_tool_path_begin_change
                                                  (GimpToolWidget        *tool_path,
                                                   const gchar           *desc,
                                                   GimpPathTool          *path_tool);
static void     gimp_path_tool_tool_path_end_change
                                                  (GimpToolWidget        *tool_path,
                                                   gboolean               success,
                                                   GimpPathTool          *path_tool);
static void     gimp_path_tool_tool_path_activate (GimpToolWidget        *tool_path,
                                                   GdkModifierType        state,
                                                   GimpPathTool          *path_tool);

static void     gimp_path_tool_path_changed       (GimpImage             *image,
                                                   GimpPathTool          *path_tool);
static void     gimp_path_tool_path_removed       (GimpPath              *path,
                                                   GimpPathTool          *path_tool);

static void     gimp_path_tool_to_selection       (GimpPathTool          *path_tool);
static void     gimp_path_tool_to_selection_extended
                                                  (GimpPathTool          *path_tool,
                                                   GdkModifierType        state);

static void     gimp_path_tool_fill_path          (GimpPathTool          *path_tool,
                                                   GtkWidget             *button);
static void     gimp_path_tool_fill_callback      (GtkWidget             *dialog,
                                                   GList                 *items,
                                                   GList                 *drawables,
                                                   GimpContext           *context,
                                                   GimpFillOptions       *options,
                                                   gpointer               data);

static void     gimp_path_tool_stroke_path        (GimpPathTool          *path_tool,
                                                   GtkWidget             *button);
static void     gimp_path_tool_stroke_callback    (GtkWidget             *dialog,
                                                   GList                 *items,
                                                   GList                 *drawables,
                                                   GimpContext           *context,
                                                   GimpStrokeOptions     *options,
                                                   gpointer               data);

static void     gimp_path_tool_create_vector_layer(GimpPathTool          *path_tool,
						                           GtkWidget             *button);

G_DEFINE_TYPE (GimpPathTool, gimp_path_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_path_tool_parent_class


void
gimp_path_tool_register (GimpToolRegisterCallback callback,
                         gpointer                 data)
{
  (* callback) (GIMP_TYPE_PATH_TOOL,
                GIMP_TYPE_PATH_OPTIONS,
                gimp_path_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK |
                GIMP_CONTEXT_PROP_MASK_PATTERN  |
                GIMP_CONTEXT_PROP_MASK_GRADIENT, /* for stroking */
                "gimp-path-tool",
                _("Paths"),
                _("Paths Tool: Create and edit paths"),
                N_("Pat_hs"), "b",
                NULL, GIMP_HELP_TOOL_PATH,
                GIMP_ICON_TOOL_PATH,
                data);
}

static void
gimp_path_tool_class_init (GimpPathToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass *tool_class   = GIMP_TOOL_CLASS (klass);

  object_class->constructed  = gimp_path_tool_constructed;
  object_class->dispose      = gimp_path_tool_dispose;

  tool_class->control        = gimp_path_tool_control;
  tool_class->button_press   = gimp_path_tool_button_press;
  tool_class->button_release = gimp_path_tool_button_release;
  tool_class->motion         = gimp_path_tool_motion;
  tool_class->modifier_key   = gimp_path_tool_modifier_key;
  tool_class->cursor_update  = gimp_path_tool_cursor_update;
}

static void
gimp_path_tool_init (GimpPathTool *path_tool)
{
  GimpTool *tool = GIMP_TOOL (path_tool);

  gimp_tool_control_set_handle_empty_image (tool->control, TRUE);
  gimp_tool_control_set_precision          (tool->control,
                                            GIMP_CURSOR_PRECISION_SUBPIXEL);
  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_PATHS);

  path_tool->saved_mode = GIMP_PATH_MODE_DESIGN;
}

static void
gimp_path_tool_constructed (GObject *object)
{
  GimpPathTool  *path_tool = GIMP_PATH_TOOL (object);
  GimpContext   *context;
  GimpToolInfo  *tool_info;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  tool_info = GIMP_TOOL (path_tool)->tool_info;

  context = gimp_get_user_context (tool_info->gimp);

  g_signal_connect_object (context, "image-changed",
                           G_CALLBACK (gimp_path_tool_image_changed),
                           path_tool, G_CONNECT_SWAPPED);

  gimp_path_tool_image_changed (path_tool,
                                gimp_context_get_image (context),
                                context);
}

static void
gimp_path_tool_dispose (GObject *object)
{
  GimpPathTool *path_tool = GIMP_PATH_TOOL (object);

  gimp_path_tool_set_path (path_tool, NULL);
  g_clear_object (&path_tool->widget);

  gimp_path_tool_image_changed (path_tool, NULL, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_path_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *display)
{
  GimpPathTool *path_tool = GIMP_PATH_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_path_tool_halt (path_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_path_tool_button_press (GimpTool            *tool,
                             const GimpCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             GimpButtonPressType  press_type,
                             GimpDisplay         *display)
{
  GimpPathTool *path_tool = GIMP_PATH_TOOL (tool);

  if (tool->display && display != tool->display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);

  if (! tool->display)
    {
      gimp_path_tool_start (path_tool, display);

      gimp_tool_widget_hover (path_tool->widget, coords, state, TRUE);
    }

  if (gimp_tool_widget_button_press (path_tool->widget, coords, time, state,
                                     press_type))
    {
      path_tool->grab_widget = path_tool->widget;
    }

  gimp_tool_control_activate (tool->control);
}

static void
gimp_path_tool_button_release (GimpTool              *tool,
                               const GimpCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type,
                               GimpDisplay           *display)
{
  GimpPathTool *path_tool = GIMP_PATH_TOOL (tool);

  gimp_tool_control_halt (tool->control);

  if (path_tool->grab_widget)
    {
      gimp_tool_widget_button_release (path_tool->grab_widget,
                                       coords, time, state, release_type);
      path_tool->grab_widget = NULL;
    }
}

static void
gimp_path_tool_motion (GimpTool         *tool,
                       const GimpCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       GimpDisplay      *display)
{
  GimpPathTool *path_tool = GIMP_PATH_TOOL (tool);

  if (path_tool->grab_widget)
    {
      gimp_tool_widget_motion (path_tool->grab_widget, coords, time, state);
    }
}

static void
gimp_path_tool_modifier_key (GimpTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             GimpDisplay     *display)
{
  GimpPathTool    *path_tool = GIMP_PATH_TOOL (tool);
  GimpPathOptions *options   = GIMP_PATH_TOOL_GET_OPTIONS (tool);

  if (key == TOGGLE_MASK)
    return;

  if (key == INSDEL_MASK || key == MOVE_MASK)
    {
      GimpPathMode button_mode = options->edit_mode;

      if (press)
        {
          if (key == (state & (INSDEL_MASK | MOVE_MASK)))
            {
              /*  first modifier pressed  */

              path_tool->saved_mode = options->edit_mode;
            }
        }
      else
        {
          if (! (state & (INSDEL_MASK | MOVE_MASK)))
            {
              /*  last modifier released  */

              button_mode = path_tool->saved_mode;
            }
        }

      if (state & MOVE_MASK)
        {
          button_mode = GIMP_PATH_MODE_MOVE;
        }
      else if (state & INSDEL_MASK)
        {
          button_mode = GIMP_PATH_MODE_EDIT;
        }

      if (button_mode != options->edit_mode)
        {
          g_object_set (options, "path-edit-mode", button_mode, NULL);
        }
    }
}

static void
gimp_path_tool_cursor_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              GimpDisplay      *display)
{
  GimpPathTool     *path_tool = GIMP_PATH_TOOL (tool);
  GimpDisplayShell *shell     = gimp_display_get_shell (display);

  if (display != tool->display || ! path_tool->widget)
    {
      GimpToolCursorType tool_cursor = GIMP_TOOL_CURSOR_PATHS;

      if (gimp_image_pick_path (gimp_display_get_image (display),
                                coords->x, coords->y,
                                FUNSCALEX (shell,
                                           GIMP_TOOL_HANDLE_SIZE_CIRCLE / 2),
                                FUNSCALEY (shell,
                                           GIMP_TOOL_HANDLE_SIZE_CIRCLE / 2)))
        {
          tool_cursor = GIMP_TOOL_CURSOR_HAND;
        }

      gimp_tool_control_set_tool_cursor (tool->control, tool_cursor);
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_path_tool_start (GimpPathTool *path_tool,
                      GimpDisplay  *display)
{
  GimpTool         *tool    = GIMP_TOOL (path_tool);
  GimpPathOptions  *options = GIMP_PATH_TOOL_GET_OPTIONS (tool);
  GimpDisplayShell *shell   = gimp_display_get_shell (display);
  GimpToolWidget   *widget;

  tool->display = display;

  path_tool->widget = widget = gimp_tool_path_new (shell);

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), widget);

  g_object_bind_property (G_OBJECT (options), "path-edit-mode",
                          G_OBJECT (widget),  "edit-mode",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);
  g_object_bind_property (G_OBJECT (options), "path-polygonal",
                          G_OBJECT (widget),  "polygonal",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  gimp_tool_path_set_path (GIMP_TOOL_PATH (widget),
                           path_tool->path);

  g_signal_connect (widget, "changed",
                    G_CALLBACK (gimp_path_tool_tool_path_changed),
                    path_tool);
  g_signal_connect (widget, "begin-change",
                    G_CALLBACK (gimp_path_tool_tool_path_begin_change),
                    path_tool);
  g_signal_connect (widget, "end-change",
                    G_CALLBACK (gimp_path_tool_tool_path_end_change),
                    path_tool);
  g_signal_connect (widget, "activate",
                    G_CALLBACK (gimp_path_tool_tool_path_activate),
                    path_tool);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}

static void
gimp_path_tool_halt (GimpPathTool *path_tool)
{
  GimpTool *tool = GIMP_TOOL (path_tool);

  if (tool->display)
    gimp_tool_pop_status (tool, tool->display);

  gimp_path_tool_set_path (path_tool, NULL);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), NULL);
  g_clear_object (&path_tool->widget);

  tool->display = NULL;
}

static void
gimp_path_tool_image_changed (GimpPathTool *path_tool,
                              GimpImage    *image,
                              GimpContext  *context)
{
  if (path_tool->current_image)
    g_signal_handlers_disconnect_by_func (path_tool->current_image,
                                          gimp_path_tool_image_selected_layers_changed,
                                          NULL);

  g_set_weak_pointer (&path_tool->current_image, image);

  if (path_tool->current_image)
    g_signal_connect_object (path_tool->current_image, "selected-layers-changed",
                             G_CALLBACK (gimp_path_tool_image_selected_layers_changed),
                             path_tool, G_CONNECT_SWAPPED);

  gimp_path_tool_image_selected_layers_changed (path_tool);
}

static void
gimp_path_tool_image_selected_layers_changed (GimpPathTool *path_tool)
{
  GList *current_layers = NULL;

  if (path_tool->current_image)
    current_layers =
      gimp_image_get_selected_layers (path_tool->current_image);

  if (current_layers)
    {
      /* If we've selected a single vector layer, make its path editable */
      if (g_list_length (current_layers) == 1 &&
          GIMP_IS_VECTOR_LAYER (current_layers->data))
        {
          GimpVectorLayer *vector_layer = current_layers->data;

          if (vector_layer->options && vector_layer->options->path)
            gimp_path_tool_set_path (path_tool,
                                       vector_layer->options->path);
        }
    }
}

static void
gimp_path_tool_tool_path_changed (GimpToolWidget *tool_path,
                                  GimpPathTool   *path_tool)
{
  GimpDisplayShell *shell = gimp_tool_widget_get_shell (tool_path);
  GimpImage        *image = gimp_display_get_image (shell->display);
  GimpPath         *path;

  g_object_get (tool_path,
                "path", &path,
                NULL);

  if (path != path_tool->path)
    {
      if (path && ! gimp_item_is_attached (GIMP_ITEM (path)))
        {
          gimp_image_add_path (image, path,
                               GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
          gimp_image_flush (image);

          gimp_path_tool_set_path (path_tool, path);
        }
      else
        {
          gimp_path_tool_set_path (path_tool, path);

          if (path)
            {
              GList *list = g_list_prepend (NULL, path);

              gimp_image_set_selected_paths (image, list);
              g_list_free (list);
            }
        }
    }

  if (path)
    g_object_unref (path);
}

static void
gimp_path_tool_tool_path_begin_change (GimpToolWidget *tool_path,
                                       const gchar    *desc,
                                       GimpPathTool   *path_tool)
{
  GimpDisplayShell *shell = gimp_tool_widget_get_shell (tool_path);
  GimpImage        *image = gimp_display_get_image (shell->display);

  gimp_image_undo_push_path_mod (image, desc, path_tool->path);
}

static void
gimp_path_tool_tool_path_end_change (GimpToolWidget *tool_path,
                                     gboolean        success,
                                     GimpPathTool   *path_tool)
{
  GimpDisplayShell *shell = gimp_tool_widget_get_shell (tool_path);
  GimpImage        *image = gimp_display_get_image (shell->display);

  if (! success)
    {
      GimpUndo            *undo;
      GimpUndoAccumulator  accum = { 0, };

      undo = gimp_undo_stack_pop_undo (gimp_image_get_undo_stack (image),
                                       GIMP_UNDO_MODE_UNDO, &accum);

      gimp_image_undo_event (image, GIMP_UNDO_EVENT_UNDO_EXPIRED, undo);

      gimp_undo_free (undo, GIMP_UNDO_MODE_UNDO);
      g_object_unref (undo);
    }

  gimp_image_flush (image);
}

static void
gimp_path_tool_tool_path_activate (GimpToolWidget  *tool_path,
                                   GdkModifierType  state,
                                   GimpPathTool    *path_tool)
{
  gimp_path_tool_to_selection_extended (path_tool, state);
}

static void
gimp_path_tool_path_changed (GimpImage    *image,
                             GimpPathTool *path_tool)
{
  GimpPath *path = NULL;

  /* The path tool can only work on one path at a time. */
  if (g_list_length (gimp_image_get_selected_paths (image)) == 1)
    path = gimp_image_get_selected_paths (image)->data;

  gimp_path_tool_set_path (path_tool, path);
}

static void
gimp_path_tool_path_removed (GimpPath     *path,
                             GimpPathTool *path_tool)
{
  gimp_path_tool_set_path (path_tool, NULL);
}

void
gimp_path_tool_set_path (GimpPathTool *path_tool,
                         GimpPath     *path)
{
  GimpTool        *tool;
  GimpItem        *item = NULL;
  GimpPathOptions *options;

  g_return_if_fail (GIMP_IS_PATH_TOOL (path_tool));
  g_return_if_fail (path == NULL || GIMP_IS_PATH (path));

  tool    = GIMP_TOOL (path_tool);
  options = GIMP_PATH_TOOL_GET_OPTIONS (path_tool);

  if (path)
    item = GIMP_ITEM (path);

  if (path == path_tool->path)
    return;

  if (path_tool->path)
    {
      GimpImage *old_image;

      old_image = gimp_item_get_image (GIMP_ITEM (path_tool->path));

      g_signal_handlers_disconnect_by_func (old_image,
                                            gimp_path_tool_path_changed,
                                            path_tool);
      g_signal_handlers_disconnect_by_func (path_tool->path,
                                            gimp_path_tool_path_removed,
                                            path_tool);

      g_clear_object (&path_tool->path);

      if (options->to_selection_button)
        {
          gtk_widget_set_sensitive (options->to_selection_button, FALSE);
          g_signal_handlers_disconnect_by_func (options->to_selection_button,
                                                gimp_path_tool_to_selection,
                                                tool);
          g_signal_handlers_disconnect_by_func (options->to_selection_button,
                                                gimp_path_tool_to_selection_extended,
                                                tool);
        }

      if (options->fill_button)
        {
          gtk_widget_set_sensitive (options->fill_button, FALSE);
          g_signal_handlers_disconnect_by_func (options->fill_button,
                                                gimp_path_tool_fill_path,
                                                tool);
        }

      if (options->stroke_button)
        {
          gtk_widget_set_sensitive (options->stroke_button, FALSE);
          g_signal_handlers_disconnect_by_func (options->stroke_button,
                                                gimp_path_tool_stroke_path,
                                                tool);
        }

      if (options->vector_layer_button)
        {
          gtk_widget_set_sensitive (options->vector_layer_button, FALSE);
          g_signal_handlers_disconnect_by_func (options->vector_layer_button,
                                                gimp_path_tool_create_vector_layer,
                                                tool);
        }
    }

  if (! path ||
      (tool->display &&
       gimp_display_get_image (tool->display) != gimp_item_get_image (item)))
    {
      gimp_path_tool_halt (path_tool);
    }

  if (! path)
    return;

  path_tool->path = g_object_ref (path);

  g_signal_connect_object (gimp_item_get_image (item), "selected-paths-changed",
                           G_CALLBACK (gimp_path_tool_path_changed),
                           path_tool, 0);
  g_signal_connect_object (path, "removed",
                           G_CALLBACK (gimp_path_tool_path_removed),
                           path_tool, 0);

  if (options->to_selection_button)
    {
      g_signal_connect_swapped (options->to_selection_button, "clicked",
                                G_CALLBACK (gimp_path_tool_to_selection),
                                tool);
      g_signal_connect_swapped (options->to_selection_button, "extended-clicked",
                                G_CALLBACK (gimp_path_tool_to_selection_extended),
                                tool);
      gtk_widget_set_sensitive (options->to_selection_button, TRUE);
    }

  if (options->fill_button)
    {
      g_signal_connect_swapped (options->fill_button, "clicked",
                                G_CALLBACK (gimp_path_tool_fill_path),
                                tool);
      gtk_widget_set_sensitive (options->fill_button, TRUE);
    }

  if (options->stroke_button)
    {
      g_signal_connect_swapped (options->stroke_button, "clicked",
                                G_CALLBACK (gimp_path_tool_stroke_path),
                                tool);
      gtk_widget_set_sensitive (options->stroke_button, TRUE);
    }

  if (options->vector_layer_button)
    {
      g_signal_connect_swapped (options->vector_layer_button, "clicked",
				                G_CALLBACK (gimp_path_tool_create_vector_layer),
				                tool);

      gtk_widget_set_sensitive (options->vector_layer_button, TRUE);
    }

  if (tool->display)
    {
      gimp_tool_path_set_path (GIMP_TOOL_PATH (path_tool->widget), path);
    }
  else
    {
      GimpContext *context = gimp_get_user_context (tool->tool_info->gimp);
      GimpDisplay *display = gimp_context_get_display (context);

      if (! display ||
          gimp_display_get_image (display) != gimp_item_get_image (item))
        {
          GList *list;

          display = NULL;

          for (list = gimp_get_display_iter (gimp_item_get_image (item)->gimp);
               list;
               list = g_list_next (list))
            {
              display = list->data;

              if (gimp_display_get_image (display) == gimp_item_get_image (item))
                {
                  gimp_context_set_display (context, display);
                  break;
                }

              display = NULL;
            }
        }

      if (display)
        gimp_path_tool_start (path_tool, display);
    }

  if (options->edit_mode != GIMP_PATH_MODE_DESIGN)
    g_object_set (options, "path-edit-mode",
                  GIMP_PATH_MODE_DESIGN, NULL);
}

static void
gimp_path_tool_to_selection (GimpPathTool *path_tool)
{
  gimp_path_tool_to_selection_extended (path_tool, 0);
}

static void
gimp_path_tool_to_selection_extended (GimpPathTool    *path_tool,
                                      GdkModifierType  state)
{
  GimpImage *image;

  if (! path_tool->path)
    return;

  image = gimp_item_get_image (GIMP_ITEM (path_tool->path));

  gimp_item_to_selection (GIMP_ITEM (path_tool->path),
                          gimp_modifiers_to_channel_op (state),
                          TRUE, FALSE, 0, 0);
  gimp_image_flush (image);
}


static void
gimp_path_tool_fill_path (GimpPathTool *path_tool,
                          GtkWidget    *button)
{
  GimpDialogConfig *config;
  GimpImage        *image;
  GList            *drawables;
  GList            *path_list = NULL;
  GtkWidget        *dialog;

  if (! path_tool->path)
    return;

  image = gimp_item_get_image (GIMP_ITEM (path_tool->path));

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  drawables = gimp_image_get_selected_drawables (image);

  if (! drawables)
    {
      gimp_tool_message (GIMP_TOOL (path_tool),
                         GIMP_TOOL (path_tool)->display,
                         _("There are no selected layers or channels to fill."));
      return;
    }

  if (g_list_length (drawables) == 1 &&
      gimp_item_is_content_locked (GIMP_ITEM (drawables->data), NULL))
    {
      gimp_tool_message (GIMP_TOOL (path_tool),
                         GIMP_TOOL (path_tool)->display,
                         _("A selected layer's pixels are locked."));
      return;
    }

  path_list = g_list_prepend (NULL, path_tool->path);
  dialog = fill_dialog_new (path_list, drawables,
                            GIMP_CONTEXT (GIMP_TOOL_GET_OPTIONS (path_tool)),
                            _("Fill Path"),
                            GIMP_ICON_TOOL_BUCKET_FILL,
                            GIMP_HELP_PATH_FILL,
                            button,
                            config->fill_options,
                            gimp_path_tool_fill_callback,
                            path_tool);
  gtk_widget_show (dialog);
  g_list_free (path_list);
  g_list_free (drawables);
}

static void
gimp_path_tool_fill_callback (GtkWidget       *dialog,
                              GList           *items,
                              GList           *drawables,
                              GimpContext     *context,
                              GimpFillOptions *options,
                              gpointer         data)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (context->gimp->config);
  GimpImage        *image  = gimp_item_get_image (items->data);
  GError           *error  = NULL;

  gimp_config_sync (G_OBJECT (options),
                    G_OBJECT (config->fill_options), 0);

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_DRAWABLE_MOD,
                               "Fill");

  for (GList *iter = items; iter; iter = iter->next)
    if (! gimp_item_fill (iter->data, drawables, options,
                          TRUE, NULL, &error))
      {
        gimp_message_literal (context->gimp,
                              G_OBJECT (dialog),
                              GIMP_MESSAGE_WARNING,
                              error ? error->message : "NULL");

        g_clear_error (&error);
        break;
      }

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
  gtk_widget_destroy (dialog);
}


static void
gimp_path_tool_stroke_path (GimpPathTool *path_tool,
                            GtkWidget    *button)
{
  GimpDialogConfig *config;
  GimpImage        *image;
  GList            *drawables;
  GList            *path_list = NULL;
  GtkWidget        *dialog;

  if (! path_tool->path)
    return;

  image = gimp_item_get_image (GIMP_ITEM (path_tool->path));

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  drawables = gimp_image_get_selected_drawables (image);

  if (! drawables)
    {
      gimp_tool_message (GIMP_TOOL (path_tool),
                         GIMP_TOOL (path_tool)->display,
                         _("There are no selected layers or channels to stroke to."));
      return;
    }

  if (g_list_length (drawables) == 1 &&
      gimp_item_is_content_locked (GIMP_ITEM (drawables->data), NULL))
    {
      gimp_tool_message (GIMP_TOOL (path_tool),
                         GIMP_TOOL (path_tool)->display,
                         _("A selected layer's pixels are locked."));
      return;
    }

  path_list = g_list_prepend (NULL, path_tool->path);
  dialog = stroke_dialog_new (path_list, drawables,
                              GIMP_CONTEXT (GIMP_TOOL_GET_OPTIONS (path_tool)),
                              _("Stroke Path"),
                              GIMP_ICON_PATH_STROKE,
                              GIMP_HELP_PATH_STROKE,
                              button,
                              config->stroke_options,
                              gimp_path_tool_stroke_callback,
                              path_tool);
  gtk_widget_show (dialog);
  g_list_free (path_list);
  g_list_free (drawables);
}

static void
gimp_path_tool_stroke_callback (GtkWidget         *dialog,
                                GList             *items,
                                GList             *drawables,
                                GimpContext       *context,
                                GimpStrokeOptions *options,
                                gpointer           data)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (context->gimp->config);
  GimpImage        *image  = gimp_item_get_image (items->data);
  GError           *error  = NULL;

  gimp_config_sync (G_OBJECT (options),
                    G_OBJECT (config->stroke_options), 0);

  gimp_image_undo_group_start (image,
                               GIMP_UNDO_GROUP_DRAWABLE_MOD,
                               "Stroke");

  for (GList *iter = items; iter; iter = iter->next)
    if (! gimp_item_stroke (iter->data, drawables, context, options, NULL,
                            TRUE, NULL, &error))
      {
        gimp_message_literal (context->gimp,
                              G_OBJECT (dialog),
                              GIMP_MESSAGE_WARNING,
                              error ? error->message : "NULL");

        g_clear_error (&error);
        break;
      }

  gimp_image_undo_group_end (image);
  gimp_image_flush (image);
  gtk_widget_destroy (dialog);
}

static void
gimp_path_tool_create_vector_layer (GimpPathTool *path_tool,
				                    GtkWidget    *button)
{
  GimpImage       *image;
  GimpVectorLayer *layer;

  if (! path_tool->path)
    return;

  image = gimp_item_get_image (GIMP_ITEM (path_tool->path));

  layer = gimp_vector_layer_new (image, path_tool->path,
				                 gimp_get_user_context (image->gimp));

  gimp_image_add_layer (image,
                        GIMP_LAYER (layer),
                        GIMP_IMAGE_ACTIVE_PARENT,
                        -1,
                        TRUE);

  gimp_vector_layer_refresh (layer);

  gimp_image_flush (image);
}
