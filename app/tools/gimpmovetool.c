/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpdisplayconfig.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimp-cairo.h"
#include "core/gimp-utils.h"
#include "core/gimpguide.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-item.h"
#include "core/gimplayer.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayermask.h"
#include "core/gimplayer-floating-selection.h"
#include "core/gimpundostack.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasitem.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-appearance.h"
#include "display/gimpdisplayshell-selection.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpeditselectiontool.h"
#include "gimpguidetool.h"
#include "gimpmoveoptions.h"
#include "gimpmovetool.h"
#include "gimptoolcontrol.h"
#include "gimptools-utils.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_move_tool_finalize       (GObject               *object);

static void   gimp_move_tool_button_press   (GimpTool              *tool,
                                             const GimpCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             GimpButtonPressType    press_type,
                                             GimpDisplay           *display);
static void   gimp_move_tool_button_release (GimpTool              *tool,
                                             const GimpCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             GimpButtonReleaseType  release_type,
                                             GimpDisplay           *display);
static gboolean gimp_move_tool_key_press    (GimpTool              *tool,
                                             GdkEventKey           *kevent,
                                             GimpDisplay           *display);
static void   gimp_move_tool_modifier_key   (GimpTool              *tool,
                                             GdkModifierType        key,
                                             gboolean               press,
                                             GdkModifierType        state,
                                             GimpDisplay           *display);
static void   gimp_move_tool_oper_update    (GimpTool              *tool,
                                             const GimpCoords      *coords,
                                             GdkModifierType        state,
                                             gboolean               proximity,
                                             GimpDisplay           *display);
static void   gimp_move_tool_cursor_update  (GimpTool              *tool,
                                             const GimpCoords      *coords,
                                             GdkModifierType        state,
                                             GimpDisplay           *display);

static void   gimp_move_tool_draw           (GimpDrawTool          *draw_tool);


G_DEFINE_TYPE (GimpMoveTool, gimp_move_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_move_tool_parent_class


void
gimp_move_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_MOVE_TOOL,
                GIMP_TYPE_MOVE_OPTIONS,
                gimp_move_options_gui,
                0,
                "gimp-move-tool",
                C_("tool", "Move"),
                _("Move Tool: Move layers, selections, and other objects"),
                N_("_Move"), "M",
                NULL, GIMP_HELP_TOOL_MOVE,
                GIMP_ICON_TOOL_MOVE,
                data);
}

static void
gimp_move_tool_class_init (GimpMoveToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = gimp_move_tool_finalize;

  tool_class->button_press   = gimp_move_tool_button_press;
  tool_class->button_release = gimp_move_tool_button_release;
  tool_class->key_press      = gimp_move_tool_key_press;
  tool_class->modifier_key   = gimp_move_tool_modifier_key;
  tool_class->oper_update    = gimp_move_tool_oper_update;
  tool_class->cursor_update  = gimp_move_tool_cursor_update;

  draw_tool_class->draw      = gimp_move_tool_draw;
}

static void
gimp_move_tool_init (GimpMoveTool *move_tool)
{
  GimpTool *tool = GIMP_TOOL (move_tool);

  gimp_tool_control_set_snap_to            (tool->control, FALSE);
  gimp_tool_control_set_handle_empty_image (tool->control, TRUE);
  gimp_tool_control_set_tool_cursor        (tool->control,
                                            GIMP_TOOL_CURSOR_MOVE);

  move_tool->floating_layer     = NULL;
  move_tool->guides             = NULL;

  move_tool->saved_type         = GIMP_TRANSFORM_TYPE_LAYER;

  move_tool->old_selected_layers   = NULL;
  move_tool->old_selected_vectors = NULL;
}

static void
gimp_move_tool_finalize (GObject *object)
{
  GimpMoveTool *move = GIMP_MOVE_TOOL (object);

  g_clear_pointer (&move->guides, g_list_free);
  g_clear_pointer (&move->old_selected_layers, g_list_free);
  g_clear_pointer (&move->old_selected_vectors, g_list_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_move_tool_button_press (GimpTool            *tool,
                             const GimpCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             GimpButtonPressType  press_type,
                             GimpDisplay         *display)
{
  GimpMoveTool      *move           = GIMP_MOVE_TOOL (tool);
  GimpMoveOptions   *options        = GIMP_MOVE_TOOL_GET_OPTIONS (tool);
  GimpDisplayShell  *shell          = gimp_display_get_shell (display);
  GimpImage         *image          = gimp_display_get_image (display);
  GimpItem          *active_item    = NULL;
  GList             *selected_items = NULL;
  GList             *iter;
  GimpTranslateMode  translate_mode = GIMP_TRANSLATE_MODE_MASK;
  const gchar       *null_message   = NULL;
  const gchar       *locked_message = NULL;
  GimpItem          *locked_item    = NULL;

  tool->display = display;

  move->floating_layer = NULL;

  g_clear_pointer (&move->guides, g_list_free);

  if (! options->move_current)
    {
      const gint snap_distance = display->config->snap_distance;

      if (options->move_type == GIMP_TRANSFORM_TYPE_PATH)
        {
          GimpPath *vectors;

          vectors = gimp_image_pick_path (image,
                                          coords->x, coords->y,
                                          FUNSCALEX (shell, snap_distance),
                                          FUNSCALEY (shell, snap_distance));
          if (vectors)
            {
              GList *new_selected_paths = g_list_prepend (NULL, vectors);

              move->old_selected_vectors =
                g_list_copy (gimp_image_get_selected_paths (image));

              gimp_image_set_selected_paths (image, new_selected_paths);
              g_list_free (new_selected_paths);
            }
          else
            {
              /*  no path picked  */
              return;
            }
        }
      else if (options->move_type == GIMP_TRANSFORM_TYPE_LAYER)
        {
          GList     *guides;
          GimpLayer *layer;

          if (gimp_display_shell_get_show_guides (shell) &&
              (guides = gimp_image_pick_guides (image,
                                                coords->x, coords->y,
                                                FUNSCALEX (shell, snap_distance),
                                                FUNSCALEY (shell, snap_distance))))
            {
              move->guides = guides;

              gimp_guide_tool_start_edit_many (tool, display, guides);

              return;
            }
          else if ((layer = gimp_image_pick_layer (image,
                                                   coords->x,
                                                   coords->y,
                                                   NULL)))
            {
              if (gimp_image_get_floating_selection (image) &&
                  ! gimp_layer_is_floating_sel (layer))
                {
                  /*  If there is a floating selection, and this aint it,
                   *  use the move tool to anchor it.
                   */
                  move->floating_layer =
                    gimp_image_get_floating_selection (image);

                  gimp_tool_control_activate (tool->control);

                  return;
                }
              else
                {
                  GList *new_selected_layers = g_list_prepend (NULL, layer);

                  move->old_selected_layers = g_list_copy (gimp_image_get_selected_layers (image));

                  gimp_image_set_selected_layers (image, new_selected_layers);
                  g_list_free (new_selected_layers);
                }
            }
          else
            {
              /*  no guide and no layer picked  */

              return;
            }
        }
    }

  switch (options->move_type)
    {
    case GIMP_TRANSFORM_TYPE_PATH:
      {
        selected_items = gimp_image_get_selected_paths (image);
        selected_items = g_list_copy (selected_items);

        translate_mode = GIMP_TRANSLATE_MODE_VECTORS;

        if (! selected_items)
          {
            null_message = _("There are no paths to move.");
          }
        else
          {
            gint n_items = 0;

            for (iter = selected_items; iter; iter = iter->next)
              {
                if (! gimp_item_is_position_locked (iter->data, &locked_item))
                  n_items++;
              }

            if (n_items == 0)
              locked_message = _("All selected path's position are locked.");
          }
      }
      break;

    case GIMP_TRANSFORM_TYPE_SELECTION:
      {
        active_item = GIMP_ITEM (gimp_image_get_mask (image));

        if (gimp_channel_is_empty (GIMP_CHANNEL (active_item)))
          active_item = NULL;

        translate_mode = GIMP_TRANSLATE_MODE_MASK;

        if (! active_item)
          {
            /* cannot happen, don't translate this message */
            null_message  = "There is no selection to move.";
          }
        else if (gimp_item_is_position_locked (active_item, &locked_item))
          {
            locked_message = "The selection's position is locked.";
          }
      }
      break;

    case GIMP_TRANSFORM_TYPE_LAYER:
      {
        selected_items = gimp_image_get_selected_drawables (image);

        if (! selected_items)
          {
            null_message = _("There is no layer to move.");
          }
        else if (GIMP_IS_LAYER_MASK (selected_items->data))
          {
            g_return_if_fail (g_list_length (selected_items) == 1);

            translate_mode = GIMP_TRANSLATE_MODE_LAYER_MASK;

            if (gimp_item_is_position_locked (selected_items->data, &locked_item))
              locked_message = _("The selected layer's position is locked.");
            else if (gimp_item_is_content_locked (selected_items->data, NULL))
              locked_message = _("The selected layer's pixels are locked.");
          }
        else if (GIMP_IS_CHANNEL (selected_items->data))
          {
            translate_mode = GIMP_TRANSLATE_MODE_CHANNEL;

            for (iter = selected_items; iter; iter = iter->next)
              if (gimp_item_is_position_locked (iter->data, &locked_item) ||
                  gimp_item_is_content_locked (iter->data, &locked_item))
              locked_message = _("A selected channel's position or pixels are locked.");
          }
        else
          {
            translate_mode = GIMP_TRANSLATE_MODE_LAYER;

            for (iter = selected_items; iter; iter = iter->next)
              if (gimp_item_is_position_locked (iter->data, &locked_item))
              locked_message = _("A selected layer's position is locked.");
          }
      }
      break;

    case GIMP_TRANSFORM_TYPE_IMAGE:
      g_return_if_reached ();
    }

  if (! active_item && ! selected_items)
    {
      gimp_tool_message_literal (tool, display, null_message);
      gimp_tools_show_tool_options (display->gimp);
      gimp_widget_blink (options->type_box);
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
      return;
    }
  else if (locked_message)
    {
      gimp_tool_message_literal (tool, display, locked_message);

      if (locked_item == NULL)
        locked_item = active_item ? active_item : selected_items->data;

      gimp_tools_blink_lock_box (display->gimp, locked_item);
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
      g_list_free (selected_items);
      return;
    }

  gimp_tool_control_activate (tool->control);

  gimp_edit_selection_tool_start (tool, display, coords,
                                  translate_mode,
                                  TRUE);
  g_list_free (selected_items);
}

static void
gimp_move_tool_button_release (GimpTool              *tool,
                               const GimpCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type,
                               GimpDisplay           *display)
{
  GimpMoveTool  *move   = GIMP_MOVE_TOOL (tool);
  GimpGuiConfig *config = GIMP_GUI_CONFIG (display->gimp->config);
  GimpImage     *image  = gimp_display_get_image (display);
  gboolean       flush  = FALSE;

  gimp_tool_control_halt (tool->control);

  if (! config->move_tool_changes_active ||
      (release_type == GIMP_BUTTON_RELEASE_CANCEL))
    {
      if (move->old_selected_layers)
        {
          gimp_image_set_selected_layers (image, move->old_selected_layers);
          g_clear_pointer (&move->old_selected_layers, g_list_free);

          flush = TRUE;
        }

      if (move->old_selected_vectors)
        {
          gimp_image_set_selected_paths (image, move->old_selected_vectors);
          g_clear_pointer (&move->old_selected_vectors, g_list_free);

          flush = TRUE;
        }
    }

  if (release_type != GIMP_BUTTON_RELEASE_CANCEL)
    {
      if (move->floating_layer)
        {
          floating_sel_anchor (move->floating_layer);

          flush = TRUE;
        }
    }

  if (flush)
    gimp_image_flush (image);
}

static gboolean
gimp_move_tool_key_press (GimpTool    *tool,
                          GdkEventKey *kevent,
                          GimpDisplay *display)
{
  GimpMoveOptions *options = GIMP_MOVE_TOOL_GET_OPTIONS (tool);

  return gimp_edit_selection_tool_translate (tool, kevent,
                                             options->move_type,
                                             display,
                                             &options->type_box);
}

static void
gimp_move_tool_modifier_key (GimpTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             GimpDisplay     *display)
{
  GimpMoveTool    *move    = GIMP_MOVE_TOOL (tool);
  GimpMoveOptions *options = GIMP_MOVE_TOOL_GET_OPTIONS (tool);

  if (key == gimp_get_extend_selection_mask ())
    {
      g_object_set (options, "move-current", ! options->move_current, NULL);
    }
  else if (key == GDK_MOD1_MASK ||
           key == gimp_get_toggle_behavior_mask ())
    {
      GimpTransformType button_type;

      button_type = options->move_type;

      if (press)
        {
          if (key == (state & (GDK_MOD1_MASK |
                               gimp_get_toggle_behavior_mask ())))
            {
              /*  first modifier pressed  */

              move->saved_type = options->move_type;
            }
        }
      else
        {
          if (! (state & (GDK_MOD1_MASK |
                          gimp_get_toggle_behavior_mask ())))
            {
              /*  last modifier released  */

              button_type = move->saved_type;
            }
        }

      if (state & GDK_MOD1_MASK)
        {
          button_type = GIMP_TRANSFORM_TYPE_SELECTION;
        }
      else if (state & gimp_get_toggle_behavior_mask ())
        {
          button_type = GIMP_TRANSFORM_TYPE_PATH;
        }

      if (button_type != options->move_type)
        {
          g_object_set (options, "move-type", button_type, NULL);
        }
    }
}

static void
gimp_move_tool_oper_update (GimpTool         *tool,
                            const GimpCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            GimpDisplay      *display)
{
  GimpMoveTool     *move    = GIMP_MOVE_TOOL (tool);
  GimpMoveOptions  *options = GIMP_MOVE_TOOL_GET_OPTIONS (tool);
  GimpDisplayShell *shell   = gimp_display_get_shell (display);
  GimpImage        *image   = gimp_display_get_image (display);
  GList            *guides  = NULL;

  if (options->move_type == GIMP_TRANSFORM_TYPE_LAYER &&
      ! options->move_current                         &&
      gimp_display_shell_get_show_guides (shell)      &&
      proximity)
    {
      gint snap_distance = display->config->snap_distance;

      guides = gimp_image_pick_guides (image, coords->x, coords->y,
                                       FUNSCALEX (shell, snap_distance),
                                       FUNSCALEY (shell, snap_distance));
    }

  if (gimp_g_list_compare (guides, move->guides))
    {
      GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

      gimp_draw_tool_pause (draw_tool);

      if (gimp_draw_tool_is_active (draw_tool) &&
          draw_tool->display != display)
        gimp_draw_tool_stop (draw_tool);

      g_clear_pointer (&move->guides, g_list_free);

      move->guides = guides;

      if (! gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_start (draw_tool, display);

      gimp_draw_tool_resume (draw_tool);
    }
  else
    {
      g_list_free (guides);
    }
}

static void
gimp_move_tool_cursor_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              GimpDisplay      *display)
{
  GimpMoveOptions    *options       = GIMP_MOVE_TOOL_GET_OPTIONS (tool);
  GimpDisplayShell   *shell         = gimp_display_get_shell (display);
  GimpImage          *image         = gimp_display_get_image (display);
  GimpCursorType      cursor        = GIMP_CURSOR_MOUSE;
  GimpToolCursorType  tool_cursor   = GIMP_TOOL_CURSOR_MOVE;
  GimpCursorModifier  modifier      = GIMP_CURSOR_MODIFIER_NONE;
  gint                snap_distance = display->config->snap_distance;

  if (options->move_type == GIMP_TRANSFORM_TYPE_PATH)
    {
      tool_cursor = GIMP_TOOL_CURSOR_PATHS;
      modifier    = GIMP_CURSOR_MODIFIER_MOVE;

      if (options->move_current)
        {
          GList *selected = gimp_image_get_selected_paths (image);
          GList *iter;
          gint   n_items = 0;

          for (iter = selected; iter; iter = iter->next)
            {
              if (! gimp_item_is_position_locked (iter->data, NULL))
                n_items++;
            }

          if (n_items == 0)
            modifier = GIMP_CURSOR_MODIFIER_BAD;
        }
      else
        {
          if (gimp_image_pick_path (image,
                                    coords->x, coords->y,
                                    FUNSCALEX (shell, snap_distance),
                                    FUNSCALEY (shell, snap_distance)))
            {
              tool_cursor = GIMP_TOOL_CURSOR_HAND;
            }
          else
            {
              modifier = GIMP_CURSOR_MODIFIER_BAD;
            }
        }
    }
  else if (options->move_type == GIMP_TRANSFORM_TYPE_SELECTION)
    {
      tool_cursor = GIMP_TOOL_CURSOR_RECT_SELECT;
      modifier    = GIMP_CURSOR_MODIFIER_MOVE;

      if (gimp_channel_is_empty (gimp_image_get_mask (image)))
        modifier = GIMP_CURSOR_MODIFIER_BAD;
    }
  else if (options->move_current)
    {
      GList *items   = gimp_image_get_selected_drawables (image);
      GList *iter;
      gint   n_items = 0;

      for (iter = items; iter; iter = iter->next)
        if (! gimp_item_is_position_locked (iter->data, NULL))
          n_items++;
      if (n_items == 0)
        modifier = GIMP_CURSOR_MODIFIER_BAD;

      g_list_free (items);
    }
  else
    {
      GimpLayer  *layer;

      if (gimp_display_shell_get_show_guides (shell) &&
          gimp_image_pick_guide (image, coords->x, coords->y,
                                 FUNSCALEX (shell, snap_distance),
                                 FUNSCALEY (shell, snap_distance)))
        {
          tool_cursor = GIMP_TOOL_CURSOR_HAND;
          modifier    = GIMP_CURSOR_MODIFIER_MOVE;
        }
      else if ((layer = gimp_image_pick_layer (image,
                                               coords->x, coords->y,
                                               NULL)))
        {
          /*  if there is a floating selection, and this aint it...  */
          if (gimp_image_get_floating_selection (image) &&
              ! gimp_layer_is_floating_sel (layer))
            {
              tool_cursor = GIMP_TOOL_CURSOR_MOVE;
              modifier    = GIMP_CURSOR_MODIFIER_ANCHOR;
            }
          else if (gimp_item_is_position_locked (GIMP_ITEM (layer), NULL))
            {
              modifier = GIMP_CURSOR_MODIFIER_BAD;
            }
          else if (! g_list_find (gimp_image_get_selected_layers (image), layer))
            {
              tool_cursor = GIMP_TOOL_CURSOR_HAND;
              modifier    = GIMP_CURSOR_MODIFIER_MOVE;
            }
        }
      else
        {
          modifier = GIMP_CURSOR_MODIFIER_BAD;
        }
    }

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_tool_cursor     (tool->control, tool_cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_move_tool_draw (GimpDrawTool *draw_tool)
{
  GimpMoveTool *move = GIMP_MOVE_TOOL (draw_tool);
  GList        *iter;

  for (iter = move->guides; iter; iter = g_list_next (iter))
    {
      GimpGuide      *guide = iter->data;
      GimpCanvasItem *item;
      GimpGuideStyle  style;

      style = gimp_guide_get_style (guide);

      item = gimp_draw_tool_add_guide (draw_tool,
                                       gimp_guide_get_orientation (guide),
                                       gimp_guide_get_position (guide),
                                       style);
      gimp_canvas_item_set_highlight (item, TRUE);
    }
}
