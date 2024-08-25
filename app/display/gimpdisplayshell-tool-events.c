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

#include <math.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"
#include "tools/tools-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimp-filter-history.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-item.h"
#include "core/gimpitem.h"

#include "menus/menus.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpcontrollers.h"
#include "widgets/gimpcontrollerkeyboard.h"
#include "widgets/gimpcontrollerwheel.h"
#include "widgets/gimpdeviceinfo.h"
#include "widgets/gimpdeviceinfo-coords.h"
#include "widgets/gimpdevicemanager.h"
#include "widgets/gimpdevices.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdoubleaction.h"
#include "widgets/gimpenumaction.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "tools/gimpguidetool.h"
#include "tools/gimpmovetool.h"
#include "tools/gimppainttool.h"
#include "tools/gimpsamplepointtool.h"
#include "tools/gimptoolcontrol.h"
#include "tools/tool_manager.h"

#include "gimpcanvas.h"
#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
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
#include "gimpmodifiersmanager.h"
#include "gimpimagewindow.h"
#include "gimpmotionbuffer.h"
#include "gimpstatusbar.h"

#include "gimp-intl.h"
#include "gimp-log.h"


/*  local function prototypes  */

static GdkModifierType
                  gimp_display_shell_key_to_state             (gint               key);
static GdkModifierType
                  gimp_display_shell_button_to_state          (gint               button);

static void       gimp_display_shell_proximity_in             (GimpDisplayShell  *shell);
static void       gimp_display_shell_proximity_out            (GimpDisplayShell  *shell);

static gboolean   gimp_display_shell_check_device             (GimpDisplayShell  *shell,
                                                               GdkEvent          *event,
                                                               gboolean          *device_changed);
static void       gimp_display_shell_check_device_cursor      (GimpDisplayShell  *shell);

static void       gimp_display_shell_start_scrolling          (GimpDisplayShell  *shell,
                                                               const GdkEvent    *event,
                                                               GdkModifierType    state,
                                                               gint               x,
                                                               gint               y);
static void       gimp_display_shell_stop_scrolling           (GimpDisplayShell  *shell,
                                                               const GdkEvent    *event);
static void       gimp_display_shell_handle_scrolling         (GimpDisplayShell  *shell,
                                                               GdkModifierType    state,
                                                               gint               x,
                                                               gint               y);

static void       gimp_display_shell_rotate_gesture_maybe_get_state (GtkGestureRotate *gesture,
                                                                     GdkEventSequence *sequence,
                                                                     guint            *maybe_out_state);

static void       gimp_display_shell_space_pressed            (GimpDisplayShell  *shell,
                                                               const GdkEvent    *event);
static void       gimp_display_shell_released                 (GimpDisplayShell  *shell,
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

static void       gimp_display_shell_activate_action          (Gimp              *gimp,
                                                               const gchar       *action_desc,
                                                               GVariant          *value);

static gboolean   gimp_display_triggers_context_menu          (const GdkEvent    *event,
                                                               GimpDisplayShell  *shell,
                                                               Gimp              *gimp,
                                                               const GimpCoords  *image_coords,
                                                               gboolean           force);


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

  /* When a display is being created, we may have some weird cases where
   * we get never-ending focus events fighting for which image window
   * should have focus, in an infinite loop. See #11957.
   * Just ignore focus events in these cases, and in particular, don't
   * set the display from such focus events.
   */
  if (! gimp_displays_accept_focus_events (gimp))
    return TRUE;

  switch (event->type)
    {
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
      {
        GdkEventKey *kevent = (GdkEventKey *) event;

        if (gimp->busy)
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
                shell->mod_action != GIMP_MODIFIER_ACTION_NONE)
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
            GimpUIManager *manager = menus_get_image_manager_singleton (shell->display->gimp);

            gimp_ui_manager_activate_action (manager, "file", "file-open");
          }
        return TRUE;
      }
      break;

    case GDK_BUTTON_PRESS:
      if (gdk_event_triggers_context_menu (event))
        {
          gimp_ui_manager_ui_popup_at_pointer (shell->popup_manager,
                                               "/image-menubar",
                                               GTK_WIDGET (shell),
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
  GimpDisplay          *display;
  GimpImage            *image;
  Gimp                 *gimp;
  GimpModifiersManager *mod_manager;
  GimpCoords            display_coords;
  GimpCoords            image_coords;
  GdkModifierType       state;
  guint32               time;
  gboolean              device_changed   = FALSE;
  gboolean              return_val       = FALSE;
  gboolean              update_sw_cursor = FALSE;

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

  if (gimp_display_shell_check_device (shell, event, &device_changed))
    return TRUE;

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

  mod_manager = GIMP_MODIFIERS_MANAGER (shell->display->config->modifiers_manager);

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
        if (shell->grab_pointer)
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
            if (shell->grab_pointer)
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
            if (shell->grab_pointer)
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
        if (gimp->busy                                     ||
            shell->mod_action != GIMP_MODIFIER_ACTION_NONE ||
            shell->grab_pointer                            ||
            shell->button1_release_pending)
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

        if (bevent->button == 1)
          {
            if (! gimp_display_shell_pointer_grab (shell, event,
                                                   GDK_POINTER_MOTION_MASK |
                                                   GDK_BUTTON_RELEASE_MASK))
              return TRUE;

            if (gimp_display_shell_initialize_tool (shell,
                                                    &image_coords, state))
              {
                GimpTool       *active_tool;
                GimpMotionMode  motion_mode;
                GimpCoords      last_motion;

                active_tool = tool_manager_get_active (gimp);
                motion_mode = gimp_tool_control_get_motion_mode (active_tool->control);

                if (motion_mode == GIMP_MOTION_MODE_EXACT)
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
        else
          {
            GdkDevice            *device;
            GimpModifierAction    action;
            const gchar          *action_desc = NULL;

            device = gdk_event_get_source_device (event);
            action = gimp_modifiers_manager_get_action (mod_manager, device,
                                                        bevent->button, bevent->state,
                                                        &action_desc);
            shell->mod_action = action;
            switch (action)
              {
              case GIMP_MODIFIER_ACTION_MENU:
                gimp_display_triggers_context_menu (event, shell, gimp, &image_coords, TRUE);
                shell->mod_action = GIMP_MODIFIER_ACTION_NONE;
                break;
              case GIMP_MODIFIER_ACTION_PANNING:
              case GIMP_MODIFIER_ACTION_ZOOMING:
              case GIMP_MODIFIER_ACTION_ROTATING:
              case GIMP_MODIFIER_ACTION_STEP_ROTATING:
              case GIMP_MODIFIER_ACTION_LAYER_PICKING:
              case GIMP_MODIFIER_ACTION_BRUSH_PIXEL_SIZE:
              case GIMP_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE:
              case GIMP_MODIFIER_ACTION_TOOL_OPACITY:
                gimp_display_shell_start_scrolling (shell, event, state,
                                                    bevent->x, bevent->y);
                break;
              case GIMP_MODIFIER_ACTION_ACTION:
                shell->mod_action_desc = g_strdup (action_desc);
                break;
              case GIMP_MODIFIER_ACTION_NONE:
                gimp_display_triggers_context_menu (event, shell, gimp, &image_coords, FALSE);
                break;
              }
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

        if (bevent->button == 1 && shell->button1_release_pending)
          {
            gimp_display_shell_released (shell, event, NULL);
            return TRUE;
          }

        if (gimp->busy)
          return TRUE;

        active_tool = tool_manager_get_active (gimp);

        state &= ~gimp_display_shell_button_to_state (bevent->button);

        if (bevent->button == 1)
          {
            /*  If we don't have a grab, this is a release paired with
             *  a button press we intentionally ignored because we had
             *  a grab on another device at the time of the press
             */
            if (! shell->grab_pointer || shell->mod_action != GIMP_MODIFIER_ACTION_NONE)
              return TRUE;

            if (active_tool &&
                (! gimp_image_is_empty (image) ||
                 gimp_tool_control_get_handle_empty_image (active_tool->control)))
              {
                gimp_motion_buffer_end_stroke (shell->motion_buffer);

                gdk_window_set_event_compression (
                  gtk_widget_get_window (canvas), TRUE);

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

            gimp_display_shell_pointer_ungrab (shell, event);
          }
        else
          {
            switch (shell->mod_action)
              {
              case GIMP_MODIFIER_ACTION_MENU:
                break;
              case GIMP_MODIFIER_ACTION_PANNING:
              case GIMP_MODIFIER_ACTION_ZOOMING:
              case GIMP_MODIFIER_ACTION_ROTATING:
              case GIMP_MODIFIER_ACTION_STEP_ROTATING:
              case GIMP_MODIFIER_ACTION_LAYER_PICKING:
                if (shell->mod_action != GIMP_MODIFIER_ACTION_NONE &&
                    ! shell->button1_release_pending &&
                    (! shell->space_release_pending ||
                     shell->display->config->space_bar_action != GIMP_SPACE_BAR_ACTION_PAN))
                  gimp_display_shell_stop_scrolling (shell, event);
                break;
              case GIMP_MODIFIER_ACTION_BRUSH_PIXEL_SIZE:
              case GIMP_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE:
              case GIMP_MODIFIER_ACTION_TOOL_OPACITY:
                gimp_display_shell_stop_scrolling (shell, event);
                break;
              case GIMP_MODIFIER_ACTION_ACTION:
                gimp_display_shell_activate_action (gimp, shell->mod_action_desc, NULL);
                g_clear_pointer (&shell->mod_action_desc, g_free);
                break;
              case GIMP_MODIFIER_ACTION_NONE:
                break;
              }

            shell->mod_action = GIMP_MODIFIER_ACTION_NONE;
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
            if (state & gimp_get_toggle_behavior_mask ())
              {
                gdouble delta;

                switch (sevent->direction)
                  {
                  case GDK_SCROLL_UP:
                    gimp_display_shell_scale (shell,
                                              GIMP_ZOOM_IN,
                                              0.0,
                                              GIMP_ZOOM_FOCUS_POINTER);
                    break;

                  case GDK_SCROLL_DOWN:
                    gimp_display_shell_scale (shell,
                                              GIMP_ZOOM_OUT,
                                              0.0,
                                              GIMP_ZOOM_FOCUS_POINTER);
                    break;

                  case GDK_SCROLL_SMOOTH:
                    gdk_event_get_scroll_deltas (event, NULL, &delta);
                    gimp_display_shell_scale (shell,
                                              GIMP_ZOOM_SMOOTH,
                                              delta,
                                              GIMP_ZOOM_FOCUS_POINTER);
                    break;

                  default:
                    break;
                  }
              }
            else
              {
                gdouble value_x;
                gdouble value_y;

                gimp_scroll_adjustment_values (sevent,
                                               shell->hsbdata,
                                               shell->vsbdata,
                                               &value_x, &value_y);

                gtk_adjustment_set_value (shell->hsbdata, value_x);
                gtk_adjustment_set_value (shell->vsbdata, value_y);
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
        GdkEventMotion *mevent = (GdkEventMotion *) event;

        if (gimp->busy)
          return TRUE;

        /*  call proximity_in() here because the pointer might already
         *  be in proximity when the canvas starts to receive events,
         *  like when a new image has been created into an empty
         *  display
         */
        gimp_display_shell_proximity_in (shell);
        update_sw_cursor = TRUE;

        if (shell->mod_action != GIMP_MODIFIER_ACTION_NONE ||
            shell->space_release_pending)
          {
            gimp_display_shell_handle_scrolling (shell,
                                                 state, mevent->x, mevent->y);
          }
        else if (state & GDK_BUTTON1_MASK)
          {
            GimpTool       *active_tool;
            GimpMotionMode  motion_mode;

            active_tool = tool_manager_get_active (gimp);
            motion_mode = gimp_tool_control_get_motion_mode (
                            active_tool->control);

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

            if (gimp_display_shell_key_to_state (kevent->keyval) == GDK_MOD1_MASK)
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
                if (shell->button1_release_pending)
                  shell->space_release_pending = TRUE;
                else
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

        if (shell->mod_action == GIMP_MODIFIER_ACTION_LAYER_PICKING)
          {
            /* As a special exception, we finalize the layer picking
             * action in the key release event. This allows one to
             * click multiple times and keep a state of the last picked
             * layer.
             * */
            GimpStatusbar *statusbar;

            statusbar = gimp_display_shell_get_statusbar (shell);
            gimp_statusbar_pop_temp (statusbar);

            shell->picked_layer = NULL;
            shell->mod_action = GIMP_MODIFIER_ACTION_NONE;
          }
        else if (shell->mod_action != GIMP_MODIFIER_ACTION_NONE &&
                 (state & gimp_get_all_modifiers_mask ()) == 0)
          {
            gimp_display_shell_stop_scrolling (shell, event);
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
                if ((state & GDK_BUTTON1_MASK))
                  {
                    shell->button1_release_pending = TRUE;
                    shell->space_release_pending   = FALSE;
                    /* We need to ungrab the pointer in order to catch
                     * button release events.
                     */
                    if (shell->grab_pointer)
                      gimp_display_shell_pointer_ungrab (shell, event);
                  }
                else
                  {
                    gimp_display_shell_released (shell, event, NULL);
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
gimp_display_shell_zoom_gesture_begin (GtkGestureZoom   *gesture,
                                       GdkEventSequence *sequence,
                                       GimpDisplayShell *shell)
{
  shell->last_zoom_scale = gtk_gesture_zoom_get_scale_delta (gesture);
}

void
gimp_display_shell_zoom_gesture_update (GtkGestureZoom   *gesture,
                                        GdkEventSequence *sequence,
                                        GimpDisplayShell *shell)
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

  gimp_display_shell_scale (shell,
                            GIMP_ZOOM_PINCH,
                            delta,
                            GIMP_ZOOM_FOCUS_POINTER);
}

void
gimp_display_shell_zoom_gesture_end (GtkGestureZoom   *gesture,
                                     GdkEventSequence *sequence,
                                     GimpDisplayShell *shell)
{
  shell->zoom_gesture_active = FALSE;
}

void
gimp_display_shell_rotate_gesture_begin (GtkGestureRotate *gesture,
                                         GdkEventSequence *sequence,
                                         GimpDisplayShell *shell)
{

  shell->initial_gesture_rotate_angle = shell->rotate_angle;
  shell->last_gesture_rotate_state    = 0;
  gimp_display_shell_rotate_gesture_maybe_get_state (gesture, sequence,
                                                     &shell->last_gesture_rotate_state);
}

void
gimp_display_shell_rotate_gesture_update (GtkGestureRotate *gesture,
                                          GdkEventSequence *sequence,
                                          GimpDisplayShell *shell)
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

  gimp_display_shell_rotate_gesture_maybe_get_state (gesture, sequence,
                                                     &shell->last_gesture_rotate_state);

  constrain = (shell->last_gesture_rotate_state & GDK_CONTROL_MASK) ? TRUE : FALSE;

  gimp_display_shell_rotate_to (shell, constrain ? RINT (angle / 15.0) * 15.0 : angle);
}

void
gimp_display_shell_rotate_gesture_end (GtkGestureRotate *gesture,
                                       GdkEventSequence *sequence,
                                       GimpDisplayShell *shell)
{
  shell->rotate_gesture_active = FALSE;
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

          if (gimp_display_shell_pointer_grab (shell, (GdkEvent *) event,
                                               GDK_POINTER_MOTION_MASK |
                                               GDK_BUTTON_RELEASE_MASK))
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

static gboolean
gimp_display_shell_check_device (GimpDisplayShell *shell,
                                 GdkEvent         *event,
                                 gboolean         *device_changed)
{
  Gimp      *gimp = gimp_display_get_gimp (shell->display);
  GdkDevice *device;
  GdkDevice *grab_device;

  /*  Find out what device the event occurred upon  */
  device = gimp_devices_get_from_event (gimp, event, &grab_device);

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
              GIMP_LOG (TOOL_EVENTS,
                        "ignoring pointer event from '%s' while waiting for event from '%s'\n",
                        gdk_device_get_name (device),
                        gdk_device_get_name (shell->grab_pointer_source));
              return TRUE;
            }
        }

      if (! gimp->busy && gimp_devices_check_change (gimp, device))
        {
          gimp_display_shell_check_device_cursor (shell);

          *device_changed = TRUE;
        }
    }

  return FALSE;
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
  GimpModifierAction mod_action = shell->mod_action;

  if (mod_action == GIMP_MODIFIER_ACTION_NONE && shell->space_release_pending)
    {
      /* XXX The space actions are still hard-coded (instead of
       * customizable throguh GimpModifiersManager). It might be
       * interesting to make them customizable later.
       */
      GdkModifierType state = 0;

      gdk_event_get_state (event, &state);
      state &= gimp_get_all_modifiers_mask ();

      if (state == 0)
        mod_action = GIMP_MODIFIER_ACTION_PANNING;
      else if (state == gimp_get_extend_selection_mask ())
        mod_action = GIMP_MODIFIER_ACTION_ROTATING;
      else if (state == (gimp_get_extend_selection_mask () | GDK_CONTROL_MASK))
        mod_action = GIMP_MODIFIER_ACTION_STEP_ROTATING;
      else if (state == gimp_get_toggle_behavior_mask ())
        mod_action = GIMP_MODIFIER_ACTION_ZOOMING;
    }

  shell->scroll_start_x    = x;
  shell->scroll_start_y    = y;
  shell->scroll_last_x     = x;
  shell->scroll_last_y     = y;
  shell->rotate_drag_angle = shell->rotate_angle;

  switch (mod_action)
    {
    case GIMP_MODIFIER_ACTION_ROTATING:
    case GIMP_MODIFIER_ACTION_STEP_ROTATING:
      gimp_display_shell_set_override_cursor (shell,
                                              (GimpCursorType) GDK_EXCHANGE);
      break;
    case GIMP_MODIFIER_ACTION_ZOOMING:
      gimp_display_shell_set_override_cursor (shell,
                                              (GimpCursorType) GIMP_CURSOR_ZOOM);
      break;
    case GIMP_MODIFIER_ACTION_LAYER_PICKING:
        {
          GimpImage  *image   = gimp_display_get_image (shell->display);
          GimpLayer  *layer;
          GimpCoords  image_coords;
          GimpCoords  display_coords;
          guint32     time;

          gimp_display_shell_set_override_cursor (shell,
                                                  (GimpCursorType) GIMP_CURSOR_CROSSHAIR);

          gimp_display_shell_get_event_coords (shell, event,
                                               &display_coords,
                                               &state, &time);
          gimp_display_shell_untransform_event_coords (shell,
                                                       &display_coords, &image_coords,
                                                       NULL);
          layer = gimp_image_pick_layer (image,
                                         (gint) image_coords.x,
                                         (gint) image_coords.y,
                                         shell->picked_layer);

          if (layer && ! gimp_image_get_floating_selection (image))
            {
              GList *layers = gimp_image_get_selected_layers (image);

              if (g_list_length (layers) != 1 || layer != layers->data)
                {
                  GimpStatusbar *statusbar;

                  layers = g_list_prepend (NULL, layer);
                  gimp_image_set_selected_layers (image, layers);
                  g_list_free (layers);

                  statusbar = gimp_display_shell_get_statusbar (shell);
                  gimp_statusbar_push_temp (statusbar, GIMP_MESSAGE_INFO,
                                            GIMP_ICON_LAYER,
                                            _("Layer picked: '%s'"),
                                            gimp_object_get_name (layer));
                }
              shell->picked_layer = layer;
            }
        }
      break;
    case GIMP_MODIFIER_ACTION_BRUSH_PIXEL_SIZE:
    case GIMP_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE:
    case GIMP_MODIFIER_ACTION_TOOL_OPACITY:
        {
          Gimp     *gimp        = gimp_display_get_gimp (shell->display);
          GimpTool *active_tool = tool_manager_get_active (gimp);

          if (GIMP_IS_PAINT_TOOL (active_tool))
            gimp_paint_tool_force_draw (GIMP_PAINT_TOOL (active_tool), TRUE);
        }
    case GIMP_MODIFIER_ACTION_MENU:
    case GIMP_MODIFIER_ACTION_PANNING:
    case GIMP_MODIFIER_ACTION_ACTION:
    case GIMP_MODIFIER_ACTION_NONE:
      gimp_display_shell_set_override_cursor (shell,
                                              (GimpCursorType) GDK_FLEUR);
      break;
    }
}

static void
gimp_display_shell_stop_scrolling (GimpDisplayShell *shell,
                                   const GdkEvent   *event)
{
  gimp_display_shell_unset_override_cursor (shell);

  switch (shell->mod_action)
    {
    case GIMP_MODIFIER_ACTION_BRUSH_PIXEL_SIZE:
    case GIMP_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE:
        {
          Gimp     *gimp        = gimp_display_get_gimp (shell->display);
          GimpTool *active_tool = tool_manager_get_active (gimp);

          if (GIMP_IS_PAINT_TOOL (active_tool))
            gimp_paint_tool_force_draw (GIMP_PAINT_TOOL (active_tool), FALSE);
        }
      break;
    default:
      break;
    }

  shell->mod_action = GIMP_MODIFIER_ACTION_NONE;

  shell->scroll_start_x    = 0;
  shell->scroll_start_y    = 0;
  shell->scroll_last_x     = 0;
  shell->scroll_last_y     = 0;
  shell->rotate_drag_angle = 0.0;
}

static void
gimp_display_shell_handle_scrolling (GimpDisplayShell *shell,
                                     GdkModifierType   state,
                                     gint              x,
                                     gint              y)
{
  GimpModifierAction mod_action      = shell->mod_action;
  gboolean           constrain       = FALSE;
  gboolean           size_update_pos = TRUE;
  gdouble            size_multiplier = 1.0;

  if (mod_action == GIMP_MODIFIER_ACTION_NONE && shell->space_release_pending)
    {
      state &= gimp_get_all_modifiers_mask ();

      if (state == 0)
        mod_action = GIMP_MODIFIER_ACTION_PANNING;
      else if (state == gimp_get_extend_selection_mask ())
        mod_action = GIMP_MODIFIER_ACTION_ROTATING;
      else if (state == (gimp_get_extend_selection_mask () | GDK_CONTROL_MASK))
        mod_action = GIMP_MODIFIER_ACTION_STEP_ROTATING;
      else if (state == gimp_get_toggle_behavior_mask ())
        mod_action = GIMP_MODIFIER_ACTION_ZOOMING;
    }
  else if (mod_action == GIMP_MODIFIER_ACTION_ROTATING ||
           mod_action == GIMP_MODIFIER_ACTION_STEP_ROTATING)
    {
      state &= gimp_get_all_modifiers_mask ();

      /* Allow switching from the constrained to non-constrained
       * variant, back and forth, during a single scroll.
       */
      if (state == gimp_get_extend_selection_mask ())
        mod_action = GIMP_MODIFIER_ACTION_ROTATING;
      else if (state == (gimp_get_extend_selection_mask () | GDK_CONTROL_MASK))
        mod_action = GIMP_MODIFIER_ACTION_STEP_ROTATING;
    }

  switch (mod_action)
    {
    case GIMP_MODIFIER_ACTION_STEP_ROTATING:
      constrain = TRUE;
    case GIMP_MODIFIER_ACTION_ROTATING:

      gimp_display_shell_rotate_drag (shell,
                                      shell->scroll_last_x,
                                      shell->scroll_last_y,
                                      x,
                                      y,
                                      constrain);
      break;
    case GIMP_MODIFIER_ACTION_ZOOMING:
      gimp_display_shell_scale_drag (shell,
                                     shell->scroll_start_x,
                                     shell->scroll_start_y,
                                     shell->scroll_last_x - x,
                                     shell->scroll_last_y - y);
      break;
    case GIMP_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE:
      size_multiplier = 2.0;
      size_update_pos = FALSE;
    case GIMP_MODIFIER_ACTION_BRUSH_PIXEL_SIZE:
        {
          GimpDisplay *display     = shell->display;
          Gimp        *gimp        = gimp_display_get_gimp (display);
          GimpTool    *active_tool = tool_manager_get_active (gimp);
          const gchar *action;
          gint         size;

          /* Size in image pixels: distance between start and current
           * position.
           */
          size = (gint) (sqrt (pow ((x - shell->scroll_start_x) / shell->scale_x, 2) +
                               pow ((y - shell->scroll_start_y) / shell->scale_y, 2)));

          /* TODO: different logics with "lock brush to view". */
          /* TODO 2: scale aware? */
          action = gimp_tool_control_get_action_pixel_size (active_tool->control);
          if (action)
            {
              gimp_display_shell_activate_action (gimp, action,
                                                  g_variant_new_double ((gdouble) size * size_multiplier));

              if (size_update_pos)
                {
                  GimpCoords display_coords;
                  GimpCoords coords;

                  display_coords.x = shell->scroll_start_x + (x - shell->scroll_start_x) / 2;
                  display_coords.y = shell->scroll_start_y + (y - shell->scroll_start_y) / 2;
                  gimp_display_shell_untransform_event_coords (shell,
                                                               &display_coords, &coords,
                                                               NULL);
                  gimp_tool_oper_update (active_tool, &coords, 0, TRUE, display);
                }
            }
          else
            {
              action = gimp_tool_control_get_action_size (active_tool->control);

              if (action)
                {
                  /* Special trick with these enum actions. If using any
                   * positive value, we get the GIMP_ACTION_SELECT_SET behavior
                   * which sets to the given value.
                   */
                  gimp_display_shell_activate_action (gimp, action,
                                                      g_variant_new_int32 (size));
                }
            }
        }
      break;
    case GIMP_MODIFIER_ACTION_TOOL_OPACITY:
        {
          GimpDisplay *display     = shell->display;
          Gimp        *gimp        = gimp_display_get_gimp (display);
          GimpTool    *active_tool = tool_manager_get_active (gimp);
          const gchar *action;
          gint         size;

          /* Size in image pixels: distance between start and current
           * position.
           */
          size = (gint) (sqrt (pow ((x - shell->scroll_start_x) / shell->scale_x, 2) +
                               pow ((y - shell->scroll_start_y) / shell->scale_y, 2)));

          action = gimp_tool_control_get_action_opacity (active_tool->control);

          if (action)
            {
              /* Special trick with these enum actions. If using any
               * positive value, we get the GIMP_ACTION_SELECT_SET behavior
               * which sets to the given value.
               */
              gimp_display_shell_activate_action (gimp, action,
                                                  g_variant_new_int32 (size));
            }
        }
      break;
    case GIMP_MODIFIER_ACTION_PANNING:
      gimp_display_shell_scroll (shell,
                                 shell->scroll_last_x - x,
                                 shell->scroll_last_y - y);
      break;
    case GIMP_MODIFIER_ACTION_LAYER_PICKING:
      /* Do nothing. We only pick the layer on click. */
    case GIMP_MODIFIER_ACTION_MENU:
    case GIMP_MODIFIER_ACTION_NONE:
    case GIMP_MODIFIER_ACTION_ACTION:
      break;
    }

  shell->scroll_last_x = x;
  shell->scroll_last_y = y;
}

static void
gimp_display_shell_rotate_gesture_maybe_get_state (GtkGestureRotate *gesture,
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
gimp_display_shell_space_pressed (GimpDisplayShell *shell,
                                  const GdkEvent   *event)
{
  Gimp *gimp = gimp_display_get_gimp (shell->display);

  if (shell->space_release_pending || shell->mod_action != GIMP_MODIFIER_ACTION_NONE)
    return;

  shell->space_release_pending = TRUE;

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
}

static void
gimp_display_shell_released (GimpDisplayShell *shell,
                             const GdkEvent   *event,
                             const GimpCoords *image_coords)
{
  Gimp *gimp = gimp_display_get_gimp (shell->display);

  if (! shell->space_release_pending &&
      ! shell->button1_release_pending)
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

  shell->space_release_pending   = FALSE;
  shell->button1_release_pending = FALSE;
}

static gboolean
gimp_display_shell_tab_pressed (GimpDisplayShell  *shell,
                                const GdkEventKey *kevent)
{
  GimpUIManager *manager = menus_get_image_manager_singleton (shell->display->gimp);
  GimpImage     *image   = gimp_display_get_image (shell->display);

  if (kevent->state & GDK_CONTROL_MASK)
    {
      if (image && ! gimp_image_is_empty (image))
        {
          if (kevent->keyval == GDK_KEY_Tab ||
              kevent->keyval == GDK_KEY_KP_Tab)
            gimp_display_shell_layer_select_init (shell, (GdkEvent *) kevent,
                                                  1);
          else
            gimp_display_shell_layer_select_init (shell, (GdkEvent *) kevent,
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

  if (active_tool && image)
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
      /*  initialize the current tool if it has no drawables  */
      if (! active_tool->drawables)
        {
          initialized = tool_manager_initialize_active (gimp, display);
        }
      else if (! gimp_image_equal_selected_drawables (image, active_tool->drawables) &&
               (! gimp_tool_control_get_preserve (active_tool->control) &&
                (gimp_tool_control_get_dirty_mask (active_tool->control) &
                 GIMP_DIRTY_ACTIVE_DRAWABLE)))
        {
          GimpProcedure *procedure = g_object_get_data (G_OBJECT (active_tool),
                                                        "gimp-gegl-procedure");

          if (image == gimp_item_get_image (GIMP_ITEM (active_tool->drawables->data)))
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
                gimp,
                gimp_tool_control_get_dirty_action (active_tool->control),
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
              GimpUIManager *manager;

              manager = menus_get_image_manager_singleton (shell->display->gimp);

              gimp_filter_history_add (gimp, procedure);
              gimp_ui_manager_activate_action (manager, "filters",
                                               "filters-reshow");

              /*  the procedure already initialized the tool; don't
               *  reinitialize it below, since this can lead to errors.
               */
              initialized = TRUE;
            }
          else
            {
              /*  create a new one, deleting the current  */
              gimp_context_tool_changed (gimp_get_user_context (gimp));
            }

          /*  make sure the newly created tool has the right state  */
          gimp_display_shell_update_focus (shell, TRUE, image_coords, state);

          if (! initialized)
            initialized = tool_manager_initialize_active (gimp, display);
        }
      else
        {
          initialized = TRUE;
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

static void
gimp_display_shell_activate_action (Gimp        *gimp,
                                    const gchar *action_name,
                                    GVariant    *value)
{
  g_return_if_fail (action_name != NULL);

  if (action_name)
    {
      GAction *action;

      action = g_action_map_lookup_action (G_ACTION_MAP (gimp->app), action_name);

      if (action == NULL)
        {
          g_printerr ("%s: ignoring unknown action '%s'.\n",
                      G_STRFUNC, action_name);
        }
      else if (GIMP_IS_ENUM_ACTION (action) &&
               GIMP_ENUM_ACTION (action)->value_variable)
        {
          gimp_action_emit_activate (GIMP_ACTION (action), value);
        }
      else if (GIMP_IS_DOUBLE_ACTION (action))
        {
          gimp_action_emit_activate (GIMP_ACTION (action), value);
        }
      else
        {
          gimp_action_activate (GIMP_ACTION (action));
        }
    }
}

/* Replace gdk_event_triggers_context_menu() as we don't want to trigger
 * anymore on right click, as we have our own system.
 * But we trigger with %GDK_MODIFIER_INTENT_CONTEXT_MENU mask.
 */
static gboolean
gimp_display_triggers_context_menu (const GdkEvent   *event,
                                    GimpDisplayShell *shell,
                                    Gimp             *gimp,
                                    const GimpCoords *image_coords,
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
          GimpUIManager   *ui_manager;
          const gchar     *ui_path;

          ui_manager = tool_manager_get_popup_active (gimp,
                                                      image_coords,
                                                      bevent->state,
                                                      shell->display,
                                                      &ui_path);

          if (! ui_manager)
            {
              ui_manager = shell->popup_manager;
              ui_path    = "/image-menubar";
            }

          gimp_ui_manager_ui_popup_at_pointer (ui_manager, ui_path,
                                               GTK_WIDGET (shell),
                                               event, NULL, NULL);
          return TRUE;
        }
    }

  return FALSE;
}
