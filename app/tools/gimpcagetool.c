/* GIMP - The GNU Image Manipulation Program
 *
 * gimpcagetool.c
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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpguiconfig.h"

#include "gegl/gimp-gegl-utils.h"

#include "operations/gimpcageconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable-filters.h"
#include "core/gimpdrawablefilter.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimpprogress.h"
#include "core/gimpprojection.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasitem.h"
#include "display/gimpdisplay.h"

#include "gimpcagetool.h"
#include "gimpcageoptions.h"
#include "gimptoolcontrol.h"
#include "gimptools-utils.h"

#include "gimp-intl.h"


/* XXX: if this state list is updated, in particular if for some reason,
   a new CAGE_STATE_* was to be inserted after CAGE_STATE_CLOSING, check
   if the function gimp_cage_tool_is_complete() has to be updated.
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


static gboolean   gimp_cage_tool_initialize         (GimpTool              *tool,
                                                     GimpDisplay           *display,
                                                     GError               **error);
static void       gimp_cage_tool_control            (GimpTool              *tool,
                                                     GimpToolAction         action,
                                                     GimpDisplay           *display);
static void       gimp_cage_tool_button_press       (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     GimpButtonPressType    press_type,
                                                     GimpDisplay           *display);
static void       gimp_cage_tool_button_release     (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     GimpButtonReleaseType  release_type,
                                                     GimpDisplay           *display);
static void       gimp_cage_tool_motion             (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     GimpDisplay           *display);
static gboolean   gimp_cage_tool_key_press          (GimpTool              *tool,
                                                     GdkEventKey           *kevent,
                                                     GimpDisplay           *display);
static void       gimp_cage_tool_cursor_update      (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     GdkModifierType        state,
                                                     GimpDisplay           *display);
static void       gimp_cage_tool_oper_update        (GimpTool              *tool,
                                                     const GimpCoords      *coords,
                                                     GdkModifierType        state,
                                                     gboolean               proximity,
                                                     GimpDisplay           *display);
static void       gimp_cage_tool_options_notify     (GimpTool              *tool,
                                                     GimpToolOptions       *options,
                                                     const GParamSpec      *pspec);

static void       gimp_cage_tool_draw               (GimpDrawTool          *draw_tool);

static void       gimp_cage_tool_start              (GimpCageTool          *ct,
                                                     GimpDisplay           *display);
static void       gimp_cage_tool_halt               (GimpCageTool          *ct);
static void       gimp_cage_tool_commit             (GimpCageTool          *ct);

static gint       gimp_cage_tool_is_on_handle       (GimpCageTool          *ct,
                                                     GimpDrawTool          *draw_tool,
                                                     GimpDisplay           *display,
                                                     gdouble                x,
                                                     gdouble                y,
                                                     gint                   handle_size);
static gint       gimp_cage_tool_is_on_edge         (GimpCageTool          *ct,
                                                     gdouble                x,
                                                     gdouble                y,
                                                     gint                   handle_size);

static gboolean   gimp_cage_tool_is_complete        (GimpCageTool          *ct);
static void       gimp_cage_tool_remove_last_handle (GimpCageTool          *ct);
static void       gimp_cage_tool_compute_coef       (GimpCageTool          *ct);
static void       gimp_cage_tool_create_filter      (GimpCageTool          *ct);
static void       gimp_cage_tool_filter_flush       (GimpDrawableFilter    *filter,
                                                     GimpTool              *tool);
static void       gimp_cage_tool_filter_update      (GimpCageTool          *ct);

static void       gimp_cage_tool_create_render_node (GimpCageTool          *ct);
static void       gimp_cage_tool_render_node_update (GimpCageTool          *ct);


G_DEFINE_TYPE (GimpCageTool, gimp_cage_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_cage_tool_parent_class


void
gimp_cage_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_CAGE_TOOL,
                GIMP_TYPE_CAGE_OPTIONS,
                gimp_cage_options_gui,
                0,
                "gimp-cage-tool",
                _("Cage Transform"),
                _("Cage Transform: Deform a selection with a cage"),
                N_("_Cage Transform"), "<shift>G",
                NULL, GIMP_HELP_TOOL_CAGE,
                GIMP_ICON_TOOL_CAGE,
                data);
}

static void
gimp_cage_tool_class_init (GimpCageToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->initialize     = gimp_cage_tool_initialize;
  tool_class->control        = gimp_cage_tool_control;
  tool_class->button_press   = gimp_cage_tool_button_press;
  tool_class->button_release = gimp_cage_tool_button_release;
  tool_class->key_press      = gimp_cage_tool_key_press;
  tool_class->motion         = gimp_cage_tool_motion;
  tool_class->cursor_update  = gimp_cage_tool_cursor_update;
  tool_class->oper_update    = gimp_cage_tool_oper_update;
  tool_class->options_notify = gimp_cage_tool_options_notify;

  draw_tool_class->draw      = gimp_cage_tool_draw;
}

static void
gimp_cage_tool_init (GimpCageTool *self)
{
  GimpTool *tool = GIMP_TOOL (self);

  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control,
                                     GIMP_DIRTY_IMAGE           |
                                     GIMP_DIRTY_IMAGE_STRUCTURE |
                                     GIMP_DIRTY_DRAWABLE        |
                                     GIMP_DIRTY_SELECTION       |
                                     GIMP_DIRTY_ACTIVE_DRAWABLE);
  gimp_tool_control_set_wants_click (tool->control, TRUE);
  gimp_tool_control_set_precision   (tool->control,
                                     GIMP_CURSOR_PRECISION_SUBPIXEL);
  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_PERSPECTIVE);

  self->config          = g_object_new (GIMP_TYPE_CAGE_CONFIG, NULL);
  self->hovering_handle = -1;
  self->tool_state      = CAGE_STATE_INIT;
}

static gboolean
gimp_cage_tool_initialize (GimpTool     *tool,
                           GimpDisplay  *display,
                           GError      **error)
{
  GimpGuiConfig *config      = GIMP_GUI_CONFIG (display->gimp->config);
  GimpImage     *image       = gimp_display_get_image (display);
  GimpItem      *locked_item = NULL;
  GList         *drawables   = gimp_image_get_selected_drawables (image);
  GimpDrawable  *drawable;

  if (g_list_length (drawables) != 1)
    {
      if (g_list_length (drawables) > 1)
        g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                                   _("Cannot modify multiple layers. Select only one layer."));
      else
        g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED, _("No selected drawables."));

      g_list_free (drawables);
      return FALSE;
    }

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
                           _("The selected item's pixels are locked."));
      if (error)
        gimp_tools_blink_lock_box (display->gimp, locked_item);
      return FALSE;
    }

  if (! gimp_item_is_visible (GIMP_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("The active item is not visible."));
      return FALSE;
    }

  gimp_cage_tool_start (GIMP_CAGE_TOOL (tool), display);

  return TRUE;
}

static void
gimp_cage_tool_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *display)
{
  GimpCageTool *ct = GIMP_CAGE_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_cage_tool_halt (ct);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_cage_tool_commit (ct);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_cage_tool_button_press (GimpTool            *tool,
                             const GimpCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             GimpButtonPressType  press_type,
                             GimpDisplay         *display)
{
  GimpCageTool *ct        = GIMP_CAGE_TOOL (tool);
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);
  gint          handle    = -1;
  gint          edge      = -1;

  gimp_tool_control_activate (tool->control);

  if (ct->config)
    {
      handle = gimp_cage_tool_is_on_handle (ct,
                                            draw_tool,
                                            display,
                                            coords->x,
                                            coords->y,
                                            GIMP_TOOL_HANDLE_SIZE_CIRCLE);
      edge = gimp_cage_tool_is_on_edge (ct,
                                        coords->x,
                                        coords->y,
                                        GIMP_TOOL_HANDLE_SIZE_CIRCLE);
    }

  ct->movement_start_x = coords->x;
  ct->movement_start_y = coords->y;

  switch (ct->tool_state)
    {
    case CAGE_STATE_INIT:
      /* No handle yet, we add the first one and switch the tool to
       * moving handle state.
       */
      gimp_cage_config_add_cage_point (ct->config,
                                       coords->x - ct->offset_x,
                                       coords->y - ct->offset_y);
      gimp_cage_config_select_point (ct->config, 0);
      ct->tool_state = CAGE_STATE_MOVE_HANDLE;
      break;

    case CAGE_STATE_WAIT:
      if (handle == -1 && edge <= 0)
        {
          /* User clicked on the background, we add a new handle
           * and move it
           */
          gimp_cage_config_add_cage_point (ct->config,
                                           coords->x - ct->offset_x,
                                           coords->y - ct->offset_y);
          gimp_cage_config_select_point (ct->config,
                                         gimp_cage_config_get_n_points (ct->config) - 1);
          ct->tool_state = CAGE_STATE_MOVE_HANDLE;
        }
      else if (handle == 0 && gimp_cage_config_get_n_points (ct->config) > 2)
        {
          /* User clicked on the first handle, we wait for
           * release for closing the cage and switching to
           * deform if possible
           */
          gimp_cage_config_select_point (ct->config, 0);
          ct->tool_state = CAGE_STATE_CLOSING;
        }
      else if (handle >= 0)
        {
          /* User clicked on a handle, so we move it */

          if (state & gimp_get_extend_selection_mask ())
            {
              /* Multiple selection */

              gimp_cage_config_toggle_point_selection (ct->config, handle);
            }
          else
            {
              /* New selection */

              if (! gimp_cage_config_point_is_selected (ct->config, handle))
                {
                  gimp_cage_config_select_point (ct->config, handle);
                }
            }

          ct->tool_state = CAGE_STATE_MOVE_HANDLE;
        }
      else if (edge > 0)
        {
          /* User clicked on an edge, we add a new handle here and select it */

          gimp_cage_config_insert_cage_point (ct->config, edge,
                                              coords->x, coords->y);
          gimp_cage_config_select_point (ct->config, edge);
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

          if (state & gimp_get_extend_selection_mask ())
            {
              /* Multiple selection */

              gimp_cage_config_toggle_point_selection (ct->config, handle);
            }
          else
            {
              /* New selection */

              if (! gimp_cage_config_point_is_selected (ct->config, handle))
                {
                  gimp_cage_config_select_point (ct->config, handle);
                }
            }

          ct->tool_state = DEFORM_STATE_MOVE_HANDLE;
        }
      break;
    }
}

void
gimp_cage_tool_button_release (GimpTool              *tool,
                               const GimpCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type,
                               GimpDisplay           *display)
{
  GimpCageTool    *ct      = GIMP_CAGE_TOOL (tool);
  GimpCageOptions *options = GIMP_CAGE_TOOL_GET_OPTIONS (ct);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (ct));

  gimp_tool_control_halt (tool->control);

  if (release_type == GIMP_BUTTON_RELEASE_CANCEL)
    {
      /* Cancelling */

      switch (ct->tool_state)
        {
        case CAGE_STATE_CLOSING:
          ct->tool_state = CAGE_STATE_WAIT;
          break;

        case CAGE_STATE_MOVE_HANDLE:
          gimp_cage_config_remove_last_cage_point (ct->config);
          ct->tool_state = CAGE_STATE_WAIT;
          break;

        case CAGE_STATE_SELECTING:
          ct->tool_state = CAGE_STATE_WAIT;
          break;

        case DEFORM_STATE_MOVE_HANDLE:
          gimp_cage_tool_filter_update (ct);
          ct->tool_state = DEFORM_STATE_WAIT;
          break;

        case DEFORM_STATE_SELECTING:
          ct->tool_state = DEFORM_STATE_WAIT;
          break;
        }

      gimp_cage_config_reset_displacement (ct->config);
    }
  else
    {
      /* Normal release */

      switch (ct->tool_state)
        {
        case CAGE_STATE_CLOSING:
          ct->dirty_coef = TRUE;
          gimp_cage_config_commit_displacement (ct->config);

          if (release_type == GIMP_BUTTON_RELEASE_CLICK)
            g_object_set (options, "cage-mode", GIMP_CAGE_MODE_DEFORM, NULL);
          break;

        case CAGE_STATE_MOVE_HANDLE:
          ct->dirty_coef = TRUE;
          ct->tool_state = CAGE_STATE_WAIT;
          gimp_cage_config_commit_displacement (ct->config);
          break;

        case CAGE_STATE_SELECTING:
          {
            GeglRectangle area =
              { MIN (ct->selection_start_x, coords->x) - ct->offset_x,
                MIN (ct->selection_start_y, coords->y) - ct->offset_y,
                ABS (ct->selection_start_x - coords->x),
                ABS (ct->selection_start_y - coords->y) };

            if (state & gimp_get_extend_selection_mask ())
              {
                gimp_cage_config_select_add_area (ct->config,
                                                  GIMP_CAGE_MODE_CAGE_CHANGE,
                                                  area);
              }
            else
              {
                gimp_cage_config_select_area (ct->config,
                                              GIMP_CAGE_MODE_CAGE_CHANGE,
                                              area);
              }

            ct->tool_state = CAGE_STATE_WAIT;
          }
          break;

        case DEFORM_STATE_MOVE_HANDLE:
          ct->tool_state = DEFORM_STATE_WAIT;
          gimp_cage_config_commit_displacement (ct->config);
          gegl_node_set (ct->cage_node,
                         "config", ct->config,
                         NULL);
          gimp_cage_tool_filter_update (ct);
          break;

        case DEFORM_STATE_SELECTING:
          {
            GeglRectangle area =
              { MIN (ct->selection_start_x, coords->x) - ct->offset_x,
                MIN (ct->selection_start_y, coords->y) - ct->offset_y,
                ABS (ct->selection_start_x - coords->x),
                ABS (ct->selection_start_y - coords->y) };

            if (state & gimp_get_extend_selection_mask ())
              {
                gimp_cage_config_select_add_area (ct->config,
                                                  GIMP_CAGE_MODE_DEFORM, area);
              }
            else
              {
                gimp_cage_config_select_area (ct->config,
                                              GIMP_CAGE_MODE_DEFORM, area);
              }

            ct->tool_state = DEFORM_STATE_WAIT;
          }
          break;
        }
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_cage_tool_motion (GimpTool         *tool,
                       const GimpCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       GimpDisplay      *display)
{
  GimpCageTool    *ct       = GIMP_CAGE_TOOL (tool);
  GimpCageOptions *options  = GIMP_CAGE_TOOL_GET_OPTIONS (ct);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  ct->cursor_x = coords->x;
  ct->cursor_y = coords->y;

  switch (ct->tool_state)
    {
    case CAGE_STATE_MOVE_HANDLE:
    case CAGE_STATE_CLOSING:
    case DEFORM_STATE_MOVE_HANDLE:
      gimp_cage_config_add_displacement (ct->config,
                                         options->cage_mode,
                                         ct->cursor_x - ct->movement_start_x,
                                         ct->cursor_y - ct->movement_start_y);
      break;
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static gboolean
gimp_cage_tool_key_press (GimpTool    *tool,
                          GdkEventKey *kevent,
                          GimpDisplay *display)
{
  GimpCageTool *ct = GIMP_CAGE_TOOL (tool);

  if (! ct->config)
    return FALSE;

  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      if (ct->tool_state == CAGE_STATE_WAIT)
        {
          if (gimp_cage_config_get_n_points (ct->config) != 0)
            gimp_cage_tool_remove_last_handle (ct);
        }
      else if (ct->tool_state == DEFORM_STATE_WAIT)
        {
          gimp_cage_config_remove_selected_points (ct->config);

          /* if the cage have less than 3 handles, we reopen it */
          if (gimp_cage_config_get_n_points (ct->config) <= 2)
            {
              ct->tool_state = CAGE_STATE_WAIT;
            }

          gimp_cage_tool_compute_coef (ct);
          gimp_cage_tool_render_node_update (ct);
        }
      return TRUE;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      if (! gimp_cage_tool_is_complete (ct) &&
          gimp_cage_config_get_n_points (ct->config) > 2)
        {
          g_object_set (gimp_tool_get_options (tool),
                        "cage-mode", GIMP_CAGE_MODE_DEFORM,
                        NULL);
        }
      else if (ct->tool_state == DEFORM_STATE_WAIT)
        {
          gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);
        }
      return TRUE;

    case GDK_KEY_Escape:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_cage_tool_oper_update (GimpTool         *tool,
                            const GimpCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            GimpDisplay      *display)
{
  GimpCageTool *ct        = GIMP_CAGE_TOOL (tool);
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tool);

  if (ct->config)
    {
      ct->hovering_handle = gimp_cage_tool_is_on_handle (ct,
                                                         draw_tool,
                                                         display,
                                                         coords->x,
                                                         coords->y,
                                                         GIMP_TOOL_HANDLE_SIZE_CIRCLE);

      ct->hovering_edge = gimp_cage_tool_is_on_edge (ct,
                                                     coords->x,
                                                     coords->y,
                                                     GIMP_TOOL_HANDLE_SIZE_CIRCLE);
    }

  gimp_draw_tool_pause (draw_tool);

  ct->cursor_x = coords->x;
  ct->cursor_y = coords->y;

  gimp_draw_tool_resume (draw_tool);
}

static void
gimp_cage_tool_cursor_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              GimpDisplay      *display)
{
  GimpCageTool       *ct       = GIMP_CAGE_TOOL (tool);
  GimpCageOptions    *options  = GIMP_CAGE_TOOL_GET_OPTIONS (ct);
  GimpGuiConfig      *config   = GIMP_GUI_CONFIG (display->gimp->config);
  GimpCursorModifier  modifier = GIMP_CURSOR_MODIFIER_PLUS;

  if (tool->display)
    {
      if (ct->hovering_handle != -1)
        {
          modifier = GIMP_CURSOR_MODIFIER_MOVE;
        }
      else if (ct->hovering_edge  != -1 &&
               options->cage_mode == GIMP_CAGE_MODE_CAGE_CHANGE)
        {
          modifier = GIMP_CURSOR_MODIFIER_PLUS;
        }
      else
        {
          if (gimp_cage_tool_is_complete (ct))
            modifier = GIMP_CURSOR_MODIFIER_BAD;
        }
    }
  else
    {
      GimpImage    *image     = gimp_display_get_image (display);
      GList        *drawables = gimp_image_get_selected_drawables (image);
      GimpDrawable *drawable  = NULL;

      if (g_list_length (drawables) == 1)
        drawable = drawables->data;
      g_list_free (drawables);

      if (! drawable                                               ||
          gimp_viewable_get_children (GIMP_VIEWABLE (drawable))    ||
          gimp_item_is_content_locked (GIMP_ITEM (drawable), NULL) ||
          ! (gimp_item_is_visible (GIMP_ITEM (drawable)) ||
             config->edit_non_visible))
        {
          modifier = GIMP_CURSOR_MODIFIER_BAD;
        }
    }

  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_cage_tool_options_notify (GimpTool         *tool,
                               GimpToolOptions  *options,
                               const GParamSpec *pspec)
{
  GimpCageTool *ct = GIMP_CAGE_TOOL (tool);

  GIMP_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! tool->display)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if (strcmp (pspec->name, "cage-mode") == 0)
    {
      GimpCageMode mode;

      g_object_get (options,
                    "cage-mode", &mode,
                    NULL);

      if (mode == GIMP_CAGE_MODE_DEFORM)
        {
          /* switch to deform mode */

          if (gimp_cage_config_get_n_points (ct->config) > 2)
            {
              gimp_cage_config_reset_displacement (ct->config);
              gimp_cage_config_reverse_cage_if_needed (ct->config);
              gimp_tool_push_status (tool, tool->display,
                                     _("Press ENTER to commit the transform"));
              ct->tool_state = DEFORM_STATE_WAIT;

              if (! ct->render_node)
                {
                  gimp_cage_tool_create_render_node (ct);
                }

              if (ct->dirty_coef)
                {
                  gimp_cage_tool_compute_coef (ct);
                  gimp_cage_tool_render_node_update (ct);
                }

              if (! ct->filter)
                gimp_cage_tool_create_filter (ct);

              gimp_cage_tool_filter_update (ct);
            }
          else
            {
              g_object_set (options,
                            "cage-mode", GIMP_CAGE_MODE_CAGE_CHANGE,
                            NULL);
            }
        }
      else
        {
          /* switch to edit mode */
          if (ct->filter)
            {
              gimp_drawable_filter_abort (ct->filter);

              gimp_tool_pop_status (tool, tool->display);
              ct->tool_state = CAGE_STATE_WAIT;
            }
        }
    }
  else if (strcmp  (pspec->name, "fill-plain-color") == 0)
    {
      if (ct->tool_state == DEFORM_STATE_WAIT)
        {
          gimp_cage_tool_render_node_update (ct);
          gimp_cage_tool_filter_update (ct);
        }
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_cage_tool_draw (GimpDrawTool *draw_tool)
{
  GimpCageTool    *ct        = GIMP_CAGE_TOOL (draw_tool);
  GimpCageOptions *options   = GIMP_CAGE_TOOL_GET_OPTIONS (ct);
  GimpCageConfig  *config    = ct->config;
  GimpCanvasGroup *stroke_group;
  gint             n_vertices;
  gint             i;
  GimpHandleType   handle;

  n_vertices = gimp_cage_config_get_n_points (config);

  if (n_vertices == 0)
    return;

  if (ct->tool_state == CAGE_STATE_INIT)
    return;

  stroke_group = gimp_draw_tool_add_stroke_group (draw_tool);

  gimp_draw_tool_push_group (draw_tool, stroke_group);

  /* If needed, draw line to the cursor. */
  if (! gimp_cage_tool_is_complete (ct))
    {
      GimpVector2 last_point;

      last_point = gimp_cage_config_get_point_coordinate (ct->config,
                                                          options->cage_mode,
                                                          n_vertices - 1);

      gimp_draw_tool_add_line (draw_tool,
                               last_point.x + ct->offset_x,
                               last_point.y + ct->offset_y,
                               ct->cursor_x,
                               ct->cursor_y);
    }

  gimp_draw_tool_pop_group (draw_tool);

  /* Draw the cage with handles. */
  for (i = 0; i < n_vertices; i++)
    {
      GimpCanvasItem *item;
      GimpVector2     point1, point2;

      point1 = gimp_cage_config_get_point_coordinate (ct->config,
                                                      options->cage_mode,
                                                      i);
      point1.x += ct->offset_x;
      point1.y += ct->offset_y;

      if (i > 0 || gimp_cage_tool_is_complete (ct))
        {
          gint index_point2;

          if (i == 0)
            index_point2 = n_vertices - 1;
          else
            index_point2 = i - 1;

          point2 = gimp_cage_config_get_point_coordinate (ct->config,
                                                          options->cage_mode,
                                                          index_point2);
          point2.x += ct->offset_x;
          point2.y += ct->offset_y;

          if (i != ct->hovering_edge ||
              gimp_cage_tool_is_complete (ct))
            {
              gimp_draw_tool_push_group (draw_tool, stroke_group);
            }

          item = gimp_draw_tool_add_line (draw_tool,
                                          point1.x,
                                          point1.y,
                                          point2.x,
                                          point2.y);

          if (i == ct->hovering_edge &&
              ! gimp_cage_tool_is_complete (ct))
            {
              gimp_canvas_item_set_highlight (item, TRUE);
            }
          else
            {
              gimp_draw_tool_pop_group (draw_tool);
            }
        }

      if (gimp_cage_config_point_is_selected (ct->config, i))
        {
          if (i == ct->hovering_handle)
            handle = GIMP_HANDLE_FILLED_SQUARE;
          else
            handle = GIMP_HANDLE_SQUARE;
        }
      else
        {
          if (i == ct->hovering_handle)
            handle = GIMP_HANDLE_FILLED_CIRCLE;
          else
            handle = GIMP_HANDLE_CIRCLE;
        }

      item = gimp_draw_tool_add_handle (draw_tool,
                                        handle,
                                        point1.x,
                                        point1.y,
                                        GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                        GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                        GIMP_HANDLE_ANCHOR_CENTER);

      if (i == ct->hovering_handle)
        gimp_canvas_item_set_highlight (item, TRUE);
    }

  if (ct->tool_state == DEFORM_STATE_SELECTING ||
      ct->tool_state == CAGE_STATE_SELECTING)
    {
      gimp_draw_tool_add_rectangle (draw_tool,
                                    FALSE,
                                    MIN (ct->selection_start_x, ct->cursor_x),
                                    MIN (ct->selection_start_y, ct->cursor_y),
                                    ABS (ct->selection_start_x - ct->cursor_x),
                                    ABS (ct->selection_start_y - ct->cursor_y));
    }
}

static void
gimp_cage_tool_start (GimpCageTool *ct,
                      GimpDisplay  *display)
{
  GimpTool     *tool     = GIMP_TOOL (ct);
  GimpImage    *image    = gimp_display_get_image (display);
  GList        *drawables = gimp_image_get_selected_drawables (image);

  g_return_if_fail (g_list_length (drawables) == 1);

  tool->display   = display;
  g_list_free (tool->drawables);
  tool->drawables = drawables;

  g_clear_object (&ct->config);

  g_clear_object (&ct->coef);
  ct->dirty_coef = TRUE;

  if (ct->filter)
    {
      gimp_drawable_filter_abort (ct->filter);
      g_clear_object (&ct->filter);
    }

  if (ct->render_node)
    {
      g_clear_object (&ct->render_node);
      ct->coef_node = NULL;
      ct->cage_node = NULL;
    }

  ct->config          = g_object_new (GIMP_TYPE_CAGE_CONFIG, NULL);
  ct->hovering_handle = -1;
  ct->hovering_edge   = -1;
  ct->tool_state      = CAGE_STATE_INIT;

  /* Setting up cage offset to convert the cage point coords to
   * drawable coords
   */
  gimp_item_get_offset (GIMP_ITEM (tool->drawables->data),
                        &ct->offset_x, &ct->offset_y);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (ct), display);
}

static void
gimp_cage_tool_halt (GimpCageTool *ct)
{
  GimpTool *tool = GIMP_TOOL (ct);

  g_clear_object (&ct->config);
  g_clear_object (&ct->coef);
  g_clear_object (&ct->render_node);
  ct->coef_node = NULL;
  ct->cage_node = NULL;

  if (ct->filter)
    {
      gimp_tool_control_push_preserve (tool->control, TRUE);

      gimp_drawable_filter_abort (ct->filter);
      g_clear_object (&ct->filter);

      gimp_tool_control_pop_preserve (tool->control);

      gimp_image_flush (gimp_display_get_image (tool->display));
    }

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;
  ct->tool_state  = CAGE_STATE_INIT;

  g_object_set (gimp_tool_get_options (tool),
                "cage-mode", GIMP_CAGE_MODE_CAGE_CHANGE,
                NULL);
}

static void
gimp_cage_tool_commit (GimpCageTool *ct)
{
  if (ct->filter)
    {
      GimpTool *tool = GIMP_TOOL (ct);

      gimp_tool_control_push_preserve (tool->control, TRUE);

      gimp_drawable_filter_commit (ct->filter, FALSE,
                                   GIMP_PROGRESS (tool), FALSE);
      g_clear_object (&ct->filter);

      gimp_tool_control_pop_preserve (tool->control);

      gimp_image_flush (gimp_display_get_image (tool->display));
    }
}

static gint
gimp_cage_tool_is_on_handle (GimpCageTool *ct,
                             GimpDrawTool *draw_tool,
                             GimpDisplay  *display,
                             gdouble       x,
                             gdouble       y,
                             gint          handle_size)
{
  GimpCageOptions *options = GIMP_CAGE_TOOL_GET_OPTIONS (ct);
  GimpCageConfig  *config  = ct->config;
  gdouble          dist    = G_MAXDOUBLE;
  gint             i;
  GimpVector2      cage_point;
  guint            n_cage_vertices;

  g_return_val_if_fail (GIMP_IS_CAGE_TOOL (ct), -1);

  n_cage_vertices = gimp_cage_config_get_n_points (config);

  if (n_cage_vertices == 0)
    return -1;

  for (i = 0; i < n_cage_vertices; i++)
    {
      cage_point = gimp_cage_config_get_point_coordinate (config,
                                                          options->cage_mode,
                                                          i);
      cage_point.x += ct->offset_x;
      cage_point.y += ct->offset_y;

      dist = gimp_draw_tool_calc_distance_square (GIMP_DRAW_TOOL (draw_tool),
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
gimp_cage_tool_is_on_edge (GimpCageTool *ct,
                           gdouble       x,
                           gdouble       y,
                           gint          handle_size)
{
  GimpCageOptions *options = GIMP_CAGE_TOOL_GET_OPTIONS (ct);
  GimpCageConfig  *config  = ct->config;
  gint             i;
  guint            n_cage_vertices;
  GimpVector2      A, B, C, AB, BC, AC;
  gdouble          lAB, lBC, lAC, lEB, lEC;

  g_return_val_if_fail (GIMP_IS_CAGE_TOOL (ct), -1);

  n_cage_vertices = gimp_cage_config_get_n_points (config);

  if (n_cage_vertices < 2)
    return -1;

  A = gimp_cage_config_get_point_coordinate (config,
                                             options->cage_mode,
                                             n_cage_vertices-1);
  B = gimp_cage_config_get_point_coordinate (config,
                                             options->cage_mode,
                                             0);
  C.x = x;
  C.y = y;

  for (i = 0; i < n_cage_vertices; i++)
    {
      gimp_vector2_sub (&AB, &A, &B);
      gimp_vector2_sub (&BC, &B, &C);
      gimp_vector2_sub (&AC, &A, &C);

      lAB = gimp_vector2_length (&AB);
      lBC = gimp_vector2_length (&BC);
      lAC = gimp_vector2_length (&AC);
      lEB = lAB / 2 + (SQR (lBC) - SQR (lAC)) / (2 * lAB);
      lEC = sqrt (SQR (lBC) - SQR (lEB));

      if ((lEC < handle_size / 2) && (ABS (SQR (lBC) - SQR (lAC)) <= SQR (lAB)))
        return i;

      A = B;
      B = gimp_cage_config_get_point_coordinate (config,
                                                 options->cage_mode,
                                                 (i+1) % n_cage_vertices);
    }

  return -1;
}

static gboolean
gimp_cage_tool_is_complete (GimpCageTool *ct)
{
  return (ct->tool_state > CAGE_STATE_CLOSING);
}

static void
gimp_cage_tool_remove_last_handle (GimpCageTool *ct)
{
  GimpCageConfig *config = ct->config;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (ct));

  gimp_cage_config_remove_last_cage_point (config);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (ct));
}

static void
gimp_cage_tool_compute_coef (GimpCageTool *ct)
{
  GimpCageConfig *config = ct->config;
  GimpProgress   *progress;
  const Babl     *format;
  GeglNode       *gegl;
  GeglNode       *input;
  GeglNode       *output;
  GeglProcessor  *processor;
  GeglBuffer     *buffer;
  gdouble         value;

  progress = gimp_progress_start (GIMP_PROGRESS (ct), FALSE,
                                  _("Computing Cage Coefficients"));

  g_clear_object (&ct->coef);

  format = babl_format_n (babl_type ("float"),
                          gimp_cage_config_get_n_points (config) * 2);


  gegl = gegl_node_new ();

  input = gegl_node_new_child (gegl,
                               "operation", "gimp:cage-coef-calc",
                               "config",    ct->config,
                               NULL);

  output = gegl_node_new_child (gegl,
                                "operation", "gegl:buffer-sink",
                                "buffer",    &buffer,
                                "format",    format,
                                NULL);

  gegl_node_link (input, output);

  processor = gegl_node_new_processor (output, NULL);

  while (gegl_processor_work (processor, &value))
    {
      if (progress)
        gimp_progress_set_value (progress, value);
    }

  if (progress)
    gimp_progress_end (progress);

  g_object_unref (processor);

  ct->coef = buffer;
  g_object_unref (gegl);

  ct->dirty_coef = FALSE;
}

static void
gimp_cage_tool_create_render_node (GimpCageTool *ct)
{
  GimpCageOptions *options = GIMP_CAGE_TOOL_GET_OPTIONS (ct);
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
                                       "operation",        "gimp:cage-transform",
                                       "config",           ct->config,
                                       "fill-plain-color", options->fill_plain_color,
                                       NULL);

  render = gegl_node_new_child (ct->render_node,
                                "operation", "gegl:map-absolute",
                                NULL);

  gegl_node_link (input, ct->cage_node);

  gegl_node_connect (ct->coef_node, "output",
                     ct->cage_node, "aux");

  gegl_node_link (input, render);

  gegl_node_connect (ct->cage_node, "output",
                     render,        "aux");

  gegl_node_link (render, output);

  gimp_gegl_progress_connect (ct->cage_node, GIMP_PROGRESS (ct),
                              _("Cage Transform"));
}

static void
gimp_cage_tool_render_node_update (GimpCageTool *ct)
{
  GimpCageOptions *options  = GIMP_CAGE_TOOL_GET_OPTIONS (ct);
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
gimp_cage_tool_create_filter (GimpCageTool *ct)
{
  if (! ct->render_node)
    gimp_cage_tool_create_render_node (ct);

  ct->filter = gimp_drawable_filter_new (GIMP_TOOL (ct)->drawables->data,
                                         _("Cage transform"),
                                         ct->render_node,
                                         GIMP_ICON_TOOL_CAGE);
  gimp_drawable_filter_set_region (ct->filter, GIMP_FILTER_REGION_DRAWABLE);

  g_signal_connect (ct->filter, "flush",
                    G_CALLBACK (gimp_cage_tool_filter_flush),
                    ct);
}

static void
gimp_cage_tool_filter_flush (GimpDrawableFilter *filter,
                             GimpTool           *tool)
{
  GimpImage *image = gimp_display_get_image (tool->display);

  gimp_projection_flush (gimp_image_get_projection (image));
}

static void
gimp_cage_tool_filter_update (GimpCageTool *ct)
{
  GimpContainer *filters;

  /* Move this operation below any non-destructive filters that
   * may be active, so that it's directly affect the raw pixels. */
  filters =
    gimp_drawable_get_filters (gimp_drawable_filter_get_drawable (ct->filter));

  if (gimp_container_have (filters, GIMP_OBJECT (ct->filter)))
  {
    gint end_index = gimp_container_get_n_children (filters) - 1;
    gint index     = gimp_container_get_child_index (filters,
                                                     GIMP_OBJECT (ct->filter));

    if (end_index > 0 && index != end_index)
      gimp_container_reorder (filters, GIMP_OBJECT (ct->filter),
                              end_index);
  }

  gimp_drawable_filter_apply (ct->filter, NULL);
}
