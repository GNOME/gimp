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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"
#include "tools/tools-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimp-filter-history.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "widgets/gimpcontrollers.h"
#include "widgets/gimpcontrollerkeyboard.h"
#include "widgets/gimpcontrollermouse.h"
#include "widgets/gimpcontrollerwheel.h"
#include "widgets/gimpdeviceinfo.h"
#include "widgets/gimpdeviceinfo-coords.h"
#include "widgets/gimpdevicemanager.h"
#include "widgets/gimpdevices.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "tools/gimpguidetool.h"
#include "tools/gimpmovetool.h"
#include "tools/gimpsamplepointtool.h"
#include "tools/gimptoolcontrol.h"
#include "tools/tool_manager.h"

#include "gimpcanvas.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-autoscroll.h"
#include "gimpdisplayshell-cursor.h"
#include "gimpdisplayshell-grab.h"
#include "gimpdisplayshell-layer-select.h"
#include "gimpdisplayshell-rotate.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-tool-events.h"
#include "gimpdisplayshell-transform.h"
#include "gimpimagewindow.h"
#include "gimpmotionbuffer.h"

#include "gimp-log.h"


/*  local function prototypes  */

static GdkModifierType
                  gimp_display_shell_key_to_state             (gint               key);
static GdkModifierType
                  gimp_display_shell_button_to_state          (gint               button);

static void       gimp_display_shell_proximity_in             (GimpDisplayShell  *shell);
static void       gimp_display_shell_proximity_out            (GimpDisplayShell  *shell);

static void       gimp_display_shell_check_device_cursor      (GimpDisplayShell  *shell);

static void       gimp_display_shell_start_scrolling          (GimpDisplayShell  *shell,
                                                               const GdkEvent    *event,
                                                               GdkModifierType    state,
                                                               gint               x,
                                                               gint               y);
static void       gimp_display_shell_stop_scrolling           (GimpDisplayShell  *shell,
                                                               const GdkEvent    *event);

static void       gimp_display_shell_space_pressed            (GimpDisplayShell  *shell,
                                                               const GdkEvent    *event);
static void       gimp_display_shell_space_released           (GimpDisplayShell  *shell,
                                                               const GdkEvent    *event,
                                                               const GimpCoords  *image_coords);

static gboolean   gimp_display_shell_tab_pressed              (GimpDisplayShell  *shell,
                                                               const GdkEventKey *event);

static void       gimp_display_shell_update_focus             (GimpDisplayShell  *shell,
                                                               gboolean           focus_in,
                                                               const GimpCoords  *image_coords,
                                                               GdkModifierType    state);
static void       gimp_display_shell_update_cursor            (GimpDisplayShell  *shell,
                                                               const GimpCoords  *display_coords,
                                                               const GimpCoords  *image_coords,
                                                               GdkModifierType    state,
                                                               gboolean           update_software_cursor);

static gboolean   gimp_display_shell_initialize_tool          (GimpDisplayShell  *shell,
                                                               const GimpCoords  *image_coords,
                                                               GdkModifierType    state);

static void       gimp_display_shell_get_event_coords         (GimpDisplayShell  *shell,
                                                               const GdkEvent    *event,
                                                               GimpCoords        *display_coords,
                                                               GdkModifierType   *state,
                                                               guint32           *time);
static void       gimp_display_shell_untransform_event_coords (GimpDisplayShell  *shell,
                                                               const GimpCoords  *display_coords,
                                                               GimpCoords        *image_coords,
                                                               gboolean          *update_software_cursor);

static GdkEvent * gimp_display_shell_compress_motion          (GimpDisplayShell  *shell);


/*  public functions  */

gboolean
gimp_display_shell_events (GtkWidget        *widget,
                           GdkEvent         *event,
                           GimpDisplayShell *shell)
{
  Gimp     *gimp;
  gboolean  set_display = FALSE;

  /*  are we in destruction?  */
  if (! shell->display || ! gimp_display_get_shell (shell->display))
    return TRUE;

  gimp = gimp_display_get_gimp (shell->display);

  switch (event->type)
    {
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
      {
        GdkEventKey *kevent = (GdkEventKey *) event;

        if (gimp->busy)
          return TRUE;

        /*  do not process any key events while BUTTON1 is down. We do this
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
                kevent->keyval == GDK_KEY_Meta_R)
              {
                break;
              }

            if (event->type == GDK_KEY_PRESS)
              {
                if ((kevent->keyval == GDK_KEY_space ||
                     kevent->keyval == GDK_KEY_KP_Space) && shell->space_release_pending)
                  {
                    shell->space_pressed         = TRUE;
                    shell->space_release_pending = FALSE;
                  }
              }
            else
              {
                if ((kevent->keyval == GDK_KEY_space ||
                     kevent->keyval == GDK_KEY_KP_Space) && shell->space_pressed)
                  {
                    shell->space_pressed         = FALSE;
                    shell->space_release_pending = TRUE;
                  }
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
            if (shell->space_pressed || shell->scrolling)
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
    gimp_context_set_display (gimp_get_user_context (gimp), shell->display);

  return FALSE;
}

static gboolean
gimp_display_shell_canvas_no_image_events (GtkWidget        *canvas,
                                           GdkEvent         *event,
                                           GimpDisplayShell *shell)
{
  switch (event->type)
    {
    case GDK_2BUTTON_PRESS:
      {
        GdkEventButton  *bevent = (GdkEventButton *) event;
        if (bevent->button == 1)
          {
            GimpImageWindow *window  = gimp_display_shell_get_window (shell);
            GimpUIManager   *manager = gimp_image_window_get_ui_manager (window);

            gimp_ui_manager_activate_action (manager, "file", "file-open");
          }
        return TRUE;
      }
      break;

    case GDK_BUTTON_PRESS:
      if (gdk_event_triggers_context_menu (event))
        {
          gimp_ui_manager_ui_popup (shell->popup_manager,
                                    "/dummy-menubar/image-popup",
                                    GTK_WIDGET (shell),
                                    NULL, NULL, NULL, NULL);
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
            return gimp_display_shell_tab_pressed (shell, kevent);
          }
      }
      break;

    default:
      break;
    }

  return FALSE;
}

gboolean
gimp_display_shell_canvas_tool_events (GtkWidget        *canvas,
                                       GdkEvent         *event,
                                       GimpDisplayShell *shell)
{
  GimpDisplay     *display;
  GimpImage       *image;
  Gimp            *gimp;
  GimpCoords       display_coords;
  GimpCoords       image_coords;
  GdkModifierType  state;
  guint32          time;
  gboolean         device_changed   = FALSE;
  gboolean         return_val       = FALSE;
  gboolean         update_sw_cursor = FALSE;

  g_return_val_if_fail (gtk_widget_get_realized (canvas), FALSE);

  /*  are we in destruction?  */
  if (! shell->display || ! gimp_display_get_shell (shell->display))
    return TRUE;

  /*  set the active display before doing any other canvas event processing  */
  if (gimp_display_shell_events (canvas, event, shell))
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
      if (gtk_widget_get_ancestor (event_widget, GIMP_TYPE_CANVAS))
        return FALSE;
    }

  display = shell->display;
  gimp    = gimp_display_get_gimp (display);
  image   = gimp_display_get_image (display);

  if (! image)
    return gimp_display_shell_canvas_no_image_events (canvas, event, shell);

  GIMP_LOG (TOOL_EVENTS, "event (display %p): %s",
            display, gimp_print_event (event));

  /* See bug 771444 */
  if (shell->pointer_grabbed &&
      event->type == GDK_MOTION_NOTIFY)
    {
      GimpDeviceManager *manager = gimp_devices_get_manager (gimp);
      GimpDeviceInfo    *info;

      info = gimp_device_manager_get_current_device (manager);

      if (info->device != event->motion.device)
        return FALSE;
    }

  /*  Find out what device the event occurred upon  */
  if (! gimp->busy && gimp_devices_check_change (gimp, event))
    {
      gimp_display_shell_check_device_cursor (shell);
      device_changed = TRUE;
    }

  gimp_display_shell_get_event_coords (shell, event,
                                       &display_coords,
                                       &state, &time);
  gimp_display_shell_untransform_event_coords (shell,
                                               &display_coords, &image_coords,
                                               &update_sw_cursor);

  /*  If the device (and maybe the tool) has changed, update the new
   *  tool's state
   */
  if (device_changed && gtk_widget_has_focus (canvas))
    {
      gimp_display_shell_update_focus (shell, TRUE,
                                       &image_coords, state);
    }

  switch (event->type)
    {
    case GDK_ENTER_NOTIFY:
      {
        GdkEventCrossing *cevent = (GdkEventCrossing *) event;

        if (cevent->mode != GDK_CROSSING_NORMAL)
          return TRUE;

        /*  ignore enter notify while we have a grab  */
        if (shell->pointer_grabbed)
          return TRUE;

        gimp_display_shell_proximity_in (shell);
        update_sw_cursor = TRUE;

        tool_manager_oper_update_active (gimp,
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
        if (shell->pointer_grabbed)
          return TRUE;

        gimp_display_shell_proximity_out (shell);

        tool_manager_oper_update_active (gimp,
                                         &image_coords, state,
                                         shell->proximity,
                                         display);
      }
      break;

    case GDK_PROXIMITY_IN:
      gimp_display_shell_proximity_in (shell);

      tool_manager_oper_update_active (gimp,
                                       &image_coords, state,
                                       shell->proximity,
                                       display);
      break;

    case GDK_PROXIMITY_OUT:
      gimp_display_shell_proximity_out (shell);

      tool_manager_oper_update_active (gimp,
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
            if (shell->pointer_grabbed)
              return TRUE;

            /*   press modifier keys when the canvas gets the focus  */
            gimp_display_shell_update_focus (shell, TRUE,
                                             &image_coords, state);
          }
        else
          {
            if (G_UNLIKELY (gtk_widget_has_focus (canvas)))
              g_warning ("%s: FOCUS_OUT but canvas has focus", G_STRFUNC);

            /*  ignore focus changes while we have a grab  */
            if (shell->pointer_grabbed)
              return TRUE;

            /*  release modifier keys when the canvas loses the focus  */
            gimp_display_shell_update_focus (shell, FALSE,
                                             &image_coords, 0);
          }
      }
      break;

    case GDK_BUTTON_PRESS:
      {
        GdkEventButton  *bevent = (GdkEventButton *) event;
        GdkModifierType  button_state;

        /*  ignore new mouse events  */
        if (gimp->busy || shell->scrolling || shell->pointer_grabbed)
          return TRUE;

        button_state = gimp_display_shell_button_to_state (bevent->button);

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
        gimp_display_shell_update_focus (shell, TRUE,
                                         &image_coords, state);
        gimp_display_shell_update_cursor (shell, &display_coords,
                                          &image_coords, state & ~button_state,
                                          FALSE);

        if (gdk_event_triggers_context_menu (event))
          {
            GimpUIManager *ui_manager;
            const gchar   *ui_path;

            ui_manager = tool_manager_get_popup_active (gimp,
                                                        &image_coords, state,
                                                        display,
                                                        &ui_path);

            if (ui_manager)
              {
                gimp_ui_manager_ui_popup (ui_manager,
                                          ui_path,
                                          GTK_WIDGET (shell),
                                          NULL, NULL, NULL, NULL);
              }
            else
              {
                gimp_ui_manager_ui_popup (shell->popup_manager,
                                          "/dummy-menubar/image-popup",
                                          GTK_WIDGET (shell),
                                          NULL, NULL, NULL, NULL);
              }
          }
        else if (bevent->button == 1)
          {
            if (! gimp_display_shell_pointer_grab (shell, NULL, 0))
              return TRUE;

            if (! shell->space_pressed && ! shell->space_release_pending)
              if (! gimp_display_shell_keyboard_grab (shell, event))
                {
                  gimp_display_shell_pointer_ungrab (shell, NULL);
                  return TRUE;
                }

            if (gimp_display_shell_initialize_tool (shell,
                                                    &image_coords, state))
              {
                GimpCoords last_motion;

                /* Use the last evaluated velocity&direction instead of the
                 * button_press event's ones because the click is
                 * usually at the same spot as the last motion event
                 * which would give us bogus derivate dynamics.
                 */
                gimp_motion_buffer_begin_stroke (shell->motion_buffer, time,
                                                 &last_motion);

                image_coords.velocity = last_motion.velocity;
                image_coords.direction = last_motion.direction;

                tool_manager_button_press_active (gimp,
                                                  &image_coords,
                                                  time, state,
                                                  GIMP_BUTTON_PRESS_NORMAL,
                                                  display);
              }
          }
        else if (bevent->button == 2)
          {
            gimp_display_shell_start_scrolling (shell, NULL, state,
                                                bevent->x, bevent->y);
          }

        return_val = TRUE;
      }
      break;

    case GDK_2BUTTON_PRESS:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;
        GimpTool       *active_tool;

        if (gimp->busy)
          return TRUE;

        active_tool = tool_manager_get_active (gimp);

        if (bevent->button == 1                                &&
            active_tool                                        &&
            gimp_tool_control_is_active (active_tool->control) &&
            gimp_tool_control_get_wants_double_click (active_tool->control))
          {
            tool_manager_button_press_active (gimp,
                                              &image_coords,
                                              time, state,
                                              GIMP_BUTTON_PRESS_DOUBLE,
                                              display);
          }

        /*  don't update the cursor again on double click  */
        return TRUE;
      }
      break;

    case GDK_3BUTTON_PRESS:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;
        GimpTool       *active_tool;

        if (gimp->busy)
          return TRUE;

        active_tool = tool_manager_get_active (gimp);

        if (bevent->button == 1                                &&
            active_tool                                        &&
            gimp_tool_control_is_active (active_tool->control) &&
            gimp_tool_control_get_wants_triple_click (active_tool->control))
          {
            tool_manager_button_press_active (gimp,
                                              &image_coords,
                                              time, state,
                                              GIMP_BUTTON_PRESS_TRIPLE,
                                              display);
          }

        /*  don't update the cursor again on triple click  */
        return TRUE;
      }
      break;

    case GDK_BUTTON_RELEASE:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;
        GimpTool       *active_tool;

        gimp_display_shell_autoscroll_stop (shell);

        if (gimp->busy)
          return TRUE;

        active_tool = tool_manager_get_active (gimp);

        state &= ~gimp_display_shell_button_to_state (bevent->button);

        if (bevent->button == 1)
          {
            if (! shell->pointer_grabbed || shell->scrolling)
              return TRUE;

            if (! shell->space_pressed && ! shell->space_release_pending)
              gimp_display_shell_keyboard_ungrab (shell, event);

            if (active_tool &&
                (! gimp_image_is_empty (image) ||
                 gimp_tool_control_get_handle_empty_image (active_tool->control)))
              {
                gimp_motion_buffer_end_stroke (shell->motion_buffer);

                if (gimp_tool_control_is_active (active_tool->control))
                  {
                    tool_manager_button_release_active (gimp,
                                                        &image_coords,
                                                        time, state,
                                                        display);
                  }
              }

            /*  update the tool's modifier state because it didn't get
             *  key events while BUTTON1 was down
             */
            if (gtk_widget_has_focus (canvas))
              gimp_display_shell_update_focus (shell, TRUE,
                                               &image_coords, state);
            else
              gimp_display_shell_update_focus (shell, FALSE,
                                               &image_coords, 0);

            gimp_display_shell_pointer_ungrab (shell, NULL);

            if (shell->space_release_pending)
              gimp_display_shell_space_released (shell, event, &image_coords);
          }
        else if (bevent->button == 2)
          {
            if (shell->scrolling)
              gimp_display_shell_stop_scrolling (shell, NULL);
          }
        else if (bevent->button == 3)
          {
            /* nop */
          }
        else
          {
            GdkEventButton *bevent = (GdkEventButton *) event;
            GimpController *mouse  = gimp_controllers_get_mouse (gimp);

            if (!(shell->scrolling || shell->pointer_grabbed) &&
                mouse && gimp_controller_mouse_button (GIMP_CONTROLLER_MOUSE (mouse),
                                                       bevent))
              {
                return TRUE;
              }
          }

        return_val = TRUE;
      }
      break;

    case GDK_SCROLL:
      {
        GdkEventScroll *sevent = (GdkEventScroll *) event;
        GimpController *wheel  = gimp_controllers_get_wheel (gimp);

        if (! wheel ||
            ! gimp_controller_wheel_scroll (GIMP_CONTROLLER_WHEEL (wheel),
                                            sevent))
          {
            GdkScrollDirection  direction = sevent->direction;

            if (state & gimp_get_toggle_behavior_mask ())
              {
                switch (direction)
                  {
                  case GDK_SCROLL_UP:
                    gimp_display_shell_scale (shell,
                                              GIMP_ZOOM_IN,
                                              0.0,
                                              GIMP_ZOOM_FOCUS_BEST_GUESS);
                    break;

                  case GDK_SCROLL_DOWN:
                    gimp_display_shell_scale (shell,
                                              GIMP_ZOOM_OUT,
                                              0.0,
                                              GIMP_ZOOM_FOCUS_BEST_GUESS);
                    break;

                  default:
                    break;
                  }
              }
            else
              {
                GtkAdjustment *adj = NULL;
                gdouble        value;

                if (state & GDK_SHIFT_MASK)
                  switch (direction)
                    {
                    case GDK_SCROLL_UP:    direction = GDK_SCROLL_LEFT;  break;
                    case GDK_SCROLL_DOWN:  direction = GDK_SCROLL_RIGHT; break;
                    case GDK_SCROLL_LEFT:  direction = GDK_SCROLL_UP;    break;
                    case GDK_SCROLL_RIGHT: direction = GDK_SCROLL_DOWN;  break;
                    }

                switch (direction)
                  {
                  case GDK_SCROLL_LEFT:
                  case GDK_SCROLL_RIGHT:
                    adj = shell->hsbdata;
                    break;

                  case GDK_SCROLL_UP:
                  case GDK_SCROLL_DOWN:
                    adj = shell->vsbdata;
                    break;
                  }

                value = (gtk_adjustment_get_value (adj) +
                         ((direction == GDK_SCROLL_UP ||
                           direction == GDK_SCROLL_LEFT) ?
                          -gtk_adjustment_get_page_increment (adj) / 2 :
                          gtk_adjustment_get_page_increment (adj) / 2));
                value = CLAMP (value,
                               gtk_adjustment_get_lower (adj),
                               gtk_adjustment_get_upper (adj) -
                               gtk_adjustment_get_page_size (adj));

                gtk_adjustment_set_value (adj, value);
              }
          }

        gimp_display_shell_untransform_event_coords (shell,
                                                     &display_coords,
                                                     &image_coords,
                                                     &update_sw_cursor);

        tool_manager_oper_update_active (gimp,
                                         &image_coords, state,
                                         shell->proximity,
                                         display);

        return_val = TRUE;
      }
      break;

    case GDK_MOTION_NOTIFY:
      {
        GdkEventMotion *mevent            = (GdkEventMotion *) event;
        GdkEvent       *compressed_motion = NULL;
        GimpMotionMode  motion_mode       = GIMP_MOTION_MODE_EXACT;
        GimpTool       *active_tool;

        if (gimp->busy)
          return TRUE;

        active_tool = tool_manager_get_active (gimp);

        if (active_tool)
          motion_mode = gimp_tool_control_get_motion_mode (active_tool->control);

        if (shell->scrolling ||
            motion_mode == GIMP_MOTION_MODE_COMPRESS)
          {
            compressed_motion = gimp_display_shell_compress_motion (shell);

            if (compressed_motion && ! shell->scrolling)
              {
                gimp_display_shell_get_event_coords (shell,
                                                     compressed_motion,
                                                     &display_coords,
                                                     &state, &time);
                gimp_display_shell_untransform_event_coords (shell,
                                                             &display_coords,
                                                             &image_coords,
                                                             NULL);
              }
          }

        /*  call proximity_in() here because the pointer might already
         *  be in proximity when the canvas starts to receive events,
         *  like when a new image has been created into an empty
         *  display
         */
        gimp_display_shell_proximity_in (shell);
        update_sw_cursor = TRUE;

        if (shell->scrolling)
          {
            const gint x = (compressed_motion
                            ? ((GdkEventMotion *) compressed_motion)->x
                            : mevent->x);
            const gint y = (compressed_motion
                            ? ((GdkEventMotion *) compressed_motion)->y
                            : mevent->y);

            if (shell->rotating)
              {
                gboolean constrain = (state & GDK_CONTROL_MASK) ? TRUE : FALSE;

                gimp_display_shell_rotate_drag (shell,
                                                shell->scroll_last_x,
                                                shell->scroll_last_y,
                                                x,
                                                y,
                                                constrain);
              }
            else if (shell->scaling)
              {
                gimp_display_shell_scale_drag (shell,
                                               shell->scroll_last_x - x,
                                               shell->scroll_last_y - y);
              }
            else
              {
                gimp_display_shell_scroll (shell,
                                           shell->scroll_last_x - x,
                                           shell->scroll_last_y - y);

              }

            shell->scroll_last_x = x;
            shell->scroll_last_y = y;
          }
        else if (state & GDK_BUTTON1_MASK)
          {
            if (active_tool                                        &&
                gimp_tool_control_is_active (active_tool->control) &&
                (! gimp_image_is_empty (image) ||
                 gimp_tool_control_get_handle_empty_image (active_tool->control)))
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
                    ! gimp_tool_control_get_scroll_lock (active_tool->control))
                  {
                    gimp_display_shell_autoscroll_start (shell, state, mevent);
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
                  gimp_motion_buffer_get_last_motion_time (shell->motion_buffer);

                if (motion_mode == GIMP_MOTION_MODE_EXACT     &&
                    shell->display->config->use_event_history &&
                    gdk_device_get_history (mevent->device, mevent->window,
                                            last_motion_time + 1,
                                            mevent->time - 1,
                                            &history_events,
                                            &n_history_events))
                  {
                    GimpDeviceInfo *device;
                    gint            i;

                    device = gimp_device_info_get_by_device (mevent->device);

                    for (i = 0; i < n_history_events; i++)
                      {
                        gimp_device_info_get_time_coords (device,
                                                          history_events[i],
                                                          &display_coords);

                        gimp_display_shell_untransform_event_coords (shell,
                                                                     &display_coords,
                                                                     &image_coords,
                                                                     NULL);

                        /* Early removal of useless events saves CPU time.
                         */
                        if (gimp_motion_buffer_motion_event (shell->motion_buffer,
                                                             &image_coords,
                                                             history_events[i]->time,
                                                             TRUE))
                          {
                            gimp_motion_buffer_request_stroke (shell->motion_buffer,
                                                               state,
                                                               history_events[i]->time);
                          }
                      }

                    gdk_device_free_history (history_events, n_history_events);
                  }
                else
                  {
                    gboolean event_fill = (motion_mode == GIMP_MOTION_MODE_EXACT);

                    /* Early removal of useless events saves CPU time.
                     */
                    if (gimp_motion_buffer_motion_event (shell->motion_buffer,
                                                         &image_coords,
                                                         time,
                                                         event_fill))
                      {
                        gimp_motion_buffer_request_stroke (shell->motion_buffer,
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
            if (gimp_motion_buffer_motion_event (shell->motion_buffer,
                                                 &image_coords,
                                                 time,
                                                 FALSE))
              {
                gimp_motion_buffer_request_hover (shell->motion_buffer,
                                                  state,
                                                  shell->proximity);
              }
          }

        if (compressed_motion)
          gdk_event_free (compressed_motion);

        return_val = TRUE;
      }
      break;

    case GDK_KEY_PRESS:
      {
        GdkEventKey *kevent = (GdkEventKey *) event;
        GimpTool    *active_tool;

        active_tool = tool_manager_get_active (gimp);

        if (state & GDK_BUTTON1_MASK)
          {
            switch (kevent->keyval)
              {
              case GDK_KEY_Alt_L:     case GDK_KEY_Alt_R:
              case GDK_KEY_Shift_L:   case GDK_KEY_Shift_R:
              case GDK_KEY_Control_L: case GDK_KEY_Control_R:
              case GDK_KEY_Meta_L:    case GDK_KEY_Meta_R:
                {
                  GdkModifierType key;

                  key = gimp_display_shell_key_to_state (kevent->keyval);
                  state |= key;

                  if (active_tool                                        &&
                      gimp_tool_control_is_active (active_tool->control) &&
                      ! gimp_image_is_empty (image))
                    {
                      tool_manager_active_modifier_state_active (gimp, state,
                                                                 display);
                    }
                }
                break;
              }
          }
        else
          {
            gboolean arrow_key = FALSE;

            tool_manager_focus_display_active (gimp, display);

            if (gimp_tool_control_get_wants_all_key_events (active_tool->control))
              {
                if (tool_manager_key_press_active (gimp, kevent, display))
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
                if (! gimp_image_is_empty (image))
                  return_val = tool_manager_key_press_active (gimp,
                                                              kevent,
                                                              display);

                if (! return_val)
                  {
                    GimpController *keyboard = gimp_controllers_get_keyboard (gimp);

                    if (keyboard)
                      return_val =
                        gimp_controller_keyboard_key_press (GIMP_CONTROLLER_KEYBOARD (keyboard),
                                                            kevent);
                  }

                /* always swallow arrow keys, we don't want focus keynav */
                if (! return_val)
                  return_val = arrow_key;
                break;

              case GDK_KEY_space:
              case GDK_KEY_KP_Space:
                gimp_display_shell_space_pressed (shell, event);
                return_val = TRUE;
                break;

              case GDK_KEY_Tab:
              case GDK_KEY_KP_Tab:
              case GDK_KEY_ISO_Left_Tab:
                gimp_display_shell_tab_pressed (shell, kevent);
                return_val = TRUE;
                break;

                /*  Update the state based on modifiers being pressed  */
              case GDK_KEY_Alt_L:     case GDK_KEY_Alt_R:
              case GDK_KEY_Shift_L:   case GDK_KEY_Shift_R:
              case GDK_KEY_Control_L: case GDK_KEY_Control_R:
              case GDK_KEY_Meta_L:    case GDK_KEY_Meta_R:
                {
                  GdkModifierType key;

                  key = gimp_display_shell_key_to_state (kevent->keyval);
                  state |= key;

                  if (! gimp_image_is_empty (image))
                    tool_manager_modifier_state_active (gimp, state, display);
                }
                break;
              }

            tool_manager_oper_update_active (gimp,
                                             &image_coords, state,
                                             shell->proximity,
                                             display);
          }
      }
      break;

    case GDK_KEY_RELEASE:
      {
        GdkEventKey *kevent = (GdkEventKey *) event;
        GimpTool    *active_tool;

        active_tool = tool_manager_get_active (gimp);

        if (state & GDK_BUTTON1_MASK)
          {
            switch (kevent->keyval)
              {
              case GDK_KEY_Alt_L:     case GDK_KEY_Alt_R:
              case GDK_KEY_Shift_L:   case GDK_KEY_Shift_R:
              case GDK_KEY_Control_L: case GDK_KEY_Control_R:
              case GDK_KEY_Meta_L:    case GDK_KEY_Meta_R:
                {
                  GdkModifierType key;

                  key = gimp_display_shell_key_to_state (kevent->keyval);
                  state &= ~key;

                  if (active_tool                                        &&
                      gimp_tool_control_is_active (active_tool->control) &&
                      ! gimp_image_is_empty (image))
                    {
                      tool_manager_active_modifier_state_active (gimp, state,
                                                                 display);
                    }
                }
                break;
              }
          }
        else
          {
            tool_manager_focus_display_active (gimp, display);

            if (gimp_tool_control_get_wants_all_key_events (active_tool->control))
              {
                if (tool_manager_key_release_active (gimp, kevent, display))
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
                gimp_display_shell_space_released (shell, event, NULL);
                return_val = TRUE;
                break;

                /*  Update the state based on modifiers being pressed  */
              case GDK_KEY_Alt_L:     case GDK_KEY_Alt_R:
              case GDK_KEY_Shift_L:   case GDK_KEY_Shift_R:
              case GDK_KEY_Control_L: case GDK_KEY_Control_R:
              case GDK_KEY_Meta_L:    case GDK_KEY_Meta_R:
                {
                  GdkModifierType key;

                  key = gimp_display_shell_key_to_state (kevent->keyval);
                  state &= ~key;

                  /*  For all modifier keys: call the tools
                   *  modifier_state *and* oper_update method so tools
                   *  can choose if they are interested in the press
                   *  itself or only in the resulting state
                   */
                  if (! gimp_image_is_empty (image))
                    tool_manager_modifier_state_active (gimp, state, display);
                }
                break;
              }

            tool_manager_oper_update_active (gimp,
                                             &image_coords, state,
                                             shell->proximity,
                                             display);
          }
      }
      break;

    default:
      break;
    }

  /*  if we reached this point in gimp_busy mode, return now  */
  if (gimp->busy)
    return return_val;

  /*  cursor update   */
  gimp_display_shell_update_cursor (shell, &display_coords, &image_coords,
                                    state, update_sw_cursor);

  return return_val;
}

void
gimp_display_shell_canvas_grab_notify (GtkWidget        *canvas,
                                       gboolean          was_grabbed,
                                       GimpDisplayShell *shell)
{
  GimpDisplay *display;
  GimpImage   *image;
  Gimp        *gimp;

  /*  are we in destruction?  */
  if (! shell->display || ! gimp_display_get_shell (shell->display))
    return;

  display = shell->display;
  gimp    = gimp_display_get_gimp (display);
  image   = gimp_display_get_image (display);

  if (! image)
    return;

  GIMP_LOG (TOOL_EVENTS, "grab_notify (display %p): was_grabbed = %s",
            display, was_grabbed ? "TRUE" : "FALSE");

  if (! was_grabbed)
    {
      if (! gimp_image_is_empty (image))
        {
          GimpTool *active_tool = tool_manager_get_active (gimp);

          if (active_tool && active_tool->focus_display == display)
            {
              tool_manager_modifier_state_active (gimp, 0, display);
            }
        }
    }
}

void
gimp_display_shell_buffer_stroke (GimpMotionBuffer *buffer,
                                  const GimpCoords *coords,
                                  guint32           time,
                                  GdkModifierType   state,
                                  GimpDisplayShell *shell)
{
  GimpDisplay *display = shell->display;
  Gimp        *gimp    = gimp_display_get_gimp (display);
  GimpTool    *active_tool;

  active_tool = tool_manager_get_active (gimp);

  if (active_tool &&
      gimp_tool_control_is_active (active_tool->control))
    {
      tool_manager_motion_active (gimp,
                                  coords, time, state,
                                  display);
    }
}

void
gimp_display_shell_buffer_hover (GimpMotionBuffer *buffer,
                                 const GimpCoords *coords,
                                 GdkModifierType   state,
                                 gboolean          proximity,
                                 GimpDisplayShell *shell)
{
  GimpDisplay *display = shell->display;
  Gimp        *gimp    = gimp_display_get_gimp (display);
  GimpTool    *active_tool;

  active_tool = tool_manager_get_active (gimp);

  if (active_tool &&
      ! gimp_tool_control_is_active (active_tool->control))
    {
      tool_manager_oper_update_active (gimp,
                                       coords, state, proximity,
                                       display);
    }
}

static gboolean
gimp_display_shell_ruler_button_press (GtkWidget           *widget,
                                       GdkEventButton      *event,
                                       GimpDisplayShell    *shell,
                                       GimpOrientationType  orientation)
{
  GimpDisplay *display = shell->display;

  if (display->gimp->busy)
    return TRUE;

  if (! gimp_display_get_image (display))
    return TRUE;

  if (event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
      GimpTool *active_tool = tool_manager_get_active (display->gimp);

      if (active_tool)
        {
          gimp_display_shell_update_focus (shell, TRUE,
                                           NULL, event->state);

          if (gimp_display_shell_pointer_grab (shell, NULL, 0))
            {
              if (gimp_display_shell_keyboard_grab (shell,
                                                    (GdkEvent *) event))
                {
                  if (event->state & gimp_get_toggle_behavior_mask ())
                    {
                      gimp_sample_point_tool_start_new (active_tool, display);
                    }
                  else
                    {
                      gimp_guide_tool_start_new (active_tool, display,
                                                 orientation);
                    }

                  return TRUE;
                }
              else
                {
                  gimp_display_shell_pointer_ungrab (shell, NULL);
                }
            }
        }
    }

  return FALSE;
}

gboolean
gimp_display_shell_hruler_button_press (GtkWidget        *widget,
                                        GdkEventButton   *event,
                                        GimpDisplayShell *shell)
{
  return gimp_display_shell_ruler_button_press (widget, event, shell,
                                                GIMP_ORIENTATION_HORIZONTAL);
}

gboolean
gimp_display_shell_vruler_button_press (GtkWidget        *widget,
                                        GdkEventButton   *event,
                                        GimpDisplayShell *shell)
{
  return gimp_display_shell_ruler_button_press (widget, event, shell,
                                                GIMP_ORIENTATION_VERTICAL);
}


/*  private functions  */

static GdkModifierType
gimp_display_shell_key_to_state (gint key)
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
gimp_display_shell_button_to_state (gint button)
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
gimp_display_shell_proximity_in (GimpDisplayShell *shell)
{
  if (! shell->proximity)
    {
      shell->proximity = TRUE;

      gimp_display_shell_check_device_cursor (shell);
    }
}

static void
gimp_display_shell_proximity_out (GimpDisplayShell *shell)
{
  if (shell->proximity)
    {
      shell->proximity = FALSE;

      gimp_display_shell_clear_software_cursor (shell);
    }
}

static void
gimp_display_shell_check_device_cursor (GimpDisplayShell *shell)
{
  GimpDeviceManager *manager;
  GimpDeviceInfo    *current_device;

  manager = gimp_devices_get_manager (shell->display->gimp);

  current_device = gimp_device_manager_get_current_device (manager);

  shell->draw_cursor = ! gimp_device_info_has_cursor (current_device);
}

static void
gimp_display_shell_start_scrolling (GimpDisplayShell *shell,
                                    const GdkEvent   *event,
                                    GdkModifierType   state,
                                    gint              x,
                                    gint              y)
{
  g_return_if_fail (! shell->scrolling);

  gimp_display_shell_pointer_grab (shell, event, GDK_POINTER_MOTION_MASK);

  shell->scrolling         = TRUE;
  shell->scroll_last_x     = x;
  shell->scroll_last_y     = y;
  shell->rotating          = (state & gimp_get_extend_selection_mask ()) ? TRUE : FALSE;
  shell->rotate_drag_angle = shell->rotate_angle;
  shell->scaling           = (state & gimp_get_toggle_behavior_mask ()) ? TRUE : FALSE;

  if (shell->rotating)
    gimp_display_shell_set_override_cursor (shell,
                                            (GimpCursorType) GDK_EXCHANGE);
  if (shell->scaling)
    gimp_display_shell_set_override_cursor (shell,
                                            (GimpCursorType) GIMP_CURSOR_ZOOM);
  else
    gimp_display_shell_set_override_cursor (shell,
                                            (GimpCursorType) GDK_FLEUR);
}

static void
gimp_display_shell_stop_scrolling (GimpDisplayShell *shell,
                                   const GdkEvent   *event)
{
  g_return_if_fail (shell->scrolling);

  gimp_display_shell_unset_override_cursor (shell);

  shell->scrolling         = FALSE;
  shell->scroll_last_x     = 0;
  shell->scroll_last_y     = 0;
  shell->rotating          = FALSE;
  shell->rotate_drag_angle = 0.0;
  shell->scaling           = FALSE;

  gimp_display_shell_pointer_ungrab (shell, event);
}

static void
gimp_display_shell_space_pressed (GimpDisplayShell *shell,
                                  const GdkEvent   *event)
{
  Gimp *gimp = gimp_display_get_gimp (shell->display);

  if (shell->space_pressed)
    return;

  if (! gimp_display_shell_keyboard_grab (shell, event))
    return;

  switch (shell->display->config->space_bar_action)
    {
    case GIMP_SPACE_BAR_ACTION_NONE:
      break;

    case GIMP_SPACE_BAR_ACTION_PAN:
      {
        GimpDeviceManager *manager;
        GimpDeviceInfo    *current_device;
        GimpCoords         coords;
        GdkModifierType    state = 0;

        manager = gimp_devices_get_manager (gimp);
        current_device = gimp_device_manager_get_current_device (manager);

        gimp_device_info_get_device_coords (current_device,
                                            gtk_widget_get_window (shell->canvas),
                                            &coords);
        gdk_event_get_state (event, &state);

        gimp_display_shell_start_scrolling (shell, event, state,
                                            coords.x, coords.y);
      }
      break;

    case GIMP_SPACE_BAR_ACTION_MOVE:
      {
        GimpTool *active_tool = tool_manager_get_active (gimp);

        if (active_tool || ! GIMP_IS_MOVE_TOOL (active_tool))
          {
            GdkModifierType state;

            shell->space_shaded_tool =
              gimp_object_get_name (active_tool->tool_info);

            gimp_context_set_tool (gimp_get_user_context (gimp),
                                   gimp_get_tool_info (gimp, "gimp-move-tool"));

            gdk_event_get_state (event, &state);

            gimp_display_shell_update_focus (shell, TRUE,
                                             NULL, state);
          }
      }
      break;
    }

  shell->space_pressed = TRUE;
}

static void
gimp_display_shell_space_released (GimpDisplayShell *shell,
                                   const GdkEvent   *event,
                                   const GimpCoords *image_coords)
{
  Gimp *gimp = gimp_display_get_gimp (shell->display);

  if (! shell->space_pressed && ! shell->space_release_pending)
    return;

  switch (shell->display->config->space_bar_action)
    {
    case GIMP_SPACE_BAR_ACTION_NONE:
      break;

    case GIMP_SPACE_BAR_ACTION_PAN:
      gimp_display_shell_stop_scrolling (shell, event);
      break;

    case GIMP_SPACE_BAR_ACTION_MOVE:
      if (shell->space_shaded_tool)
        {
          gimp_context_set_tool (gimp_get_user_context (gimp),
                                 gimp_get_tool_info (gimp,
                                                     shell->space_shaded_tool));
          shell->space_shaded_tool = NULL;

          if (gtk_widget_has_focus (shell->canvas))
            {
              GdkModifierType state;

              gdk_event_get_state (event, &state);

              gimp_display_shell_update_focus (shell, TRUE,
                                               image_coords, state);
            }
          else
            {
              gimp_display_shell_update_focus (shell, FALSE,
                                               image_coords, 0);
            }
        }
      break;
    }

  gimp_display_shell_keyboard_ungrab (shell, event);

  shell->space_pressed         = FALSE;
  shell->space_release_pending = FALSE;
}

static gboolean
gimp_display_shell_tab_pressed (GimpDisplayShell  *shell,
                                const GdkEventKey *kevent)
{
  GimpImageWindow *window  = gimp_display_shell_get_window (shell);
  GimpUIManager   *manager = gimp_image_window_get_ui_manager (window);
  GimpImage       *image   = gimp_display_get_image (shell->display);

  if (kevent->state & GDK_CONTROL_MASK)
    {
      if (image && ! gimp_image_is_empty (image))
        {
          if (kevent->keyval == GDK_KEY_Tab ||
              kevent->keyval == GDK_KEY_KP_Tab)
            gimp_display_shell_layer_select_init (shell,
                                                  1, kevent->time);
          else
            gimp_display_shell_layer_select_init (shell,
                                                  -1, kevent->time);

          return TRUE;
        }
    }
  else if (kevent->state & GDK_MOD1_MASK)
    {
      if (image)
        {
          if (kevent->keyval == GDK_KEY_Tab ||
              kevent->keyval == GDK_KEY_KP_Tab)
            gimp_ui_manager_activate_action (manager, "windows",
                                             "windows-show-display-next");
          else
            gimp_ui_manager_activate_action (manager, "windows",
                                             "windows-show-display-previous");

          return TRUE;
        }
    }
  else
    {
      gimp_ui_manager_activate_action (manager, "windows",
                                       "windows-hide-docks");

      return TRUE;
    }

  return FALSE;
}

static void
gimp_display_shell_update_focus (GimpDisplayShell *shell,
                                 gboolean          focus_in,
                                 const GimpCoords *image_coords,
                                 GdkModifierType   state)
{
  Gimp *gimp = gimp_display_get_gimp (shell->display);

  if (focus_in)
    {
      tool_manager_focus_display_active (gimp, shell->display);
      tool_manager_modifier_state_active (gimp, state, shell->display);
    }
  else
    {
      tool_manager_focus_display_active (gimp, NULL);
    }

  if (image_coords)
    tool_manager_oper_update_active (gimp,
                                     image_coords, state,
                                     shell->proximity,
                                     shell->display);
}

static void
gimp_display_shell_update_cursor (GimpDisplayShell *shell,
                                  const GimpCoords *display_coords,
                                  const GimpCoords *image_coords,
                                  GdkModifierType   state,
                                  gboolean          update_software_cursor)
{
  GimpDisplay *display = shell->display;
  Gimp        *gimp    = gimp_display_get_gimp (display);
  GimpImage   *image   = gimp_display_get_image (display);
  GimpTool    *active_tool;

  if (! shell->display->config->cursor_updating)
    return;

  active_tool = tool_manager_get_active (gimp);

  if (active_tool)
    {
      if ((! gimp_image_is_empty (image) ||
           gimp_tool_control_get_handle_empty_image (active_tool->control)) &&
          ! (state & (GDK_BUTTON1_MASK |
                      GDK_BUTTON2_MASK |
                      GDK_BUTTON3_MASK)))
        {
          tool_manager_cursor_update_active (gimp,
                                             image_coords, state,
                                             display);
        }
      else if (gimp_image_is_empty (image) &&
               ! gimp_tool_control_get_handle_empty_image (active_tool->control))
        {
          gimp_display_shell_set_cursor (shell,
                                         GIMP_CURSOR_MOUSE,
                                         gimp_tool_control_get_tool_cursor (active_tool->control),
                                         GIMP_CURSOR_MODIFIER_BAD);
        }
    }
  else
    {
      gimp_display_shell_set_cursor (shell,
                                     GIMP_CURSOR_MOUSE,
                                     GIMP_TOOL_CURSOR_NONE,
                                     GIMP_CURSOR_MODIFIER_BAD);
    }

  if (update_software_cursor)
    {
      GimpCursorPrecision precision = GIMP_CURSOR_PRECISION_PIXEL_CENTER;

      if (active_tool)
        precision = gimp_tool_control_get_precision (active_tool->control);

      gimp_display_shell_update_software_cursor (shell,
                                                 precision,
                                                 (gint) display_coords->x,
                                                 (gint) display_coords->y,
                                                 image_coords->x,
                                                 image_coords->y);
    }
}

static gboolean
gimp_display_shell_initialize_tool (GimpDisplayShell *shell,
                                    const GimpCoords *image_coords,
                                    GdkModifierType   state)
{
  GimpDisplay *display     = shell->display;
  GimpImage   *image       = gimp_display_get_image (display);
  Gimp        *gimp        = gimp_display_get_gimp (display);
  gboolean     initialized = FALSE;
  GimpTool    *active_tool;

  active_tool = tool_manager_get_active (gimp);

  if (active_tool &&
      (! gimp_image_is_empty (image) ||
       gimp_tool_control_get_handle_empty_image (active_tool->control)))
    {
      initialized = TRUE;

      /*  initialize the current tool if it has no drawable  */
      if (! active_tool->drawable)
        {
          initialized = tool_manager_initialize_active (gimp, display);
        }
      else if ((active_tool->drawable !=
                gimp_image_get_active_drawable (image)) &&
               (! gimp_tool_control_get_preserve (active_tool->control) &&
                (gimp_tool_control_get_dirty_mask (active_tool->control) &
                 GIMP_DIRTY_ACTIVE_DRAWABLE)))
        {
          GimpProcedure *procedure = g_object_get_data (G_OBJECT (active_tool),
                                                        "gimp-gegl-procedure");

          if (image == gimp_item_get_image (GIMP_ITEM (active_tool->drawable)))
            {
              /*  When changing between drawables if the *same* image,
               *  halt the tool so it doesn't get committed on tool
               *  change. This is a pure "probably better this way"
               *  decision because the user is likely changing their
               *  mind or was simply on the wrong layer. See bug
               *  #776370.
               */
              tool_manager_control_active (gimp, GIMP_TOOL_ACTION_HALT,
                                           active_tool->display);
            }

          if (procedure)
            {
              /*  We can't just recreate an operation tool, we must
               *  make sure the right stuff gets set on it, so
               *  re-activate the procedure that created it instead of
               *  just calling gimp_context_tool_changed(). See
               *  GimpGeglProcedure and bug #776370.
               */
              GimpImageWindow *window;
              GimpUIManager   *manager;

              window  = gimp_display_shell_get_window (shell);
              manager = gimp_image_window_get_ui_manager (window);

              gimp_filter_history_add (gimp, procedure);
              gimp_ui_manager_activate_action (manager, "filters",
                                               "filters-reshow");
            }
          else
            {
              /*  create a new one, deleting the current  */
              gimp_context_tool_changed (gimp_get_user_context (gimp));
            }

          /*  make sure the newly created tool has the right state  */
          gimp_display_shell_update_focus (shell, TRUE, image_coords, state);

          initialized = tool_manager_initialize_active (gimp, display);
        }
    }

  return initialized;
}

static void
gimp_display_shell_get_event_coords (GimpDisplayShell *shell,
                                     const GdkEvent   *event,
                                     GimpCoords       *display_coords,
                                     GdkModifierType  *state,
                                     guint32          *time)
{
  Gimp              *gimp = gimp_display_get_gimp (shell->display);
  GimpDeviceManager *manager;
  GimpDeviceInfo    *current_device;

  manager = gimp_devices_get_manager (gimp);
  current_device = gimp_device_manager_get_current_device (manager);

  gimp_device_info_get_event_coords (current_device,
                                     gtk_widget_get_window (shell->canvas),
                                     event,
                                     display_coords);

  gimp_device_info_get_event_state (current_device,
                                    gtk_widget_get_window (shell->canvas),
                                    event,
                                    state);

  *time = gdk_event_get_time (event);
}

static void
gimp_display_shell_untransform_event_coords (GimpDisplayShell *shell,
                                             const GimpCoords *display_coords,
                                             GimpCoords       *image_coords,
                                             gboolean         *update_software_cursor)
{
  Gimp     *gimp = gimp_display_get_gimp (shell->display);
  GimpTool *active_tool;

  /*  GimpCoords passed to tools are ALWAYS in image coordinates  */
  gimp_display_shell_untransform_coords (shell,
                                         display_coords,
                                         image_coords);

  active_tool = tool_manager_get_active (gimp);

  if (active_tool && gimp_tool_control_get_snap_to (active_tool->control))
    {
      gint x, y, width, height;

      gimp_tool_control_get_snap_offsets (active_tool->control,
                                          &x, &y, &width, &height);

      if (gimp_display_shell_snap_coords (shell,
                                          image_coords,
                                          x, y, width, height))
        {
          if (update_software_cursor)
            *update_software_cursor = TRUE;
        }
    }
}

/* gimp_display_shell_compress_motion:
 *
 * This function walks the whole GDK event queue seeking motion events
 * corresponding to the widget 'widget'.  If it finds any it will
 * remove them from the queue, and return the most recent motion event.
 * Otherwise it will return NULL.
 *
 * The gimp_display_shell_compress_motion function source may be re-used under
 * the XFree86-style license. <adam@gimp.org>
 */
static GdkEvent *
gimp_display_shell_compress_motion (GimpDisplayShell *shell)
{
  GList       *requeued_events = NULL;
  const GList *list;
  GdkEvent    *last_motion = NULL;

  /*  Move the entire GDK event queue to a private list, filtering
   *  out any motion events for the desired widget.
   */
  while (gdk_events_pending ())
    {
      GdkEvent *event = gdk_event_get ();

      if (!event)
        {
          /* Do nothing */
        }
      else if ((gtk_get_event_widget (event) == shell->canvas) &&
               (event->any.type == GDK_MOTION_NOTIFY))
        {
          if (last_motion)
            gdk_event_free (last_motion);

          last_motion = event;
        }
      else if ((gtk_get_event_widget (event) == shell->canvas) &&
               (event->any.type == GDK_BUTTON_RELEASE))
        {
          requeued_events = g_list_prepend (requeued_events, event);

          while (gdk_events_pending ())
            if ((event = gdk_event_get ()))
              requeued_events = g_list_prepend (requeued_events, event);

          break;
        }
      else
        {
          requeued_events = g_list_prepend (requeued_events, event);
        }
    }

  /* Replay the remains of our private event list back into the
   * event queue in order.
   */
  requeued_events = g_list_reverse (requeued_events);

  for (list = requeued_events; list; list = g_list_next (list))
    {
      GdkEvent *event = list->data;

      gdk_event_put (event);
      gdk_event_free (event);
    }

  g_list_free (requeued_events);

  return last_motion;
}
