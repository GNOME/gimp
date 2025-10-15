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
#include "core/gimprasterizable.h"
#include "core/gimpstrokeoptions.h"
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
#include "widgets/gimpviewabledialog.h"
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

#define RESPONSE_NEW 1

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

static void     gimp_path_tool_create_vector_layer(GimpPathTool          *path_tool);
static void     gimp_path_tool_vector_change_notify
                                                  (GObject               *options,
                                                   const GParamSpec      *pspec,
                                                   GimpVectorLayer       *layer);

static void     gimp_path_tool_layer_rasterized   (GimpVectorLayer       *layer,
                                                   gboolean               is_rasterized,
                                                   GimpPathTool          *tool);

static void     gimp_path_tool_set_layer          (GimpPathTool          *path_tool,
                                                   GimpVectorLayer       *vector_layer);

static void     gimp_path_tool_confirm_dialog     (GimpPathTool          *path_tool,
                                                   GimpVectorLayer       *layer);
static void     gimp_path_tool_confirm_response   (GimpViewableDialog    *dialog,
                                                   gint                   response_id,
                                                   GimpPathTool          *path_tool);


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
  tool_class->is_destructive = FALSE;
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

  gimp_path_tool_set_path (path_tool, NULL, NULL);
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

  gimp_path_tool_set_path (path_tool, NULL, NULL);

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
  GimpPathOptions *options = GIMP_PATH_TOOL_GET_OPTIONS (path_tool);

  if (path_tool->current_image)
    {
      g_signal_handlers_disconnect_by_func (path_tool->current_image,
                                            gimp_path_tool_image_selected_layers_changed,
                                            NULL);

      g_signal_handlers_disconnect_by_func (options->vector_layer_button,
                                            gimp_path_tool_create_vector_layer,
                                            path_tool);
    }

  g_set_weak_pointer (&path_tool->current_image, image);

  if (path_tool->current_image)
    {
      g_signal_connect_object (path_tool->current_image, "selected-layers-changed",
                               G_CALLBACK (gimp_path_tool_image_selected_layers_changed),
                               path_tool, G_CONNECT_SWAPPED);

      g_signal_connect_object (options->vector_layer_button, "clicked",
                               G_CALLBACK (gimp_path_tool_create_vector_layer),
                               path_tool, G_CONNECT_SWAPPED);
    }

  gimp_path_tool_image_selected_layers_changed (path_tool);

  if (options->vector_layer_button)
    gtk_widget_set_sensitive (options->vector_layer_button,
                              path_tool->current_image ? TRUE : FALSE);
}

static void
gimp_path_tool_image_selected_layers_changed (GimpPathTool *path_tool)
{
  GList *current_layers = NULL;

  if (path_tool->current_image)
    current_layers = gimp_image_get_selected_layers (path_tool->current_image);

  /* If we've selected a single vector layer, make its path editable */
  if (current_layers                      &&
      g_list_length (current_layers) == 1 &&
      GIMP_IS_VECTOR_LAYER (GIMP_ITEM (current_layers->data)))
    {
      GimpVectorLayer *vector_layer = current_layers->data;

      if (gimp_item_is_vector_layer (GIMP_ITEM (current_layers->data)))
        gimp_path_tool_set_layer (path_tool, vector_layer);
      else
        gimp_path_tool_confirm_dialog (path_tool, vector_layer);
    }
  else
    {
      gimp_path_tool_set_layer (path_tool, NULL);
    }
}

static void
gimp_path_tool_tool_path_changed (GimpToolWidget *tool_path,
                                  GimpPathTool   *path_tool)
{
  GimpDisplayShell *shell = gimp_tool_widget_get_shell (tool_path);
  GimpImage        *image = gimp_display_get_image (shell->display);
  GimpPath         *path;

  g_object_get (tool_path, "path", &path, NULL);

  if (path != path_tool->path)
    {
      if (path && ! gimp_item_is_attached (GIMP_ITEM (path)))
        {
          gimp_image_add_path (image, path,
                               GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
          gimp_image_flush (image);

          gimp_path_tool_set_path (path_tool, NULL, path);
        }
      else
        {
          gimp_path_tool_set_path (path_tool, NULL, path);

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

  gimp_path_tool_set_path (path_tool, NULL, path);
}

static void
gimp_path_tool_path_removed (GimpPath     *path,
                             GimpPathTool *path_tool)
{
  gimp_path_tool_set_path (path_tool, NULL, NULL);
}

void
gimp_path_tool_set_path (GimpPathTool    *path_tool,
                         GimpVectorLayer *layer,
                         GimpPath        *path)
{
  GimpTool        *tool;
  GimpItem        *item = NULL;
  GimpPathOptions *options;

  g_return_if_fail (GIMP_IS_PATH_TOOL (path_tool));
  g_return_if_fail (path == NULL || GIMP_IS_PATH (path));
  g_return_if_fail (layer == NULL || path == NULL);

  tool = GIMP_TOOL (path_tool);options = GIMP_PATH_TOOL_GET_OPTIONS (path_tool);

  if (layer != NULL)
    path = gimp_vector_layer_get_path (layer);

  if (path)
    item = GIMP_ITEM (path);

  if (path == path_tool->path)
    return;

  if (path_tool->current_vector_layer && path && ! layer)
    {
      GimpImage *image      = path_tool->current_image;
      GList     *all_layers = gimp_image_get_layer_list (image);
      GList     *layers     = NULL;

      for (GList *iter = all_layers; iter; iter = iter->next)
        {
          if (gimp_item_is_vector_layer (iter->data) &&
              gimp_vector_layer_get_path (iter->data) == path)
            layers = g_list_prepend (layers, iter->data);
        }
      gimp_image_set_selected_layers (image, layers);
      g_list_free (all_layers);
      g_list_free (layers);
    }

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
    }

  if (! path ||
      (tool->display &&
       gimp_display_get_image (tool->display) != gimp_item_get_image (item)))
    {
      gimp_path_tool_halt (path_tool);
    }

  if (layer || ! path)
    gtk_button_set_label (GTK_BUTTON (options->vector_layer_button),
                          _("Create New Vector Layer"));
  else
    gtk_button_set_label (GTK_BUTTON (options->vector_layer_button),
                          _("Create Vector Layer from Path"));

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
gimp_path_tool_create_vector_layer (GimpPathTool *path_tool)
{
  GimpImage       *image;
  GimpVectorLayer *layer;
  GimpPathOptions *options = GIMP_PATH_TOOL_GET_OPTIONS (path_tool);

  if (! path_tool->current_image)
    return;

  image = path_tool->current_image;

  g_signal_handlers_block_by_func (image,
                                   gimp_path_tool_image_selected_layers_changed,
                                   path_tool);

  /* If there's already an active vector layer, or no paths are
   * selected, create a new blank path.
   */
  if (path_tool->current_vector_layer || ! path_tool->path)
    {
      GimpPath *new_path = gimp_path_new (image, _("Vector Layer"));

      gimp_image_add_path (image, new_path, GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
      gimp_path_tool_set_path (path_tool, NULL, new_path);

      g_signal_handlers_disconnect_by_func (options,
                                            gimp_path_tool_vector_change_notify,
                                            path_tool->current_vector_layer);
      g_signal_handlers_disconnect_by_func (options->fill_options,
                                            gimp_path_tool_vector_change_notify,
                                            path_tool->current_vector_layer);
      g_signal_handlers_disconnect_by_func (options->stroke_options,
                                            gimp_path_tool_vector_change_notify,
                                            path_tool->current_vector_layer);
    }

  layer = gimp_vector_layer_new (image, path_tool->path,
                                 gimp_get_user_context (image->gimp));
  g_object_set (layer->options,
                "enable-fill",   options->enable_fill,
                "enable-stroke", options->enable_stroke,
                NULL);
  gimp_config_sync (G_OBJECT (options->fill_options),
                    G_OBJECT (layer->options->fill_options), 0);
  gimp_config_sync (G_OBJECT (options->stroke_options),
                    G_OBJECT (layer->options->stroke_options), 0);

  gimp_image_add_layer (image,
                        GIMP_LAYER (layer),
                        GIMP_IMAGE_ACTIVE_PARENT,
                        -1,
                        TRUE);

  gimp_path_tool_set_layer (path_tool, layer);

  gimp_vector_layer_refresh (layer);
  gimp_image_flush (image);

  g_signal_handlers_unblock_by_func (image,
                                     gimp_path_tool_image_selected_layers_changed,
                                     path_tool);
}

static void
gimp_path_tool_vector_change_notify (GObject          *options,
                                     const GParamSpec *pspec,
                                     GimpVectorLayer  *layer)
{
  GimpImage *image;
  gboolean   needs_update = FALSE;

  image = gimp_item_get_image (GIMP_ITEM (layer));

  if (GIMP_IS_STROKE_OPTIONS (options))
    {
      GimpStrokeOptions *stroke_options = GIMP_STROKE_OPTIONS (options);

      gimp_config_sync (G_OBJECT (stroke_options),
                        G_OBJECT (layer->options->stroke_options), 0);

      needs_update = TRUE;
    }
  else if (GIMP_IS_FILL_OPTIONS (options))
    {
      GimpFillOptions *fill_options = GIMP_FILL_OPTIONS (options);

      gimp_config_sync (G_OBJECT (fill_options),
                        G_OBJECT (layer->options->fill_options), 0);

      needs_update = TRUE;
    }
  else if (GIMP_IS_PATH_OPTIONS (options))
    {
      GimpPathOptions *path_options = GIMP_PATH_OPTIONS (options);

      g_object_set (layer->options,
                    "enable-fill", path_options->enable_fill,
                    "enable-stroke", path_options->enable_stroke,
                    NULL);

      needs_update = TRUE;
    }

  if (needs_update)
    {
      gimp_vector_layer_refresh (layer);
      gimp_image_flush (image);
    }
}

static void
gimp_path_tool_layer_rasterized (GimpVectorLayer  *layer,
                                 gboolean          is_rasterized,
                                 GimpPathTool     *tool)
{
  if (! is_rasterized && ! GIMP_TOOL (tool)->display)
    {
      GList *current_layers = NULL;

      if (tool->current_image)
        current_layers = gimp_image_get_selected_layers (tool->current_image);

      if (current_layers                      &&
          g_list_length (current_layers) == 1 &&
          current_layers->data == layer)
        gimp_path_tool_set_layer (tool, layer);
    }
  else if (is_rasterized && GIMP_TOOL (tool)->display)
    {
      gimp_path_tool_set_layer (tool, NULL);
    }
}

static void
gimp_path_tool_set_layer (GimpPathTool    *path_tool,
                          GimpVectorLayer *vector_layer)
{
  GimpPathOptions *options;

  g_return_if_fail (GIMP_IS_PATH_TOOL (path_tool));

  options = GIMP_PATH_TOOL_GET_OPTIONS (path_tool);

  if (path_tool->current_vector_layer)
    {
      g_signal_handlers_disconnect_by_func (options,
                                            gimp_path_tool_vector_change_notify,
                                            path_tool->current_vector_layer);
      g_signal_handlers_disconnect_by_func (options->fill_options,
                                            gimp_path_tool_vector_change_notify,
                                            path_tool->current_vector_layer);
      g_signal_handlers_disconnect_by_func (options->stroke_options,
                                            gimp_path_tool_vector_change_notify,
                                            path_tool->current_vector_layer);

      g_signal_handlers_disconnect_by_func (path_tool->current_vector_layer,
                                            G_CALLBACK (gimp_path_tool_layer_rasterized),
                                            path_tool);
    }

  path_tool->current_vector_layer = vector_layer;
  gimp_path_tool_set_path (path_tool, vector_layer, NULL);

  if (vector_layer)
    {
      g_object_set (options,
                    "enable-fill",   vector_layer->options->enable_fill,
                    "enable_stroke", vector_layer->options->enable_stroke,
                    NULL);
      gimp_config_sync (G_OBJECT (vector_layer->options->stroke_options),
                        G_OBJECT (options->stroke_options), 0);
      gimp_config_sync (G_OBJECT (vector_layer->options->fill_options),
                        G_OBJECT (options->fill_options), 0);

      g_signal_connect_object (options, "notify::enable-fill",
                               G_CALLBACK (gimp_path_tool_vector_change_notify),
                               vector_layer, 0);
      g_signal_connect_object (options, "notify::enable-stroke",
                               G_CALLBACK (gimp_path_tool_vector_change_notify),
                               vector_layer, 0);
      g_signal_connect_object (options->fill_options, "notify",
                               G_CALLBACK (gimp_path_tool_vector_change_notify),
                               vector_layer, 0);
      g_signal_connect_object (options->stroke_options, "notify",
                               G_CALLBACK (gimp_path_tool_vector_change_notify),
                               vector_layer, 0);

      g_signal_connect_object (vector_layer, "set-rasterized",
                               G_CALLBACK (gimp_path_tool_layer_rasterized),
                               path_tool, 0);
    }

  if (vector_layer)
    gtk_button_set_label (GTK_BUTTON (options->vector_layer_button),
                          _("Create New Vector Layer"));
}

static void
gimp_path_tool_confirm_dialog (GimpPathTool    *path_tool,
                               GimpVectorLayer *layer)
{
  GimpTool         *tool    = GIMP_TOOL (path_tool);
  GimpContext      *context = gimp_get_user_context (tool->tool_info->gimp);
  GimpDisplay      *display = gimp_context_get_display (context);
  GimpDisplayShell *shell   = display ? gimp_display_get_shell (display) : NULL;
  GtkWidget        *dialog;
  GtkWidget        *vbox;
  GtkWidget        *label;

  if (path_tool->confirm_dialog)
    {
      gtk_window_present (GTK_WINDOW (path_tool->confirm_dialog));
      return;
    }

  dialog = gimp_viewable_dialog_new (g_list_prepend (NULL, layer),
                                     GIMP_CONTEXT (gimp_tool_get_options (tool)),
                                     _("Confirm Path Editing"),
                                     "gimp-path-tool-confirm",
                                     GIMP_ICON_LAYER_VECTOR_LAYER,
                                     _("Confirm Path Editing"),
                                     GTK_WIDGET (shell),
                                     gimp_standard_help_func, NULL,

                                     _("Create _New Layer"), RESPONSE_NEW,
                                     _("_Cancel"),           GTK_RESPONSE_CANCEL,
                                     _("_Edit Anyway"),      GTK_RESPONSE_ACCEPT,

                                     NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            RESPONSE_NEW,
                                            GTK_RESPONSE_ACCEPT,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gimp_path_tool_confirm_response),
                    path_tool);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("The vector layer you picked was rasterized. "
                           "Editing its path will discard any modifications."));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dialog);

  path_tool->confirm_dialog = dialog;
  g_signal_connect_swapped (dialog, "destroy",
                            G_CALLBACK (g_nullify_pointer),
                            &path_tool->confirm_dialog);
}

static void
gimp_path_tool_confirm_response (GimpViewableDialog *dialog,
                                 gint                response_id,
                                 GimpPathTool       *path_tool)
{
  GimpVectorLayer *layer = dialog->viewables ? dialog->viewables->data : NULL;
  GimpImage       *image;
  GimpPath        *path;

  gtk_widget_destroy (GTK_WIDGET (dialog));

  if (layer && layer->options)
    {
      switch (response_id)
        {
        case RESPONSE_NEW:
          path  = gimp_vector_layer_get_path (layer);
          image = gimp_item_get_image (GIMP_ITEM (path));
          path  = GIMP_PATH (gimp_item_duplicate (GIMP_ITEM (path),
                                                  G_TYPE_FROM_INSTANCE (path)));
          gimp_image_add_path (image, path, GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
          gimp_path_tool_set_path (path_tool, NULL, path);
          gimp_path_tool_create_vector_layer (path_tool);
          break;

        case GTK_RESPONSE_ACCEPT:
          gimp_rasterizable_restore (GIMP_RASTERIZABLE (layer));
          gimp_path_tool_set_layer (path_tool, layer);
          break;

        default:
          gimp_path_tool_set_layer (path_tool, NULL);
          break;
        }
    }
}
