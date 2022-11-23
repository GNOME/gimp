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

#include <math.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "display-types.h"
#include "tools/tools-types.h"

#include "config/ligmadisplayconfig.h"

#include "core/ligma.h"
#include "core/ligma-filter-history.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-pick-item.h"
#include "core/ligmaitem.h"

#include "widgets/ligmaaction.h"
#include "widgets/ligmacontrollers.h"
#include "widgets/ligmacontrollerkeyboard.h"
#include "widgets/ligmacontrollerwheel.h"
#include "widgets/ligmadeviceinfo.h"
#include "widgets/ligmadeviceinfo-coords.h"
#include "widgets/ligmadevicemanager.h"
#include "widgets/ligmadevices.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadoubleaction.h"
#include "widgets/ligmaenumaction.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmawidgets-utils.h"

#include "tools/ligmaguidetool.h"
#include "tools/ligmamovetool.h"
#include "tools/ligmapainttool.h"
#include "tools/ligmasamplepointtool.h"
#include "tools/ligmatoolcontrol.h"
#include "tools/tool_manager.h"

#include "ligmacanvas.h"
#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-autoscroll.h"
#include "ligmadisplayshell-cursor.h"
#include "ligmadisplayshell-grab.h"
#include "ligmadisplayshell-layer-select.h"
#include "ligmadisplayshell-rotate.h"
#include "ligmadisplayshell-scale.h"
#include "ligmadisplayshell-scroll.h"
#include "ligmadisplayshell-tool-events.h"
#include "ligmadisplayshell-transform.h"
#include "ligmamodifiersmanager.h"
#include "ligmaimagewindow.h"
#include "ligmamotionbuffer.h"
#include "ligmastatusbar.h"

#include "ligma-intl.h"
#include "ligma-log.h"


/*  local function prototypes  */

static GdkModifierType
                  ligma_display_shell_key_to_state             (gint               key);
static GdkModifierType
                  ligma_display_shell_button_to_state          (gint               button);

static void       ligma_display_shell_proximity_in             (LigmaDisplayShell  *shell);
static void       ligma_display_shell_proximity_out            (LigmaDisplayShell  *shell);

static gboolean   ligma_display_shell_check_device             (LigmaDisplayShell  *shell,
                                                               GdkEvent          *event,
                                                               gboolean          *device_changed);
static void       ligma_display_shell_check_device_cursor      (LigmaDisplayShell  *shell);

static void       ligma_display_shell_start_scrolling          (LigmaDisplayShell  *shell,
                                                               const GdkEvent    *event,
                                                               GdkModifierType    state,
                                                               gint               x,
                                                               gint               y);
static void       ligma_display_shell_stop_scrolling           (LigmaDisplayShell  *shell,
                                                               const GdkEvent    *event);
static void       ligma_display_shell_handle_scrolling         (LigmaDisplayShell  *shell,
                                                               GdkModifierType    state,
                                                               gint               x,
                                                               gint               y);

static void       ligma_display_shell_rotate_gesture_maybe_get_state (GtkGestureRotate *gesture,
                                                                     GdkEventSequence *sequence,
                                                                     guint            *maybe_out_state);

static void       ligma_display_shell_space_pressed            (LigmaDisplayShell  *shell,
                                                               const GdkEvent    *event);
static void       ligma_display_shell_released                 (LigmaDisplayShell  *shell,
                                                               const GdkEvent    *event,
                                                               const LigmaCoords  *image_coords);

static gboolean   ligma_display_shell_tab_pressed              (LigmaDisplayShell  *shell,
                                                               const GdkEventKey *event);

static void       ligma_display_shell_update_focus             (LigmaDisplayShell  *shell,
                                                               gboolean           focus_in,
                                                               const LigmaCoords  *image_coords,
                                                               GdkModifierType    state);
static void       ligma_display_shell_update_cursor            (LigmaDisplayShell  *shell,
                                                               const LigmaCoords  *display_coords,
                                                               const LigmaCoords  *image_coords,
                                                               GdkModifierType    state,
                                                               gboolean           update_software_cursor);

static gboolean   ligma_display_shell_initialize_tool          (LigmaDisplayShell  *shell,
                                                               const LigmaCoords  *image_coords,
                                                               GdkModifierType    state);

static void       ligma_display_shell_get_event_coords         (LigmaDisplayShell  *shell,
                                                               const GdkEvent    *event,
                                                               LigmaCoords        *display_coords,
                                                               GdkModifierType   *state,
                                                               guint32           *time);
static void       ligma_display_shell_untransform_event_coords (LigmaDisplayShell  *shell,
                                                               const LigmaCoords  *display_coords,
                                                               LigmaCoords        *image_coords,
                                                               gboolean          *update_software_cursor);

static void       ligma_display_shell_activate_action          (LigmaUIManager     *manager,
                                                               const gchar       *action_desc,
                                                               GVariant          *value);

static gboolean   ligma_display_triggers_context_menu          (const GdkEvent    *event,
                                                               LigmaDisplayShell  *shell,
                                                               Ligma              *ligma,
                                                               const LigmaCoords  *image_coords,
                                                               gboolean           force);


/*  public functions  */

gboolean
ligma_display_shell_events (GtkWidget        *widget,
                           GdkEvent         *event,
                           LigmaDisplayShell *shell)
{
  Ligma     *ligma;
  gboolean  set_display = FALSE;

  /*  are we in destruction?  */
  if (! shell->display || ! ligma_display_get_shell (shell->display))
    return TRUE;

  ligma = ligma_display_get_ligma (shell->display);

  switch (event->type)
    {
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
      {
        GdkEventKey *kevent = (GdkEventKey *) event;

        if (ligma->busy)
          return TRUE;

        /*  do not process most key events while BUTTON1 is down. We do this
         *  so tools keep the modifier state they were in when BUTTON1 was
         *  pressed and to prevent accelerators from being invoked.
         */
        if (kevent->state & GDK_BUTTON1_MASK)
          {
            if (kevent->keyval == GDK_KEY_Shift_L   ||
                kevent->keyval == GDK_KEY_Shift_R   ||
                kevent->keyval == GDK_KEY_Control_L ||
                kevent->keyval == GDK_KEY_Control_R ||
                kevent->keyval == GDK_KEY_Alt_L     ||
                kevent->keyval == GDK_KEY_Alt_R     ||
                kevent->keyval == GDK_KEY_Meta_L    ||
                kevent->keyval == GDK_KEY_Meta_R    ||
                kevent->keyval == GDK_KEY_space     ||
                kevent->keyval == GDK_KEY_KP_Space)
              {
                break;
              }

            return TRUE;
          }

        switch (kevent->keyval)
          {
          case GDK_KEY_Left:      case GDK_KEY_Right:
          case GDK_KEY_Up:        case GDK_KEY_Down:
          case GDK_KEY_space:
          case GDK_KEY_KP_Space:
          case GDK_KEY_Tab:
          case GDK_KEY_KP_Tab:
          case GDK_KEY_ISO_Left_Tab:
          case GDK_KEY_Alt_L:     case GDK_KEY_Alt_R:
          case GDK_KEY_Shift_L:   case GDK_KEY_Shift_R:
          case GDK_KEY_Control_L: case GDK_KEY_Control_R:
          case GDK_KEY_Meta_L:    case GDK_KEY_Meta_R:
          case GDK_KEY_Return:
          case GDK_KEY_KP_Enter:
          case GDK_KEY_ISO_Enter:
          case GDK_KEY_BackSpace:
          case GDK_KEY_Escape:
            break;

          default:
            if (shell->space_release_pending   ||
                shell->button1_release_pending ||
                shell->mod_action != LIGMA_MODIFIER_ACTION_NONE)
              return TRUE;
            break;
          }

        set_display = TRUE;
        break;
      }

    case GDK_BUTTON_PRESS:
    case GDK_SCROLL:
      set_display = TRUE;
      break;

    case GDK_FOCUS_CHANGE:
      {
        GdkEventFocus *fevent = (GdkEventFocus *) event;

        if (fevent->in && shell->display->config->activate_on_focus)
          set_display = TRUE;
      }
      break;

    default:
      break;
    }

  /*  Setting the context's display automatically sets the image, too  */
  if (set_display)
    ligma_context_set_display (ligma_get_user_context (ligma), shell->display);

  return FALSE;
}

static gboolean
ligma_display_shell_canvas_no_image_events (GtkWidget        *canvas,
                                           GdkEvent         *event,
                                           LigmaDisplayShell *shell)
{
  switch (event->type)
    {
    case GDK_2BUTTON_PRESS:
      {
        GdkEventButton  *bevent = (GdkEventButton *) event;
        if (bevent->button == 1)
          {
            LigmaImageWindow *window  = ligma_display_shell_get_window (shell);
            LigmaUIManager   *manager = ligma_image_window_get_ui_manager (window);

            ligma_ui_manager_activate_action (manager, "file", "file-open");
          }
        return TRUE;
      }
      break;

    case GDK_BUTTON_PRESS:
      if (gdk_event_triggers_context_menu (event))
        {
          ligma_ui_manager_ui_popup_at_pointer (shell->popup_manager,
                                               "/dummy-menubar/image-popup",
                                               (GdkEvent *) event,
                                               NULL, NULL);
          return TRUE;
        }
      break;

    case GDK_KEY_PRESS:
      {
        GdkEventKey *kevent = (GdkEventKey *) event;

        if (kevent->keyval == GDK_KEY_Tab    ||
            kevent->keyval == GDK_KEY_KP_Tab ||
            kevent->keyval == GDK_KEY_ISO_Left_Tab)
          {
            return ligma_display_shell_tab_pressed (shell, kevent);
          }
      }
      break;

    default:
      break;
    }

  return FALSE;
}

gboolean
ligma_display_shell_canvas_tool_events (GtkWidget        *canvas,
                                       GdkEvent         *event,
                                       LigmaDisplayShell *shell)
{
  LigmaDisplay          *display;
  LigmaImage            *image;
  Ligma                 *ligma;
  LigmaModifiersManager *mod_manager;
  LigmaCoords            display_coords;
  LigmaCoords            image_coords;
  GdkModifierType       state;
  guint32               time;
  gboolean              device_changed   = FALSE;
  gboolean              return_val       = FALSE;
  gboolean              update_sw_cursor = FALSE;

  g_return_val_if_fail (gtk_widget_get_realized (canvas), FALSE);

  /*  are we in destruction?  */
  if (! shell->display || ! ligma_display_get_shell (shell->display))
    return TRUE;

  /*  set the active display before doing any other canvas event processing  */
  if (ligma_display_shell_events (canvas, event, shell))
    return TRUE;

  /*  events on overlays have a different window, but these windows'
   *  user_data can still be the canvas, we need to check manually if
   *  the event's window and the canvas' window are different.
   */
  if (event->any.window != gtk_widget_get_window (canvas))
    {
      GtkWidget *event_widget;

      gdk_window_get_user_data (event->any.window, (gpointer) &event_widget);

      /*  if the event came from a different window than the canvas',
       *  check if it came from a canvas child and bail out.
       */
      if (gtk_widget_get_ancestor (event_widget, LIGMA_TYPE_CANVAS))
        return FALSE;
    }

  display = shell->display;
  ligma    = ligma_display_get_ligma (display);
  image   = ligma_display_get_image (display);

  if (! image)
    return ligma_display_shell_canvas_no_image_events (canvas, event, shell);

  LIGMA_LOG (TOOL_EVENTS, "event (display %p): %s",
            display, ligma_print_event (event));

  if (ligma_display_shell_check_device (shell, event, &device_changed))
    return TRUE;

  ligma_display_shell_get_event_coords (shell, event,
                                       &display_coords,
                                       &state, &time);
  ligma_display_shell_untransform_event_coords (shell,
                                               &display_coords, &image_coords,
                                               &update_sw_cursor);

  /*  If the device (and maybe the tool) has changed, update the new
   *  tool's state
   */
  if (device_changed && gtk_widget_has_focus (canvas))
    {
      ligma_display_shell_update_focus (shell, TRUE,
                                       &image_coords, state);
    }

  mod_manager = LIGMA_MODIFIERS_MANAGER (shell->display->config->modifiers_manager);

  switch (event->type)
    {
    case GDK_ENTER_NOTIFY:
      {
        GdkEventCrossing *cevent = (GdkEventCrossing *) event;

        if (cevent->mode != GDK_CROSSING_NORMAL)
          return TRUE;

        /*  ignore enter notify while we have a grab  */
        if (shell->grab_pointer)
          return TRUE;

        ligma_display_shell_proximity_in (shell);
        update_sw_cursor = TRUE;

        tool_manager_oper_update_active (ligma,
                                         &image_coords, state,
                                         shell->proximity,
                                         display);
      }
      break;

    case GDK_LEAVE_NOTIFY:
      {
        GdkEventCrossing *cevent = (GdkEventCrossing *) event;

        if (cevent->mode != GDK_CROSSING_NORMAL)
          return TRUE;

        /*  ignore leave notify while we have a grab  */
        if (shell->grab_pointer)
          return TRUE;

        ligma_display_shell_proximity_out (shell);

        tool_manager_oper_update_active (ligma,
                                         &image_coords, state,
                                         shell->proximity,
                                         display);
      }
      break;

    case GDK_PROXIMITY_IN:
      ligma_display_shell_proximity_in (shell);

      tool_manager_oper_update_active (ligma,
                                       &image_coords, state,
                                       shell->proximity,
                                       display);
      break;

    case GDK_PROXIMITY_OUT:
      ligma_display_shell_proximity_out (shell);

      tool_manager_oper_update_active (ligma,
                                       &image_coords, state,
                                       shell->proximity,
                                       display);
      break;

    case GDK_FOCUS_CHANGE:
      {
        GdkEventFocus *fevent = (GdkEventFocus *) event;

        if (fevent->in)
          {
            if (G_UNLIKELY (! gtk_widget_has_focus (canvas)))
              g_warning ("%s: FOCUS_IN but canvas has no focus", G_STRFUNC);

            /*  ignore focus changes while we have a grab  */
            if (shell->grab_pointer)
              return TRUE;

            /*   press modifier keys when the canvas gets the focus  */
            ligma_display_shell_update_focus (shell, TRUE,
                                             &image_coords, state);
          }
        else
          {
            if (G_UNLIKELY (gtk_widget_has_focus (canvas)))
              g_warning ("%s: FOCUS_OUT but canvas has focus", G_STRFUNC);

            /*  ignore focus changes while we have a grab  */
            if (shell->grab_pointer)
              return TRUE;

            /*  release modifier keys when the canvas loses the focus  */
            ligma_display_shell_update_focus (shell, FALSE,
                                             &image_coords, 0);
          }
      }
      break;

    case GDK_BUTTON_PRESS:
      {
        GdkEventButton  *bevent = (GdkEventButton *) event;
        GdkModifierType  button_state;

        /*  ignore new mouse events  */
        if (ligma->busy                                     ||
            shell->mod_action != LIGMA_MODIFIER_ACTION_NONE ||
            shell->grab_pointer                            ||
            shell->button1_release_pending)
          return TRUE;

        button_state = ligma_display_shell_button_to_state (bevent->button);

        state |= button_state;

        /* ignore new buttons while another button is down */
        if (((state & (GDK_BUTTON1_MASK)) && (state & (GDK_BUTTON2_MASK |
                                                       GDK_BUTTON3_MASK))) ||
            ((state & (GDK_BUTTON2_MASK)) && (state & (GDK_BUTTON1_MASK |
                                                       GDK_BUTTON3_MASK))) ||
            ((state & (GDK_BUTTON3_MASK)) && (state & (GDK_BUTTON1_MASK |
                                                       GDK_BUTTON2_MASK))))
          return TRUE;

        /*  focus the widget if it isn't; if the toplevel window
         *  already has focus, this will generate a FOCUS_IN on the
         *  canvas immediately, therefore we do this before logging
         *  the BUTTON_PRESS.
         */
        if (! gtk_widget_has_focus (canvas))
          gtk_widget_grab_focus (canvas);

        /*  if the toplevel window didn't have focus, the above
         *  gtk_widget_grab_focus() didn't set the canvas' HAS_FOCUS
         *  flags, and didn't trigger a FOCUS_IN, but the tool needs
         *  to be set up correctly regardless, so simply do the
         *  same things here, it's safe to do them redundantly.
         */
        ligma_display_shell_update_focus (shell, TRUE,
                                         &image_coords, state);
        ligma_display_shell_update_cursor (shell, &display_coords,
                                          &image_coords, state & ~button_state,
                                          FALSE);

        if (bevent->button == 1)
          {
            if (! ligma_display_shell_pointer_grab (shell, event,
                                                   GDK_POINTER_MOTION_MASK |
                                                   GDK_BUTTON_RELEASE_MASK))
              return TRUE;

            if (ligma_display_shell_initialize_tool (shell,
                                                    &image_coords, state))
              {
                LigmaTool       *active_tool;
                LigmaMotionMode  motion_mode;
                LigmaCoords      last_motion;

                active_tool = tool_manager_get_active (ligma);
                motion_mode = ligma_tool_control_get_motion_mode (active_tool->control);

                if (motion_mode == LIGMA_MOTION_MODE_EXACT)
                  {
                    /* enable motion compression for the canvas window for the
                     * duration of the stroke
                     */
                    gdk_window_set_event_compression (gtk_widget_get_window (canvas), FALSE);
                  }

                /* Use the last evaluated velocity&direction instead of the
                 * button_press event's ones because the click is
                 * usually at the same spot as the last motion event
                 * which would give us bogus derivate dynamics.
                 */
                ligma_motion_buffer_begin_stroke (shell->motion_buffer, time,
                                                 &last_motion);

                image_coords.velocity = last_motion.velocity;
                image_coords.direction = last_motion.direction;

                tool_manager_button_press_active (ligma,
                                                  &image_coords,
                                                  time, state,
                                                  LIGMA_BUTTON_PRESS_NORMAL,
                                                  display);
              }
          }
        else
          {
            GdkDevice            *device;
            LigmaModifierAction    action;
            const gchar          *action_desc = NULL;

            device = gdk_event_get_source_device (event);
            action = ligma_modifiers_manager_get_action (mod_manager, device,
                                                        bevent->button, bevent->state,
                                                        &action_desc);
            shell->mod_action = action;
            switch (action)
              {
              case LIGMA_MODIFIER_ACTION_MENU:
                ligma_display_triggers_context_menu (event, shell, ligma, &image_coords, TRUE);
                break;
              case LIGMA_MODIFIER_ACTION_PANNING:
              case LIGMA_MODIFIER_ACTION_ZOOMING:
              case LIGMA_MODIFIER_ACTION_ROTATING:
              case LIGMA_MODIFIER_ACTION_STEP_ROTATING:
              case LIGMA_MODIFIER_ACTION_LAYER_PICKING:
              case LIGMA_MODIFIER_ACTION_BRUSH_PIXEL_SIZE:
              case LIGMA_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE:
              case LIGMA_MODIFIER_ACTION_TOOL_OPACITY:
                ligma_display_shell_start_scrolling (shell, event, state,
                                                    bevent->x, bevent->y);
                break;
              case LIGMA_MODIFIER_ACTION_ACTION:
                shell->mod_action_desc = g_strdup (action_desc);
                break;
              case LIGMA_MODIFIER_ACTION_NONE:
                ligma_display_triggers_context_menu (event, shell, ligma, &image_coords, FALSE);
                break;
              }
          }

        return_val = TRUE;
      }
      break;

    case GDK_2BUTTON_PRESS:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;
        LigmaTool       *active_tool;

        if (ligma->busy)
          return TRUE;

        active_tool = tool_manager_get_active (ligma);

        if (bevent->button == 1                                &&
            active_tool                                        &&
            ligma_tool_control_is_active (active_tool->control) &&
            ligma_tool_control_get_wants_double_click (active_tool->control))
          {
            tool_manager_button_press_active (ligma,
                                              &image_coords,
                                              time, state,
                                              LIGMA_BUTTON_PRESS_DOUBLE,
                                              display);
          }

        /*  don't update the cursor again on double click  */
        return TRUE;
      }
      break;

    case GDK_3BUTTON_PRESS:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;
        LigmaTool       *active_tool;

        if (ligma->busy)
          return TRUE;

        active_tool = tool_manager_get_active (ligma);

        if (bevent->button == 1                                &&
            active_tool                                        &&
            ligma_tool_control_is_active (active_tool->control) &&
            ligma_tool_control_get_wants_triple_click (active_tool->control))
          {
            tool_manager_button_press_active (ligma,
                                              &image_coords,
                                              time, state,
                                              LIGMA_BUTTON_PRESS_TRIPLE,
                                              display);
          }

        /*  don't update the cursor again on triple click  */
        return TRUE;
      }
      break;

    case GDK_BUTTON_RELEASE:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;
        LigmaTool       *active_tool;

        ligma_display_shell_autoscroll_stop (shell);

        if (bevent->button == 1 && shell->button1_release_pending)
          {
            ligma_display_shell_released (shell, event, NULL);
            return TRUE;
          }

        if (ligma->busy)
          return TRUE;

        active_tool = tool_manager_get_active (ligma);

        state &= ~ligma_display_shell_button_to_state (bevent->button);

        if (bevent->button == 1)
          {
            /*  If we don't have a grab, this is a release paired with
             *  a button press we intentionally ignored because we had
             *  a grab on another device at the time of the press
             */
            if (! shell->grab_pointer || shell->mod_action != LIGMA_MODIFIER_ACTION_NONE)
              return TRUE;

            if (active_tool &&
                (! ligma_image_is_empty (image) ||
                 ligma_tool_control_get_handle_empty_image (active_tool->control)))
              {
                ligma_motion_buffer_end_stroke (shell->motion_buffer);

                gdk_window_set_event_compression (
                  gtk_widget_get_window (canvas), TRUE);

                if (ligma_tool_control_is_active (active_tool->control))
                  {
                    tool_manager_button_release_active (ligma,
                                                        &image_coords,
                                                        time, state,
                                                        display);
                  }
              }

            /*  update the tool's modifier state because it didn't get
             *  key events while BUTTON1 was down
             */
            if (gtk_widget_has_focus (canvas))
              ligma_display_shell_update_focus (shell, TRUE,
                                               &image_coords, state);
            else
              ligma_display_shell_update_focus (shell, FALSE,
                                               &image_coords, 0);

            ligma_display_shell_pointer_ungrab (shell, event);
          }
        else
          {
            LigmaImageWindow *window;
            LigmaUIManager   *manager;

            window  = ligma_display_shell_get_window (shell);
            manager = ligma_image_window_get_ui_manager (window);

            switch (shell->mod_action)
              {
              case LIGMA_MODIFIER_ACTION_MENU:
                break;
              case LIGMA_MODIFIER_ACTION_PANNING:
              case LIGMA_MODIFIER_ACTION_ZOOMING:
              case LIGMA_MODIFIER_ACTION_ROTATING:
              case LIGMA_MODIFIER_ACTION_STEP_ROTATING:
              case LIGMA_MODIFIER_ACTION_LAYER_PICKING:
                if (shell->mod_action != LIGMA_MODIFIER_ACTION_NONE &&
                    ! shell->button1_release_pending &&
                    (! shell->space_release_pending ||
                     shell->display->config->space_bar_action != LIGMA_SPACE_BAR_ACTION_PAN))
                  ligma_display_shell_stop_scrolling (shell, event);
                break;
              case LIGMA_MODIFIER_ACTION_BRUSH_PIXEL_SIZE:
              case LIGMA_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE:
              case LIGMA_MODIFIER_ACTION_TOOL_OPACITY:
                ligma_display_shell_stop_scrolling (shell, event);
                break;
              case LIGMA_MODIFIER_ACTION_ACTION:
                ligma_display_shell_activate_action (manager, shell->mod_action_desc, NULL);
                g_clear_pointer (&shell->mod_action_desc, g_free);
                break;
              case LIGMA_MODIFIER_ACTION_NONE:
                break;
              }

            shell->mod_action = LIGMA_MODIFIER_ACTION_NONE;
          }

        return_val = TRUE;
      }
      break;

    case GDK_SCROLL:
      {
        GdkEventScroll *sevent = (GdkEventScroll *) event;
        LigmaController *wheel  = ligma_controllers_get_wheel (ligma);

        if (! wheel ||
            ! ligma_controller_wheel_scroll (LIGMA_CONTROLLER_WHEEL (wheel),
                                            sevent))
          {
            if (state & ligma_get_toggle_behavior_mask ())
              {
                gdouble delta;

                switch (sevent->direction)
                  {
                  case GDK_SCROLL_UP:
                    ligma_display_shell_scale (shell,
                                              LIGMA_ZOOM_IN,
                                              0.0,
                                              LIGMA_ZOOM_FOCUS_POINTER);
                    break;

                  case GDK_SCROLL_DOWN:
                    ligma_display_shell_scale (shell,
                                              LIGMA_ZOOM_OUT,
                                              0.0,
                                              LIGMA_ZOOM_FOCUS_POINTER);
                    break;

                  case GDK_SCROLL_SMOOTH:
                    gdk_event_get_scroll_deltas (event, NULL, &delta);
                    ligma_display_shell_scale (shell,
                                              LIGMA_ZOOM_SMOOTH,
                                              delta,
                                              LIGMA_ZOOM_FOCUS_POINTER);
                    break;

                  default:
                    break;
                  }
              }
            else
              {
                gdouble value_x;
                gdouble value_y;

                ligma_scroll_adjustment_values (sevent,
                                               shell->hsbdata,
                                               shell->vsbdata,
                                               &value_x, &value_y);

                gtk_adjustment_set_value (shell->hsbdata, value_x);
                gtk_adjustment_set_value (shell->vsbdata, value_y);
              }
          }

        ligma_display_shell_untransform_event_coords (shell,
                                                     &display_coords,
                                                     &image_coords,
                                                     &update_sw_cursor);

        tool_manager_oper_update_active (ligma,
                                         &image_coords, state,
                                         shell->proximity,
                                         display);

        return_val = TRUE;
      }
      break;

    case GDK_MOTION_NOTIFY:
      {
        GdkEventMotion *mevent = (GdkEventMotion *) event;

        if (ligma->busy)
          return TRUE;

        /*  call proximity_in() here because the pointer might already
         *  be in proximity when the canvas starts to receive events,
         *  like when a new image has been created into an empty
         *  display
         */
        ligma_display_shell_proximity_in (shell);
        update_sw_cursor = TRUE;

        if (shell->mod_action != LIGMA_MODIFIER_ACTION_NONE ||
            shell->space_release_pending)
          {
            ligma_display_shell_handle_scrolling (shell,
                                                 state, mevent->x, mevent->y);
          }
        else if (state & GDK_BUTTON1_MASK)
          {
            LigmaTool       *active_tool;
            LigmaMotionMode  motion_mode;

            active_tool = tool_manager_get_active (ligma);
            motion_mode = ligma_tool_control_get_motion_mode (
                            active_tool->control);

            if (active_tool                                        &&
                ligma_tool_control_is_active (active_tool->control) &&
                (! ligma_image_is_empty (image) ||
                 ligma_tool_control_get_handle_empty_image (active_tool->control)))
              {
                GdkTimeCoord **history_events;
                gint           n_history_events;
                guint32        last_motion_time;

                /*  if the first mouse button is down, check for automatic
                 *  scrolling...
                 */
                if ((mevent->x < 0                 ||
                     mevent->y < 0                 ||
                     mevent->x > shell->disp_width ||
                     mevent->y > shell->disp_height) &&
                    ! ligma_tool_control_get_scroll_lock (active_tool->control))
                  {
                    ligma_display_shell_autoscroll_start (shell, state, mevent);
                  }

                /* gdk_device_get_history() has several quirks. First
                 * is that events with borderline timestamps at both
                 * ends are included. Because of that we need to add 1
                 * to lower border. The second is due to poor X event
                 * resolution. We need to do -1 to ensure that the
                 * amount of events between timestamps is final or
                 * risk losing some.
                 */
                last_motion_time =
                  ligma_motion_buffer_get_last_motion_time (shell->motion_buffer);

                if (motion_mode == LIGMA_MOTION_MODE_EXACT     &&
                    shell->display->config->use_event_history &&
                    gdk_device_get_history (mevent->device, mevent->window,
                                            last_motion_time + 1,
                                            mevent->time - 1,
                                            &history_events,
                                            &n_history_events))
                  {
                    LigmaDeviceInfo *device;
                    gint            i;

                    device = ligma_device_info_get_by_device (mevent->device);

                    for (i = 0; i < n_history_events; i++)
                      {
                        ligma_device_info_get_time_coords (device,
                                                          history_events[i],
                                                          &display_coords);

                        ligma_display_shell_untransform_event_coords (shell,
                                                                     &display_coords,
                                                                     &image_coords,
                                                                     NULL);

                        /* Early removal of useless events saves CPU time.
                         */
                        if (ligma_motion_buffer_motion_event (shell->motion_buffer,
                                                             &image_coords,
                                                             history_events[i]->time,
                                                             TRUE))
                          {
                            ligma_motion_buffer_request_stroke (shell->motion_buffer,
                                                               state,
                                                               history_events[i]->time);
                          }
                      }

                    gdk_device_free_history (history_events, n_history_events);
                  }
                else
                  {
                    gboolean event_fill = (motion_mode == LIGMA_MOTION_MODE_EXACT);

                    /* Early removal of useless events saves CPU time.
                     */
                    if (ligma_motion_buffer_motion_event (shell->motion_buffer,
                                                         &image_coords,
                                                         time,
                                                         event_fill))
                      {
                        ligma_motion_buffer_request_stroke (shell->motion_buffer,
                                                           state,
                                                           time);
                      }
                  }
              }
          }

        if (! (state &
               (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
          {
            /* Early removal of useless events saves CPU time.
             * Pass event_fill = FALSE since we are only hovering.
             */
            if (ligma_motion_buffer_motion_event (shell->motion_buffer,
                                                 &image_coords,
                                                 time,
                                                 FALSE))
              {
                ligma_motion_buffer_request_hover (shell->motion_buffer,
                                                  state,
                                                  shell->proximity);
              }
          }

        return_val = TRUE;
      }
      break;

    case GDK_KEY_PRESS:
      {
        GdkEventKey *kevent = (GdkEventKey *) event;
        LigmaTool    *active_tool;

        active_tool = tool_manager_get_active (ligma);

        if (state & GDK_BUTTON1_MASK)
          {
            if (kevent->keyval == GDK_KEY_Alt_L     ||
                kevent->keyval == GDK_KEY_Alt_R     ||
                kevent->keyval == GDK_KEY_Shift_L   ||
                kevent->keyval == GDK_KEY_Shift_R   ||
                kevent->keyval == GDK_KEY_Control_L ||
                kevent->keyval == GDK_KEY_Control_R ||
                kevent->keyval == GDK_KEY_Meta_L    ||
                kevent->keyval == GDK_KEY_Meta_R)
              {
                GdkModifierType key;

                key = ligma_display_shell_key_to_state (kevent->keyval);
                state |= key;

                if (active_tool                                        &&
                    ligma_tool_control_is_active (active_tool->control) &&
                    ! ligma_image_is_empty (image))
                  {
                    tool_manager_active_modifier_state_active (ligma, state,
                                                               display);
                  }
              }
          }
        else
          {
            gboolean arrow_key = FALSE;

            tool_manager_focus_display_active (ligma, display);

            if (ligma_tool_control_get_wants_all_key_events (active_tool->control))
              {
                if (tool_manager_key_press_active (ligma, kevent, display))
                  {
                    /* FIXME: need to do some of the stuff below, like
                     * calling oper_update()
                     */

                    return TRUE;
                  }
              }

            if (! gtk_widget_has_focus (shell->canvas))
              {
                /*  The event was in an overlay widget and not handled
                 *  there, make sure the overlay widgets are keyboard
                 *  navigatable by letting the generic widget handlers
                 *  deal with the event.
                 */
                return FALSE;
              }

            if (ligma_display_shell_key_to_state (kevent->keyval) == GDK_MOD1_MASK)
              /* Make sure the picked layer is reset. */
              shell->picked_layer = NULL;

            switch (kevent->keyval)
              {
              case GDK_KEY_Left:
              case GDK_KEY_Right:
              case GDK_KEY_Up:
              case GDK_KEY_Down:
                arrow_key = TRUE;

              case GDK_KEY_Return:
              case GDK_KEY_KP_Enter:
              case GDK_KEY_ISO_Enter:
              case GDK_KEY_BackSpace:
              case GDK_KEY_Escape:
                if (! ligma_image_is_empty (image))
                  return_val = tool_manager_key_press_active (ligma,
                                                              kevent,
                                                              display);

                if (! return_val)
                  {
                    LigmaController *keyboard = ligma_controllers_get_keyboard (ligma);

                    if (keyboard)
                      return_val =
                        ligma_controller_keyboard_key_press (LIGMA_CONTROLLER_KEYBOARD (keyboard),
                                                            kevent);
                  }

                /* always swallow arrow keys, we don't want focus keynav */
                if (! return_val)
                  return_val = arrow_key;
                break;

              case GDK_KEY_space:
              case GDK_KEY_KP_Space:
                if (shell->button1_release_pending)
                  shell->space_release_pending = TRUE;
                else
                  ligma_display_shell_space_pressed (shell, event);
                return_val = TRUE;
                break;

              case GDK_KEY_Tab:
              case GDK_KEY_KP_Tab:
              case GDK_KEY_ISO_Left_Tab:
                ligma_display_shell_tab_pressed (shell, kevent);
                return_val = TRUE;
                break;

                /*  Update the state based on modifiers being pressed  */
              case GDK_KEY_Alt_L:     case GDK_KEY_Alt_R:
              case GDK_KEY_Shift_L:   case GDK_KEY_Shift_R:
              case GDK_KEY_Control_L: case GDK_KEY_Control_R:
              case GDK_KEY_Meta_L:    case GDK_KEY_Meta_R:
                {
                  GdkModifierType key;

                  key = ligma_display_shell_key_to_state (kevent->keyval);
                  state |= key;

                  if (! ligma_image_is_empty (image))
                    tool_manager_modifier_state_active (ligma, state, display);
                }
                break;
              }

            tool_manager_oper_update_active (ligma,
                                             &image_coords, state,
                                             shell->proximity,
                                             display);
          }
      }
      break;

    case GDK_KEY_RELEASE:
      {
        GdkEventKey *kevent = (GdkEventKey *) event;
        LigmaTool    *active_tool;

        active_tool = tool_manager_get_active (ligma);

        if (shell->mod_action == LIGMA_MODIFIER_ACTION_LAYER_PICKING)
          {
            /* As a special exception, we finalize the layer picking
             * action in the key release event. This allows one to
             * click multiple times and keep a state of the last picked
             * layer.
             * */
            LigmaStatusbar *statusbar;

            statusbar = ligma_display_shell_get_statusbar (shell);
            ligma_statusbar_pop_temp (statusbar);

            shell->picked_layer = NULL;
            shell->mod_action = LIGMA_MODIFIER_ACTION_NONE;
          }
        else if (shell->mod_action != LIGMA_MODIFIER_ACTION_NONE &&
                 (state & ligma_get_all_modifiers_mask ()) == 0)
          {
            ligma_display_shell_stop_scrolling (shell, event);
          }

        if ((state & GDK_BUTTON1_MASK)      &&
            (! shell->space_release_pending ||
             (kevent->keyval != GDK_KEY_space &&
              kevent->keyval != GDK_KEY_KP_Space)))
          {
            if (kevent->keyval == GDK_KEY_Alt_L     ||
                kevent->keyval == GDK_KEY_Alt_R     ||
                kevent->keyval == GDK_KEY_Shift_L   ||
                kevent->keyval == GDK_KEY_Shift_R   ||
                kevent->keyval == GDK_KEY_Control_L ||
                kevent->keyval == GDK_KEY_Control_R ||
                kevent->keyval == GDK_KEY_Meta_L    ||
                kevent->keyval == GDK_KEY_Meta_R)
              {
                GdkModifierType key;

                key = ligma_display_shell_key_to_state (kevent->keyval);
                state &= ~key;

                if (active_tool                                        &&
                    ligma_tool_control_is_active (active_tool->control) &&
                    ! ligma_image_is_empty (image))
                  {
                    tool_manager_active_modifier_state_active (ligma, state,
                                                               display);
                  }
              }
          }
        else
          {
            tool_manager_focus_display_active (ligma, display);

            if (ligma_tool_control_get_wants_all_key_events (active_tool->control))
              {
                if (tool_manager_key_release_active (ligma, kevent, display))
                  {
                    /* FIXME: need to do some of the stuff below, like
                     * calling oper_update()
                     */

                    return TRUE;
                  }
              }

            if (! gtk_widget_has_focus (shell->canvas))
              {
                /*  The event was in an overlay widget and not handled
                 *  there, make sure the overlay widgets are keyboard
                 *  navigatable by letting the generic widget handlers
                 *  deal with the event.
                 */
                return FALSE;
              }

            switch (kevent->keyval)
              {
              case GDK_KEY_space:
              case GDK_KEY_KP_Space:
                if ((state & GDK_BUTTON1_MASK))
                  {
                    shell->button1_release_pending = TRUE;
                    shell->space_release_pending   = FALSE;
                    /* We need to ungrab the pointer in order to catch
                     * button release events.
                     */
                    if (shell->grab_pointer)
                      ligma_display_shell_pointer_ungrab (shell, event);
                  }
                else
                  {
                    ligma_display_shell_released (shell, event, NULL);
                  }
                return_val = TRUE;
                break;

                /*  Update the state based on modifiers being pressed  */
              case GDK_KEY_Alt_L:     case GDK_KEY_Alt_R:
              case GDK_KEY_Shift_L:   case GDK_KEY_Shift_R:
              case GDK_KEY_Control_L: case GDK_KEY_Control_R:
              case GDK_KEY_Meta_L:    case GDK_KEY_Meta_R:
                {
                  GdkModifierType key;

                  key = ligma_display_shell_key_to_state (kevent->keyval);
                  state &= ~key;

                  /*  For all modifier keys: call the tools
                   *  modifier_state *and* oper_update method so tools
                   *  can choose if they are interested in the press
                   *  itself or only in the resulting state
                   */
                  if (! ligma_image_is_empty (image))
                    tool_manager_modifier_state_active (ligma, state, display);
                }
                break;
              }

            tool_manager_oper_update_active (ligma,
                                             &image_coords, state,
                                             shell->proximity,
                                             display);
          }
      }
      break;

    default:
      break;
    }

  /*  if we reached this point in ligma_busy mode, return now  */
  if (ligma->busy)
    return return_val;

  /*  cursor update   */
  ligma_display_shell_update_cursor (shell, &display_coords, &image_coords,
                                    state, update_sw_cursor);

  return return_val;
}

void
ligma_display_shell_canvas_grab_notify (GtkWidget        *canvas,
                                       gboolean          was_grabbed,
                                       LigmaDisplayShell *shell)
{
  LigmaDisplay *display;
  LigmaImage   *image;
  Ligma        *ligma;

  /*  are we in destruction?  */
  if (! shell->display || ! ligma_display_get_shell (shell->display))
    return;

  display = shell->display;
  ligma    = ligma_display_get_ligma (display);
  image   = ligma_display_get_image (display);

  if (! image)
    return;

  LIGMA_LOG (TOOL_EVENTS, "grab_notify (display %p): was_grabbed = %s",
            display, was_grabbed ? "TRUE" : "FALSE");

  if (! was_grabbed)
    {
      if (! ligma_image_is_empty (image))
        {
          LigmaTool *active_tool = tool_manager_get_active (ligma);

          if (active_tool && active_tool->focus_display == display)
            {
              tool_manager_modifier_state_active (ligma, 0, display);
            }
        }
    }
}

/* The ratio of the following defines what finger movement we interpret as
 * a rotation versus zoom gesture. If finger movement is partially a zoom
 * and partially a rotation, the detected gesture will be whichever gesture
 * we detect first
 *
 * Let's define "finger movement angle" as the angle between the direction of
 * finger movement and the line between fingers. If this angle is zero then
 * the gesture is completely a zoom gesture. If this angle is 90 degrees
 * then the gesture is completely a rotation gesture.
 *
 * The boundary finger movement angle (below which the gesture is zoom gesture
 * and above which the gesture is rotate gesture) will be defined as follows:
 *
 *    boundary = arctan(deg2rad(ROTATE_GESTURE_ACTIVATION_DEG_DIFF) /
 *                      (ZOOM_GESTURE_ACTIVATION_SCALE_DIFF / 2))
 *
 * Note that ZOOM_GESTURE_ACTIVATION_SCALE_DIFF needs to be divided by 2
 * because both fingers are moving so the distance between them is increasing
 * twice as fast.
 *
 * We probably want boundary angle to be around 60 degrees to prevent
 * accidentally starting rotations.
 *
 * With ZOOM_GESTURE_ACTIVATION_SCALE_DIFF==0.02 and
 * ROTATE_GESTURE_ACTIVATION_DEG_DIFF==1 boundary is 60.2 degrees.
 */
#define ZOOM_GESTURE_ACTIVATION_SCALE_DIFF 0.02
#define ROTATE_GESTURE_ACTIVATION_DEG_DIFF 1

void
ligma_display_shell_zoom_gesture_begin (GtkGestureZoom   *gesture,
                                       GdkEventSequence *sequence,
                                       LigmaDisplayShell *shell)
{
  shell->last_zoom_scale = gtk_gesture_zoom_get_scale_delta (gesture);
}

void
ligma_display_shell_zoom_gesture_update (GtkGestureZoom   *gesture,
                                        GdkEventSequence *sequence,
                                        LigmaDisplayShell *shell)
{
  gdouble current_scale;
  gdouble delta;

  if (shell->rotate_gesture_active)
    return;

  /* we only activate zoom gesture handling if rotate gesture was inactive and
   * the zoom difference is significant enough */
  current_scale = gtk_gesture_zoom_get_scale_delta (gesture);
  if (!shell->zoom_gesture_active                              &&
      current_scale > (1 - ZOOM_GESTURE_ACTIVATION_SCALE_DIFF) &&
      current_scale < (1 + ZOOM_GESTURE_ACTIVATION_SCALE_DIFF))
    return;

  shell->zoom_gesture_active = TRUE;

  delta = (current_scale - shell->last_zoom_scale) / shell->last_zoom_scale;
  shell->last_zoom_scale = current_scale;

  ligma_display_shell_scale (shell,
                            LIGMA_ZOOM_PINCH,
                            delta,
                            LIGMA_ZOOM_FOCUS_POINTER);
}

void
ligma_display_shell_zoom_gesture_end (GtkGestureZoom   *gesture,
                                     GdkEventSequence *sequence,
                                     LigmaDisplayShell *shell)
{
  shell->zoom_gesture_active = FALSE;
}

void
ligma_display_shell_rotate_gesture_begin (GtkGestureRotate *gesture,
                                         GdkEventSequence *sequence,
                                         LigmaDisplayShell *shell)
{

  shell->initial_gesture_rotate_angle = shell->rotate_angle;
  shell->last_gesture_rotate_state    = 0;
  ligma_display_shell_rotate_gesture_maybe_get_state (gesture, sequence,
                                                     &shell->last_gesture_rotate_state);
}

void
ligma_display_shell_rotate_gesture_update (GtkGestureRotate *gesture,
                                          GdkEventSequence *sequence,
                                          LigmaDisplayShell *shell)
{
  gdouble  angle;
  gdouble  angle_delta_deg;
  gboolean constrain;

  /* we only activate rotate gesture handling if zoom gesture was inactive and
   * the rotation is significant enough */
  if (shell->zoom_gesture_active)
    return;

  angle_delta_deg = 180.0 * gtk_gesture_rotate_get_angle_delta (gesture) / G_PI;
  if (!shell->rotate_gesture_active                         &&
      angle_delta_deg > -ROTATE_GESTURE_ACTIVATION_DEG_DIFF &&
      angle_delta_deg < ROTATE_GESTURE_ACTIVATION_DEG_DIFF)
    return;

  shell->rotate_gesture_active = TRUE;

  angle = shell->initial_gesture_rotate_angle + angle_delta_deg;

  ligma_display_shell_rotate_gesture_maybe_get_state (gesture, sequence,
                                                     &shell->last_gesture_rotate_state);

  constrain = (shell->last_gesture_rotate_state & GDK_CONTROL_MASK) ? TRUE : FALSE;

  ligma_display_shell_rotate_to (shell, constrain ? RINT (angle / 15.0) * 15.0 : angle);
}

void
ligma_display_shell_rotate_gesture_end (GtkGestureRotate *gesture,
                                       GdkEventSequence *sequence,
                                       LigmaDisplayShell *shell)
{
  shell->rotate_gesture_active = FALSE;
}

void
ligma_display_shell_buffer_stroke (LigmaMotionBuffer *buffer,
                                  const LigmaCoords *coords,
                                  guint32           time,
                                  GdkModifierType   state,
                                  LigmaDisplayShell *shell)
{
  LigmaDisplay *display = shell->display;
  Ligma        *ligma    = ligma_display_get_ligma (display);
  LigmaTool    *active_tool;

  active_tool = tool_manager_get_active (ligma);

  if (active_tool &&
      ligma_tool_control_is_active (active_tool->control))
    {
      tool_manager_motion_active (ligma,
                                  coords, time, state,
                                  display);
    }
}

void
ligma_display_shell_buffer_hover (LigmaMotionBuffer *buffer,
                                 const LigmaCoords *coords,
                                 GdkModifierType   state,
                                 gboolean          proximity,
                                 LigmaDisplayShell *shell)
{
  LigmaDisplay *display = shell->display;
  Ligma        *ligma    = ligma_display_get_ligma (display);
  LigmaTool    *active_tool;

  active_tool = tool_manager_get_active (ligma);

  if (active_tool &&
      ! ligma_tool_control_is_active (active_tool->control))
    {
      tool_manager_oper_update_active (ligma,
                                       coords, state, proximity,
                                       display);
    }
}

static gboolean
ligma_display_shell_ruler_button_press (GtkWidget           *widget,
                                       GdkEventButton      *event,
                                       LigmaDisplayShell    *shell,
                                       LigmaOrientationType  orientation)
{
  LigmaDisplay *display = shell->display;

  if (display->ligma->busy)
    return TRUE;

  if (! ligma_display_get_image (display))
    return TRUE;

  if (event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
      LigmaTool *active_tool = tool_manager_get_active (display->ligma);

      if (active_tool)
        {
          ligma_display_shell_update_focus (shell, TRUE,
                                           NULL, event->state);

          if (ligma_display_shell_pointer_grab (shell, (GdkEvent *) event,
                                               GDK_POINTER_MOTION_MASK |
                                               GDK_BUTTON_RELEASE_MASK))
            {
              if (event->state & ligma_get_toggle_behavior_mask ())
                {
                  ligma_sample_point_tool_start_new (active_tool, display);
                }
              else
                {
                  ligma_guide_tool_start_new (active_tool, display,
                                             orientation);
                }

              return TRUE;
            }
        }
    }

  return FALSE;
}

gboolean
ligma_display_shell_hruler_button_press (GtkWidget        *widget,
                                        GdkEventButton   *event,
                                        LigmaDisplayShell *shell)
{
  return ligma_display_shell_ruler_button_press (widget, event, shell,
                                                LIGMA_ORIENTATION_HORIZONTAL);
}

gboolean
ligma_display_shell_vruler_button_press (GtkWidget        *widget,
                                        GdkEventButton   *event,
                                        LigmaDisplayShell *shell)
{
  return ligma_display_shell_ruler_button_press (widget, event, shell,
                                                LIGMA_ORIENTATION_VERTICAL);
}


/*  private functions  */

static GdkModifierType
ligma_display_shell_key_to_state (gint key)
{
  /* FIXME: need some proper GDK API to figure this */

  switch (key)
    {
    case GDK_KEY_Alt_L:
    case GDK_KEY_Alt_R:
      return GDK_MOD1_MASK;

    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
      return GDK_SHIFT_MASK;

    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
      return GDK_CONTROL_MASK;

#ifdef GDK_WINDOWING_QUARTZ
    case GDK_KEY_Meta_L:
    case GDK_KEY_Meta_R:
      return GDK_MOD2_MASK;
#endif

    default:
      return 0;
    }
}

static GdkModifierType
ligma_display_shell_button_to_state (gint button)
{
  if (button == 1)
    return GDK_BUTTON1_MASK;
  else if (button == 2)
    return GDK_BUTTON2_MASK;
  else if (button == 3)
    return GDK_BUTTON3_MASK;

  return 0;
}

static void
ligma_display_shell_proximity_in (LigmaDisplayShell *shell)
{
  if (! shell->proximity)
    {
      shell->proximity = TRUE;

      ligma_display_shell_check_device_cursor (shell);
    }
}

static void
ligma_display_shell_proximity_out (LigmaDisplayShell *shell)
{
  if (shell->proximity)
    {
      shell->proximity = FALSE;

      ligma_display_shell_clear_software_cursor (shell);
    }
}

static gboolean
ligma_display_shell_check_device (LigmaDisplayShell *shell,
                                 GdkEvent         *event,
                                 gboolean         *device_changed)
{
  Ligma      *ligma = ligma_display_get_ligma (shell->display);
  GdkDevice *device;
  GdkDevice *grab_device;

  /*  Find out what device the event occurred upon  */
  device = ligma_devices_get_from_event (ligma, event, &grab_device);

  if (device)
    {
      /*  While we have a grab, ignore all events from all other devices
       *  of the same type
       */
      if (event->type != GDK_KEY_PRESS   &&
          event->type != GDK_KEY_RELEASE &&
          event->type != GDK_FOCUS_CHANGE)
        {
          if ((shell->grab_pointer && (shell->grab_pointer != grab_device)) ||
              (shell->grab_pointer_source && (shell->grab_pointer_source != device)))
            {
              LIGMA_LOG (TOOL_EVENTS,
                        "ignoring pointer event from '%s' while waiting for event from '%s'\n",
                        gdk_device_get_name (device),
                        gdk_device_get_name (shell->grab_pointer_source));
              return TRUE;
            }
        }

      if (! ligma->busy && ligma_devices_check_change (ligma, device))
        {
          ligma_display_shell_check_device_cursor (shell);

          *device_changed = TRUE;
        }
    }

  return FALSE;
}

static void
ligma_display_shell_check_device_cursor (LigmaDisplayShell *shell)
{
  LigmaDeviceManager *manager;
  LigmaDeviceInfo    *current_device;

  manager = ligma_devices_get_manager (shell->display->ligma);

  current_device = ligma_device_manager_get_current_device (manager);

  shell->draw_cursor = ! ligma_device_info_has_cursor (current_device);
}

static void
ligma_display_shell_start_scrolling (LigmaDisplayShell *shell,
                                    const GdkEvent   *event,
                                    GdkModifierType   state,
                                    gint              x,
                                    gint              y)
{
  LigmaModifierAction mod_action = shell->mod_action;

  if (mod_action == LIGMA_MODIFIER_ACTION_NONE && shell->space_release_pending)
    {
      /* XXX The space actions are still hard-coded (instead of
       * customizable throguh LigmaModifiersManager). It might be
       * interesting to make them customizable later.
       */
      GdkModifierType state = 0;

      gdk_event_get_state (event, &state);
      state &= ligma_get_all_modifiers_mask ();

      if (state == 0)
        mod_action = LIGMA_MODIFIER_ACTION_PANNING;
      else if (state == ligma_get_extend_selection_mask ())
        mod_action = LIGMA_MODIFIER_ACTION_ROTATING;
      else if (state == (ligma_get_extend_selection_mask () | GDK_CONTROL_MASK))
        mod_action = LIGMA_MODIFIER_ACTION_STEP_ROTATING;
      else if (state == ligma_get_toggle_behavior_mask ())
        mod_action = LIGMA_MODIFIER_ACTION_ZOOMING;
    }

  ligma_display_shell_pointer_grab (shell, event,
                                   GDK_POINTER_MOTION_MASK |
                                   GDK_BUTTON_RELEASE_MASK);

  shell->scroll_start_x    = x;
  shell->scroll_start_y    = y;
  shell->scroll_last_x     = x;
  shell->scroll_last_y     = y;
  shell->rotate_drag_angle = shell->rotate_angle;

  switch (mod_action)
    {
    case LIGMA_MODIFIER_ACTION_ROTATING:
    case LIGMA_MODIFIER_ACTION_STEP_ROTATING:
      ligma_display_shell_set_override_cursor (shell,
                                              (LigmaCursorType) GDK_EXCHANGE);
      break;
    case LIGMA_MODIFIER_ACTION_ZOOMING:
      ligma_display_shell_set_override_cursor (shell,
                                              (LigmaCursorType) LIGMA_CURSOR_ZOOM);
      break;
    case LIGMA_MODIFIER_ACTION_LAYER_PICKING:
        {
          LigmaImage  *image   = ligma_display_get_image (shell->display);
          LigmaLayer  *layer;
          LigmaCoords  image_coords;
          LigmaCoords  display_coords;
          guint32     time;

          ligma_display_shell_set_override_cursor (shell,
                                                  (LigmaCursorType) LIGMA_CURSOR_CROSSHAIR);

          ligma_display_shell_get_event_coords (shell, event,
                                               &display_coords,
                                               &state, &time);
          ligma_display_shell_untransform_event_coords (shell,
                                                       &display_coords, &image_coords,
                                                       NULL);
          layer = ligma_image_pick_layer (image,
                                         (gint) image_coords.x,
                                         (gint) image_coords.y,
                                         shell->picked_layer);

          if (layer && ! ligma_image_get_floating_selection (image))
            {
              GList *layers = ligma_image_get_selected_layers (image);

              if (g_list_length (layers) != 1 || layer != layers->data)
                {
                  LigmaStatusbar *statusbar;

                  layers = g_list_prepend (NULL, layer);
                  ligma_image_set_selected_layers (image, layers);
                  g_list_free (layers);

                  statusbar = ligma_display_shell_get_statusbar (shell);
                  ligma_statusbar_push_temp (statusbar, LIGMA_MESSAGE_INFO,
                                            LIGMA_ICON_LAYER,
                                            _("Layer picked: '%s'"),
                                            ligma_object_get_name (layer));
                }
              shell->picked_layer = layer;
            }
        }
      break;
    case LIGMA_MODIFIER_ACTION_BRUSH_PIXEL_SIZE:
    case LIGMA_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE:
    case LIGMA_MODIFIER_ACTION_TOOL_OPACITY:
        {
          Ligma     *ligma        = ligma_display_get_ligma (shell->display);
          LigmaTool *active_tool = tool_manager_get_active (ligma);

          if (LIGMA_IS_PAINT_TOOL (active_tool))
            ligma_paint_tool_force_draw (LIGMA_PAINT_TOOL (active_tool), TRUE);
        }
    case LIGMA_MODIFIER_ACTION_MENU:
    case LIGMA_MODIFIER_ACTION_PANNING:
    case LIGMA_MODIFIER_ACTION_ACTION:
    case LIGMA_MODIFIER_ACTION_NONE:
      ligma_display_shell_set_override_cursor (shell,
                                              (LigmaCursorType) GDK_FLEUR);
      break;
    }
}

static void
ligma_display_shell_stop_scrolling (LigmaDisplayShell *shell,
                                   const GdkEvent   *event)
{
  ligma_display_shell_unset_override_cursor (shell);

  switch (shell->mod_action)
    {
    case LIGMA_MODIFIER_ACTION_BRUSH_PIXEL_SIZE:
    case LIGMA_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE:
        {
          Ligma     *ligma        = ligma_display_get_ligma (shell->display);
          LigmaTool *active_tool = tool_manager_get_active (ligma);

          if (LIGMA_IS_PAINT_TOOL (active_tool))
            ligma_paint_tool_force_draw (LIGMA_PAINT_TOOL (active_tool), FALSE);
        }
      break;
    default:
      break;
    }

  shell->mod_action = LIGMA_MODIFIER_ACTION_NONE;

  shell->scroll_start_x    = 0;
  shell->scroll_start_y    = 0;
  shell->scroll_last_x     = 0;
  shell->scroll_last_y     = 0;
  shell->rotate_drag_angle = 0.0;

  /* We may have ungrabbed the pointer when space was released while
   * mouse was down, to be able to catch a GDK_BUTTON_RELEASE event.
   */
  if (shell->grab_pointer)
    ligma_display_shell_pointer_ungrab (shell, event);
}

static void
ligma_display_shell_handle_scrolling (LigmaDisplayShell *shell,
                                     GdkModifierType   state,
                                     gint              x,
                                     gint              y)
{
  LigmaModifierAction mod_action      = shell->mod_action;
  gboolean           constrain       = FALSE;
  gboolean           size_update_pos = TRUE;
  gdouble            size_multiplier = 1.0;

  if (mod_action == LIGMA_MODIFIER_ACTION_NONE && shell->space_release_pending)
    {
      state &= ligma_get_all_modifiers_mask ();

      if (state == 0)
        mod_action = LIGMA_MODIFIER_ACTION_PANNING;
      else if (state == ligma_get_extend_selection_mask ())
        mod_action = LIGMA_MODIFIER_ACTION_ROTATING;
      else if (state == (ligma_get_extend_selection_mask () | GDK_CONTROL_MASK))
        mod_action = LIGMA_MODIFIER_ACTION_STEP_ROTATING;
      else if (state == ligma_get_toggle_behavior_mask ())
        mod_action = LIGMA_MODIFIER_ACTION_ZOOMING;
    }
  else if (mod_action == LIGMA_MODIFIER_ACTION_ROTATING ||
           mod_action == LIGMA_MODIFIER_ACTION_STEP_ROTATING)
    {
      state &= ligma_get_all_modifiers_mask ();

      /* Allow switching from the constrained to non-constrained
       * variant, back and forth, during a single scroll.
       */
      if (state == ligma_get_extend_selection_mask ())
        mod_action = LIGMA_MODIFIER_ACTION_ROTATING;
      else if (state == (ligma_get_extend_selection_mask () | GDK_CONTROL_MASK))
        mod_action = LIGMA_MODIFIER_ACTION_STEP_ROTATING;
    }

  switch (mod_action)
    {
    case LIGMA_MODIFIER_ACTION_STEP_ROTATING:
      constrain = TRUE;
    case LIGMA_MODIFIER_ACTION_ROTATING:

      ligma_display_shell_rotate_drag (shell,
                                      shell->scroll_last_x,
                                      shell->scroll_last_y,
                                      x,
                                      y,
                                      constrain);
      break;
    case LIGMA_MODIFIER_ACTION_ZOOMING:
      ligma_display_shell_scale_drag (shell,
                                     shell->scroll_start_x,
                                     shell->scroll_start_y,
                                     shell->scroll_last_x - x,
                                     shell->scroll_last_y - y);
      break;
    case LIGMA_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE:
      size_multiplier = 2.0;
      size_update_pos = FALSE;
    case LIGMA_MODIFIER_ACTION_BRUSH_PIXEL_SIZE:
        {
          LigmaDisplay *display     = shell->display;
          Ligma        *ligma        = ligma_display_get_ligma (display);
          LigmaTool    *active_tool = tool_manager_get_active (ligma);
          const gchar *action;
          gint         size;

          /* Size in image pixels: distance between start and current
           * position.
           */
          size = (gint) (sqrt (pow ((x - shell->scroll_start_x) / shell->scale_x, 2) +
                               pow ((y - shell->scroll_start_y) / shell->scale_y, 2)));

          /* TODO: different logics with "lock brush to view". */
          /* TODO 2: scale aware? */
          action = ligma_tool_control_get_action_pixel_size (active_tool->control);
          if (action)
            {
              LigmaImageWindow *window  = ligma_display_shell_get_window (shell);
              LigmaUIManager   *manager = ligma_image_window_get_ui_manager (window);

              ligma_display_shell_activate_action (manager, action,
                                                  g_variant_new_double ((gdouble) size * size_multiplier));

              if (size_update_pos)
                {
                  LigmaCoords display_coords;
                  LigmaCoords coords;

                  display_coords.x = shell->scroll_start_x + (x - shell->scroll_start_x) / 2;
                  display_coords.y = shell->scroll_start_y + (y - shell->scroll_start_y) / 2;
                  ligma_display_shell_untransform_event_coords (shell,
                                                               &display_coords, &coords,
                                                               NULL);
                  ligma_tool_oper_update (active_tool, &coords, 0, TRUE, display);
                }
            }
          else
            {
              action = ligma_tool_control_get_action_size (active_tool->control);

              if (action)
                {
                  LigmaImageWindow *window  = ligma_display_shell_get_window (shell);
                  LigmaUIManager   *manager = ligma_image_window_get_ui_manager (window);

                  /* Special trick with these enum actions. If using any
                   * positive value, we get the LIGMA_ACTION_SELECT_SET behavior
                   * which sets to the given value.
                   */
                  ligma_display_shell_activate_action (manager, action,
                                                      g_variant_new_int32 (size));
                }
            }
        }
      break;
    case LIGMA_MODIFIER_ACTION_TOOL_OPACITY:
        {
          LigmaDisplay *display     = shell->display;
          Ligma        *ligma        = ligma_display_get_ligma (display);
          LigmaTool    *active_tool = tool_manager_get_active (ligma);
          const gchar *action;
          gint         size;

          /* Size in image pixels: distance between start and current
           * position.
           */
          size = (gint) (sqrt (pow ((x - shell->scroll_start_x) / shell->scale_x, 2) +
                               pow ((y - shell->scroll_start_y) / shell->scale_y, 2)));

          action = ligma_tool_control_get_action_opacity (active_tool->control);

          if (action)
            {
              LigmaImageWindow *window  = ligma_display_shell_get_window (shell);
              LigmaUIManager   *manager = ligma_image_window_get_ui_manager (window);

              /* Special trick with these enum actions. If using any
               * positive value, we get the LIGMA_ACTION_SELECT_SET behavior
               * which sets to the given value.
               */
              ligma_display_shell_activate_action (manager, action,
                                                  g_variant_new_int32 (size));
            }
        }
      break;
    case LIGMA_MODIFIER_ACTION_PANNING:
      ligma_display_shell_scroll (shell,
                                 shell->scroll_last_x - x,
                                 shell->scroll_last_y - y);
      break;
    case LIGMA_MODIFIER_ACTION_LAYER_PICKING:
      /* Do nothing. We only pick the layer on click. */
    case LIGMA_MODIFIER_ACTION_MENU:
    case LIGMA_MODIFIER_ACTION_NONE:
    case LIGMA_MODIFIER_ACTION_ACTION:
      break;
    }

  shell->scroll_last_x = x;
  shell->scroll_last_y = y;
}

static void
ligma_display_shell_rotate_gesture_maybe_get_state (GtkGestureRotate *gesture,
                                                   GdkEventSequence *sequence,
                                                   guint            *maybe_out_state)
{
  /* The only way to get any access to any data about events handled by the
   * GtkGestureRotate is through its last_event. The set of events handled by
   * GtkGestureRotate is not fully defined so we can't guarantee that last_event
   * will be of event type that has a state field (though touch and gesture
   * events do have that).
   *
   * Usually this would not be a problem, but when handling a gesture we don't
   * want to repeatedly switch between a valid state and its default value if
   * last_event happens to not have it. Thus we store the last valid state
   * and only update it if we get a valid state from last_event.
   */
  guint           state = 0;
  const GdkEvent *last_event;

  last_event = gtk_gesture_get_last_event (GTK_GESTURE (gesture), sequence);
  if (last_event == NULL)
    return;

  if (gdk_event_get_state (last_event, &state))
    *maybe_out_state = state;
}

static void
ligma_display_shell_space_pressed (LigmaDisplayShell *shell,
                                  const GdkEvent   *event)
{
  Ligma *ligma = ligma_display_get_ligma (shell->display);

  if (shell->space_release_pending || shell->mod_action != LIGMA_MODIFIER_ACTION_NONE)
    return;

  shell->space_release_pending = TRUE;

  switch (shell->display->config->space_bar_action)
    {
    case LIGMA_SPACE_BAR_ACTION_NONE:
      break;

    case LIGMA_SPACE_BAR_ACTION_PAN:
      {
        LigmaDeviceManager *manager;
        LigmaDeviceInfo    *current_device;
        LigmaCoords         coords;
        GdkModifierType    state = 0;

        manager = ligma_devices_get_manager (ligma);
        current_device = ligma_device_manager_get_current_device (manager);

        ligma_device_info_get_device_coords (current_device,
                                            gtk_widget_get_window (shell->canvas),
                                            &coords);
        gdk_event_get_state (event, &state);

        ligma_display_shell_start_scrolling (shell, event, state,
                                            coords.x, coords.y);
      }
      break;

    case LIGMA_SPACE_BAR_ACTION_MOVE:
      {
        LigmaTool *active_tool = tool_manager_get_active (ligma);

        if (active_tool || ! LIGMA_IS_MOVE_TOOL (active_tool))
          {
            GdkModifierType state;

            shell->space_shaded_tool =
              ligma_object_get_name (active_tool->tool_info);

            ligma_context_set_tool (ligma_get_user_context (ligma),
                                   ligma_get_tool_info (ligma, "ligma-move-tool"));

            gdk_event_get_state (event, &state);

            ligma_display_shell_update_focus (shell, TRUE,
                                             NULL, state);
          }
      }
      break;
    }
}

static void
ligma_display_shell_released (LigmaDisplayShell *shell,
                             const GdkEvent   *event,
                             const LigmaCoords *image_coords)
{
  Ligma *ligma = ligma_display_get_ligma (shell->display);

  if (! shell->space_release_pending &&
      ! shell->button1_release_pending)
    return;

  switch (shell->display->config->space_bar_action)
    {
    case LIGMA_SPACE_BAR_ACTION_NONE:
      break;

    case LIGMA_SPACE_BAR_ACTION_PAN:
      ligma_display_shell_stop_scrolling (shell, event);
      break;

    case LIGMA_SPACE_BAR_ACTION_MOVE:
      if (shell->space_shaded_tool)
        {
          ligma_context_set_tool (ligma_get_user_context (ligma),
                                 ligma_get_tool_info (ligma,
                                                     shell->space_shaded_tool));
          shell->space_shaded_tool = NULL;

          if (gtk_widget_has_focus (shell->canvas))
            {
              GdkModifierType state;

              gdk_event_get_state (event, &state);

              ligma_display_shell_update_focus (shell, TRUE,
                                               image_coords, state);
            }
          else
            {
              ligma_display_shell_update_focus (shell, FALSE,
                                               image_coords, 0);
            }
        }
      break;
    }

  shell->space_release_pending   = FALSE;
  shell->button1_release_pending = FALSE;
}

static gboolean
ligma_display_shell_tab_pressed (LigmaDisplayShell  *shell,
                                const GdkEventKey *kevent)
{
  LigmaImageWindow *window  = ligma_display_shell_get_window (shell);
  LigmaUIManager   *manager = ligma_image_window_get_ui_manager (window);
  LigmaImage       *image   = ligma_display_get_image (shell->display);

  if (kevent->state & GDK_CONTROL_MASK)
    {
      if (image && ! ligma_image_is_empty (image))
        {
          if (kevent->keyval == GDK_KEY_Tab ||
              kevent->keyval == GDK_KEY_KP_Tab)
            ligma_display_shell_layer_select_init (shell, (GdkEvent *) kevent,
                                                  1);
          else
            ligma_display_shell_layer_select_init (shell, (GdkEvent *) kevent,
                                                  -1);

          return TRUE;
        }
    }
  else if (kevent->state & GDK_MOD1_MASK)
    {
      if (image)
        {
          if (kevent->keyval == GDK_KEY_Tab ||
              kevent->keyval == GDK_KEY_KP_Tab)
            ligma_ui_manager_activate_action (manager, "windows",
                                             "windows-show-display-next");
          else
            ligma_ui_manager_activate_action (manager, "windows",
                                             "windows-show-display-previous");

          return TRUE;
        }
    }
  else
    {
      ligma_ui_manager_activate_action (manager, "windows",
                                       "windows-hide-docks");

      return TRUE;
    }

  return FALSE;
}

static void
ligma_display_shell_update_focus (LigmaDisplayShell *shell,
                                 gboolean          focus_in,
                                 const LigmaCoords *image_coords,
                                 GdkModifierType   state)
{
  Ligma *ligma = ligma_display_get_ligma (shell->display);

  if (focus_in)
    {
      tool_manager_focus_display_active (ligma, shell->display);
      tool_manager_modifier_state_active (ligma, state, shell->display);
    }
  else
    {
      tool_manager_focus_display_active (ligma, NULL);
    }

  if (image_coords)
    tool_manager_oper_update_active (ligma,
                                     image_coords, state,
                                     shell->proximity,
                                     shell->display);
}

static void
ligma_display_shell_update_cursor (LigmaDisplayShell *shell,
                                  const LigmaCoords *display_coords,
                                  const LigmaCoords *image_coords,
                                  GdkModifierType   state,
                                  gboolean          update_software_cursor)
{
  LigmaDisplay *display = shell->display;
  Ligma        *ligma    = ligma_display_get_ligma (display);
  LigmaImage   *image   = ligma_display_get_image (display);
  LigmaTool    *active_tool;

  if (! shell->display->config->cursor_updating)
    return;

  active_tool = tool_manager_get_active (ligma);

  if (active_tool && image)
    {
      if ((! ligma_image_is_empty (image) ||
           ligma_tool_control_get_handle_empty_image (active_tool->control)) &&
          ! (state & (GDK_BUTTON1_MASK |
                      GDK_BUTTON2_MASK |
                      GDK_BUTTON3_MASK)))
        {
          tool_manager_cursor_update_active (ligma,
                                             image_coords, state,
                                             display);
        }
      else if (ligma_image_is_empty (image) &&
               ! ligma_tool_control_get_handle_empty_image (active_tool->control))
        {
          ligma_display_shell_set_cursor (shell,
                                         LIGMA_CURSOR_MOUSE,
                                         ligma_tool_control_get_tool_cursor (active_tool->control),
                                         LIGMA_CURSOR_MODIFIER_BAD);
        }
    }
  else
    {
      ligma_display_shell_set_cursor (shell,
                                     LIGMA_CURSOR_MOUSE,
                                     LIGMA_TOOL_CURSOR_NONE,
                                     LIGMA_CURSOR_MODIFIER_BAD);
    }

  if (update_software_cursor)
    {
      LigmaCursorPrecision precision = LIGMA_CURSOR_PRECISION_PIXEL_CENTER;

      if (active_tool)
        precision = ligma_tool_control_get_precision (active_tool->control);

      ligma_display_shell_update_software_cursor (shell,
                                                 precision,
                                                 (gint) display_coords->x,
                                                 (gint) display_coords->y,
                                                 image_coords->x,
                                                 image_coords->y);
    }
}

static gboolean
ligma_display_shell_initialize_tool (LigmaDisplayShell *shell,
                                    const LigmaCoords *image_coords,
                                    GdkModifierType   state)
{
  LigmaDisplay *display     = shell->display;
  LigmaImage   *image       = ligma_display_get_image (display);
  Ligma        *ligma        = ligma_display_get_ligma (display);
  gboolean     initialized = FALSE;
  LigmaTool    *active_tool;

  active_tool = tool_manager_get_active (ligma);

  if (active_tool &&
      (! ligma_image_is_empty (image) ||
       ligma_tool_control_get_handle_empty_image (active_tool->control)))
    {
      /*  initialize the current tool if it has no drawables  */
      if (! active_tool->drawables)
        {
          initialized = tool_manager_initialize_active (ligma, display);
        }
      else if (! ligma_image_equal_selected_drawables (image, active_tool->drawables) &&
               (! ligma_tool_control_get_preserve (active_tool->control) &&
                (ligma_tool_control_get_dirty_mask (active_tool->control) &
                 LIGMA_DIRTY_ACTIVE_DRAWABLE)))
        {
          LigmaProcedure *procedure = g_object_get_data (G_OBJECT (active_tool),
                                                        "ligma-gegl-procedure");

          if (image == ligma_item_get_image (LIGMA_ITEM (active_tool->drawables->data)))
            {
              /*  When changing between drawables if the *same* image,
               *  stop the tool using its dirty action, so it doesn't
               *  get committed on tool change, in case its dirty action
               *  is HALT. This is a pure "probably better this way"
               *  decision because the user is likely changing their
               *  mind or was simply on the wrong layer. See bug #776370.
               *
               *  See also issues #1180 and #1202 for cases where we
               *  actually *don't* want to halt the tool here, but rather
               *  commit it, hence the use of the tool's dirty action.
               */
              tool_manager_control_active (
                ligma,
                ligma_tool_control_get_dirty_action (active_tool->control),
                active_tool->display);
            }

          if (procedure)
            {
              /*  We can't just recreate an operation tool, we must
               *  make sure the right stuff gets set on it, so
               *  re-activate the procedure that created it instead of
               *  just calling ligma_context_tool_changed(). See
               *  LigmaGeglProcedure and bug #776370.
               */
              LigmaImageWindow *window;
              LigmaUIManager   *manager;

              window  = ligma_display_shell_get_window (shell);
              manager = ligma_image_window_get_ui_manager (window);

              ligma_filter_history_add (ligma, procedure);
              ligma_ui_manager_activate_action (manager, "filters",
                                               "filters-reshow");

              /*  the procedure already initialized the tool; don't
               *  reinitialize it below, since this can lead to errors.
               */
              initialized = TRUE;
            }
          else
            {
              /*  create a new one, deleting the current  */
              ligma_context_tool_changed (ligma_get_user_context (ligma));
            }

          /*  make sure the newly created tool has the right state  */
          ligma_display_shell_update_focus (shell, TRUE, image_coords, state);

          if (! initialized)
            initialized = tool_manager_initialize_active (ligma, display);
        }
      else
        {
          initialized = TRUE;
        }
    }

  return initialized;
}

static void
ligma_display_shell_get_event_coords (LigmaDisplayShell *shell,
                                     const GdkEvent   *event,
                                     LigmaCoords       *display_coords,
                                     GdkModifierType  *state,
                                     guint32          *time)
{
  Ligma              *ligma = ligma_display_get_ligma (shell->display);
  LigmaDeviceManager *manager;
  LigmaDeviceInfo    *current_device;

  manager = ligma_devices_get_manager (ligma);
  current_device = ligma_device_manager_get_current_device (manager);

  ligma_device_info_get_event_coords (current_device,
                                     gtk_widget_get_window (shell->canvas),
                                     event,
                                     display_coords);

  ligma_device_info_get_event_state (current_device,
                                    gtk_widget_get_window (shell->canvas),
                                    event,
                                    state);

  *time = gdk_event_get_time (event);
}

static void
ligma_display_shell_untransform_event_coords (LigmaDisplayShell *shell,
                                             const LigmaCoords *display_coords,
                                             LigmaCoords       *image_coords,
                                             gboolean         *update_software_cursor)
{
  Ligma     *ligma = ligma_display_get_ligma (shell->display);
  LigmaTool *active_tool;

  /*  LigmaCoords passed to tools are ALWAYS in image coordinates  */
  ligma_display_shell_untransform_coords (shell,
                                         display_coords,
                                         image_coords);

  active_tool = tool_manager_get_active (ligma);

  if (active_tool && ligma_tool_control_get_snap_to (active_tool->control))
    {
      gint x, y, width, height;

      ligma_tool_control_get_snap_offsets (active_tool->control,
                                          &x, &y, &width, &height);

      if (ligma_display_shell_snap_coords (shell,
                                          image_coords,
                                          x, y, width, height))
        {
          if (update_software_cursor)
            *update_software_cursor = TRUE;
        }
    }
}

static void
ligma_display_shell_activate_action (LigmaUIManager *manager,
                                    const gchar   *action_desc,
                                    GVariant      *value)
{
  gchar *group_name;
  gchar *action_name;

  g_return_if_fail (action_desc != NULL);

  group_name  = g_strdup (action_desc);
  action_name = strchr (group_name, '/');

  if (action_name)
    {
      LigmaAction *action;

      *action_name++ = '\0';

      action = ligma_ui_manager_find_action (manager, group_name, action_name);

      if (LIGMA_IS_ENUM_ACTION (action) &&
          LIGMA_ENUM_ACTION (action)->value_variable)
        {
          ligma_action_emit_activate (action, value);
        }
      else if (LIGMA_IS_DOUBLE_ACTION (action))
        {
          ligma_action_emit_activate (action, value);
        }
      else
        {
          ligma_action_activate (action);
        }
    }

  g_free (group_name);
}

/* Replace gdk_event_triggers_context_menu() as we don't want to trigger
 * anymore on right click, as we have our own system.
 * But we trigger with %GDK_MODIFIER_INTENT_CONTEXT_MENU mask.
 */
static gboolean
ligma_display_triggers_context_menu (const GdkEvent   *event,
                                    LigmaDisplayShell *shell,
                                    Ligma             *ligma,
                                    const LigmaCoords *image_coords,
                                    gboolean          force)
{
  if (event->type == GDK_BUTTON_PRESS)
    {
      const GdkEventButton *bevent   = (const GdkEventButton *) event;
      gboolean              triggers = force;

      g_return_val_if_fail (GDK_IS_WINDOW (bevent->window), FALSE);

      if (! force)
        {
          GdkDisplay      *display;
          GdkModifierType  modifier;

          display  = gdk_window_get_display (bevent->window);
          modifier = gdk_keymap_get_modifier_mask (gdk_keymap_get_for_display (display),
                                                   GDK_MODIFIER_INTENT_CONTEXT_MENU);

          triggers = (modifier != 0 &&
                      bevent->button == GDK_BUTTON_PRIMARY &&
                      ! (bevent->state & (GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)) &&
                      (bevent->state & modifier));
        }

      if (triggers)
        {
          LigmaUIManager   *ui_manager;
          const gchar     *ui_path;

          ui_manager = tool_manager_get_popup_active (ligma,
                                                      image_coords,
                                                      bevent->state,
                                                      shell->display,
                                                      &ui_path);

          if (! ui_manager)
            {
              ui_manager = shell->popup_manager;
              ui_path    = "/dummy-menubar/image-popup";
            }

          ligma_ui_manager_ui_popup_at_pointer (ui_manager, ui_path, event,
                                               NULL, NULL);
          return TRUE;
        }
    }

  return FALSE;
}
