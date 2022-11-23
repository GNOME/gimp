/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmacagetool.c
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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
#include <gdk/gdkkeysyms.h>

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmaguiconfig.h"

#include "gegl/ligma-gegl-utils.h"

#include "operations/ligmacageconfig.h"

#include "core/ligma.h"
#include "core/ligmadrawablefilter.h"
#include "core/ligmaerror.h"
#include "core/ligmaimage.h"
#include "core/ligmaitem.h"
#include "core/ligmaprogress.h"
#include "core/ligmaprojection.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmacanvasitem.h"
#include "display/ligmadisplay.h"

#include "ligmacagetool.h"
#include "ligmacageoptions.h"
#include "ligmatoolcontrol.h"
#include "ligmatools-utils.h"

#include "ligma-intl.h"


/* XXX: if this state list is updated, in particular if for some reason,
   a new CAGE_STATE_* was to be inserted after CAGE_STATE_CLOSING, check
   if the function ligma_cage_tool_is_complete() has to be updated.
   Current algorithm is that all DEFORM_* states are complete states,
   and all CAGE_* states are incomplete states. */
enum
{
  CAGE_STATE_INIT,
  CAGE_STATE_WAIT,
  CAGE_STATE_MOVE_HANDLE,
  CAGE_STATE_SELECTING,
  CAGE_STATE_CLOSING,
  DEFORM_STATE_WAIT,
  DEFORM_STATE_MOVE_HANDLE,
  DEFORM_STATE_SELECTING
};


static gboolean   ligma_cage_tool_initialize         (LigmaTool              *tool,
                                                     LigmaDisplay           *display,
                                                     GError               **error);
static void       ligma_cage_tool_control            (LigmaTool              *tool,
                                                     LigmaToolAction         action,
                                                     LigmaDisplay           *display);
static void       ligma_cage_tool_button_press       (LigmaTool              *tool,
                                                     const LigmaCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     LigmaButtonPressType    press_type,
                                                     LigmaDisplay           *display);
static void       ligma_cage_tool_button_release     (LigmaTool              *tool,
                                                     const LigmaCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     LigmaButtonReleaseType  release_type,
                                                     LigmaDisplay           *display);
static void       ligma_cage_tool_motion             (LigmaTool              *tool,
                                                     const LigmaCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     LigmaDisplay           *display);
static gboolean   ligma_cage_tool_key_press          (LigmaTool              *tool,
                                                     GdkEventKey           *kevent,
                                                     LigmaDisplay           *display);
static void       ligma_cage_tool_cursor_update      (LigmaTool              *tool,
                                                     const LigmaCoords      *coords,
                                                     GdkModifierType        state,
                                                     LigmaDisplay           *display);
static void       ligma_cage_tool_oper_update        (LigmaTool              *tool,
                                                     const LigmaCoords      *coords,
                                                     GdkModifierType        state,
                                                     gboolean               proximity,
                                                     LigmaDisplay           *display);
static void       ligma_cage_tool_options_notify     (LigmaTool              *tool,
                                                     LigmaToolOptions       *options,
                                                     const GParamSpec      *pspec);

static void       ligma_cage_tool_draw               (LigmaDrawTool          *draw_tool);

static void       ligma_cage_tool_start              (LigmaCageTool          *ct,
                                                     LigmaDisplay           *display);
static void       ligma_cage_tool_halt               (LigmaCageTool          *ct);
static void       ligma_cage_tool_commit             (LigmaCageTool          *ct);

static gint       ligma_cage_tool_is_on_handle       (LigmaCageTool          *ct,
                                                     LigmaDrawTool          *draw_tool,
                                                     LigmaDisplay           *display,
                                                     gdouble                x,
                                                     gdouble                y,
                                                     gint                   handle_size);
static gint       ligma_cage_tool_is_on_edge         (LigmaCageTool          *ct,
                                                     gdouble                x,
                                                     gdouble                y,
                                                     gint                   handle_size);

static gboolean   ligma_cage_tool_is_complete        (LigmaCageTool          *ct);
static void       ligma_cage_tool_remove_last_handle (LigmaCageTool          *ct);
static void       ligma_cage_tool_compute_coef       (LigmaCageTool          *ct);
static void       ligma_cage_tool_create_filter      (LigmaCageTool          *ct);
static void       ligma_cage_tool_filter_flush       (LigmaDrawableFilter    *filter,
                                                     LigmaTool              *tool);
static void       ligma_cage_tool_filter_update      (LigmaCageTool          *ct);

static void       ligma_cage_tool_create_render_node (LigmaCageTool          *ct);
static void       ligma_cage_tool_render_node_update (LigmaCageTool          *ct);


G_DEFINE_TYPE (LigmaCageTool, ligma_cage_tool, LIGMA_TYPE_DRAW_TOOL)

#define parent_class ligma_cage_tool_parent_class


void
ligma_cage_tool_register (LigmaToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (LIGMA_TYPE_CAGE_TOOL,
                LIGMA_TYPE_CAGE_OPTIONS,
                ligma_cage_options_gui,
                0,
                "ligma-cage-tool",
                _("Cage Transform"),
                _("Cage Transform: Deform a selection with a cage"),
                N_("_Cage Transform"), "<shift>G",
                NULL, LIGMA_HELP_TOOL_CAGE,
                LIGMA_ICON_TOOL_CAGE,
                data);
}

static void
ligma_cage_tool_class_init (LigmaCageToolClass *klass)
{
  LigmaToolClass     *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);

  tool_class->initialize     = ligma_cage_tool_initialize;
  tool_class->control        = ligma_cage_tool_control;
  tool_class->button_press   = ligma_cage_tool_button_press;
  tool_class->button_release = ligma_cage_tool_button_release;
  tool_class->key_press      = ligma_cage_tool_key_press;
  tool_class->motion         = ligma_cage_tool_motion;
  tool_class->cursor_update  = ligma_cage_tool_cursor_update;
  tool_class->oper_update    = ligma_cage_tool_oper_update;
  tool_class->options_notify = ligma_cage_tool_options_notify;

  draw_tool_class->draw      = ligma_cage_tool_draw;
}

static void
ligma_cage_tool_init (LigmaCageTool *self)
{
  LigmaTool *tool = LIGMA_TOOL (self);

  ligma_tool_control_set_preserve    (tool->control, FALSE);
  ligma_tool_control_set_dirty_mask  (tool->control,
                                     LIGMA_DIRTY_IMAGE           |
                                     LIGMA_DIRTY_IMAGE_STRUCTURE |
                                     LIGMA_DIRTY_DRAWABLE        |
                                     LIGMA_DIRTY_SELECTION       |
                                     LIGMA_DIRTY_ACTIVE_DRAWABLE);
  ligma_tool_control_set_wants_click (tool->control, TRUE);
  ligma_tool_control_set_precision   (tool->control,
                                     LIGMA_CURSOR_PRECISION_SUBPIXEL);
  ligma_tool_control_set_tool_cursor (tool->control,
                                     LIGMA_TOOL_CURSOR_PERSPECTIVE);

  self->config          = g_object_new (LIGMA_TYPE_CAGE_CONFIG, NULL);
  self->hovering_handle = -1;
  self->tool_state      = CAGE_STATE_INIT;
}

static gboolean
ligma_cage_tool_initialize (LigmaTool     *tool,
                           LigmaDisplay  *display,
                           GError      **error)
{
  LigmaGuiConfig *config      = LIGMA_GUI_CONFIG (display->ligma->config);
  LigmaImage     *image       = ligma_display_get_image (display);
  LigmaItem      *locked_item = NULL;
  GList         *drawables   = ligma_image_get_selected_drawables (image);
  LigmaDrawable  *drawable;

  if (g_list_length (drawables) != 1)
    {
      if (g_list_length (drawables) > 1)
        g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                                   _("Cannot modify multiple layers. Select only one layer."));
      else
        g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED, _("No selected drawables."));

      g_list_free (drawables);
      return FALSE;
    }

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
                           _("The selected item's pixels are locked."));
      if (error)
        ligma_tools_blink_lock_box (display->ligma, locked_item);
      return FALSE;
    }

  if (! ligma_item_is_visible (LIGMA_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("The active item is not visible."));
      return FALSE;
    }

  ligma_cage_tool_start (LIGMA_CAGE_TOOL (tool), display);

  return TRUE;
}

static void
ligma_cage_tool_control (LigmaTool       *tool,
                        LigmaToolAction  action,
                        LigmaDisplay    *display)
{
  LigmaCageTool *ct = LIGMA_CAGE_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_cage_tool_halt (ct);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      ligma_cage_tool_commit (ct);
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_cage_tool_button_press (LigmaTool            *tool,
                             const LigmaCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             LigmaButtonPressType  press_type,
                             LigmaDisplay         *display)
{
  LigmaCageTool *ct        = LIGMA_CAGE_TOOL (tool);
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);
  gint          handle    = -1;
  gint          edge      = -1;

  ligma_tool_control_activate (tool->control);

  if (ct->config)
    {
      handle = ligma_cage_tool_is_on_handle (ct,
                                            draw_tool,
                                            display,
                                            coords->x,
                                            coords->y,
                                            LIGMA_TOOL_HANDLE_SIZE_CIRCLE);
      edge = ligma_cage_tool_is_on_edge (ct,
                                        coords->x,
                                        coords->y,
                                        LIGMA_TOOL_HANDLE_SIZE_CIRCLE);
    }

  ct->movement_start_x = coords->x;
  ct->movement_start_y = coords->y;

  switch (ct->tool_state)
    {
    case CAGE_STATE_INIT:
      /* No handle yet, we add the first one and switch the tool to
       * moving handle state.
       */
      ligma_cage_config_add_cage_point (ct->config,
                                       coords->x - ct->offset_x,
                                       coords->y - ct->offset_y);
      ligma_cage_config_select_point (ct->config, 0);
      ct->tool_state = CAGE_STATE_MOVE_HANDLE;
      break;

    case CAGE_STATE_WAIT:
      if (handle == -1 && edge <= 0)
        {
          /* User clicked on the background, we add a new handle
           * and move it
           */
          ligma_cage_config_add_cage_point (ct->config,
                                           coords->x - ct->offset_x,
                                           coords->y - ct->offset_y);
          ligma_cage_config_select_point (ct->config,
                                         ligma_cage_config_get_n_points (ct->config) - 1);
          ct->tool_state = CAGE_STATE_MOVE_HANDLE;
        }
      else if (handle == 0 && ligma_cage_config_get_n_points (ct->config) > 2)
        {
          /* User clicked on the first handle, we wait for
           * release for closing the cage and switching to
           * deform if possible
           */
          ligma_cage_config_select_point (ct->config, 0);
          ct->tool_state = CAGE_STATE_CLOSING;
        }
      else if (handle >= 0)
        {
          /* User clicked on a handle, so we move it */

          if (state & ligma_get_extend_selection_mask ())
            {
              /* Multiple selection */

              ligma_cage_config_toggle_point_selection (ct->config, handle);
            }
          else
            {
              /* New selection */

              if (! ligma_cage_config_point_is_selected (ct->config, handle))
                {
                  ligma_cage_config_select_point (ct->config, handle);
                }
            }

          ct->tool_state = CAGE_STATE_MOVE_HANDLE;
        }
      else if (edge > 0)
        {
          /* User clicked on an edge, we add a new handle here and select it */

          ligma_cage_config_insert_cage_point (ct->config, edge,
                                              coords->x, coords->y);
          ligma_cage_config_select_point (ct->config, edge);
          ct->tool_state = CAGE_STATE_MOVE_HANDLE;
        }
      break;

    case DEFORM_STATE_WAIT:
      if (handle == -1)
        {
          /* User clicked on the background, we start a rubber band
           * selection
           */
          ct->selection_start_x = coords->x;
          ct->selection_start_y = coords->y;
          ct->tool_state = DEFORM_STATE_SELECTING;
        }

      if (handle >= 0)
        {
          /* User clicked on a handle, so we move it */

          if (state & ligma_get_extend_selection_mask ())
            {
              /* Multiple selection */

              ligma_cage_config_toggle_point_selection (ct->config, handle);
            }
          else
            {
              /* New selection */

              if (! ligma_cage_config_point_is_selected (ct->config, handle))
                {
                  ligma_cage_config_select_point (ct->config, handle);
                }
            }

          ct->tool_state = DEFORM_STATE_MOVE_HANDLE;
        }
      break;
    }
}

void
ligma_cage_tool_button_release (LigmaTool              *tool,
                               const LigmaCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               LigmaButtonReleaseType  release_type,
                               LigmaDisplay           *display)
{
  LigmaCageTool    *ct      = LIGMA_CAGE_TOOL (tool);
  LigmaCageOptions *options = LIGMA_CAGE_TOOL_GET_OPTIONS (ct);

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (ct));

  ligma_tool_control_halt (tool->control);

  if (release_type == LIGMA_BUTTON_RELEASE_CANCEL)
    {
      /* Cancelling */

      switch (ct->tool_state)
        {
        case CAGE_STATE_CLOSING:
          ct->tool_state = CAGE_STATE_WAIT;
          break;

        case CAGE_STATE_MOVE_HANDLE:
          ligma_cage_config_remove_last_cage_point (ct->config);
          ct->tool_state = CAGE_STATE_WAIT;
          break;

        case CAGE_STATE_SELECTING:
          ct->tool_state = CAGE_STATE_WAIT;
          break;

        case DEFORM_STATE_MOVE_HANDLE:
          ligma_cage_tool_filter_update (ct);
          ct->tool_state = DEFORM_STATE_WAIT;
          break;

        case DEFORM_STATE_SELECTING:
          ct->tool_state = DEFORM_STATE_WAIT;
          break;
        }

      ligma_cage_config_reset_displacement (ct->config);
    }
  else
    {
      /* Normal release */

      switch (ct->tool_state)
        {
        case CAGE_STATE_CLOSING:
          ct->dirty_coef = TRUE;
          ligma_cage_config_commit_displacement (ct->config);

          if (release_type == LIGMA_BUTTON_RELEASE_CLICK)
            g_object_set (options, "cage-mode", LIGMA_CAGE_MODE_DEFORM, NULL);
          break;

        case CAGE_STATE_MOVE_HANDLE:
          ct->dirty_coef = TRUE;
          ct->tool_state = CAGE_STATE_WAIT;
          ligma_cage_config_commit_displacement (ct->config);
          break;

        case CAGE_STATE_SELECTING:
          {
            GeglRectangle area =
              { MIN (ct->selection_start_x, coords->x) - ct->offset_x,
                MIN (ct->selection_start_y, coords->y) - ct->offset_y,
                ABS (ct->selection_start_x - coords->x),
                ABS (ct->selection_start_y - coords->y) };

            if (state & ligma_get_extend_selection_mask ())
              {
                ligma_cage_config_select_add_area (ct->config,
                                                  LIGMA_CAGE_MODE_CAGE_CHANGE,
                                                  area);
              }
            else
              {
                ligma_cage_config_select_area (ct->config,
                                              LIGMA_CAGE_MODE_CAGE_CHANGE,
                                              area);
              }

            ct->tool_state = CAGE_STATE_WAIT;
          }
          break;

        case DEFORM_STATE_MOVE_HANDLE:
          ct->tool_state = DEFORM_STATE_WAIT;
          ligma_cage_config_commit_displacement (ct->config);
          gegl_node_set (ct->cage_node,
                         "config", ct->config,
                         NULL);
          ligma_cage_tool_filter_update (ct);
          break;

        case DEFORM_STATE_SELECTING:
          {
            GeglRectangle area =
              { MIN (ct->selection_start_x, coords->x) - ct->offset_x,
                MIN (ct->selection_start_y, coords->y) - ct->offset_y,
                ABS (ct->selection_start_x - coords->x),
                ABS (ct->selection_start_y - coords->y) };

            if (state & ligma_get_extend_selection_mask ())
              {
                ligma_cage_config_select_add_area (ct->config,
                                                  LIGMA_CAGE_MODE_DEFORM, area);
              }
            else
              {
                ligma_cage_config_select_area (ct->config,
                                              LIGMA_CAGE_MODE_DEFORM, area);
              }

            ct->tool_state = DEFORM_STATE_WAIT;
          }
          break;
        }
    }

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

static void
ligma_cage_tool_motion (LigmaTool         *tool,
                       const LigmaCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       LigmaDisplay      *display)
{
  LigmaCageTool    *ct       = LIGMA_CAGE_TOOL (tool);
  LigmaCageOptions *options  = LIGMA_CAGE_TOOL_GET_OPTIONS (ct);

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  ct->cursor_x = coords->x;
  ct->cursor_y = coords->y;

  switch (ct->tool_state)
    {
    case CAGE_STATE_MOVE_HANDLE:
    case CAGE_STATE_CLOSING:
    case DEFORM_STATE_MOVE_HANDLE:
      ligma_cage_config_add_displacement (ct->config,
                                         options->cage_mode,
                                         ct->cursor_x - ct->movement_start_x,
                                         ct->cursor_y - ct->movement_start_y);
      break;
    }

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

static gboolean
ligma_cage_tool_key_press (LigmaTool    *tool,
                          GdkEventKey *kevent,
                          LigmaDisplay *display)
{
  LigmaCageTool *ct = LIGMA_CAGE_TOOL (tool);

  if (! ct->config)
    return FALSE;

  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      if (ct->tool_state == CAGE_STATE_WAIT)
        {
          if (ligma_cage_config_get_n_points (ct->config) != 0)
            ligma_cage_tool_remove_last_handle (ct);
        }
      else if (ct->tool_state == DEFORM_STATE_WAIT)
        {
          ligma_cage_config_remove_selected_points (ct->config);

          /* if the cage have less than 3 handles, we reopen it */
          if (ligma_cage_config_get_n_points (ct->config) <= 2)
            {
              ct->tool_state = CAGE_STATE_WAIT;
            }

          ligma_cage_tool_compute_coef (ct);
          ligma_cage_tool_render_node_update (ct);
        }
      return TRUE;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      if (! ligma_cage_tool_is_complete (ct) &&
          ligma_cage_config_get_n_points (ct->config) > 2)
        {
          g_object_set (ligma_tool_get_options (tool),
                        "cage-mode", LIGMA_CAGE_MODE_DEFORM,
                        NULL);
        }
      else if (ct->tool_state == DEFORM_STATE_WAIT)
        {
          ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, display);
        }
      return TRUE;

    case GDK_KEY_Escape:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static void
ligma_cage_tool_oper_update (LigmaTool         *tool,
                            const LigmaCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            LigmaDisplay      *display)
{
  LigmaCageTool *ct        = LIGMA_CAGE_TOOL (tool);
  LigmaDrawTool *draw_tool = LIGMA_DRAW_TOOL (tool);

  if (ct->config)
    {
      ct->hovering_handle = ligma_cage_tool_is_on_handle (ct,
                                                         draw_tool,
                                                         display,
                                                         coords->x,
                                                         coords->y,
                                                         LIGMA_TOOL_HANDLE_SIZE_CIRCLE);

      ct->hovering_edge = ligma_cage_tool_is_on_edge (ct,
                                                     coords->x,
                                                     coords->y,
                                                     LIGMA_TOOL_HANDLE_SIZE_CIRCLE);
    }

  ligma_draw_tool_pause (draw_tool);

  ct->cursor_x = coords->x;
  ct->cursor_y = coords->y;

  ligma_draw_tool_resume (draw_tool);
}

static void
ligma_cage_tool_cursor_update (LigmaTool         *tool,
                              const LigmaCoords *coords,
                              GdkModifierType   state,
                              LigmaDisplay      *display)
{
  LigmaCageTool       *ct       = LIGMA_CAGE_TOOL (tool);
  LigmaCageOptions    *options  = LIGMA_CAGE_TOOL_GET_OPTIONS (ct);
  LigmaGuiConfig      *config   = LIGMA_GUI_CONFIG (display->ligma->config);
  LigmaCursorModifier  modifier = LIGMA_CURSOR_MODIFIER_PLUS;

  if (tool->display)
    {
      if (ct->hovering_handle != -1)
        {
          modifier = LIGMA_CURSOR_MODIFIER_MOVE;
        }
      else if (ct->hovering_edge  != -1 &&
               options->cage_mode == LIGMA_CAGE_MODE_CAGE_CHANGE)
        {
          modifier = LIGMA_CURSOR_MODIFIER_PLUS;
        }
      else
        {
          if (ligma_cage_tool_is_complete (ct))
            modifier = LIGMA_CURSOR_MODIFIER_BAD;
        }
    }
  else
    {
      LigmaImage    *image     = ligma_display_get_image (display);
      GList        *drawables = ligma_image_get_selected_drawables (image);
      LigmaDrawable *drawable  = NULL;

      if (g_list_length (drawables) == 1)
        drawable = drawables->data;
      g_list_free (drawables);

      if (! drawable                                               ||
          ligma_viewable_get_children (LIGMA_VIEWABLE (drawable))    ||
          ligma_item_is_content_locked (LIGMA_ITEM (drawable), NULL) ||
          ! (ligma_item_is_visible (LIGMA_ITEM (drawable)) ||
             config->edit_non_visible))
        {
          modifier = LIGMA_CURSOR_MODIFIER_BAD;
        }
    }

  ligma_tool_control_set_cursor_modifier (tool->control, modifier);

  LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
ligma_cage_tool_options_notify (LigmaTool         *tool,
                               LigmaToolOptions  *options,
                               const GParamSpec *pspec)
{
  LigmaCageTool *ct = LIGMA_CAGE_TOOL (tool);

  LIGMA_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! tool->display)
    return;

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  if (strcmp (pspec->name, "cage-mode") == 0)
    {
      LigmaCageMode mode;

      g_object_get (options,
                    "cage-mode", &mode,
                    NULL);

      if (mode == LIGMA_CAGE_MODE_DEFORM)
        {
          /* switch to deform mode */

          if (ligma_cage_config_get_n_points (ct->config) > 2)
            {
              ligma_cage_config_reset_displacement (ct->config);
              ligma_cage_config_reverse_cage_if_needed (ct->config);
              ligma_tool_push_status (tool, tool->display,
                                     _("Press ENTER to commit the transform"));
              ct->tool_state = DEFORM_STATE_WAIT;

              if (! ct->render_node)
                {
                  ligma_cage_tool_create_render_node (ct);
                }

              if (ct->dirty_coef)
                {
                  ligma_cage_tool_compute_coef (ct);
                  ligma_cage_tool_render_node_update (ct);
                }

              if (! ct->filter)
                ligma_cage_tool_create_filter (ct);

              ligma_cage_tool_filter_update (ct);
            }
          else
            {
              g_object_set (options,
                            "cage-mode", LIGMA_CAGE_MODE_CAGE_CHANGE,
                            NULL);
            }
        }
      else
        {
          /* switch to edit mode */
          if (ct->filter)
            {
              ligma_drawable_filter_abort (ct->filter);

              ligma_tool_pop_status (tool, tool->display);
              ct->tool_state = CAGE_STATE_WAIT;
            }
        }
    }
  else if (strcmp  (pspec->name, "fill-plain-color") == 0)
    {
      if (ct->tool_state == DEFORM_STATE_WAIT)
        {
          ligma_cage_tool_render_node_update (ct);
          ligma_cage_tool_filter_update (ct);
        }
    }

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

static void
ligma_cage_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaCageTool    *ct        = LIGMA_CAGE_TOOL (draw_tool);
  LigmaCageOptions *options   = LIGMA_CAGE_TOOL_GET_OPTIONS (ct);
  LigmaCageConfig  *config    = ct->config;
  LigmaCanvasGroup *stroke_group;
  gint             n_vertices;
  gint             i;
  LigmaHandleType   handle;

  n_vertices = ligma_cage_config_get_n_points (config);

  if (n_vertices == 0)
    return;

  if (ct->tool_state == CAGE_STATE_INIT)
    return;

  stroke_group = ligma_draw_tool_add_stroke_group (draw_tool);

  ligma_draw_tool_push_group (draw_tool, stroke_group);

  /* If needed, draw line to the cursor. */
  if (! ligma_cage_tool_is_complete (ct))
    {
      LigmaVector2 last_point;

      last_point = ligma_cage_config_get_point_coordinate (ct->config,
                                                          options->cage_mode,
                                                          n_vertices - 1);

      ligma_draw_tool_add_line (draw_tool,
                               last_point.x + ct->offset_x,
                               last_point.y + ct->offset_y,
                               ct->cursor_x,
                               ct->cursor_y);
    }

  ligma_draw_tool_pop_group (draw_tool);

  /* Draw the cage with handles. */
  for (i = 0; i < n_vertices; i++)
    {
      LigmaCanvasItem *item;
      LigmaVector2     point1, point2;

      point1 = ligma_cage_config_get_point_coordinate (ct->config,
                                                      options->cage_mode,
                                                      i);
      point1.x += ct->offset_x;
      point1.y += ct->offset_y;

      if (i > 0 || ligma_cage_tool_is_complete (ct))
        {
          gint index_point2;

          if (i == 0)
            index_point2 = n_vertices - 1;
          else
            index_point2 = i - 1;

          point2 = ligma_cage_config_get_point_coordinate (ct->config,
                                                          options->cage_mode,
                                                          index_point2);
          point2.x += ct->offset_x;
          point2.y += ct->offset_y;

          if (i != ct->hovering_edge ||
              ligma_cage_tool_is_complete (ct))
            {
              ligma_draw_tool_push_group (draw_tool, stroke_group);
            }

          item = ligma_draw_tool_add_line (draw_tool,
                                          point1.x,
                                          point1.y,
                                          point2.x,
                                          point2.y);

          if (i == ct->hovering_edge &&
              ! ligma_cage_tool_is_complete (ct))
            {
              ligma_canvas_item_set_highlight (item, TRUE);
            }
          else
            {
              ligma_draw_tool_pop_group (draw_tool);
            }
        }

      if (ligma_cage_config_point_is_selected (ct->config, i))
        {
          if (i == ct->hovering_handle)
            handle = LIGMA_HANDLE_FILLED_SQUARE;
          else
            handle = LIGMA_HANDLE_SQUARE;
        }
      else
        {
          if (i == ct->hovering_handle)
            handle = LIGMA_HANDLE_FILLED_CIRCLE;
          else
            handle = LIGMA_HANDLE_CIRCLE;
        }

      item = ligma_draw_tool_add_handle (draw_tool,
                                        handle,
                                        point1.x,
                                        point1.y,
                                        LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                        LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                        LIGMA_HANDLE_ANCHOR_CENTER);

      if (i == ct->hovering_handle)
        ligma_canvas_item_set_highlight (item, TRUE);
    }

  if (ct->tool_state == DEFORM_STATE_SELECTING ||
      ct->tool_state == CAGE_STATE_SELECTING)
    {
      ligma_draw_tool_add_rectangle (draw_tool,
                                    FALSE,
                                    MIN (ct->selection_start_x, ct->cursor_x),
                                    MIN (ct->selection_start_y, ct->cursor_y),
                                    ABS (ct->selection_start_x - ct->cursor_x),
                                    ABS (ct->selection_start_y - ct->cursor_y));
    }
}

static void
ligma_cage_tool_start (LigmaCageTool *ct,
                      LigmaDisplay  *display)
{
  LigmaTool     *tool     = LIGMA_TOOL (ct);
  LigmaImage    *image    = ligma_display_get_image (display);
  GList        *drawables = ligma_image_get_selected_drawables (image);

  g_return_if_fail (g_list_length (drawables) == 1);

  tool->display   = display;
  g_list_free (tool->drawables);
  tool->drawables = drawables;

  g_clear_object (&ct->config);

  g_clear_object (&ct->coef);
  ct->dirty_coef = TRUE;

  if (ct->filter)
    {
      ligma_drawable_filter_abort (ct->filter);
      g_clear_object (&ct->filter);
    }

  if (ct->render_node)
    {
      g_clear_object (&ct->render_node);
      ct->coef_node = NULL;
      ct->cage_node = NULL;
    }

  ct->config          = g_object_new (LIGMA_TYPE_CAGE_CONFIG, NULL);
  ct->hovering_handle = -1;
  ct->hovering_edge   = -1;
  ct->tool_state      = CAGE_STATE_INIT;

  /* Setting up cage offset to convert the cage point coords to
   * drawable coords
   */
  ligma_item_get_offset (LIGMA_ITEM (tool->drawables->data),
                        &ct->offset_x, &ct->offset_y);

  ligma_draw_tool_start (LIGMA_DRAW_TOOL (ct), display);
}

static void
ligma_cage_tool_halt (LigmaCageTool *ct)
{
  LigmaTool *tool = LIGMA_TOOL (ct);

  g_clear_object (&ct->config);
  g_clear_object (&ct->coef);
  g_clear_object (&ct->render_node);
  ct->coef_node = NULL;
  ct->cage_node = NULL;

  if (ct->filter)
    {
      ligma_tool_control_push_preserve (tool->control, TRUE);

      ligma_drawable_filter_abort (ct->filter);
      g_clear_object (&ct->filter);

      ligma_tool_control_pop_preserve (tool->control);

      ligma_image_flush (ligma_display_get_image (tool->display));
    }

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;
  ct->tool_state  = CAGE_STATE_INIT;

  g_object_set (ligma_tool_get_options (tool),
                "cage-mode", LIGMA_CAGE_MODE_CAGE_CHANGE,
                NULL);
}

static void
ligma_cage_tool_commit (LigmaCageTool *ct)
{
  if (ct->filter)
    {
      LigmaTool *tool = LIGMA_TOOL (ct);

      ligma_tool_control_push_preserve (tool->control, TRUE);

      ligma_drawable_filter_commit (ct->filter, LIGMA_PROGRESS (tool), FALSE);
      g_clear_object (&ct->filter);

      ligma_tool_control_pop_preserve (tool->control);

      ligma_image_flush (ligma_display_get_image (tool->display));
    }
}

static gint
ligma_cage_tool_is_on_handle (LigmaCageTool *ct,
                             LigmaDrawTool *draw_tool,
                             LigmaDisplay  *display,
                             gdouble       x,
                             gdouble       y,
                             gint          handle_size)
{
  LigmaCageOptions *options = LIGMA_CAGE_TOOL_GET_OPTIONS (ct);
  LigmaCageConfig  *config  = ct->config;
  gdouble          dist    = G_MAXDOUBLE;
  gint             i;
  LigmaVector2      cage_point;
  guint            n_cage_vertices;

  g_return_val_if_fail (LIGMA_IS_CAGE_TOOL (ct), -1);

  n_cage_vertices = ligma_cage_config_get_n_points (config);

  if (n_cage_vertices == 0)
    return -1;

  for (i = 0; i < n_cage_vertices; i++)
    {
      cage_point = ligma_cage_config_get_point_coordinate (config,
                                                          options->cage_mode,
                                                          i);
      cage_point.x += ct->offset_x;
      cage_point.y += ct->offset_y;

      dist = ligma_draw_tool_calc_distance_square (LIGMA_DRAW_TOOL (draw_tool),
                                                  display,
                                                  x, y,
                                                  cage_point.x,
                                                  cage_point.y);

      if (dist <= SQR (handle_size / 2))
        return i;
    }

  return -1;
}

static gint
ligma_cage_tool_is_on_edge (LigmaCageTool *ct,
                           gdouble       x,
                           gdouble       y,
                           gint          handle_size)
{
  LigmaCageOptions *options = LIGMA_CAGE_TOOL_GET_OPTIONS (ct);
  LigmaCageConfig  *config  = ct->config;
  gint             i;
  guint            n_cage_vertices;
  LigmaVector2      A, B, C, AB, BC, AC;
  gdouble          lAB, lBC, lAC, lEB, lEC;

  g_return_val_if_fail (LIGMA_IS_CAGE_TOOL (ct), -1);

  n_cage_vertices = ligma_cage_config_get_n_points (config);

  if (n_cage_vertices < 2)
    return -1;

  A = ligma_cage_config_get_point_coordinate (config,
                                             options->cage_mode,
                                             n_cage_vertices-1);
  B = ligma_cage_config_get_point_coordinate (config,
                                             options->cage_mode,
                                             0);
  C.x = x;
  C.y = y;

  for (i = 0; i < n_cage_vertices; i++)
    {
      ligma_vector2_sub (&AB, &A, &B);
      ligma_vector2_sub (&BC, &B, &C);
      ligma_vector2_sub (&AC, &A, &C);

      lAB = ligma_vector2_length (&AB);
      lBC = ligma_vector2_length (&BC);
      lAC = ligma_vector2_length (&AC);
      lEB = lAB / 2 + (SQR (lBC) - SQR (lAC)) / (2 * lAB);
      lEC = sqrt (SQR (lBC) - SQR (lEB));

      if ((lEC < handle_size / 2) && (ABS (SQR (lBC) - SQR (lAC)) <= SQR (lAB)))
        return i;

      A = B;
      B = ligma_cage_config_get_point_coordinate (config,
                                                 options->cage_mode,
                                                 (i+1) % n_cage_vertices);
    }

  return -1;
}

static gboolean
ligma_cage_tool_is_complete (LigmaCageTool *ct)
{
  return (ct->tool_state > CAGE_STATE_CLOSING);
}

static void
ligma_cage_tool_remove_last_handle (LigmaCageTool *ct)
{
  LigmaCageConfig *config = ct->config;

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (ct));

  ligma_cage_config_remove_last_cage_point (config);

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (ct));
}

static void
ligma_cage_tool_compute_coef (LigmaCageTool *ct)
{
  LigmaCageConfig *config = ct->config;
  LigmaProgress   *progress;
  const Babl     *format;
  GeglNode       *gegl;
  GeglNode       *input;
  GeglNode       *output;
  GeglProcessor  *processor;
  GeglBuffer     *buffer;
  gdouble         value;

  progress = ligma_progress_start (LIGMA_PROGRESS (ct), FALSE,
                                  _("Computing Cage Coefficients"));

  g_clear_object (&ct->coef);

  format = babl_format_n (babl_type ("float"),
                          ligma_cage_config_get_n_points (config) * 2);


  gegl = gegl_node_new ();

  input = gegl_node_new_child (gegl,
                               "operation", "ligma:cage-coef-calc",
                               "config",    ct->config,
                               NULL);

  output = gegl_node_new_child (gegl,
                                "operation", "gegl:buffer-sink",
                                "buffer",    &buffer,
                                "format",    format,
                                NULL);

  gegl_node_connect_to (input, "output",
                        output, "input");

  processor = gegl_node_new_processor (output, NULL);

  while (gegl_processor_work (processor, &value))
    {
      if (progress)
        ligma_progress_set_value (progress, value);
    }

  if (progress)
    ligma_progress_end (progress);

  g_object_unref (processor);

  ct->coef = buffer;
  g_object_unref (gegl);

  ct->dirty_coef = FALSE;
}

static void
ligma_cage_tool_create_render_node (LigmaCageTool *ct)
{
  LigmaCageOptions *options = LIGMA_CAGE_TOOL_GET_OPTIONS (ct);
  GeglNode        *render;
  GeglNode        *input;
  GeglNode        *output;

  g_return_if_fail (ct->render_node == NULL);
  /* render_node is not supposed to be recreated */

  ct->render_node = gegl_node_new ();

  input  = gegl_node_get_input_proxy  (ct->render_node, "input");
  output = gegl_node_get_output_proxy (ct->render_node, "output");

  ct->coef_node = gegl_node_new_child (ct->render_node,
                                       "operation", "gegl:buffer-source",
                                       "buffer",    ct->coef,
                                       NULL);

  ct->cage_node = gegl_node_new_child (ct->render_node,
                                       "operation",        "ligma:cage-transform",
                                       "config",           ct->config,
                                       "fill-plain-color", options->fill_plain_color,
                                       NULL);

  render = gegl_node_new_child (ct->render_node,
                                "operation", "gegl:map-absolute",
                                NULL);

  gegl_node_connect_to (input,         "output",
                        ct->cage_node, "input");

  gegl_node_connect_to (ct->coef_node, "output",
                        ct->cage_node, "aux");

  gegl_node_connect_to (input,  "output",
                        render, "input");

  gegl_node_connect_to (ct->cage_node, "output",
                        render,        "aux");

  gegl_node_connect_to (render, "output",
                        output, "input");

  ligma_gegl_progress_connect (ct->cage_node, LIGMA_PROGRESS (ct),
                              _("Cage Transform"));
}

static void
ligma_cage_tool_render_node_update (LigmaCageTool *ct)
{
  LigmaCageOptions *options  = LIGMA_CAGE_TOOL_GET_OPTIONS (ct);
  gboolean         fill;
  GeglBuffer      *buffer;

  gegl_node_get (ct->cage_node,
                 "fill-plain-color", &fill,
                 NULL);

  if (fill != options->fill_plain_color)
    {
      gegl_node_set (ct->cage_node,
                     "fill-plain-color", options->fill_plain_color,
                     NULL);
    }

  gegl_node_get (ct->coef_node,
                 "buffer", &buffer,
                 NULL);

  if (buffer != ct->coef)
    {
      gegl_node_set (ct->coef_node,
                     "buffer", ct->coef,
                     NULL);
    }

  if (buffer)
    g_object_unref (buffer);
}

static void
ligma_cage_tool_create_filter (LigmaCageTool *ct)
{
  if (! ct->render_node)
    ligma_cage_tool_create_render_node (ct);

  ct->filter = ligma_drawable_filter_new (LIGMA_TOOL (ct)->drawables->data,
                                         _("Cage transform"),
                                         ct->render_node,
                                         LIGMA_ICON_TOOL_CAGE);
  ligma_drawable_filter_set_region (ct->filter, LIGMA_FILTER_REGION_DRAWABLE);

  g_signal_connect (ct->filter, "flush",
                    G_CALLBACK (ligma_cage_tool_filter_flush),
                    ct);
}

static void
ligma_cage_tool_filter_flush (LigmaDrawableFilter *filter,
                             LigmaTool           *tool)
{
  LigmaImage *image = ligma_display_get_image (tool->display);

  ligma_projection_flush (ligma_image_get_projection (image));
}

static void
ligma_cage_tool_filter_update (LigmaCageTool *ct)
{
  ligma_drawable_filter_apply (ct->filter, NULL);
}
