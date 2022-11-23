/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Vector tool
 * Copyright (C) 2003 Simon Budig  <simon@ligma.org>
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

#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmadialogconfig.h"

#include "core/ligma.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-pick-item.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaimage-undo-push.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmaundostack.h"

#include "paint/ligmapaintoptions.h" /* LIGMA_PAINT_OPTIONS_CONTEXT_MASK */

#include "vectors/ligmavectors.h"

#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadockcontainer.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmamenufactory.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmatoolpath.h"

#include "ligmatoolcontrol.h"
#include "ligmavectoroptions.h"
#include "ligmavectortool.h"

#include "dialogs/fill-dialog.h"
#include "dialogs/stroke-dialog.h"

#include "ligma-intl.h"


#define TOGGLE_MASK  ligma_get_extend_selection_mask ()
#define MOVE_MASK    GDK_MOD1_MASK
#define INSDEL_MASK  ligma_get_toggle_behavior_mask ()


/*  local function prototypes  */

static void     ligma_vector_tool_dispose         (GObject               *object);

static void     ligma_vector_tool_control         (LigmaTool              *tool,
                                                  LigmaToolAction         action,
                                                  LigmaDisplay           *display);
static void     ligma_vector_tool_button_press    (LigmaTool              *tool,
                                                  const LigmaCoords      *coords,
                                                  guint32                time,
                                                  GdkModifierType        state,
                                                  LigmaButtonPressType    press_type,
                                                  LigmaDisplay           *display);
static void     ligma_vector_tool_button_release  (LigmaTool              *tool,
                                                  const LigmaCoords      *coords,
                                                  guint32                time,
                                                  GdkModifierType        state,
                                                  LigmaButtonReleaseType  release_type,
                                                  LigmaDisplay           *display);
static void     ligma_vector_tool_motion          (LigmaTool              *tool,
                                                  const LigmaCoords      *coords,
                                                  guint32                time,
                                                  GdkModifierType        state,
                                                  LigmaDisplay           *display);
static void     ligma_vector_tool_modifier_key    (LigmaTool              *tool,
                                                  GdkModifierType        key,
                                                  gboolean               press,
                                                  GdkModifierType        state,
                                                  LigmaDisplay           *display);
static void     ligma_vector_tool_cursor_update   (LigmaTool              *tool,
                                                  const LigmaCoords      *coords,
                                                  GdkModifierType        state,
                                                  LigmaDisplay           *display);

static void     ligma_vector_tool_start           (LigmaVectorTool        *vector_tool,
                                                  LigmaDisplay           *display);
static void     ligma_vector_tool_halt            (LigmaVectorTool        *vector_tool);

static void     ligma_vector_tool_path_changed    (LigmaToolWidget        *path,
                                                  LigmaVectorTool        *vector_tool);
static void     ligma_vector_tool_path_begin_change
                                                 (LigmaToolWidget        *path,
                                                  const gchar           *desc,
                                                  LigmaVectorTool        *vector_tool);
static void     ligma_vector_tool_path_end_change (LigmaToolWidget        *path,
                                                  gboolean               success,
                                                  LigmaVectorTool        *vector_tool);
static void     ligma_vector_tool_path_activate   (LigmaToolWidget        *path,
                                                  GdkModifierType        state,
                                                  LigmaVectorTool        *vector_tool);

static void     ligma_vector_tool_vectors_changed (LigmaImage             *image,
                                                  LigmaVectorTool        *vector_tool);
static void     ligma_vector_tool_vectors_removed (LigmaVectors           *vectors,
                                                  LigmaVectorTool        *vector_tool);

static void     ligma_vector_tool_to_selection    (LigmaVectorTool        *vector_tool);
static void     ligma_vector_tool_to_selection_extended
                                                 (LigmaVectorTool        *vector_tool,
                                                  GdkModifierType        state);

static void     ligma_vector_tool_fill_vectors    (LigmaVectorTool        *vector_tool,
                                                  GtkWidget             *button);
static void     ligma_vector_tool_fill_callback   (GtkWidget             *dialog,
                                                  LigmaItem              *item,
                                                  GList                 *drawables,
                                                  LigmaContext           *context,
                                                  LigmaFillOptions       *options,
                                                  gpointer               data);

static void     ligma_vector_tool_stroke_vectors  (LigmaVectorTool        *vector_tool,
                                                  GtkWidget             *button);
static void     ligma_vector_tool_stroke_callback (GtkWidget             *dialog,
                                                  LigmaItem              *item,
                                                  GList                 *drawables,
                                                  LigmaContext           *context,
                                                  LigmaStrokeOptions     *options,
                                                  gpointer               data);


G_DEFINE_TYPE (LigmaVectorTool, ligma_vector_tool, LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_vector_tool_parent_class


void
ligma_vector_tool_register (LigmaToolRegisterCallback callback,
                           gpointer                 data)
{
  (* callback) (LIGMA_TYPE_VECTOR_TOOL,
                LIGMA_TYPE_VECTOR_OPTIONS,
                ligma_vector_options_gui,
                LIGMA_PAINT_OPTIONS_CONTEXT_MASK |
                LIGMA_CONTEXT_PROP_MASK_PATTERN  |
                LIGMA_CONTEXT_PROP_MASK_GRADIENT, /* for stroking */
                "ligma-vector-tool",
                _("Paths"),
                _("Paths Tool: Create and edit paths"),
                N_("Pat_hs"), "b",
                NULL, LIGMA_HELP_TOOL_PATH,
                LIGMA_ICON_TOOL_PATH,
                data);
}

static void
ligma_vector_tool_class_init (LigmaVectorToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  LigmaToolClass *tool_class   = LIGMA_TOOL_CLASS (klass);

  object_class->dispose      = ligma_vector_tool_dispose;

  tool_class->control        = ligma_vector_tool_control;
  tool_class->button_press   = ligma_vector_tool_button_press;
  tool_class->button_release = ligma_vector_tool_button_release;
  tool_class->motion         = ligma_vector_tool_motion;
  tool_class->modifier_key   = ligma_vector_tool_modifier_key;
  tool_class->cursor_update  = ligma_vector_tool_cursor_update;
}

static void
ligma_vector_tool_init (LigmaVectorTool *vector_tool)
{
  LigmaTool *tool = LIGMA_TOOL (vector_tool);

  ligma_tool_control_set_handle_empty_image (tool->control, TRUE);
  ligma_tool_control_set_precision          (tool->control,
                                            LIGMA_CURSOR_PRECISION_SUBPIXEL);
  ligma_tool_control_set_tool_cursor        (tool->control,
                                            LIGMA_TOOL_CURSOR_PATHS);

  vector_tool->saved_mode = LIGMA_VECTOR_MODE_DESIGN;
}

static void
ligma_vector_tool_dispose (GObject *object)
{
  LigmaVectorTool *vector_tool = LIGMA_VECTOR_TOOL (object);

  ligma_vector_tool_set_vectors (vector_tool, NULL);
  g_clear_object (&vector_tool->widget);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_vector_tool_control (LigmaTool       *tool,
                          LigmaToolAction  action,
                          LigmaDisplay    *display)
{
  LigmaVectorTool *vector_tool = LIGMA_VECTOR_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_vector_tool_halt (vector_tool);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_vector_tool_button_press (LigmaTool            *tool,
                               const LigmaCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               LigmaButtonPressType  press_type,
                               LigmaDisplay         *display)
{
  LigmaVectorTool *vector_tool = LIGMA_VECTOR_TOOL (tool);

  if (tool->display && display != tool->display)
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);

  if (! tool->display)
    {
      ligma_vector_tool_start (vector_tool, display);

      ligma_tool_widget_hover (vector_tool->widget, coords, state, TRUE);
    }

  if (ligma_tool_widget_button_press (vector_tool->widget, coords, time, state,
                                     press_type))
    {
      vector_tool->grab_widget = vector_tool->widget;
    }

  ligma_tool_control_activate (tool->control);
}

static void
ligma_vector_tool_button_release (LigmaTool              *tool,
                                 const LigmaCoords      *coords,
                                 guint32                time,
                                 GdkModifierType        state,
                                 LigmaButtonReleaseType  release_type,
                                 LigmaDisplay           *display)
{
  LigmaVectorTool *vector_tool = LIGMA_VECTOR_TOOL (tool);

  ligma_tool_control_halt (tool->control);

  if (vector_tool->grab_widget)
    {
      ligma_tool_widget_button_release (vector_tool->grab_widget,
                                       coords, time, state, release_type);
      vector_tool->grab_widget = NULL;
    }
}

static void
ligma_vector_tool_motion (LigmaTool         *tool,
                         const LigmaCoords *coords,
                         guint32           time,
                         GdkModifierType   state,
                         LigmaDisplay      *display)
{
  LigmaVectorTool *vector_tool = LIGMA_VECTOR_TOOL (tool);

  if (vector_tool->grab_widget)
    {
      ligma_tool_widget_motion (vector_tool->grab_widget, coords, time, state);
    }
}

static void
ligma_vector_tool_modifier_key (LigmaTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               LigmaDisplay     *display)
{
  LigmaVectorTool    *vector_tool = LIGMA_VECTOR_TOOL (tool);
  LigmaVectorOptions *options     = LIGMA_VECTOR_TOOL_GET_OPTIONS (tool);

  if (key == TOGGLE_MASK)
    return;

  if (key == INSDEL_MASK || key == MOVE_MASK)
    {
      LigmaVectorMode button_mode = options->edit_mode;

      if (press)
        {
          if (key == (state & (INSDEL_MASK | MOVE_MASK)))
            {
              /*  first modifier pressed  */

              vector_tool->saved_mode = options->edit_mode;
            }
        }
      else
        {
          if (! (state & (INSDEL_MASK | MOVE_MASK)))
            {
              /*  last modifier released  */

              button_mode = vector_tool->saved_mode;
            }
        }

      if (state & MOVE_MASK)
        {
          button_mode = LIGMA_VECTOR_MODE_MOVE;
        }
      else if (state & INSDEL_MASK)
        {
          button_mode = LIGMA_VECTOR_MODE_EDIT;
        }

      if (button_mode != options->edit_mode)
        {
          g_object_set (options, "vectors-edit-mode", button_mode, NULL);
        }
    }
}

static void
ligma_vector_tool_cursor_update (LigmaTool         *tool,
                                const LigmaCoords *coords,
                                GdkModifierType   state,
                                LigmaDisplay      *display)
{
  LigmaVectorTool   *vector_tool = LIGMA_VECTOR_TOOL (tool);
  LigmaDisplayShell *shell       = ligma_display_get_shell (display);

  if (display != tool->display || ! vector_tool->widget)
    {
      LigmaToolCursorType tool_cursor = LIGMA_TOOL_CURSOR_PATHS;

      if (ligma_image_pick_vectors (ligma_display_get_image (display),
                                   coords->x, coords->y,
                                   FUNSCALEX (shell,
                                              LIGMA_TOOL_HANDLE_SIZE_CIRCLE / 2),
                                   FUNSCALEY (shell,
                                              LIGMA_TOOL_HANDLE_SIZE_CIRCLE / 2)))
        {
          tool_cursor = LIGMA_TOOL_CURSOR_HAND;
        }

      ligma_tool_control_set_tool_cursor (tool->control, tool_cursor);
    }

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
ligma_vector_tool_start (LigmaVectorTool *vector_tool,
                        LigmaDisplay    *display)
{
  LigmaTool          *tool    = LIGMA_TOOL (vector_tool);
  LigmaVectorOptions *options = LIGMA_VECTOR_TOOL_GET_OPTIONS (tool);
  LigmaDisplayShell  *shell   = ligma_display_get_shell (display);
  LigmaToolWidget    *widget;

  tool->display = display;

  vector_tool->widget = widget = ligma_tool_path_new (shell);

  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (tool), widget);

  g_object_bind_property (G_OBJECT (options), "vectors-edit-mode",
                          G_OBJECT (widget),  "edit-mode",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);
  g_object_bind_property (G_OBJECT (options), "vectors-polygonal",
                          G_OBJECT (widget),  "polygonal",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  ligma_tool_path_set_vectors (LIGMA_TOOL_PATH (widget),
                              vector_tool->vectors);

  g_signal_connect (widget, "changed",
                    G_CALLBACK (ligma_vector_tool_path_changed),
                    vector_tool);
  g_signal_connect (widget, "begin-change",
                    G_CALLBACK (ligma_vector_tool_path_begin_change),
                    vector_tool);
  g_signal_connect (widget, "end-change",
                    G_CALLBACK (ligma_vector_tool_path_end_change),
                    vector_tool);
  g_signal_connect (widget, "activate",
                    G_CALLBACK (ligma_vector_tool_path_activate),
                    vector_tool);

  ligma_draw_tool_start (LIGMA_DRAW_TOOL (tool), display);
}

static void
ligma_vector_tool_halt (LigmaVectorTool *vector_tool)
{
  LigmaTool *tool = LIGMA_TOOL (vector_tool);

  if (tool->display)
    ligma_tool_pop_status (tool, tool->display);

  ligma_vector_tool_set_vectors (vector_tool, NULL);

  if (ligma_draw_tool_is_active (LIGMA_DRAW_TOOL (tool)))
    ligma_draw_tool_stop (LIGMA_DRAW_TOOL (tool));

  ligma_draw_tool_set_widget (LIGMA_DRAW_TOOL (tool), NULL);
  g_clear_object (&vector_tool->widget);

  tool->display = NULL;
}

static void
ligma_vector_tool_path_changed (LigmaToolWidget *path,
                               LigmaVectorTool *vector_tool)
{
  LigmaDisplayShell *shell = ligma_tool_widget_get_shell (path);
  LigmaImage        *image = ligma_display_get_image (shell->display);
  LigmaVectors      *vectors;

  g_object_get (path,
                "vectors", &vectors,
                NULL);

  if (vectors != vector_tool->vectors)
    {
      if (vectors && ! ligma_item_is_attached (LIGMA_ITEM (vectors)))
        {
          ligma_image_add_vectors (image, vectors,
                                  LIGMA_IMAGE_ACTIVE_PARENT, -1, TRUE);
          ligma_image_flush (image);

          ligma_vector_tool_set_vectors (vector_tool, vectors);
        }
      else
        {
          ligma_vector_tool_set_vectors (vector_tool, vectors);

          if (vectors)
            ligma_image_set_active_vectors (image, vectors);
        }
    }

  if (vectors)
    g_object_unref (vectors);
}

static void
ligma_vector_tool_path_begin_change (LigmaToolWidget *path,
                                    const gchar    *desc,
                                    LigmaVectorTool *vector_tool)
{
  LigmaDisplayShell *shell = ligma_tool_widget_get_shell (path);
  LigmaImage        *image = ligma_display_get_image (shell->display);

  ligma_image_undo_push_vectors_mod (image, desc, vector_tool->vectors);
}

static void
ligma_vector_tool_path_end_change (LigmaToolWidget *path,
                                  gboolean        success,
                                  LigmaVectorTool *vector_tool)
{
  LigmaDisplayShell *shell = ligma_tool_widget_get_shell (path);
  LigmaImage        *image = ligma_display_get_image (shell->display);

  if (! success)
    {
      LigmaUndo            *undo;
      LigmaUndoAccumulator  accum = { 0, };

      undo = ligma_undo_stack_pop_undo (ligma_image_get_undo_stack (image),
                                       LIGMA_UNDO_MODE_UNDO, &accum);

      ligma_image_undo_event (image, LIGMA_UNDO_EVENT_UNDO_EXPIRED, undo);

      ligma_undo_free (undo, LIGMA_UNDO_MODE_UNDO);
      g_object_unref (undo);
    }

  ligma_image_flush (image);
}

static void
ligma_vector_tool_path_activate (LigmaToolWidget  *path,
                                GdkModifierType  state,
                                LigmaVectorTool  *vector_tool)
{
  ligma_vector_tool_to_selection_extended (vector_tool, state);
}

static void
ligma_vector_tool_vectors_changed (LigmaImage      *image,
                                  LigmaVectorTool *vector_tool)
{
  LigmaVectors *vectors = NULL;

  /* The vectors tool can only work on one path at a time. */
  if (g_list_length (ligma_image_get_selected_vectors (image)) == 1)
    vectors = ligma_image_get_selected_vectors (image)->data;

  ligma_vector_tool_set_vectors (vector_tool, vectors);
}

static void
ligma_vector_tool_vectors_removed (LigmaVectors    *vectors,
                                  LigmaVectorTool *vector_tool)
{
  ligma_vector_tool_set_vectors (vector_tool, NULL);
}

void
ligma_vector_tool_set_vectors (LigmaVectorTool *vector_tool,
                              LigmaVectors    *vectors)
{
  LigmaTool          *tool;
  LigmaItem          *item = NULL;
  LigmaVectorOptions *options;

  g_return_if_fail (LIGMA_IS_VECTOR_TOOL (vector_tool));
  g_return_if_fail (vectors == NULL || LIGMA_IS_VECTORS (vectors));

  tool    = LIGMA_TOOL (vector_tool);
  options = LIGMA_VECTOR_TOOL_GET_OPTIONS (vector_tool);

  if (vectors)
    item = LIGMA_ITEM (vectors);

  if (vectors == vector_tool->vectors)
    return;

  if (vector_tool->vectors)
    {
      LigmaImage *old_image;

      old_image = ligma_item_get_image (LIGMA_ITEM (vector_tool->vectors));

      g_signal_handlers_disconnect_by_func (old_image,
                                            ligma_vector_tool_vectors_changed,
                                            vector_tool);
      g_signal_handlers_disconnect_by_func (vector_tool->vectors,
                                            ligma_vector_tool_vectors_removed,
                                            vector_tool);

      g_clear_object (&vector_tool->vectors);

      if (options->to_selection_button)
        {
          gtk_widget_set_sensitive (options->to_selection_button, FALSE);
          g_signal_handlers_disconnect_by_func (options->to_selection_button,
                                                ligma_vector_tool_to_selection,
                                                tool);
          g_signal_handlers_disconnect_by_func (options->to_selection_button,
                                                ligma_vector_tool_to_selection_extended,
                                                tool);
        }

      if (options->fill_button)
        {
          gtk_widget_set_sensitive (options->fill_button, FALSE);
          g_signal_handlers_disconnect_by_func (options->fill_button,
                                                ligma_vector_tool_fill_vectors,
                                                tool);
        }

      if (options->stroke_button)
        {
          gtk_widget_set_sensitive (options->stroke_button, FALSE);
          g_signal_handlers_disconnect_by_func (options->stroke_button,
                                                ligma_vector_tool_stroke_vectors,
                                                tool);
        }
    }

  if (! vectors ||
      (tool->display &&
       ligma_display_get_image (tool->display) != ligma_item_get_image (item)))
    {
      ligma_vector_tool_halt (vector_tool);
    }

  if (! vectors)
    return;

  vector_tool->vectors = g_object_ref (vectors);

  g_signal_connect_object (ligma_item_get_image (item), "selected-vectors-changed",
                           G_CALLBACK (ligma_vector_tool_vectors_changed),
                           vector_tool, 0);
  g_signal_connect_object (vectors, "removed",
                           G_CALLBACK (ligma_vector_tool_vectors_removed),
                           vector_tool, 0);

  if (options->to_selection_button)
    {
      g_signal_connect_swapped (options->to_selection_button, "clicked",
                                G_CALLBACK (ligma_vector_tool_to_selection),
                                tool);
      g_signal_connect_swapped (options->to_selection_button, "extended-clicked",
                                G_CALLBACK (ligma_vector_tool_to_selection_extended),
                                tool);
      gtk_widget_set_sensitive (options->to_selection_button, TRUE);
    }

  if (options->fill_button)
    {
      g_signal_connect_swapped (options->fill_button, "clicked",
                                G_CALLBACK (ligma_vector_tool_fill_vectors),
                                tool);
      gtk_widget_set_sensitive (options->fill_button, TRUE);
    }

  if (options->stroke_button)
    {
      g_signal_connect_swapped (options->stroke_button, "clicked",
                                G_CALLBACK (ligma_vector_tool_stroke_vectors),
                                tool);
      gtk_widget_set_sensitive (options->stroke_button, TRUE);
    }

  if (tool->display)
    {
      ligma_tool_path_set_vectors (LIGMA_TOOL_PATH (vector_tool->widget), vectors);
    }
  else
    {
      LigmaContext *context = ligma_get_user_context (tool->tool_info->ligma);
      LigmaDisplay *display = ligma_context_get_display (context);

      if (! display ||
          ligma_display_get_image (display) != ligma_item_get_image (item))
        {
          GList *list;

          display = NULL;

          for (list = ligma_get_display_iter (ligma_item_get_image (item)->ligma);
               list;
               list = g_list_next (list))
            {
              display = list->data;

              if (ligma_display_get_image (display) == ligma_item_get_image (item))
                {
                  ligma_context_set_display (context, display);
                  break;
                }

              display = NULL;
            }
        }

      if (display)
        ligma_vector_tool_start (vector_tool, display);
    }

  if (options->edit_mode != LIGMA_VECTOR_MODE_DESIGN)
    g_object_set (options, "vectors-edit-mode",
                  LIGMA_VECTOR_MODE_DESIGN, NULL);
}

static void
ligma_vector_tool_to_selection (LigmaVectorTool *vector_tool)
{
  ligma_vector_tool_to_selection_extended (vector_tool, 0);
}

static void
ligma_vector_tool_to_selection_extended (LigmaVectorTool  *vector_tool,
                                        GdkModifierType  state)
{
  LigmaImage *image;

  if (! vector_tool->vectors)
    return;

  image = ligma_item_get_image (LIGMA_ITEM (vector_tool->vectors));

  ligma_item_to_selection (LIGMA_ITEM (vector_tool->vectors),
                          ligma_modifiers_to_channel_op (state),
                          TRUE, FALSE, 0, 0);
  ligma_image_flush (image);
}


static void
ligma_vector_tool_fill_vectors (LigmaVectorTool *vector_tool,
                               GtkWidget      *button)
{
  LigmaDialogConfig *config;
  LigmaImage        *image;
  GList            *drawables;
  GtkWidget        *dialog;

  if (! vector_tool->vectors)
    return;

  image = ligma_item_get_image (LIGMA_ITEM (vector_tool->vectors));

  config = LIGMA_DIALOG_CONFIG (image->ligma->config);

  drawables = ligma_image_get_selected_drawables (image);

  if (! drawables)
    {
      ligma_tool_message (LIGMA_TOOL (vector_tool),
                         LIGMA_TOOL (vector_tool)->display,
                         _("There are no selected layers or channels to fill."));
      return;
    }

  dialog = fill_dialog_new (LIGMA_ITEM (vector_tool->vectors),
                            drawables,
                            LIGMA_CONTEXT (LIGMA_TOOL_GET_OPTIONS (vector_tool)),
                            _("Fill Path"),
                            LIGMA_ICON_TOOL_BUCKET_FILL,
                            LIGMA_HELP_PATH_FILL,
                            button,
                            config->fill_options,
                            ligma_vector_tool_fill_callback,
                            vector_tool);
  gtk_widget_show (dialog);

  g_list_free (drawables);
}

static void
ligma_vector_tool_fill_callback (GtkWidget       *dialog,
                                LigmaItem        *item,
                                GList           *drawables,
                                LigmaContext     *context,
                                LigmaFillOptions *options,
                                gpointer         data)
{
  LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (context->ligma->config);
  LigmaImage        *image  = ligma_item_get_image (item);
  GError           *error  = NULL;

  ligma_config_sync (G_OBJECT (options),
                    G_OBJECT (config->fill_options), 0);

  if (! ligma_item_fill (item, drawables, options,
                        TRUE, NULL, &error))
    {
      ligma_message_literal (context->ligma,
                            G_OBJECT (dialog),
                            LIGMA_MESSAGE_WARNING,
                            error ? error->message : "NULL");

      g_clear_error (&error);
      return;
    }

  ligma_image_flush (image);

  gtk_widget_destroy (dialog);
}


static void
ligma_vector_tool_stroke_vectors (LigmaVectorTool *vector_tool,
                                 GtkWidget      *button)
{
  LigmaDialogConfig *config;
  LigmaImage        *image;
  GList            *drawables;
  GtkWidget        *dialog;

  if (! vector_tool->vectors)
    return;

  image = ligma_item_get_image (LIGMA_ITEM (vector_tool->vectors));

  config = LIGMA_DIALOG_CONFIG (image->ligma->config);

  drawables = ligma_image_get_selected_drawables (image);

  if (! drawables)
    {
      ligma_tool_message (LIGMA_TOOL (vector_tool),
                         LIGMA_TOOL (vector_tool)->display,
                         _("There are no selected layers or channels to stroke to."));
      return;
    }

  dialog = stroke_dialog_new (LIGMA_ITEM (vector_tool->vectors),
                              drawables,
                              LIGMA_CONTEXT (LIGMA_TOOL_GET_OPTIONS (vector_tool)),
                              _("Stroke Path"),
                              LIGMA_ICON_PATH_STROKE,
                              LIGMA_HELP_PATH_STROKE,
                              button,
                              config->stroke_options,
                              ligma_vector_tool_stroke_callback,
                              vector_tool);
  gtk_widget_show (dialog);
  g_list_free (drawables);
}

static void
ligma_vector_tool_stroke_callback (GtkWidget         *dialog,
                                  LigmaItem          *item,
                                  GList             *drawables,
                                  LigmaContext       *context,
                                  LigmaStrokeOptions *options,
                                  gpointer           data)
{
  LigmaDialogConfig *config = LIGMA_DIALOG_CONFIG (context->ligma->config);
  LigmaImage        *image  = ligma_item_get_image (item);
  GError           *error  = NULL;

  ligma_config_sync (G_OBJECT (options),
                    G_OBJECT (config->stroke_options), 0);

  if (! ligma_item_stroke (item, drawables, context, options, NULL,
                          TRUE, NULL, &error))
    {
      ligma_message_literal (context->ligma,
                            G_OBJECT (dialog),
                            LIGMA_MESSAGE_WARNING,
                            error ? error->message : "NULL");

      g_clear_error (&error);
      return;
    }

  ligma_image_flush (image);

  gtk_widget_destroy (dialog);
}
