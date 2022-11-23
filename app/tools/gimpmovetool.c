/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmadisplayconfig.h"
#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligma-cairo.h"
#include "core/ligma-utils.h"
#include "core/ligmaguide.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-pick-item.h"
#include "core/ligmalayer.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmalayermask.h"
#include "core/ligmalayer-floating-selection.h"
#include "core/ligmaundostack.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmacanvasitem.h"
#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-appearance.h"
#include "display/ligmadisplayshell-selection.h"
#include "display/ligmadisplayshell-transform.h"

#include "ligmaeditselectiontool.h"
#include "ligmaguidetool.h"
#include "ligmamoveoptions.h"
#include "ligmamovetool.h"
#include "ligmatoolcontrol.h"
#include "ligmatools-utils.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   ligma_move_tool_finalize       (GObject               *object);

static void   ligma_move_tool_button_press   (LigmaTool              *tool,
                                             const LigmaCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             LigmaButtonPressType    press_type,
                                             LigmaDisplay           *display);
static void   ligma_move_tool_button_release (LigmaTool              *tool,
                                             const LigmaCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             LigmaButtonReleaseType  release_type,
                                             LigmaDisplay           *display);
static gboolean ligma_move_tool_key_press    (LigmaTool              *tool,
                                             GdkEventKey           *kevent,
                                             LigmaDisplay           *display);
static void   ligma_move_tool_modifier_key   (LigmaTool              *tool,
                                             GdkModifierType        key,
                                             gboolean               press,
                                             GdkModifierType        state,
                                             LigmaDisplay           *display);
static void   ligma_move_tool_oper_update    (LigmaTool              *tool,
                                             const LigmaCoords      *coords,
                                             GdkModifierType        state,
                                             gboolean               proximity,
                                             LigmaDisplay           *display);
static void   ligma_move_tool_cursor_update  (LigmaTool              *tool,
                                             const LigmaCoords      *coords,
                                             GdkModifierType        state,
                                             LigmaDisplay           *display);

static void   ligma_move_tool_draw           (LigmaDrawTool          *draw_tool);


G_DEFINE_TYPE (LigmaMoveTool, ligma_move_tool, LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_move_tool_parent_class


void
ligma_move_tool_register (LigmaToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (LIGMA_TYPE_MOVE_TOOL,
                LIGMA_TYPE_MOVE_OPTIONS,
                ligma_move_options_gui,
                0,
                "ligma-move-tool",
                C_("tool", "Move"),
                _("Move Tool: Move layers, selections, and other objects"),
                N_("_Move"), "M",
                NULL, LIGMA_HELP_TOOL_MOVE,
                LIGMA_ICON_TOOL_MOVE,
                data);
}

static void
ligma_move_tool_class_init (LigmaMoveToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  LigmaToolClass     *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = ligma_move_tool_finalize;

  tool_class->button_press   = ligma_move_tool_button_press;
  tool_class->button_release = ligma_move_tool_button_release;
  tool_class->key_press      = ligma_move_tool_key_press;
  tool_class->modifier_key   = ligma_move_tool_modifier_key;
  tool_class->oper_update    = ligma_move_tool_oper_update;
  tool_class->cursor_update  = ligma_move_tool_cursor_update;

  draw_tool_class->draw      = ligma_move_tool_draw;
}

static void
ligma_move_tool_init (LigmaMoveTool *move_tool)
{
  LigmaTool *tool = LIGMA_TOOL (move_tool);

  ligma_tool_control_set_snap_to            (tool->control, FALSE);
  ligma_tool_control_set_handle_empty_image (tool->control, TRUE);
  ligma_tool_control_set_tool_cursor        (tool->control,
                                            LIGMA_TOOL_CURSOR_MOVE);

  move_tool->floating_layer     = NULL;
  move_tool->guides             = NULL;

  move_tool->saved_type         = LIGMA_TRANSFORM_TYPE_LAYER;

  move_tool->old_selected_layers   = NULL;
  move_tool->old_selected_vectors = NULL;
}

static void
ligma_move_tool_finalize (GObject *object)
{
  LigmaMoveTool *move = LIGMA_MOVE_TOOL (object);

  g_clear_pointer (&move->guides, g_list_free);
  g_clear_pointer (&move->old_selected_layers, g_list_free);
  g_clear_pointer (&move->old_selected_vectors, g_list_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_move_tool_button_press (LigmaTool            *tool,
                             const LigmaCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             LigmaButtonPressType  press_type,
                             LigmaDisplay         *display)
{
  LigmaMoveTool      *move           = LIGMA_MOVE_TOOL (tool);
  LigmaMoveOptions   *options        = LIGMA_MOVE_TOOL_GET_OPTIONS (tool);
  LigmaDisplayShell  *shell          = ligma_display_get_shell (display);
  LigmaImage         *image          = ligma_display_get_image (display);
  LigmaItem          *active_item    = NULL;
  GList             *selected_items = NULL;
  GList             *iter;
  LigmaTranslateMode  translate_mode = LIGMA_TRANSLATE_MODE_MASK;
  const gchar       *null_message   = NULL;
  const gchar       *locked_message = NULL;
  LigmaItem          *locked_item    = NULL;

  tool->display = display;

  move->floating_layer = NULL;

  g_clear_pointer (&move->guides, g_list_free);

  if (! options->move_current)
    {
      const gint snap_distance = display->config->snap_distance;

      if (options->move_type == LIGMA_TRANSFORM_TYPE_PATH)
        {
          LigmaVectors *vectors;

          vectors = ligma_image_pick_vectors (image,
                                             coords->x, coords->y,
                                             FUNSCALEX (shell, snap_distance),
                                             FUNSCALEY (shell, snap_distance));
          if (vectors)
            {
              GList *new_selected_vectors = g_list_prepend (NULL, vectors);

              move->old_selected_vectors =
                g_list_copy (ligma_image_get_selected_vectors (image));

              ligma_image_set_selected_vectors (image, new_selected_vectors);
              g_list_free (new_selected_vectors);
            }
          else
            {
              /*  no path picked  */
              return;
            }
        }
      else if (options->move_type == LIGMA_TRANSFORM_TYPE_LAYER)
        {
          GList     *guides;
          LigmaLayer *layer;

          if (ligma_display_shell_get_show_guides (shell) &&
              (guides = ligma_image_pick_guides (image,
                                                coords->x, coords->y,
                                                FUNSCALEX (shell, snap_distance),
                                                FUNSCALEY (shell, snap_distance))))
            {
              move->guides = guides;

              ligma_guide_tool_start_edit_many (tool, display, guides);

              return;
            }
          else if ((layer = ligma_image_pick_layer (image,
                                                   coords->x,
                                                   coords->y,
                                                   NULL)))
            {
              if (ligma_image_get_floating_selection (image) &&
                  ! ligma_layer_is_floating_sel (layer))
                {
                  /*  If there is a floating selection, and this aint it,
                   *  use the move tool to anchor it.
                   */
                  move->floating_layer =
                    ligma_image_get_floating_selection (image);

                  ligma_tool_control_activate (tool->control);

                  return;
                }
              else
                {
                  GList *new_selected_layers = g_list_prepend (NULL, layer);

                  move->old_selected_layers = g_list_copy (ligma_image_get_selected_layers (image));

                  ligma_image_set_selected_layers (image, new_selected_layers);
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
    case LIGMA_TRANSFORM_TYPE_PATH:
      {
        selected_items = ligma_image_get_selected_vectors (image);
        selected_items = g_list_copy (selected_items);

        translate_mode = LIGMA_TRANSLATE_MODE_VECTORS;

        if (! selected_items)
          {
            null_message = _("There are no paths to move.");
          }
        else
          {
            gint n_items = 0;

            for (iter = selected_items; iter; iter = iter->next)
              {
                if (! ligma_item_is_position_locked (iter->data, &locked_item))
                  n_items++;
              }

            if (n_items == 0)
              locked_message = _("All selected path's position are locked.");
          }
      }
      break;

    case LIGMA_TRANSFORM_TYPE_SELECTION:
      {
        active_item = LIGMA_ITEM (ligma_image_get_mask (image));

        if (ligma_channel_is_empty (LIGMA_CHANNEL (active_item)))
          active_item = NULL;

        translate_mode = LIGMA_TRANSLATE_MODE_MASK;

        if (! active_item)
          {
            /* cannot happen, don't translate this message */
            null_message  = "There is no selection to move.";
          }
        else if (ligma_item_is_position_locked (active_item, &locked_item))
          {
            locked_message = "The selection's position is locked.";
          }
      }
      break;

    case LIGMA_TRANSFORM_TYPE_LAYER:
      {
        selected_items = ligma_image_get_selected_drawables (image);

        if (! selected_items)
          {
            null_message = _("There is no layer to move.");
          }
        else if (LIGMA_IS_LAYER_MASK (selected_items->data))
          {
            g_return_if_fail (g_list_length (selected_items) == 1);

            translate_mode = LIGMA_TRANSLATE_MODE_LAYER_MASK;

            if (ligma_item_is_position_locked (selected_items->data, &locked_item))
              locked_message = _("The selected layer's position is locked.");
            else if (ligma_item_is_content_locked (selected_items->data, NULL))
              locked_message = _("The selected layer's pixels are locked.");
          }
        else if (LIGMA_IS_CHANNEL (selected_items->data))
          {
            translate_mode = LIGMA_TRANSLATE_MODE_CHANNEL;

            for (iter = selected_items; iter; iter = iter->next)
              if (ligma_item_is_position_locked (iter->data, &locked_item) ||
                  ligma_item_is_content_locked (iter->data, &locked_item))
              locked_message = _("A selected channel's position or pixels are locked.");
          }
        else
          {
            translate_mode = LIGMA_TRANSLATE_MODE_LAYER;

            for (iter = selected_items; iter; iter = iter->next)
              if (ligma_item_is_position_locked (iter->data, &locked_item))
              locked_message = _("A selected layer's position is locked.");
          }
      }
      break;

    case LIGMA_TRANSFORM_TYPE_IMAGE:
      g_return_if_reached ();
    }

  if (! active_item && ! selected_items)
    {
      ligma_tool_message_literal (tool, display, null_message);
      ligma_tools_show_tool_options (display->ligma);
      ligma_widget_blink (options->type_box);
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);
      return;
    }
  else if (locked_message)
    {
      ligma_tool_message_literal (tool, display, locked_message);

      if (locked_item == NULL)
        locked_item = active_item ? active_item : selected_items->data;

      ligma_tools_blink_lock_box (display->ligma, locked_item);
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);
      g_list_free (selected_items);
      return;
    }

  ligma_tool_control_activate (tool->control);

  ligma_edit_selection_tool_start (tool, display, coords,
                                  translate_mode,
                                  TRUE);
  g_list_free (selected_items);
}

static void
ligma_move_tool_button_release (LigmaTool              *tool,
                               const LigmaCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               LigmaButtonReleaseType  release_type,
                               LigmaDisplay           *display)
{
  LigmaMoveTool  *move   = LIGMA_MOVE_TOOL (tool);
  LigmaGuiConfig *config = LIGMA_GUI_CONFIG (display->ligma->config);
  LigmaImage     *image  = ligma_display_get_image (display);
  gboolean       flush  = FALSE;

  ligma_tool_control_halt (tool->control);

  if (! config->move_tool_changes_active ||
      (release_type == LIGMA_BUTTON_RELEASE_CANCEL))
    {
      if (move->old_selected_layers)
        {
          ligma_image_set_selected_layers (image, move->old_selected_layers);
          g_clear_pointer (&move->old_selected_layers, g_list_free);

          flush = TRUE;
        }

      if (move->old_selected_vectors)
        {
          ligma_image_set_selected_vectors (image, move->old_selected_vectors);
          g_clear_pointer (&move->old_selected_vectors, g_list_free);

          flush = TRUE;
        }
    }

  if (release_type != LIGMA_BUTTON_RELEASE_CANCEL)
    {
      if (move->floating_layer)
        {
          floating_sel_anchor (move->floating_layer);

          flush = TRUE;
        }
    }

  if (flush)
    ligma_image_flush (image);
}

static gboolean
ligma_move_tool_key_press (LigmaTool    *tool,
                          GdkEventKey *kevent,
                          LigmaDisplay *display)
{
  LigmaMoveOptions *options = LIGMA_MOVE_TOOL_GET_OPTIONS (tool);

  return ligma_edit_selection_tool_translate (tool, kevent,
                                             options->move_type,
                                             display,
                                             &options->type_box);
}

static void
ligma_move_tool_modifier_key (LigmaTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             LigmaDisplay     *display)
{
  LigmaMoveTool    *move    = LIGMA_MOVE_TOOL (tool);
  LigmaMoveOptions *options = LIGMA_MOVE_TOOL_GET_OPTIONS (tool);

  if (key == ligma_get_extend_selection_mask ())
    {
      g_object_set (options, "move-current", ! options->move_current, NULL);
    }
  else if (key == GDK_MOD1_MASK ||
           key == ligma_get_toggle_behavior_mask ())
    {
      LigmaTransformType button_type;

      button_type = options->move_type;

      if (press)
        {
          if (key == (state & (GDK_MOD1_MASK |
                               ligma_get_toggle_behavior_mask ())))
            {
              /*  first modifier pressed  */

              move->saved_type = options->move_type;
            }
        }
      else
        {
          if (! (state & (GDK_MOD1_MASK |
                          ligma_get_toggle_behavior_mask ())))
            {
              /*  last modifier released  */

              button_type = move->saved_type;
            }
        }

      if (state & GDK_MOD1_MASK)
        {
          button_type = LIGMA_TRANSFORM_TYPE_SELECTION;
        }
      else if (state & ligma_get_toggle_behavior_mask ())
        {
          button_type = LIGMA_TRANSFORM_TYPE_PATH;
        }

      if (button_type != options->move_type)
        {
          g_object_set (options, "move-type", button_type, NULL);
        }
    }
}

static void
ligma_move_tool_oper_update (LigmaTool         *tool,
                            const LigmaCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            LigmaDisplay      *display)
{
  LigmaMoveTool     *move    = LIGMA_MOVE_TOOL (tool);
  LigmaMoveOptions  *options = LIGMA_MOVE_TOOL_GET_OPTIONS (tool);
  LigmaDisplayShell *shell   = ligma_display_get_shell (display);
  LigmaImage        *image   = ligma_display_get_image (display);
  GList            *guides  = NULL;

  if (options->move_type == LIGMA_TRANSFORM_TYPE_LAYER &&
      ! options->move_current                         &&
      ligma_display_shell_get_show_guides (shell)      &&
      proximity)
    {
      gint snap_distance = display->config->snap_distance;

      guides = ligma_image_pick_guides (image, coords->x, coords->y,
                                       FUNSCALEX (shell, snap_distance),
                                       FUNSCALEY (shell, snap_distance));
    }

  if (ligma_g_list_compare (guides, move->guides))
    {
      LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

      ligma_draw_tool_pause (draw_tool);

      if (ligma_draw_tool_is_active (draw_tool) &&
          draw_tool->display != display)
        ligma_draw_tool_stop (draw_tool);

      g_clear_pointer (&move->guides, g_list_free);

      move->guides = guides;

      if (! ligma_draw_tool_is_active (draw_tool))
        ligma_draw_tool_start (draw_tool, display);

      ligma_draw_tool_resume (draw_tool);
    }
  else
    {
      g_list_free (guides);
    }
}

static void
ligma_move_tool_cursor_update (LigmaTool         *tool,
                              const LigmaCoords *coords,
                              GdkModifierType   state,
                              LigmaDisplay      *display)
{
  LigmaMoveOptions    *options       = LIGMA_MOVE_TOOL_GET_OPTIONS (tool);
  LigmaDisplayShell   *shell         = ligma_display_get_shell (display);
  LigmaImage          *image         = ligma_display_get_image (display);
  LigmaCursorType      cursor        = LIGMA_CURSOR_MOUSE;
  LigmaToolCursorType  tool_cursor   = LIGMA_TOOL_CURSOR_MOVE;
  LigmaCursorModifier  modifier      = LIGMA_CURSOR_MODIFIER_NONE;
  gint                snap_distance = display->config->snap_distance;

  if (options->move_type == LIGMA_TRANSFORM_TYPE_PATH)
    {
      tool_cursor = LIGMA_TOOL_CURSOR_PATHS;
      modifier    = LIGMA_CURSOR_MODIFIER_MOVE;

      if (options->move_current)
        {
          GList *selected = ligma_image_get_selected_vectors (image);
          GList *iter;
          gint   n_items = 0;

          for (iter = selected; iter; iter = iter->next)
            {
              if (! ligma_item_is_position_locked (iter->data, NULL))
                n_items++;
            }

          if (n_items == 0)
            modifier = LIGMA_CURSOR_MODIFIER_BAD;
        }
      else
        {
          if (ligma_image_pick_vectors (image,
                                       coords->x, coords->y,
                                       FUNSCALEX (shell, snap_distance),
                                       FUNSCALEY (shell, snap_distance)))
            {
              tool_cursor = LIGMA_TOOL_CURSOR_HAND;
            }
          else
            {
              modifier = LIGMA_CURSOR_MODIFIER_BAD;
            }
        }
    }
  else if (options->move_type == LIGMA_TRANSFORM_TYPE_SELECTION)
    {
      tool_cursor = LIGMA_TOOL_CURSOR_RECT_SELECT;
      modifier    = LIGMA_CURSOR_MODIFIER_MOVE;

      if (ligma_channel_is_empty (ligma_image_get_mask (image)))
        modifier = LIGMA_CURSOR_MODIFIER_BAD;
    }
  else if (options->move_current)
    {
      GList *items   = ligma_image_get_selected_drawables (image);
      GList *iter;
      gint   n_items = 0;

      for (iter = items; iter; iter = iter->next)
        if (! ligma_item_is_position_locked (iter->data, NULL))
          n_items++;
      if (n_items == 0)
        modifier = LIGMA_CURSOR_MODIFIER_BAD;

      g_list_free (items);
    }
  else
    {
      LigmaLayer  *layer;

      if (ligma_display_shell_get_show_guides (shell) &&
          ligma_image_pick_guide (image, coords->x, coords->y,
                                 FUNSCALEX (shell, snap_distance),
                                 FUNSCALEY (shell, snap_distance)))
        {
          tool_cursor = LIGMA_TOOL_CURSOR_HAND;
          modifier    = LIGMA_CURSOR_MODIFIER_MOVE;
        }
      else if ((layer = ligma_image_pick_layer (image,
                                               coords->x, coords->y,
                                               NULL)))
        {
          /*  if there is a floating selection, and this aint it...  */
          if (ligma_image_get_floating_selection (image) &&
              ! ligma_layer_is_floating_sel (layer))
            {
              tool_cursor = LIGMA_TOOL_CURSOR_MOVE;
              modifier    = LIGMA_CURSOR_MODIFIER_ANCHOR;
            }
          else if (ligma_item_is_position_locked (LIGMA_ITEM (layer), NULL))
            {
              modifier = LIGMA_CURSOR_MODIFIER_BAD;
            }
          else if (! g_list_find (ligma_image_get_selected_layers (image), layer))
            {
              tool_cursor = LIGMA_TOOL_CURSOR_HAND;
              modifier    = LIGMA_CURSOR_MODIFIER_MOVE;
            }
        }
      else
        {
          modifier = LIGMA_CURSOR_MODIFIER_BAD;
        }
    }

  ligma_tool_control_set_cursor          (tool->control, cursor);
  ligma_tool_control_set_tool_cursor     (tool->control, tool_cursor);
  ligma_tool_control_set_cursor_modifier (tool->control, modifier);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
ligma_move_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaMoveTool *move = LIGMA_MOVE_TOOL (draw_tool);
  GList        *iter;

  for (iter = move->guides; iter; iter = g_list_next (iter))
    {
      LigmaGuide      *guide = iter->data;
      LigmaCanvasItem *item;
      LigmaGuideStyle  style;

      style = ligma_guide_get_style (guide);

      item = ligma_draw_tool_add_guide (draw_tool,
                                       ligma_guide_get_orientation (guide),
                                       ligma_guide_get_position (guide),
                                       style);
      ligma_canvas_item_set_highlight (item, TRUE);
    }
}
