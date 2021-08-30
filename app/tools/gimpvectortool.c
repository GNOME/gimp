/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Vector tool
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

#include "vectors/gimpvectorlayer.h"
#include "vectors/gimpvectors.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockcontainer.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimptoolpath.h"

#include "gimptoolcontrol.h"
#include "gimpvectoroptions.h"
#include "gimpvectortool.h"

#include "dialogs/fill-dialog.h"
#include "dialogs/stroke-dialog.h"

#include "gimp-intl.h"


#define TOGGLE_MASK  gimp_get_extend_selection_mask ()
#define MOVE_MASK    GDK_MOD1_MASK
#define INSDEL_MASK  gimp_get_toggle_behavior_mask ()


/*  local function prototypes  */

static void     gimp_vector_tool_dispose         (GObject               *object);

static void     gimp_vector_tool_control         (GimpTool              *tool,
                                                  GimpToolAction         action,
                                                  GimpDisplay           *display);
static void     gimp_vector_tool_button_press    (GimpTool              *tool,
                                                  const GimpCoords      *coords,
                                                  guint32                time,
                                                  GdkModifierType        state,
                                                  GimpButtonPressType    press_type,
                                                  GimpDisplay           *display);
static void     gimp_vector_tool_button_release  (GimpTool              *tool,
                                                  const GimpCoords      *coords,
                                                  guint32                time,
                                                  GdkModifierType        state,
                                                  GimpButtonReleaseType  release_type,
                                                  GimpDisplay           *display);
static void     gimp_vector_tool_motion          (GimpTool              *tool,
                                                  const GimpCoords      *coords,
                                                  guint32                time,
                                                  GdkModifierType        state,
                                                  GimpDisplay           *display);
static void     gimp_vector_tool_modifier_key    (GimpTool              *tool,
                                                  GdkModifierType        key,
                                                  gboolean               press,
                                                  GdkModifierType        state,
                                                  GimpDisplay           *display);
static void     gimp_vector_tool_cursor_update   (GimpTool              *tool,
                                                  const GimpCoords      *coords,
                                                  GdkModifierType        state,
                                                  GimpDisplay           *display);

static void     gimp_vector_tool_start           (GimpVectorTool        *vector_tool,
                                                  GimpDisplay           *display);
static void     gimp_vector_tool_halt            (GimpVectorTool        *vector_tool);

static void     gimp_vector_tool_path_changed    (GimpToolWidget        *path,
                                                  GimpVectorTool        *vector_tool);
static void     gimp_vector_tool_path_begin_change
                                                 (GimpToolWidget        *path,
                                                  const gchar           *desc,
                                                  GimpVectorTool        *vector_tool);
static void     gimp_vector_tool_path_end_change (GimpToolWidget        *path,
                                                  gboolean               success,
                                                  GimpVectorTool        *vector_tool);
static void     gimp_vector_tool_path_activate   (GimpToolWidget        *path,
                                                  GdkModifierType        state,
                                                  GimpVectorTool        *vector_tool);

static void     gimp_vector_tool_vectors_changed (GimpImage             *image,
                                                  GimpVectorTool        *vector_tool);
static void     gimp_vector_tool_vectors_removed (GimpVectors           *vectors,
                                                  GimpVectorTool        *vector_tool);

static void     gimp_vector_tool_to_selection    (GimpVectorTool        *vector_tool);
static void     gimp_vector_tool_to_selection_extended
                                                 (GimpVectorTool        *vector_tool,
                                                  GdkModifierType        state);

static void     gimp_vector_tool_fill_vectors    (GimpVectorTool        *vector_tool,
                                                  GtkWidget             *button);
static void     gimp_vector_tool_fill_callback   (GtkWidget             *dialog,
                                                  GimpItem              *item,
                                                  GList                 *drawables,
                                                  GimpContext           *context,
                                                  GimpFillOptions       *options,
                                                  gpointer               data);

static void     gimp_vector_tool_stroke_vectors  (GimpVectorTool        *vector_tool,
                                                  GtkWidget             *button);
static void     gimp_vector_tool_stroke_callback (GtkWidget             *dialog,
                                                  GimpItem              *item,
                                                  GList                 *drawables,
                                                  GimpContext           *context,
                                                  GimpStrokeOptions     *options,
                                                  gpointer               data);

static void gimp_vector_tool_create_vector_layer (GimpVectorTool        *vector_tool,
                                                  GtkWidget             *button);

G_DEFINE_TYPE (GimpVectorTool, gimp_vector_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_vector_tool_parent_class


void
gimp_vector_tool_register (GimpToolRegisterCallback callback,
                           gpointer                 data)
{
  (* callback) (GIMP_TYPE_VECTOR_TOOL,
                GIMP_TYPE_VECTOR_OPTIONS,
                gimp_vector_options_gui,
                GIMP_PAINT_OPTIONS_CONTEXT_MASK |
                GIMP_CONTEXT_PROP_MASK_PATTERN  |
                GIMP_CONTEXT_PROP_MASK_GRADIENT, /* for stroking */
                "gimp-vector-tool",
                _("Paths"),
                _("Paths Tool: Create and edit paths"),
                N_("Pat_hs"), "b",
                NULL, GIMP_HELP_TOOL_PATH,
                GIMP_ICON_TOOL_PATH,
                data);
}

static void
gimp_vector_tool_class_init (GimpVectorToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass *tool_class   = GIMP_TOOL_CLASS (klass);

  object_class->dispose      = gimp_vector_tool_dispose;

  tool_class->control        = gimp_vector_tool_control;
  tool_class->button_press   = gimp_vector_tool_button_press;
  tool_class->button_release = gimp_vector_tool_button_release;
  tool_class->motion         = gimp_vector_tool_motion;
  tool_class->modifier_key   = gimp_vector_tool_modifier_key;
  tool_class->cursor_update  = gimp_vector_tool_cursor_update;
}

static void
gimp_vector_tool_init (GimpVectorTool *vector_tool)
{
  GimpTool *tool = GIMP_TOOL (vector_tool);

  gimp_tool_control_set_handle_empty_image (tool->control, TRUE);
  gimp_tool_control_set_precision          (tool->control,
                                            GIMP_CURSOR_PRECISION_SUBPIXEL);
  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_PATHS);

  vector_tool->saved_mode = GIMP_VECTOR_MODE_DESIGN;
}

static void
gimp_vector_tool_dispose (GObject *object)
{
  GimpVectorTool *vector_tool = GIMP_VECTOR_TOOL (object);

  gimp_vector_tool_set_vectors (vector_tool, NULL);
  g_clear_object (&vector_tool->widget);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_vector_tool_control (GimpTool       *tool,
                          GimpToolAction  action,
                          GimpDisplay    *display)
{
  GimpVectorTool *vector_tool = GIMP_VECTOR_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_vector_tool_halt (vector_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_vector_tool_button_press (GimpTool            *tool,
                               const GimpCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               GimpButtonPressType  press_type,
                               GimpDisplay         *display)
{
  GimpVectorTool *vector_tool = GIMP_VECTOR_TOOL (tool);

  if (tool->display && display != tool->display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);

  if (! tool->display)
    {
      gimp_vector_tool_start (vector_tool, display);

      gimp_tool_widget_hover (vector_tool->widget, coords, state, TRUE);
    }

  if (gimp_tool_widget_button_press (vector_tool->widget, coords, time, state,
                                     press_type))
    {
      vector_tool->grab_widget = vector_tool->widget;
    }

  gimp_tool_control_activate (tool->control);
}

static void
gimp_vector_tool_button_release (GimpTool              *tool,
                                 const GimpCoords      *coords,
                                 guint32                time,
                                 GdkModifierType        state,
                                 GimpButtonReleaseType  release_type,
                                 GimpDisplay           *display)
{
  GimpVectorTool *vector_tool = GIMP_VECTOR_TOOL (tool);

  gimp_tool_control_halt (tool->control);

  if (vector_tool->grab_widget)
    {
      gimp_tool_widget_button_release (vector_tool->grab_widget,
                                       coords, time, state, release_type);
      vector_tool->grab_widget = NULL;
    }
}

static void
gimp_vector_tool_motion (GimpTool         *tool,
                         const GimpCoords *coords,
                         guint32           time,
                         GdkModifierType   state,
                         GimpDisplay      *display)
{
  GimpVectorTool *vector_tool = GIMP_VECTOR_TOOL (tool);

  if (vector_tool->grab_widget)
    {
      gimp_tool_widget_motion (vector_tool->grab_widget, coords, time, state);
    }
}

static void
gimp_vector_tool_modifier_key (GimpTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               GimpDisplay     *display)
{
  GimpVectorTool    *vector_tool = GIMP_VECTOR_TOOL (tool);
  GimpVectorOptions *options     = GIMP_VECTOR_TOOL_GET_OPTIONS (tool);

  if (key == TOGGLE_MASK)
    return;

  if (key == INSDEL_MASK || key == MOVE_MASK)
    {
      GimpVectorMode button_mode = options->edit_mode;

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
          button_mode = GIMP_VECTOR_MODE_MOVE;
        }
      else if (state & INSDEL_MASK)
        {
          button_mode = GIMP_VECTOR_MODE_EDIT;
        }

      if (button_mode != options->edit_mode)
        {
          g_object_set (options, "vectors-edit-mode", button_mode, NULL);
        }
    }
}

static void
gimp_vector_tool_cursor_update (GimpTool         *tool,
                                const GimpCoords *coords,
                                GdkModifierType   state,
                                GimpDisplay      *display)
{
  GimpVectorTool   *vector_tool = GIMP_VECTOR_TOOL (tool);
  GimpDisplayShell *shell       = gimp_display_get_shell (display);

  if (display != tool->display || ! vector_tool->widget)
    {
      GimpToolCursorType tool_cursor = GIMP_TOOL_CURSOR_PATHS;

      if (gimp_image_pick_vectors (gimp_display_get_image (display),
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
gimp_vector_tool_start (GimpVectorTool *vector_tool,
                        GimpDisplay    *display)
{
  GimpTool          *tool    = GIMP_TOOL (vector_tool);
  GimpVectorOptions *options = GIMP_VECTOR_TOOL_GET_OPTIONS (tool);
  GimpDisplayShell  *shell   = gimp_display_get_shell (display);
  GimpToolWidget    *widget;

  tool->display = display;

  vector_tool->widget = widget = gimp_tool_path_new (shell);

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), widget);

  g_object_bind_property (G_OBJECT (options), "vectors-edit-mode",
                          G_OBJECT (widget),  "edit-mode",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);
  g_object_bind_property (G_OBJECT (options), "vectors-polygonal",
                          G_OBJECT (widget),  "polygonal",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  gimp_tool_path_set_vectors (GIMP_TOOL_PATH (widget),
                              vector_tool->vectors);

  g_signal_connect (widget, "changed",
                    G_CALLBACK (gimp_vector_tool_path_changed),
                    vector_tool);
  g_signal_connect (widget, "begin-change",
                    G_CALLBACK (gimp_vector_tool_path_begin_change),
                    vector_tool);
  g_signal_connect (widget, "end-change",
                    G_CALLBACK (gimp_vector_tool_path_end_change),
                    vector_tool);
  g_signal_connect (widget, "activate",
                    G_CALLBACK (gimp_vector_tool_path_activate),
                    vector_tool);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}

static void
gimp_vector_tool_halt (GimpVectorTool *vector_tool)
{
  GimpTool *tool = GIMP_TOOL (vector_tool);

  if (tool->display)
    gimp_tool_pop_status (tool, tool->display);

  gimp_vector_tool_set_vectors (vector_tool, NULL);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_draw_tool_set_widget (GIMP_DRAW_TOOL (tool), NULL);
  g_clear_object (&vector_tool->widget);

  tool->display = NULL;
}

static void
gimp_vector_tool_path_changed (GimpToolWidget *path,
                               GimpVectorTool *vector_tool)
{
  GimpDisplayShell *shell = gimp_tool_widget_get_shell (path);
  GimpImage        *image = gimp_display_get_image (shell->display);
  GimpVectors      *vectors;

  g_object_get (path,
                "vectors", &vectors,
                NULL);

  if (vectors != vector_tool->vectors)
    {
      if (vectors && ! gimp_item_is_attached (GIMP_ITEM (vectors)))
        {
          gimp_image_add_vectors (image, vectors,
                                  GIMP_IMAGE_ACTIVE_PARENT, -1, TRUE);
          gimp_image_flush (image);

          gimp_vector_tool_set_vectors (vector_tool, vectors);
        }
      else
        {
          gimp_vector_tool_set_vectors (vector_tool, vectors);

          if (vectors)
            gimp_image_set_active_vectors (image, vectors);
        }
    }

  if (vectors)
    g_object_unref (vectors);
}

static void
gimp_vector_tool_path_begin_change (GimpToolWidget *path,
                                    const gchar    *desc,
                                    GimpVectorTool *vector_tool)
{
  GimpDisplayShell *shell = gimp_tool_widget_get_shell (path);
  GimpImage        *image = gimp_display_get_image (shell->display);

  gimp_image_undo_push_vectors_mod (image, desc, vector_tool->vectors);
}

static void
gimp_vector_tool_path_end_change (GimpToolWidget *path,
                                  gboolean        success,
                                  GimpVectorTool *vector_tool)
{
  GimpDisplayShell *shell = gimp_tool_widget_get_shell (path);
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
gimp_vector_tool_path_activate (GimpToolWidget  *path,
                                GdkModifierType  state,
                                GimpVectorTool  *vector_tool)
{
  gimp_vector_tool_to_selection_extended (vector_tool, state);
}

static void
gimp_vector_tool_vectors_changed (GimpImage      *image,
                                  GimpVectorTool *vector_tool)
{
  gimp_vector_tool_set_vectors (vector_tool,
                                gimp_image_get_active_vectors (image));
}

static void
gimp_vector_tool_vectors_removed (GimpVectors    *vectors,
                                  GimpVectorTool *vector_tool)
{
  gimp_vector_tool_set_vectors (vector_tool, NULL);
}

void
gimp_vector_tool_set_vectors (GimpVectorTool *vector_tool,
                              GimpVectors    *vectors)
{
  GimpTool          *tool;
  GimpItem          *item = NULL;
  GimpVectorOptions *options;

  g_return_if_fail (GIMP_IS_VECTOR_TOOL (vector_tool));
  g_return_if_fail (vectors == NULL || GIMP_IS_VECTORS (vectors));

  tool    = GIMP_TOOL (vector_tool);
  options = GIMP_VECTOR_TOOL_GET_OPTIONS (vector_tool);

  if (vectors)
    item = GIMP_ITEM (vectors);

  if (vectors == vector_tool->vectors)
    return;

#if 0
  if (!vector_tool->modifier_lock)
    {
      if (state & TOGGLE_MASK)
        {
          vector_tool->restriction = GIMP_ANCHOR_FEATURE_SYMMETRIC;
        }
      else
        {
          vector_tool->restriction = GIMP_ANCHOR_FEATURE_NONE;
        }
    }

  switch (vector_tool->function)
    {
    case VECTORS_MOVE_ANCHOR:
    case VECTORS_MOVE_HANDLE:
      anchor = vector_tool->cur_anchor;

      if (anchor)
        {
          gimp_stroke_anchor_move_absolute (vector_tool->cur_stroke,
                                            vector_tool->cur_anchor,
                                            &position,
                                            vector_tool->restriction);
          vector_tool->undo_motion = TRUE;
        }
      break;

    case VECTORS_MOVE_CURVE:
      if (options->polygonal)
        {
          gimp_vector_tool_move_selected_anchors (vector_tool,
                                                  coords->x - vector_tool->last_x,
                                                  coords->y - vector_tool->last_y);
          vector_tool->undo_motion = TRUE;
        }
      else
        {
          gimp_stroke_point_move_absolute (vector_tool->cur_stroke,
                                           vector_tool->cur_anchor,
                                           vector_tool->cur_position,
                                           &position,
                                           vector_tool->restriction);
          vector_tool->undo_motion = TRUE;
        }
      break;

    case VECTORS_MOVE_ANCHORSET:
      gimp_vector_tool_move_selected_anchors (vector_tool,
                                              coords->x - vector_tool->last_x,
                                              coords->y - vector_tool->last_y);
      vector_tool->undo_motion = TRUE;
      break;

    case VECTORS_MOVE_STROKE:
      if (vector_tool->cur_stroke)
        {
          gimp_stroke_translate (vector_tool->cur_stroke,
                                 coords->x - vector_tool->last_x,
                                 coords->y - vector_tool->last_y);
          vector_tool->undo_motion = TRUE;
        }
      else if (vector_tool->sel_stroke)
        {
          gimp_stroke_translate (vector_tool->sel_stroke,
                                 coords->x - vector_tool->last_x,
                                 coords->y - vector_tool->last_y);
          vector_tool->undo_motion = TRUE;
        }
      break;

    case VECTORS_MOVE_VECTORS:
      gimp_item_translate (GIMP_ITEM (vector_tool->vectors),
                           coords->x - vector_tool->last_x,
                           coords->y - vector_tool->last_y, FALSE);
      vector_tool->undo_motion = TRUE;
      break;

    default:
      break;
    }

  vector_tool->last_x = coords->x;
  vector_tool->last_y = coords->y;

  gimp_vectors_thaw (vector_tool->vectors);

  image = gimp_item_get_image (GIMP_ITEM (vector_tool->vectors));
  gimp_image_flush (image);
}

static gboolean
gimp_vector_tool_key_press (GimpTool     *tool,
                            GdkEventKey  *kevent,
                            GimpDisplay  *display)
{
  GimpVectorTool    *vector_tool = GIMP_VECTOR_TOOL (tool);
  GimpDrawTool      *draw_tool   = GIMP_DRAW_TOOL (tool);
  GimpVectorOptions *options     = GIMP_VECTOR_TOOL_GET_OPTIONS (tool);
  GimpDisplayShell  *shell;
  gdouble            xdist, ydist;
  gdouble            pixels = 1.0;

  if (! vector_tool->vectors)
    return FALSE;

  if (display != draw_tool->display)
    return FALSE;

  shell = GIMP_DISPLAY_SHELL (draw_tool->display->shell);

  if (kevent->state & GDK_SHIFT_MASK)
    pixels = 10.0;

  if (kevent->state & GDK_CONTROL_MASK)
    pixels = 50.0;

  switch (kevent->keyval)
    {
    case GDK_Return:
    case GDK_KP_Enter:
    case GDK_ISO_Enter:
      gimp_vector_tool_to_selection_extended (vector_tool, kevent->state);
      break;

    case GDK_BackSpace:
    case GDK_Delete:
      gimp_vector_tool_delete_selected_anchors (vector_tool);
      break;

    case GDK_Left:
    case GDK_Right:
    case GDK_Up:
    case GDK_Down:
      xdist = FUNSCALEX (shell, pixels);
      ydist = FUNSCALEY (shell, pixels);

      gimp_vector_tool_undo_push (vector_tool, _("Move Anchors"));

      gimp_vectors_freeze (vector_tool->vectors);

      switch (kevent->keyval)
        {
        case GDK_Left:
          gimp_vector_tool_move_selected_anchors (vector_tool, -xdist, 0);
          break;

        case GDK_Right:
          gimp_vector_tool_move_selected_anchors (vector_tool, xdist, 0);
          break;

        case GDK_Up:
          gimp_vector_tool_move_selected_anchors (vector_tool, 0, -ydist);
          break;

        case GDK_Down:
          gimp_vector_tool_move_selected_anchors (vector_tool, 0, ydist);
          break;

        default:
          break;
        }

      gimp_vectors_thaw (vector_tool->vectors);
      vector_tool->have_undo = FALSE;
      break;

    case GDK_Escape:
      if (options->edit_mode != GIMP_VECTOR_MODE_DESIGN)
        g_object_set (options, "vectors-edit-mode",
                      GIMP_VECTOR_MODE_DESIGN, NULL);
      break;

    default:
      return FALSE;
    }

  gimp_image_flush (display->image);

  return TRUE;
}

static void
gimp_vector_tool_modifier_key (GimpTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               GimpDisplay     *display)
{
  GimpVectorTool    *vector_tool = GIMP_VECTOR_TOOL (tool);
  GimpVectorOptions *options     = GIMP_VECTOR_TOOL_GET_OPTIONS (tool);

  if (key == TOGGLE_MASK)
    return;

  if (key == INSDEL_MASK || key == MOVE_MASK)
    {
      GimpVectorMode button_mode = options->edit_mode;

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
          button_mode = GIMP_VECTOR_MODE_MOVE;
        }
      else if (state & INSDEL_MASK)
        {
          button_mode = GIMP_VECTOR_MODE_EDIT;
        }

      if (button_mode != options->edit_mode)
        {
          g_object_set (options, "vectors-edit-mode", button_mode, NULL);
        }
    }
}

static void
gimp_vector_tool_oper_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity,
                              GimpDisplay      *display)
{
  GimpVectorTool    *vector_tool = GIMP_VECTOR_TOOL (tool);
  GimpDrawTool      *draw_tool   = GIMP_DRAW_TOOL (tool);
  GimpVectorOptions *options     = GIMP_VECTOR_TOOL_GET_OPTIONS (tool);
  GimpAnchor        *anchor      = NULL;
  GimpAnchor        *anchor2     = NULL;
  GimpStroke        *stroke      = NULL;
  gdouble            position    = -1;
  gboolean           on_handle   = FALSE;
  gboolean           on_curve    = FALSE;
  gboolean           on_vectors  = FALSE;

  vector_tool->modifier_lock = FALSE;

  /* are we hovering the current vectors on the current display? */
  if (vector_tool->vectors && GIMP_DRAW_TOOL (tool)->display == display)
    {
      on_handle = gimp_draw_tool_on_vectors_handle (GIMP_DRAW_TOOL (tool),
                                                    display,
                                                    vector_tool->vectors,
                                                    coords,
                                                    TARGET, TARGET,
                                                    GIMP_ANCHOR_ANCHOR,
                                                    vector_tool->sel_count > 2,
                                                    &anchor, &stroke);

      if (! on_handle)
        on_curve = gimp_draw_tool_on_vectors_curve (GIMP_DRAW_TOOL (tool),
                                                    display,
                                                    vector_tool->vectors,
                                                    coords,
                                                    TARGET, TARGET,
                                                    NULL,
                                                    &position, &anchor,
                                                    &anchor2, &stroke);
    }

  if (on_handle || on_curve)
    {
      vector_tool->cur_vectors = NULL;
    }
  else
    {
      on_vectors = gimp_draw_tool_on_vectors (draw_tool, display, coords,
                                              TARGET, TARGET,
                                              NULL, NULL, NULL, NULL, NULL,
                                              &(vector_tool->cur_vectors));
    }

  vector_tool->cur_position   = position;
  vector_tool->cur_anchor     = anchor;
  vector_tool->cur_anchor2    = anchor2;
  vector_tool->cur_stroke     = stroke;

  switch (options->edit_mode)
    {
    case GIMP_VECTOR_MODE_DESIGN:
      if (! vector_tool->vectors || GIMP_DRAW_TOOL (tool)->display != display)
        {
          if (on_vectors)
            {
              vector_tool->function = VECTORS_SELECT_VECTOR;
            }
          else
            {
              vector_tool->function = VECTORS_CREATE_VECTOR;
              vector_tool->restriction = GIMP_ANCHOR_FEATURE_SYMMETRIC;
              vector_tool->modifier_lock = TRUE;
            }
        }
      else if (on_handle)
        {
          if (anchor->type == GIMP_ANCHOR_ANCHOR)
            {
              if (state & TOGGLE_MASK)
                {
                  vector_tool->function = VECTORS_MOVE_ANCHORSET;
                }
              else
                {
                  if (vector_tool->sel_count >= 2 && anchor->selected)
                    vector_tool->function = VECTORS_MOVE_ANCHORSET;
                  else
                    vector_tool->function = VECTORS_MOVE_ANCHOR;
                }
            }
          else
            {
              vector_tool->function = VECTORS_MOVE_HANDLE;

              if (state & TOGGLE_MASK)
                vector_tool->restriction = GIMP_ANCHOR_FEATURE_SYMMETRIC;
              else
                vector_tool->restriction = GIMP_ANCHOR_FEATURE_NONE;
            }
        }
      else if (on_curve)
        {
          if (gimp_stroke_point_is_movable (stroke, anchor, position))
            {
              vector_tool->function = VECTORS_MOVE_CURVE;

              if (state & TOGGLE_MASK)
                vector_tool->restriction = GIMP_ANCHOR_FEATURE_SYMMETRIC;
              else
                vector_tool->restriction = GIMP_ANCHOR_FEATURE_NONE;
            }
          else
            {
              vector_tool->function = VECTORS_FINISHED;
            }
        }
      else
        {
          if (vector_tool->sel_stroke && vector_tool->sel_anchor &&
              gimp_stroke_is_extendable (vector_tool->sel_stroke,
                                         vector_tool->sel_anchor) &&
              !(state & TOGGLE_MASK))
            vector_tool->function = VECTORS_ADD_ANCHOR;
          else
            vector_tool->function = VECTORS_CREATE_STROKE;

          vector_tool->restriction = GIMP_ANCHOR_FEATURE_SYMMETRIC;
          vector_tool->modifier_lock = TRUE;
        }

      break;

    case GIMP_VECTOR_MODE_EDIT:
      if (! vector_tool->vectors || GIMP_DRAW_TOOL (tool)->display != display)
        {
          if (on_vectors)
            {
              vector_tool->function = VECTORS_SELECT_VECTOR;
            }
          else
            {
              vector_tool->function = VECTORS_FINISHED;
            }
        }
      else if (on_handle)
        {
          if (anchor->type == GIMP_ANCHOR_ANCHOR)
            {
              if (!(state & TOGGLE_MASK) && vector_tool->sel_anchor &&
                  vector_tool->sel_anchor != anchor &&
                  gimp_stroke_is_extendable (vector_tool->sel_stroke,
                                             vector_tool->sel_anchor) &&
                  gimp_stroke_is_extendable (stroke, anchor))
                {
                  vector_tool->function = VECTORS_CONNECT_STROKES;
                }
              else
                {
                  if (state & TOGGLE_MASK)
                    {
                      vector_tool->function = VECTORS_DELETE_ANCHOR;
                    }
                  else
                    {
                      if (options->polygonal)
                        vector_tool->function = VECTORS_MOVE_ANCHOR;
                      else
                        vector_tool->function = VECTORS_MOVE_HANDLE;
                    }
                }
            }
          else
            {
              if (state & TOGGLE_MASK)
                vector_tool->function = VECTORS_CONVERT_EDGE;
              else
                vector_tool->function = VECTORS_MOVE_HANDLE;
            }
        }
      else if (on_curve)
        {
          if (state & TOGGLE_MASK)
            {
              vector_tool->function = VECTORS_DELETE_SEGMENT;
            }
          else if (gimp_stroke_anchor_is_insertable (stroke, anchor, position))
            {
              vector_tool->function = VECTORS_INSERT_ANCHOR;
            }
          else
            {
              vector_tool->function = VECTORS_FINISHED;
            }
        }
      else
        {
          vector_tool->function = VECTORS_FINISHED;
        }

      break;

    case GIMP_VECTOR_MODE_MOVE:
      if (! vector_tool->vectors || GIMP_DRAW_TOOL (tool)->display != display)
        {
          if (on_vectors)
            {
              vector_tool->function = VECTORS_SELECT_VECTOR;
            }
          else
            {
              vector_tool->function = VECTORS_FINISHED;
            }
        }
      else if (on_handle || on_curve)
        {
          if (state & TOGGLE_MASK)
            {
              vector_tool->function = VECTORS_MOVE_VECTORS;
            }
          else
            {
              vector_tool->function = VECTORS_MOVE_STROKE;
            }
        }
      else
        {
          if (on_vectors)
            {
              vector_tool->function = VECTORS_SELECT_VECTOR;
            }
          else
            {
              vector_tool->function = VECTORS_MOVE_VECTORS;
            }
        }
      break;
    }

  gimp_vector_tool_status_update (tool, display, state, proximity);
}


static void
gimp_vector_tool_status_update (GimpTool        *tool,
                                GimpDisplay     *display,
                                GdkModifierType  state,
                                gboolean         proximity)
{
  GimpVectorTool    *vector_tool = GIMP_VECTOR_TOOL (tool);
  GimpVectorOptions *options     = GIMP_VECTOR_TOOL_GET_OPTIONS (tool);

  gimp_tool_pop_status (tool, display);

  if (proximity)
    {
      const gchar *status      = NULL;
      gboolean     free_status = FALSE;

      switch (vector_tool->function)
        {
        case VECTORS_SELECT_VECTOR:
          status = _("Click to pick path to edit");
          break;

        case VECTORS_CREATE_VECTOR:
          status = _("Click to create a new path");
          break;

        case VECTORS_CREATE_STROKE:
          status = _("Click to create a new component of the path");
          break;

        case VECTORS_ADD_ANCHOR:
          status = gimp_suggest_modifiers (_("Click or Click-Drag to create "
                                             "a new anchor"),
                                           GDK_SHIFT_MASK & ~state,
                                           NULL, NULL, NULL);
          free_status = TRUE;
          break;

        case VECTORS_MOVE_ANCHOR:
          if (options->edit_mode != GIMP_VECTOR_MODE_EDIT)
            {
              status = gimp_suggest_modifiers (_("Click-Drag to move the "
                                                 "anchor around"),
                                               GDK_CONTROL_MASK & ~state,
                                               NULL, NULL, NULL);
              free_status = TRUE;
            }
          else
            status = _("Click-Drag to move the anchor around");
          break;

        case VECTORS_MOVE_ANCHORSET:
          status = _("Click-Drag to move the anchors around");
          break;

        case VECTORS_MOVE_HANDLE:
          status = gimp_suggest_modifiers (_("Click-Drag to move the handle "
                                             "around"),
                                           GDK_SHIFT_MASK & ~state,
                                           NULL, NULL, NULL);
          free_status = TRUE;
          break;

        case VECTORS_MOVE_CURVE:
          if (GIMP_VECTOR_TOOL_GET_OPTIONS (tool)->polygonal)
            status = gimp_suggest_modifiers (_("Click-Drag to move the "
                                               "anchors around"),
                                             GDK_SHIFT_MASK & ~state,
                                             NULL, NULL, NULL);
          else
            status = gimp_suggest_modifiers (_("Click-Drag to change the "
                                               "shape of the curve"),
                                             GDK_SHIFT_MASK & ~state,
                                             _("%s: symmetrical"), NULL, NULL);
          free_status = TRUE;
          break;

        case VECTORS_MOVE_STROKE:
          status = gimp_suggest_modifiers (_("Click-Drag to move the "
                                             "component around"),
                                           GDK_SHIFT_MASK & ~state,
                                           NULL, NULL, NULL);
          free_status = TRUE;
          break;

        case VECTORS_MOVE_VECTORS:
          status = _("Click-Drag to move the path around");
          break;

        case VECTORS_INSERT_ANCHOR:
          status = gimp_suggest_modifiers (_("Click-Drag to insert an anchor "
                                             "on the path"),
                                           GDK_SHIFT_MASK & ~state,
                                           NULL, NULL, NULL);
          free_status = TRUE;
          break;

        case VECTORS_DELETE_ANCHOR:
          status = _("Click to delete this anchor");
          break;

        case VECTORS_CONNECT_STROKES:
          status = _("Click to connect this anchor "
                     "with the selected endpoint");
          break;

        case VECTORS_DELETE_SEGMENT:
          status = _("Click to open up the path");
          break;

        case VECTORS_CONVERT_EDGE:
          status = _("Click to make this node angular");
          break;

        case VECTORS_FINISHED:
          status = NULL;
          break;
      }

      if (status)
        gimp_tool_push_status (tool, display, "%s", status);

      if (free_status)
        g_free ((gchar *) status);
    }
}

static void
gimp_vector_tool_cursor_update (GimpTool         *tool,
                                const GimpCoords *coords,
                                GdkModifierType   state,
                                GimpDisplay      *display)
{
  GimpVectorTool     *vector_tool = GIMP_VECTOR_TOOL (tool);
  GimpToolCursorType  tool_cursor = GIMP_TOOL_CURSOR_PATHS;
  GimpCursorModifier  modifier    = GIMP_CURSOR_MODIFIER_NONE;

  switch (vector_tool->function)
    {
    case VECTORS_SELECT_VECTOR:
      tool_cursor = GIMP_TOOL_CURSOR_HAND;
      break;

    case VECTORS_CREATE_VECTOR:
    case VECTORS_CREATE_STROKE:
      modifier = GIMP_CURSOR_MODIFIER_CONTROL;
      break;

    case VECTORS_ADD_ANCHOR:
    case VECTORS_INSERT_ANCHOR:
      tool_cursor = GIMP_TOOL_CURSOR_PATHS_ANCHOR;
      modifier    = GIMP_CURSOR_MODIFIER_PLUS;
      break;

    case VECTORS_DELETE_ANCHOR:
      tool_cursor = GIMP_TOOL_CURSOR_PATHS_ANCHOR;
      modifier    = GIMP_CURSOR_MODIFIER_MINUS;
      break;

    case VECTORS_DELETE_SEGMENT:
      tool_cursor = GIMP_TOOL_CURSOR_PATHS_SEGMENT;
      modifier    = GIMP_CURSOR_MODIFIER_MINUS;
      break;

    case VECTORS_MOVE_HANDLE:
      tool_cursor = GIMP_TOOL_CURSOR_PATHS_CONTROL;
      modifier    = GIMP_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_CONVERT_EDGE:
      tool_cursor = GIMP_TOOL_CURSOR_PATHS_CONTROL;
      modifier    = GIMP_CURSOR_MODIFIER_MINUS;
      break;

    case VECTORS_MOVE_ANCHOR:
      tool_cursor = GIMP_TOOL_CURSOR_PATHS_ANCHOR;
      modifier    = GIMP_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_MOVE_CURVE:
      tool_cursor = GIMP_TOOL_CURSOR_PATHS_SEGMENT;
      modifier    = GIMP_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_MOVE_STROKE:
    case VECTORS_MOVE_VECTORS:
      modifier = GIMP_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_MOVE_ANCHORSET:
      tool_cursor = GIMP_TOOL_CURSOR_PATHS_ANCHOR;
      modifier    = GIMP_CURSOR_MODIFIER_MOVE;
      break;

    case VECTORS_CONNECT_STROKES:
      tool_cursor = GIMP_TOOL_CURSOR_PATHS_SEGMENT;
      modifier    = GIMP_CURSOR_MODIFIER_JOIN;
      break;

    default:
      modifier = GIMP_CURSOR_MODIFIER_BAD;
      break;
    }

  gimp_tool_control_set_tool_cursor     (tool->control, tool_cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_vector_tool_draw (GimpDrawTool *draw_tool)
{
  GimpVectorTool  *vector_tool = GIMP_VECTOR_TOOL (draw_tool);
  GimpAnchor      *cur_anchor  = NULL;
  GimpStroke      *cur_stroke  = NULL;
  GimpVectors     *vectors;
  GArray          *coords;
  gboolean         closed;
  GList           *draw_anchors;
  GList           *list;

  vectors = vector_tool->vectors;

  if (!vectors)
    return;

  while ((cur_stroke = gimp_vectors_stroke_get_next (vectors, cur_stroke)))
    {
      /* anchor handles */
      draw_anchors = gimp_stroke_get_draw_anchors (cur_stroke);

      for (list = draw_anchors; list; list = g_list_next (list))
        {
          cur_anchor = GIMP_ANCHOR (list->data);

          if (cur_anchor->type == GIMP_ANCHOR_ANCHOR)
            {
              gimp_draw_tool_draw_handle (draw_tool,
                                          cur_anchor->selected ?
                                          GIMP_HANDLE_CIRCLE :
                                          GIMP_HANDLE_FILLED_CIRCLE,
                                          cur_anchor->position.x,
                                          cur_anchor->position.y,
                                          TARGET,
                                          TARGET,
                                          GTK_ANCHOR_CENTER,
                                          FALSE);
            }
        }

      g_list_free (draw_anchors);

      if (vector_tool->sel_count <= 2)
        {
          /* control handles */
          draw_anchors = gimp_stroke_get_draw_controls (cur_stroke);

          for (list = draw_anchors; list; list = g_list_next (list))
            {
              cur_anchor = GIMP_ANCHOR (list->data);

              gimp_draw_tool_draw_handle (draw_tool,
                                          GIMP_HANDLE_SQUARE,
                                          cur_anchor->position.x,
                                          cur_anchor->position.y,
                                          TARGET - 3,
                                          TARGET - 3,
                                          GTK_ANCHOR_CENTER,
                                          FALSE);
            }

          g_list_free (draw_anchors);

          /* the lines to the control handles */
          coords = gimp_stroke_get_draw_lines (cur_stroke);

          if (coords)
            {
              if (coords->len % 2 == 0)
                {
                  gint i;

                  for (i = 0; i < coords->len; i += 2)
                    {
                      gimp_draw_tool_draw_dashed_line (draw_tool,
                                    g_array_index (coords, GimpCoords, i).x,
                                    g_array_index (coords, GimpCoords, i).y,
                                    g_array_index (coords, GimpCoords, i + 1).x,
                                    g_array_index (coords, GimpCoords, i + 1).y,
                                    FALSE);
                    }
                }

              g_array_free (coords, TRUE);
            }
        }

      /* the stroke itself */
      if (! gimp_item_get_visible (GIMP_ITEM (vectors)))
        {
          coords = gimp_stroke_interpolate (cur_stroke, 1.0, &closed);

          if (coords)
            {
              if (coords->len)
                gimp_draw_tool_draw_strokes (draw_tool,
                                             &g_array_index (coords,
                                                             GimpCoords, 0),
                                             coords->len, FALSE, FALSE);

              g_array_free (coords, TRUE);
            }
        }
    }
}

static void
gimp_vector_tool_vectors_changed (GimpImage      *image,
                                  GimpVectorTool *vector_tool)
{
  gimp_vector_tool_set_vectors (vector_tool,
                                gimp_image_get_active_vectors (image));
}

static void
gimp_vector_tool_vectors_removed (GimpVectors    *vectors,
                                  GimpVectorTool *vector_tool)
{
  gimp_vector_tool_set_vectors (vector_tool, NULL);
}

static void
gimp_vector_tool_vectors_visible (GimpVectors    *vectors,
                                  GimpVectorTool *vector_tool)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (vector_tool);

  if (gimp_draw_tool_is_active (draw_tool) && draw_tool->paused_count == 0)
    {
      GimpStroke *stroke = NULL;

      while ((stroke = gimp_vectors_stroke_get_next (vectors, stroke)))
        {
          GArray   *coords;
          gboolean  closed;

          coords = gimp_stroke_interpolate (stroke, 1.0, &closed);

          if (coords)
            {
              if (coords->len)
                gimp_draw_tool_draw_strokes (draw_tool,
                                             &g_array_index (coords,
                                                             GimpCoords, 0),
                                             coords->len, FALSE, FALSE);

              g_array_free (coords, TRUE);
            }
        }
    }
}

static void
gimp_vector_tool_vectors_freeze (GimpVectors    *vectors,
                                 GimpVectorTool *vector_tool)
{
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (vector_tool));
}

static void
gimp_vector_tool_vectors_thaw (GimpVectors    *vectors,
                               GimpVectorTool *vector_tool)
{
  /* Ok, the vector might have changed externally (e.g. Undo)
   * we need to validate our internal state. */
  gimp_vector_tool_verify_state (vector_tool);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (vector_tool));
}

void
gimp_vector_tool_set_vectors (GimpVectorTool *vector_tool,
                              GimpVectors    *vectors)
{
  GimpDrawTool      *draw_tool;
  GimpTool          *tool;
  GimpItem          *item = NULL;
  GimpVectorOptions *options;

  g_return_if_fail (GIMP_IS_VECTOR_TOOL (vector_tool));
  g_return_if_fail (vectors == NULL || GIMP_IS_VECTORS (vectors));

  draw_tool = GIMP_DRAW_TOOL (vector_tool);
  tool      = GIMP_TOOL (vector_tool);
  options   = GIMP_VECTOR_TOOL_GET_OPTIONS (vector_tool);

  if (vectors)
    item = GIMP_ITEM (vectors);

  if (vectors == vector_tool->vectors)
    return;

  gimp_draw_tool_pause (draw_tool);

  if (gimp_draw_tool_is_active (draw_tool) &&
      (! vectors || draw_tool->display->image != gimp_item_get_image (item)))
    {
      gimp_draw_tool_stop (draw_tool);
    }
#endif

  if (vector_tool->vectors)
    {
      GimpImage *old_image;

      old_image = gimp_item_get_image (GIMP_ITEM (vector_tool->vectors));

      g_signal_handlers_disconnect_by_func (old_image,
                                            gimp_vector_tool_vectors_changed,
                                            vector_tool);
      g_signal_handlers_disconnect_by_func (vector_tool->vectors,
                                            gimp_vector_tool_vectors_removed,
                                            vector_tool);

      g_clear_object (&vector_tool->vectors);

      if (options->to_selection_button)
        {
          gtk_widget_set_sensitive (options->to_selection_button, FALSE);
          g_signal_handlers_disconnect_by_func (options->to_selection_button,
                                                gimp_vector_tool_to_selection,
                                                tool);
          g_signal_handlers_disconnect_by_func (options->to_selection_button,
                                                gimp_vector_tool_to_selection_extended,
                                                tool);
        }

      if (options->fill_button)
        {
          gtk_widget_set_sensitive (options->fill_button, FALSE);
          g_signal_handlers_disconnect_by_func (options->fill_button,
                                                gimp_vector_tool_fill_vectors,
                                                tool);
        }

      if (options->stroke_button)
        {
          gtk_widget_set_sensitive (options->stroke_button, FALSE);
          g_signal_handlers_disconnect_by_func (options->stroke_button,
                                                gimp_vector_tool_stroke_vectors,
                                                tool);
        }

      if (options->vector_layer_button)
        {
          gtk_widget_set_sensitive (options->vector_layer_button, FALSE);
          g_signal_handlers_disconnect_by_func (options->vector_layer_button,
                                                gimp_vector_tool_create_vector_layer,
                                                tool);
        }
    }

  if (! vectors ||
      (tool->display &&
       gimp_display_get_image (tool->display) != gimp_item_get_image (item)))
    {
      gimp_vector_tool_halt (vector_tool);
    }

  if (! vectors)
    return;

  vector_tool->vectors = g_object_ref (vectors);

  g_signal_connect_object (gimp_item_get_image (item), "selected-vectors-changed",
                           G_CALLBACK (gimp_vector_tool_vectors_changed),
                           vector_tool, 0);
  g_signal_connect_object (vectors, "removed",
                           G_CALLBACK (gimp_vector_tool_vectors_removed),
                           vector_tool, 0);

  if (options->to_selection_button)
    {
      g_signal_connect_swapped (options->to_selection_button, "clicked",
                                G_CALLBACK (gimp_vector_tool_to_selection),
                                tool);
      g_signal_connect_swapped (options->to_selection_button, "extended-clicked",
                                G_CALLBACK (gimp_vector_tool_to_selection_extended),
                                tool);
      gtk_widget_set_sensitive (options->to_selection_button, TRUE);
    }

  if (options->fill_button)
    {
      g_signal_connect_swapped (options->fill_button, "clicked",
                                G_CALLBACK (gimp_vector_tool_fill_vectors),
                                tool);
      gtk_widget_set_sensitive (options->fill_button, TRUE);
    }

  if (options->stroke_button)
    {
      g_signal_connect_swapped (options->stroke_button, "clicked",
                                G_CALLBACK (gimp_vector_tool_stroke_vectors),
                                tool);
      gtk_widget_set_sensitive (options->stroke_button, TRUE);
    }

  if (options->vector_layer_button)
    {
      g_signal_connect_swapped (options->vector_layer_button, "clicked",
                                G_CALLBACK (gimp_vector_tool_create_vector_layer),
                                tool);

      gtk_widget_set_sensitive (options->vector_layer_button, TRUE);
    }

  if (tool->display)
    {
      gimp_tool_path_set_vectors (GIMP_TOOL_PATH (vector_tool->widget), vectors);
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
        gimp_vector_tool_start (vector_tool, display);
    }

  if (options->edit_mode != GIMP_VECTOR_MODE_DESIGN)
    g_object_set (options, "vectors-edit-mode",
                  GIMP_VECTOR_MODE_DESIGN, NULL);
}

static void
gimp_vector_tool_to_selection (GimpVectorTool *vector_tool)
{
  gimp_vector_tool_to_selection_extended (vector_tool, 0);
}

static void
gimp_vector_tool_to_selection_extended (GimpVectorTool  *vector_tool,
                                        GdkModifierType  state)
{
  GimpImage *image;

  if (! vector_tool->vectors)
    return;

  image = gimp_item_get_image (GIMP_ITEM (vector_tool->vectors));

  gimp_item_to_selection (GIMP_ITEM (vector_tool->vectors),
                          gimp_modifiers_to_channel_op (state),
                          TRUE, FALSE, 0, 0);
  gimp_image_flush (image);
}


static void
gimp_vector_tool_fill_vectors (GimpVectorTool *vector_tool,
                               GtkWidget      *button)
{
  GimpDialogConfig *config;
  GimpImage        *image;
  GList            *drawables;
  GtkWidget        *dialog;

  if (! vector_tool->vectors)
    return;

  image = gimp_item_get_image (GIMP_ITEM (vector_tool->vectors));

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  drawables = gimp_image_get_selected_drawables (image);

  if (! drawables)
    {
      gimp_tool_message (GIMP_TOOL (vector_tool),
                         GIMP_TOOL (vector_tool)->display,
                         _("There are no selected layers or channels to fill."));
      return;
    }

  dialog = fill_dialog_new (GIMP_ITEM (vector_tool->vectors),
                            drawables,
                            GIMP_CONTEXT (GIMP_TOOL_GET_OPTIONS (vector_tool)),
                            _("Fill Path"),
                            GIMP_ICON_TOOL_BUCKET_FILL,
                            GIMP_HELP_PATH_FILL,
                            button,
                            config->fill_options,
                            gimp_vector_tool_fill_callback,
                            vector_tool);
  gtk_widget_show (dialog);

  g_list_free (drawables);
}

static void
gimp_vector_tool_fill_callback (GtkWidget       *dialog,
                                GimpItem        *item,
                                GList           *drawables,
                                GimpContext     *context,
                                GimpFillOptions *options,
                                gpointer         data)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (context->gimp->config);
  GimpImage        *image  = gimp_item_get_image (item);
  GError           *error  = NULL;

  gimp_config_sync (G_OBJECT (options),
                    G_OBJECT (config->fill_options), 0);

  if (! gimp_item_fill (item, drawables, options,
                        TRUE, NULL, &error))
    {
      gimp_message_literal (context->gimp,
                            G_OBJECT (dialog),
                            GIMP_MESSAGE_WARNING,
                            error ? error->message : "NULL");

      g_clear_error (&error);
      return;
    }

  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}


static void
gimp_vector_tool_stroke_vectors (GimpVectorTool *vector_tool,
                                 GtkWidget      *button)
{
  GimpDialogConfig *config;
  GimpImage        *image;
  GList            *drawables;
  GtkWidget        *dialog;

  if (! vector_tool->vectors)
    return;

  image = gimp_item_get_image (GIMP_ITEM (vector_tool->vectors));

  config = GIMP_DIALOG_CONFIG (image->gimp->config);

  drawables = gimp_image_get_selected_drawables (image);

  if (! drawables)
    {
      gimp_tool_message (GIMP_TOOL (vector_tool),
                         GIMP_TOOL (vector_tool)->display,
                         _("There are no selected layers or channels to stroke to."));
      return;
    }

  dialog = stroke_dialog_new (GIMP_ITEM (vector_tool->vectors),
                              drawables,
                              GIMP_CONTEXT (GIMP_TOOL_GET_OPTIONS (vector_tool)),
                              _("Stroke Path"),
                              GIMP_ICON_PATH_STROKE,
                              GIMP_HELP_PATH_STROKE,
                              button,
                              config->stroke_options,
                              gimp_vector_tool_stroke_callback,
                              vector_tool);
  gtk_widget_show (dialog);
  g_list_free (drawables);
}

static void
gimp_vector_tool_stroke_callback (GtkWidget         *dialog,
                                  GimpItem          *item,
                                  GList             *drawables,
                                  GimpContext       *context,
                                  GimpStrokeOptions *options,
                                  gpointer           data)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (context->gimp->config);
  GimpImage        *image  = gimp_item_get_image (item);
  GError           *error  = NULL;

  gimp_config_sync (G_OBJECT (options),
                    G_OBJECT (config->stroke_options), 0);

  if (! gimp_item_stroke (item, drawables, context, options, NULL,
                          TRUE, NULL, &error))
    {
      gimp_message_literal (context->gimp,
                            G_OBJECT (dialog),
                            GIMP_MESSAGE_WARNING,
                            error ? error->message : "NULL");

      g_clear_error (&error);
      return;
    }

  gimp_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
gimp_vector_tool_create_vector_layer (GimpVectorTool *vector_tool,
                                      GtkWidget      *button)
{
  GimpImage       *image;
  //GtkWidget       *dialog;
  GimpVectorLayer *layer;

  if (! vector_tool->vectors)
    return;

  image = gimp_item_get_image (GIMP_ITEM (vector_tool->vectors));

  layer = gimp_vector_layer_new (image, vector_tool->vectors,
                                 gimp_get_user_context (image->gimp));

  gimp_image_add_layer (image,
                        GIMP_LAYER (layer),
                        GIMP_IMAGE_ACTIVE_PARENT,
                        -1,
                        TRUE);

  gimp_vector_layer_refresh (layer);

  gimp_image_flush (image);
}
