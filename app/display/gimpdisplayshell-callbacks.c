/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"
#include "tools/tools-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-qmask.h"
#include "core/gimplayer.h"
#include "core/gimptoolinfo.h"

#include "libgimptool/gimptoolcontrol.h"

#include "tools/gimpmovetool.h"
#include "tools/tool_manager.h"

#include "widgets/gimpcursor.h"
#include "widgets/gimpdevices.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpitemfactory.h"

#include "gui/dialogs.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-cursor.h"
#include "gimpdisplayshell-layer-select.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-selection.h"
#include "gimpdisplayshell-title.h"
#include "gimpdisplayshell-transform.h"
#include "gimpnavigationview.h"
#include "gimpstatusbar.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void     gimp_display_shell_vscrollbar_update (GtkAdjustment    *adjustment,
                                                      GimpDisplayShell *shell);
static void     gimp_display_shell_hscrollbar_update (GtkAdjustment    *adjustment,
                                                      GimpDisplayShell *shell);

static gboolean gimp_display_shell_get_coords        (GimpDisplayShell *shell,
                                                      GdkEvent         *event,
                                                      GdkDevice        *device,
                                                      GimpCoords       *coords);
static void     gimp_display_shell_get_device_coords (GimpDisplayShell *shell,
                                                      GdkDevice        *device,
                                                      GimpCoords       *coords);
static gboolean gimp_display_shell_get_state         (GimpDisplayShell *shell,
                                                      GdkEvent         *event,
                                                      GdkDevice        *device,
                                                      GdkModifierType  *state);
static void     gimp_display_shell_get_device_state  (GimpDisplayShell *shell,
                                                      GdkDevice        *device,
                                                      GdkModifierType  *state);

static GdkModifierType
                gimp_display_shell_key_to_state      (gint              key);

static void gimp_display_shell_update_tool_modifiers (GimpDisplayShell *shell,
                                                      GdkModifierType   old_state,
                                                      GdkModifierType   current_state);

static void  gimp_display_shell_origin_menu_position (GtkMenu          *menu,
                                                      gint             *x,
                                                      gint             *y,
                                                      gpointer          data);
static void     gimp_display_shell_origin_menu_popup (GimpDisplayShell *shell,
                                                      guint             button,
                                                      guint32           time);

GdkEvent *      gimp_display_shell_compress_motion   (GimpDisplayShell *shell);


/*  public functions  */

gboolean
gimp_display_shell_events (GtkWidget        *widget,
                           GdkEvent         *event,
                           GimpDisplayShell *shell)
{
  Gimp     *gimp        = shell->gdisp->gimage->gimp;
  gboolean  set_display = FALSE;
  gboolean  popup_menu  = FALSE;

  switch (event->type)
    {
      GdkEventKey *kevent;

    case GDK_KEY_PRESS:
      if (! GTK_WIDGET_VISIBLE (GTK_ITEM_FACTORY (shell->menubar_factory)->widget))
        {
          gchar *accel = NULL;

          g_object_get (gtk_widget_get_settings (widget),
                        "gtk-menu-bar-accel", &accel,
                        NULL);

          if (accel)
            {
              guint           keyval = 0;
              GdkModifierType mods = 0;

              gtk_accelerator_parse (accel, &keyval, &mods);

              if (keyval == 0)
                g_warning ("Failed to parse menu bar accelerator '%s'\n", accel);

              kevent = (GdkEventKey *) event;

              /* FIXME this is wrong, needs to be in the global accel
               * resolution thing, to properly consider i18n etc., but
               * that probably requires AccelGroup changes etc.
               */
              if (kevent->keyval == keyval &&
                  ((kevent->state & gtk_accelerator_get_default_mod_mask ()) ==
                   (mods & gtk_accelerator_get_default_mod_mask ())))
                {
                  popup_menu = TRUE;
                }

              g_free (accel);
            }
        }
      /* fallthru */

    case GDK_KEY_RELEASE:
      kevent = (GdkEventKey *) event;

      if (gimp->busy)
        return TRUE;

      /*  do not process any key events while BUTTON1 is down. We do this
       *  so tools keep the modifier state they were in when BUTTON1 was
       *  pressed and to prevent accelerators from being invoked.
       */
      if (kevent->state & GDK_BUTTON1_MASK)
        {
          if (event->type == GDK_KEY_PRESS)
            {
              if (kevent->keyval == GDK_space && shell->space_release_pending)
                {
                  shell->space_pressed         = TRUE;
                  shell->space_release_pending = FALSE;
                }
            }
          else
            {
              if (kevent->keyval == GDK_space && shell->space_pressed)
                {
                  shell->space_pressed         = FALSE;
                  shell->space_release_pending = TRUE;
                }
            }

          return TRUE;
        }

      switch (kevent->keyval)
        {
        case GDK_Left: case GDK_Right:
        case GDK_Up: case GDK_Down:
        case GDK_space:
        case GDK_Tab:
        case GDK_Alt_L: case GDK_Alt_R:
        case GDK_Shift_L: case GDK_Shift_R:
        case GDK_Control_L: case GDK_Control_R:
          break;

        default:
          if (shell->space_pressed)
            return TRUE;
          break;
       }

      set_display = TRUE;
      break;

    case GDK_BUTTON_PRESS:
    case GDK_SCROLL:
      set_display = TRUE;
      break;

    default:
      break;
    }

  if (set_display)
    {
      /*  Setting the context's display automatically sets the image, too  */
      gimp_context_set_display (gimp_get_user_context (shell->gdisp->gimage->gimp),
				shell->gdisp);
    }

  if (popup_menu)
    {
      gimp_display_shell_origin_menu_popup (shell, 0, event->key.time);
      return TRUE;
    }

  return FALSE;
}

void
gimp_display_shell_canvas_realize (GtkWidget        *canvas,
                                   GimpDisplayShell *shell)
{
  GimpDisplayConfig *config;
  GimpDisplay       *gdisp;

  gdisp  = shell->gdisp;
  config = GIMP_DISPLAY_CONFIG (gdisp->gimage->gimp->config);

  gtk_widget_grab_focus (shell->canvas);

  shell->padding_gc = gdk_gc_new (canvas->window);

  gimp_display_shell_set_padding (shell,
                                  shell->padding_mode,
                                  &shell->padding_color);

  gdk_window_set_back_pixmap (shell->canvas->window, NULL, FALSE);

  gimp_statusbar_resize_cursor (GIMP_STATUSBAR (shell->statusbar));

  gimp_display_shell_update_title (shell);

  /*  create the selection object  */
  shell->select = gimp_display_shell_selection_create (canvas->window,
                                                       shell,
                                                       gdisp->gimage->height,
                                                       gdisp->gimage->width);

  shell->disp_width  = canvas->allocation.width;
  shell->disp_height = canvas->allocation.height;

  /*  create GC for rendering  */
  shell->render_gc = gdk_gc_new (shell->canvas->window);
  gdk_gc_set_exposures (shell->render_gc, TRUE);

  /*  set up the scrollbar observers  */
  g_signal_connect (shell->hsbdata, "value_changed",
                    G_CALLBACK (gimp_display_shell_hscrollbar_update),
                    shell);
  g_signal_connect (shell->vsbdata, "value_changed",
                    G_CALLBACK (gimp_display_shell_vscrollbar_update),
                    shell);

  /*  setup scale properly  */
  gimp_display_shell_scale_setup (shell);

  /*  set the initial cursor  */
  gimp_display_shell_set_cursor (shell,
                                 GDK_TOP_LEFT_ARROW,
                                 GIMP_TOOL_CURSOR_NONE,
                                 GIMP_CURSOR_MODIFIER_NONE);

  /*  allow shrinking  */
  gtk_widget_set_size_request (GTK_WIDGET (shell), 0, 0);
}

gboolean
gimp_display_shell_canvas_configure (GtkWidget         *widget,
                                     GdkEventConfigure *cevent,
                                     GimpDisplayShell  *shell)
{
  if ((shell->disp_width  != shell->canvas->allocation.width) ||
      (shell->disp_height != shell->canvas->allocation.height))
    {
      shell->disp_width  = shell->canvas->allocation.width;
      shell->disp_height = shell->canvas->allocation.height;

      gimp_display_shell_scale_resize (shell, FALSE, FALSE);
    }

  return TRUE;
}

gboolean
gimp_display_shell_canvas_expose (GtkWidget        *widget,
                                  GdkEventExpose   *eevent,
                                  GimpDisplayShell *shell)
{
  glong x1, y1, x2, y2;

  x1 = eevent->area.x;
  y1 = eevent->area.y;
  x2 = (eevent->area.x + eevent->area.width);
  y2 = (eevent->area.y + eevent->area.height);

  x1 = CLAMP (x1, 0, shell->disp_width);
  y1 = CLAMP (y1, 0, shell->disp_height);
  x2 = CLAMP (x2, 0, shell->disp_width);
  y2 = CLAMP (y2, 0, shell->disp_height);

  if ((x2 - x1) && (y2 - y1))
    {
      gimp_display_shell_add_expose_area (shell,
                                          x1, y1,
                                          (x2 - x1), (y2 - y1));
      gimp_display_shell_flush (shell);
    }

  return TRUE;
}

static void
gimp_display_shell_check_device_cursor (GimpDisplayShell *shell)
{
  GdkDevice *current_device;

  current_device = gimp_devices_get_current (shell->gdisp->gimage->gimp);

  shell->draw_cursor = ! current_device->has_cursor;
}

gboolean
gimp_display_shell_canvas_tool_events (GtkWidget        *canvas,
                                       GdkEvent         *event,
                                       GimpDisplayShell *shell)
{
  GimpDisplay     *gdisp;
  GimpImage       *gimage;
  Gimp            *gimp;
  GimpCoords       display_coords;
  GimpCoords       image_coords;
  GdkModifierType  state;
  guint32          time;
  gboolean         return_val    = FALSE;
  gboolean         update_cursor = FALSE;

  static GdkModifierType  button_press_state = 0;
  static GdkModifierType  space_press_state  = 0;

  static GimpToolInfo    *space_shaded_tool  = NULL;

  static gboolean         scrolling          = FALSE;
  static gint             scroll_start_x     = 0;
  static gint             scroll_start_y     = 0;

  static gboolean         button_press_before_focus = FALSE;

  if (! canvas->window)
    {
      g_warning ("%s: called unrealized", G_STRLOC);
      return FALSE;
    }

  gdisp  = shell->gdisp;
  gimage = gdisp->gimage;
  gimp   = gimage->gimp;

  /*  Find out what device the event occurred upon  */
  if (! gimp->busy && gimp_devices_check_change (gimp, event))
    {
      gimp_display_shell_check_device_cursor (shell);
    }

  gimp_display_shell_get_coords (shell, event,
                                 gimp_devices_get_current (gimp),
                                 &display_coords);
  gimp_display_shell_get_state (shell, event,
                                gimp_devices_get_current (gimp),
                                &state);
  time = gdk_event_get_time (event);

  /*  GimpCoords passed to tools are ALWAYS in image coordinates  */
  gimp_display_shell_untransform_coords (shell,
                                         &display_coords,
                                         &image_coords);

  switch (event->type)
    {
    case GDK_ENTER_NOTIFY:
      {
        GdkEventCrossing *cevent;

        cevent = (GdkEventCrossing *) event;

        if (cevent->mode != GDK_CROSSING_NORMAL)
          return TRUE;
      }
      break;

    case GDK_LEAVE_NOTIFY:
      {
        GdkEventCrossing *cevent;

        cevent = (GdkEventCrossing *) event;

        if (cevent->mode != GDK_CROSSING_NORMAL)
          return TRUE;

        gimp_display_shell_update_cursor (shell, -1, -1);

        shell->proximity = FALSE;
      }
      break;

    case GDK_PROXIMITY_IN:
      break;

    case GDK_PROXIMITY_OUT:
      shell->proximity = FALSE;
      break;

    case GDK_FOCUS_CHANGE:
      {
        GdkEventFocus *fevent;

        fevent = (GdkEventFocus *) event;

        if (fevent->in)
          {
            GTK_WIDGET_SET_FLAGS (canvas, GTK_HAS_FOCUS);

            /*  press modifier keys when the canvas gets the focus
             *
             *  in "click to focus" mode, we did this on BUTTON_PRESS, so
             *  do it here only if button_press_before_focus is FALSE
             */
            if (state && ! button_press_before_focus)
              {
                gimp_display_shell_update_tool_modifiers (shell, 0, state);

                tool_manager_oper_update_active (gimp,
                                                 &image_coords, state,
                                                 gdisp);
              }
          }
        else
          {
            GTK_WIDGET_UNSET_FLAGS (canvas, GTK_HAS_FOCUS);

            /*  reset it here to be prepared for the next
             *  FOCUS_IN / BUTTON_PRESS confusion
             */
            button_press_before_focus = FALSE;

            /*  release modifier keys when the canvas loses the focus  */
            if (state)
              {
                gimp_display_shell_update_tool_modifiers (shell, state, 0);

                tool_manager_oper_update_active (gimp,
                                                 &image_coords, 0,
                                                 gdisp);
              }
          }

        /*  stop the signal because otherwise gtk+ exposes the whole
         *  canvas to get the non-existant focus indicator drawn
         */
        return_val = TRUE;
      }
      break;

    case GDK_BUTTON_PRESS:
      {
        GdkEventButton *bevent;
        GimpTool       *active_tool;

        bevent = (GdkEventButton *) event;

        /*  in "click to focus" mode, the BUTTON_PRESS arrives before
         *  FOCUS_IN, so we have to update the tool's modifier state here
         */
        if (state && ! GTK_WIDGET_HAS_FOCUS (canvas))
          {
            gimp_display_shell_update_tool_modifiers (shell, 0, state);

            tool_manager_oper_update_active (gimp,
                                             &image_coords, state,
                                             gdisp);

            button_press_before_focus = TRUE;
          }

        /*  ignore new mouse events  */
        if (gimp->busy)
          return TRUE;

        active_tool = tool_manager_get_active (gimp);

        switch (bevent->button)
          {
          case 1:
            state |= GDK_BUTTON1_MASK;

            if (((gimp_tool_control_motion_mode (active_tool->control) ==
                  GIMP_MOTION_MODE_EXACT) &&
                 GIMP_DISPLAY_CONFIG (gimp->config)->perfect_mouse) ||

                /*  don't request motion hins for XInput devices because
                 *  the wacom driver is known to report crappy hints
                 *  (#6901) --mitch
                 */
                (gimp_devices_get_current (gimp) !=
                 gdk_device_get_core_pointer ()))
              {
                gdk_pointer_grab (canvas->window, FALSE,
                                  GDK_BUTTON1_MOTION_MASK |
                                  GDK_BUTTON_RELEASE_MASK,
                                  NULL, NULL, time);
              }
            else
              {
                gdk_pointer_grab (canvas->window, FALSE,
                                  GDK_POINTER_MOTION_HINT_MASK |
                                  GDK_BUTTON1_MOTION_MASK |
                                  GDK_BUTTON_RELEASE_MASK,
                                  NULL, NULL, time);
              }

            /*  save the current modifier state because tools don't get
             *  key events while BUTTON1 is down
             */
            button_press_state = state;

            if (active_tool &&
                (! gimp_image_is_empty (gimage) ||
                 gimp_tool_control_handles_empty_image (active_tool->control)))
              {
                if (gimp_tool_control_auto_snap_to (active_tool->control))
                  {
                    gimp_display_shell_snap_point (shell,
                                                   display_coords.x,
                                                   display_coords.y,
                                                   &display_coords.x,
                                                   &display_coords.y);

                    gimp_display_shell_untransform_coords (shell,
                                                           &display_coords,
                                                           &image_coords);

                    update_cursor = TRUE;
                  }

                /*  initialize the current tool if it has no drawable
                 */
                if (! active_tool->drawable)
                  {
                    tool_manager_initialize_active (gimp, gdisp);
                  }
                else if ((active_tool->drawable !=
                          gimp_image_active_drawable (gimage)) &&
                         ! gimp_tool_control_preserve (active_tool->control))
                  {
                    /*  create a new one, deleting the current
                     */
                    gimp_context_tool_changed (gimp_get_user_context (gimp));

                    tool_manager_initialize_active (gimp, gdisp);
                  }

                tool_manager_button_press_active (gimp,
                                                  &image_coords, time, state,
                                                  gdisp);
              }
            break;

          case 2:
            state |= GDK_BUTTON2_MASK;

            scrolling = TRUE;

            scroll_start_x = bevent->x + shell->offset_x;
            scroll_start_y = bevent->y + shell->offset_y;

            gtk_grab_add (canvas);

            gimp_display_shell_set_override_cursor (shell, GDK_FLEUR);
            break;

          case 3:
            state |= GDK_BUTTON3_MASK;
            gimp_item_factory_popup_with_data (shell->popup_factory,
                                               gimage,
                                               NULL);
            return_val = TRUE;
            break;

          default:
            break;
          }
      }
      break;

    case GDK_BUTTON_RELEASE:
      {
        GdkEventButton *bevent;
        GimpTool       *active_tool;

        bevent = (GdkEventButton *) event;

        active_tool = tool_manager_get_active (gimp);

        if (gimp->busy)
          return TRUE;

        switch (bevent->button)
          {
          case 1:
            state &= ~GDK_BUTTON1_MASK;

            if (active_tool &&
                (! gimp_image_is_empty (gimage) ||
                 gimp_tool_control_handles_empty_image (active_tool->control)))
              {
                if (gimp_tool_control_is_active (active_tool->control))
                  {
                    if (gimp_tool_control_auto_snap_to (active_tool->control))
                      {
                        gimp_display_shell_snap_point (shell,
                                                       display_coords.x,
                                                       display_coords.y,
                                                       &display_coords.x,
                                                       &display_coords.y);

                        gimp_display_shell_untransform_coords (shell,
                                                               &display_coords,
                                                               &image_coords);

                        update_cursor = TRUE;
                      }

                    tool_manager_button_release_active (gimp,
                                                        &image_coords, time, state,
                                                        gdisp);
                  }
              }

            gdk_pointer_ungrab (time);

            /*  restore the tool's modifier state because it didn't get
             *  key events while BUTTON1 was down
             */
            gimp_display_shell_update_tool_modifiers (shell,
                                                      button_press_state,
                                                      state);

            tool_manager_oper_update_active (gimp,
                                             &image_coords, state,
                                             gdisp);

            if (shell->space_release_pending)
              {
                gdk_keyboard_ungrab (time);

                gimp_display_shell_update_tool_modifiers (shell, state, 0);

                gimp_context_set_tool (gimp_get_user_context (gimp),
                                       space_shaded_tool);
                space_shaded_tool = NULL;

                gimp_display_shell_update_tool_modifiers (shell,
                                                          space_press_state,
                                                          state);

                tool_manager_oper_update_active (gimp,
                                                 &image_coords, state,
                                                 gdisp);

                shell->space_release_pending = FALSE;
              }
            break;

          case 2:
            state &= ~GDK_BUTTON2_MASK;

            scrolling = FALSE;

            scroll_start_x = 0;
            scroll_start_y = 0;

            gtk_grab_remove (canvas);

            gimp_display_shell_unset_override_cursor (shell);
            break;

          case 3:
            state &= ~GDK_BUTTON3_MASK;
            break;

          default:
            break;
          }
      }
      break;

    case GDK_SCROLL:
      {
        GdkEventScroll *sevent;

        sevent = (GdkEventScroll *) event;

        if (state & GDK_SHIFT_MASK)
          {
            if (sevent->direction == GDK_SCROLL_UP)
              gimp_display_shell_scale (shell, GIMP_ZOOM_IN);
            else
              gimp_display_shell_scale (shell, GIMP_ZOOM_OUT);
          }
        else
          {
            GtkAdjustment *adj;
            gdouble        value;

            if (state & GDK_CONTROL_MASK)
              adj = shell->hsbdata;
            else
              adj = shell->vsbdata;

            value = adj->value + ((sevent->direction == GDK_SCROLL_UP) ?
                                  -adj->page_increment / 2 :
                                  adj->page_increment / 2);
            value = CLAMP (value, adj->lower, adj->upper - adj->page_size);

            gtk_adjustment_set_value (adj, value);
          }

        gimp_display_shell_untransform_coords (shell,
                                               &display_coords,
                                               &image_coords);

        tool_manager_oper_update_active (gimp,
                                         &image_coords, state,
                                         gdisp);

        return_val = TRUE;
      }
      break;

    case GDK_MOTION_NOTIFY:
      {
        GdkEventMotion *mevent;
        GdkEvent       *compressed_motion = NULL;
        GimpTool       *active_tool;

        mevent = (GdkEventMotion *) event;

        if (gimp->busy)
          return TRUE;

        active_tool = tool_manager_get_active (gimp);

	switch (gimp_tool_control_motion_mode (active_tool->control))
          {
          case GIMP_MOTION_MODE_EXACT:
          case GIMP_MOTION_MODE_HINT:
            break;

          case GIMP_MOTION_MODE_COMPRESS:
            compressed_motion = gimp_display_shell_compress_motion (shell);
            break;
          }

        if (compressed_motion)
          {
            g_print ("gimp_display_shell_compress_motion() returned an event\n");

            gimp_display_shell_get_coords (shell, compressed_motion,
                                           gimp_devices_get_current (gimp),
                                           &display_coords);
            gimp_display_shell_get_state (shell, compressed_motion,
                                          gimp_devices_get_current (gimp),
                                          &state);
            time = gdk_event_get_time (event);

            /*  GimpCoords passed to tools are ALWAYS in image coordinates  */
            gimp_display_shell_untransform_coords (shell,
                                                   &display_coords,
                                                   &image_coords);
          }

        /* Ask for the pointer position, but ignore it except for cursor
         * handling, so motion events sync with the button press/release events 
         */
        if (mevent->is_hint)
          {
            gimp_display_shell_get_device_coords (shell,
                                                  mevent->device,
                                                  &display_coords);
          }

        update_cursor = TRUE;

        if (! shell->proximity)
          {
            shell->proximity = TRUE;
            gimp_display_shell_check_device_cursor (shell);
          }

        if (state & GDK_BUTTON1_MASK)
          {
            if (active_tool                                        &&
                gimp_tool_control_is_active (active_tool->control) &&
                (! gimp_image_is_empty (gimage) ||
                 gimp_tool_control_handles_empty_image (active_tool->control)))
              {
                /*  if the first mouse button is down, check for automatic
                 *  scrolling...
                 */
                if ((mevent->x < 0                 ||
                     mevent->y < 0                 ||
                     mevent->x > shell->disp_width ||
                     mevent->y > shell->disp_height) &&
                    ! gimp_tool_control_scroll_lock (active_tool->control))
                  {
                    gint off_x, off_y;

                    off_x = off_y = 0;

                    /*  The cases for scrolling  */
                    if (mevent->x < 0)
                      off_x = mevent->x;
                    else if (mevent->x > shell->disp_width)
                      off_x = mevent->x - shell->disp_width;

                    if (mevent->y < 0)
                      off_y = mevent->y;
                    else if (mevent->y > shell->disp_height)
                      off_y = mevent->y - shell->disp_height;

                    if (gimp_display_shell_scroll (shell, off_x, off_y))
                      {
                        GimpCoords device_coords;

                        gimp_display_shell_get_device_coords (shell,
                                                              mevent->device,
                                                              &device_coords);

                        if (device_coords.x == mevent->x &&
                            device_coords.y == mevent->y)
                          {
                            /*  Put this event back on the queue
                             *  so it keeps scrolling
                             */
                            gdk_event_put ((GdkEvent *) mevent);
                          }
                      }
                  }

                if (gimp_tool_control_auto_snap_to (active_tool->control))
                  {
                    gimp_display_shell_snap_point (shell,
                                                   display_coords.x,
                                                   display_coords.y,
                                                   &display_coords.x,
                                                   &display_coords.y);

                    gimp_display_shell_untransform_coords (shell,
                                                           &display_coords,
                                                           &image_coords);

                    update_cursor = TRUE;
                  }

                tool_manager_motion_active (gimp,
                                            &image_coords, time, state,
                                            gdisp);
              }
          }
        else if (state & GDK_BUTTON2_MASK)
          {
            if (scrolling)
              {
                gimp_display_shell_scroll (shell,
                                           (scroll_start_x - mevent->x -
                                            shell->offset_x),
                                           (scroll_start_y - mevent->y -
                                            shell->offset_y));
              }
          }

        if (! (state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
          {
            tool_manager_oper_update_active (gimp,
                                             &image_coords, state,
                                             gdisp);
          }
      }
      break;

    case GDK_KEY_PRESS:
      {
        GdkEventKey *kevent;

        kevent = (GdkEventKey *) event;

        switch (kevent->keyval)
          {
          case GDK_Left: case GDK_Right:
          case GDK_Up: case GDK_Down:
            if (! gimp_image_is_empty (gimage))
              {
                tool_manager_arrow_key_active (gimp,
                                               kevent,
                                               gdisp);
              }

            return_val = TRUE;
            break;

          case GDK_space:
            {
              GimpTool *active_tool;

              active_tool = tool_manager_get_active (gimp);

              if (! shell->space_pressed && ! GIMP_IS_MOVE_TOOL (active_tool))
                {
                  GimpToolInfo *move_tool_info;

                  space_shaded_tool = active_tool->tool_info;
                  space_press_state = state;

                  g_print ("%s: pushing move tool\n", G_GNUC_FUNCTION);

                  gdk_keyboard_grab (canvas->window, FALSE, time);

                  move_tool_info =
                    tool_manager_get_info_by_type (gimp,
                                                   GIMP_TYPE_MOVE_TOOL);

                  gimp_context_set_tool (gimp_get_user_context (gimp),
                                         move_tool_info);

                  gimp_display_shell_update_tool_modifiers (shell, 0, state);

                  shell->space_pressed = TRUE;
                }
            }

            return_val = TRUE;
            break;

          case GDK_Tab:
            if (! gimp_image_is_empty (gimage))
              {
                if (state & GDK_MOD1_MASK)
                  {
                    gimp_display_shell_layer_select_init (gdisp->gimage,
                                                          1, kevent->time);
                  }
                else if (state & GDK_CONTROL_MASK)
                  {
                    gimp_display_shell_layer_select_init (gdisp->gimage,
                                                          -1, kevent->time);
                  }
              }
            else if (! state)
              {
                /* Hide or show all dialogs */

                gimp_dialog_factories_toggle (global_toolbox_factory);
              }

            return_val = TRUE;
            break;

            /*  Update the state based on modifiers being pressed  */
          case GDK_Alt_L: case GDK_Alt_R:
          case GDK_Shift_L: case GDK_Shift_R:
          case GDK_Control_L: case GDK_Control_R:
            {
              GdkModifierType key;

              key = gimp_display_shell_key_to_state (kevent->keyval);
              state |= key;

              /*  For all modifier keys: call the tools modifier_key *and*
               *  oper_update method so tools can choose if they are interested
               *  in the release itself or only in the resulting state
               */
              if (! gimp_image_is_empty (gimage))
                {
                  tool_manager_modifier_key_active (gimp,
                                                    key, TRUE, state,
                                                    gdisp);
                }
            }

            break;
          }

        tool_manager_oper_update_active (gimp,
                                         &image_coords, state,
                                         gdisp);
      }
      break;

    case GDK_KEY_RELEASE:
      {
        GdkEventKey *kevent;

        kevent = (GdkEventKey *) event;

        switch (kevent->keyval)
          {
          case GDK_space:
            if (shell->space_pressed)
              {
                g_print ("%s: popping move tool\n", G_GNUC_FUNCTION);

                gdk_keyboard_ungrab (time);

                gimp_display_shell_update_tool_modifiers (shell, state, 0);

                gimp_context_set_tool (gimp_get_user_context (gimp),
                                       space_shaded_tool);
                space_shaded_tool = NULL;

                gimp_display_shell_update_tool_modifiers (shell,
                                                          space_press_state,
                                                          state);

                shell->space_pressed = FALSE;
              }

            return_val = TRUE;
            break;

            /*  Update the state based on modifiers being pressed  */
          case GDK_Alt_L: case GDK_Alt_R:
          case GDK_Shift_L: case GDK_Shift_R:
          case GDK_Control_L: case GDK_Control_R:
            {
              GdkModifierType key;

              key = gimp_display_shell_key_to_state (kevent->keyval);
              state &= ~key;

              /*  For all modifier keys: call the tools modifier_key *and*
               *  oper_update method so tools can choose if they are interested
               *  in the press itself or only in the resulting state
               */
              if (! gimp_image_is_empty (gimage))
                {
                  tool_manager_modifier_key_active (gimp,
                                                    key, FALSE, state,
                                                    gdisp);
                }
            }

            break;
          }

        tool_manager_oper_update_active (gimp,
                                         &image_coords, state,
                                         gdisp);
      }
      break;

    default:
      break;
    }

  /*  if we reached this point in gimp_busy mode, return now  */
  if (gimp->busy)
    return TRUE;

  /*  cursor update support  */

  if (GIMP_DISPLAY_CONFIG (gimp->config)->cursor_updating)
    {
      GimpTool *active_tool;

      active_tool = tool_manager_get_active (gimp);

      if (active_tool)
        {
          if ((! gimp_image_is_empty (gimage) ||
               gimp_tool_control_handles_empty_image (active_tool->control)) &&
              ! (state & (GDK_BUTTON1_MASK |
                          GDK_BUTTON2_MASK |
                          GDK_BUTTON3_MASK)))
            {
              tool_manager_cursor_update_active (gimp,
                                                 &image_coords, state,
                                                 gdisp);
            }
          else if (gimp_image_is_empty (gimage))
            {
              gimp_display_shell_set_cursor (shell,
                                             GIMP_BAD_CURSOR,
                                             gimp_tool_control_get_tool_cursor (active_tool->control),
                                             GIMP_CURSOR_MODIFIER_NONE);
            }
        }
      else
        {
          gimp_display_shell_set_cursor (shell,
                                         GIMP_BAD_CURSOR,
                                         GIMP_TOOL_CURSOR_NONE,
                                         GIMP_CURSOR_MODIFIER_NONE);
        }
    }

  if (update_cursor)
    {
      gimp_display_shell_update_cursor (shell,
                                        display_coords.x,
                                        display_coords.y);
    }

  return return_val;
}

gboolean
gimp_display_shell_hruler_button_press (GtkWidget        *widget,
                                        GdkEventButton   *event,
                                        GimpDisplayShell *shell)
{
  GimpDisplay *gdisp;

  gdisp = shell->gdisp;

  if (gdisp->gimage->gimp->busy)
    return TRUE;

  if (event->button == 1)
    {
      GimpToolInfo *tool_info;
      GimpTool     *active_tool;

      tool_info = (GimpToolInfo *)
        gimp_container_get_child_by_name (gdisp->gimage->gimp->tool_info_list,
                                          "gimp-move-tool");

      if (tool_info)
	{
	  gimp_context_set_tool (gimp_get_user_context (gdisp->gimage->gimp),
				 tool_info);

	  active_tool = tool_manager_get_active (gdisp->gimage->gimp);

	  if (active_tool)
	    {
              gdk_pointer_grab (shell->canvas->window, FALSE,
                                GDK_POINTER_MOTION_HINT_MASK |
                                GDK_BUTTON1_MOTION_MASK |
                                GDK_BUTTON_RELEASE_MASK,
                                NULL, NULL, event->time);

	      gimp_move_tool_start_hguide (active_tool, gdisp);
	    }
	}
    }

  return FALSE;
}

gboolean
gimp_display_shell_vruler_button_press (GtkWidget        *widget,
                                        GdkEventButton   *event,
                                        GimpDisplayShell *shell)
{
  GimpDisplay *gdisp;

  gdisp = shell->gdisp;

  if (gdisp->gimage->gimp->busy)
    return TRUE;

  if (event->button == 1)
    {
      GimpToolInfo *tool_info;
      GimpTool     *active_tool;

      tool_info = (GimpToolInfo *)
        gimp_container_get_child_by_name (gdisp->gimage->gimp->tool_info_list,
                                          "gimp-move-tool");

      if (tool_info)
	{
	  gimp_context_set_tool (gimp_get_user_context (gdisp->gimage->gimp),
				 tool_info);

	  active_tool = tool_manager_get_active (gdisp->gimage->gimp);

	  if (active_tool)
	    {
              gdk_pointer_grab (shell->canvas->window, FALSE,
                                GDK_POINTER_MOTION_HINT_MASK |
                                GDK_BUTTON1_MOTION_MASK |
                                GDK_BUTTON_RELEASE_MASK,
                                NULL, NULL, event->time);

	      gimp_move_tool_start_vguide (active_tool, gdisp);
	    }
	}
    }

  return FALSE;
}

gboolean
gimp_display_shell_origin_button_press (GtkWidget        *widget,
                                        GdkEventButton   *event,
                                        GimpDisplayShell *shell)
{
  if (! shell->gdisp->gimage->gimp->busy && event->button == 1)
    {
      gimp_display_shell_origin_menu_popup (shell,
                                            event->button,
                                            event->time);
    }

  /* Return TRUE to stop signal emission so the button doesn't grab the
   * pointer away from us.
   */
  return TRUE;
}

gboolean
gimp_display_shell_color_button_press (GtkWidget        *widget,
                                       GdkEventButton   *bevent,
                                       GimpDisplayShell *shell)
{
  if (bevent->button == 3)
    {
      GimpColorButton *color_button;
      guchar           r, g, b;
      GimpRGB          color;

      color_button = GIMP_COLOR_BUTTON (widget);

      r = shell->canvas->style->bg[GTK_STATE_NORMAL].red   >> 8;
      g = shell->canvas->style->bg[GTK_STATE_NORMAL].green >> 8;
      b = shell->canvas->style->bg[GTK_STATE_NORMAL].blue  >> 8;

      gimp_rgba_set_uchar (&color, r, g, b, 255);
      gimp_item_factory_set_color (color_button->item_factory,
                                   "/From Theme", &color, FALSE);

      gimp_rgba_set_uchar (&color,
                           render_blend_light_check[0],
                           render_blend_light_check[1],
                           render_blend_light_check[2],
                           255);
      gimp_item_factory_set_color (color_button->item_factory,
                                   "/Light Check Color", &color, FALSE);

      gimp_rgba_set_uchar (&color,
                           render_blend_dark_check[0],
                           render_blend_dark_check[1],
                           render_blend_dark_check[2],
                           255);
      gimp_item_factory_set_color (color_button->item_factory,
                                   "/Dark Check Color", &color, FALSE);
    }

  return FALSE;
}

void
gimp_display_shell_color_button_changed (GtkWidget        *widget,
                                         GimpDisplayShell *shell)
{
  GimpRGB color;

  shell->padding_mode_set = TRUE;

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (widget), &color);

  gimp_display_shell_set_padding (shell,
                                  GIMP_DISPLAY_PADDING_MODE_CUSTOM,
                                  &color);

  gimp_display_shell_expose_full (shell);
  gimp_display_shell_flush (shell);
}

void  
gimp_display_shell_color_button_menu_callback (gpointer   callback_data, 
                                               guint      callback_action, 
                                               GtkWidget *widget)
{
  GimpDisplayShell *shell;

  shell = GIMP_DISPLAY_SHELL (callback_data);

  if (callback_action == GIMP_DISPLAY_PADDING_MODE_CUSTOM)
    {
      gtk_button_clicked (GTK_BUTTON (shell->padding_button));
    }
  else
    {
      if (callback_action == 0xffff)
        {
          GimpDisplayConfig *config;

          config = GIMP_DISPLAY_CONFIG (shell->gdisp->gimage->gimp->config);

          shell->padding_mode_set = FALSE;

          gimp_display_shell_set_padding (shell,
                                          config->canvas_padding_mode,
                                          &config->canvas_padding_color);
        }
      else
        {
          shell->padding_mode_set = TRUE;

          gimp_display_shell_set_padding (shell, callback_action,
                                          &shell->padding_color);
        }

      gimp_display_shell_expose_full (shell);
      gimp_display_shell_flush (shell);
    }
}

gboolean
gimp_display_shell_qmask_button_press (GtkWidget        *widget,
                                       GdkEventButton   *bevent,
                                       GimpDisplayShell *shell)
{
  if ((bevent->type == GDK_BUTTON_PRESS) && (bevent->button == 3))
    {
      GimpItemFactory *factory;

      factory = gimp_item_factory_from_path ("<QMask>");

      gimp_item_factory_popup_with_data (factory, shell, NULL);

      return TRUE;
    }

  return FALSE;
}

void
gimp_display_shell_qmask_toggled (GtkWidget        *widget,
                                  GimpDisplayShell *shell)
{
  gimp_image_set_qmask_state (shell->gdisp->gimage,
                              GTK_TOGGLE_BUTTON (widget)->active);

  gimp_image_flush (shell->gdisp->gimage);
}

gboolean
gimp_display_shell_nav_button_press (GtkWidget        *widget,
                                     GdkEventButton   *bevent,
                                     GimpDisplayShell *shell)
{
  if ((bevent->type == GDK_BUTTON_PRESS) && (bevent->button == 1))
    {
      gimp_navigation_view_popup (shell, widget, bevent->x, bevent->y);
    }

  return TRUE;
}


/*  private functions  */

static void
gimp_display_shell_vscrollbar_update (GtkAdjustment    *adjustment,
                                      GimpDisplayShell *shell)
{
  gimp_display_shell_scroll (shell, 0, (adjustment->value - shell->offset_y));
}

static void
gimp_display_shell_hscrollbar_update (GtkAdjustment    *adjustment,
                                      GimpDisplayShell *shell)
{
  gimp_display_shell_scroll (shell, (adjustment->value - shell->offset_x), 0);
}

static gboolean
gimp_display_shell_get_coords (GimpDisplayShell *shell,
                               GdkEvent         *event,
                               GdkDevice        *device,
                               GimpCoords       *coords)
{
  /*  initialize extended axes to something meaningful because each of
   *  the following *_get_axis() calls may return FALSE and leave the
   *  passed gdouble location untouched
   */
  coords->pressure = 1.0;
  coords->xtilt    = 0.5;
  coords->ytilt    = 0.5;
  coords->wheel    = 0.5;

  if (gdk_event_get_axis (event, GDK_AXIS_X, &coords->x))
    {
      gdk_event_get_axis (event, GDK_AXIS_Y,        &coords->y);
      gdk_event_get_axis (event, GDK_AXIS_PRESSURE, &coords->pressure);
      gdk_event_get_axis (event, GDK_AXIS_XTILT,    &coords->xtilt);
      gdk_event_get_axis (event, GDK_AXIS_YTILT,    &coords->ytilt);
      gdk_event_get_axis (event, GDK_AXIS_WHEEL,    &coords->wheel);

      return TRUE;
    }

  gimp_display_shell_get_device_coords (shell, device, coords);

  return FALSE;
}

static void
gimp_display_shell_get_device_coords (GimpDisplayShell *shell,
                                      GdkDevice        *device,
                                      GimpCoords       *coords)
{
  gdouble axes[GDK_AXIS_LAST];

  gdk_device_get_state (device, shell->canvas->window, axes, NULL);

  gdk_device_get_axis (device, axes, GDK_AXIS_X,        &coords->x);
  gdk_device_get_axis (device, axes, GDK_AXIS_Y,        &coords->y);
  gdk_device_get_axis (device, axes, GDK_AXIS_PRESSURE, &coords->pressure);
  gdk_device_get_axis (device, axes, GDK_AXIS_XTILT,    &coords->xtilt);
  gdk_device_get_axis (device, axes, GDK_AXIS_YTILT,    &coords->ytilt);
  gdk_device_get_axis (device, axes, GDK_AXIS_WHEEL,    &coords->wheel);
}

static gboolean
gimp_display_shell_get_state (GimpDisplayShell *shell,
                              GdkEvent         *event,
                              GdkDevice        *device,
                              GdkModifierType  *state)
{
  if (gdk_event_get_state (event, state))
    {
      return TRUE;
    }

  gimp_display_shell_get_device_state (shell, device, state);

  return FALSE;
}

static void
gimp_display_shell_get_device_state (GimpDisplayShell *shell,
                                     GdkDevice        *device,
                                     GdkModifierType  *state)
{
  gdk_device_get_state (device, shell->canvas->window, NULL, state);
}

static GdkModifierType
gimp_display_shell_key_to_state (gint key)
{
  switch (key)
    {
    case GDK_Alt_L:
    case GDK_Alt_R:
      return GDK_MOD1_MASK;
    case GDK_Shift_L:
    case GDK_Shift_R:
      return GDK_SHIFT_MASK;
    case GDK_Control_L:
    case GDK_Control_R:
      return GDK_CONTROL_MASK;
    default:
      return 0;
    }
}

static void
gimp_display_shell_update_tool_modifiers (GimpDisplayShell *shell,
                                          GdkModifierType   old_state,
                                          GdkModifierType   current_state)
{
  Gimp *gimp;

  gimp = shell->gdisp->gimage->gimp;

  if ((old_state & GDK_SHIFT_MASK) != (current_state & GDK_SHIFT_MASK))
    {
      tool_manager_modifier_key_active (gimp,
                                        GDK_SHIFT_MASK,
                                        (current_state & GDK_SHIFT_MASK) ?
                                        TRUE : FALSE,
                                        current_state,
                                        shell->gdisp);
    }

  if ((old_state & GDK_CONTROL_MASK) != (current_state & GDK_CONTROL_MASK))
    {
      tool_manager_modifier_key_active (gimp,
                                        GDK_CONTROL_MASK,
                                        (current_state & GDK_CONTROL_MASK) ?
                                        TRUE : FALSE,
                                        current_state,
                                        shell->gdisp);
    }

  if ((old_state & GDK_MOD1_MASK) != (current_state & GDK_MOD1_MASK))
    {
      tool_manager_modifier_key_active (gimp,
                                        GDK_MOD1_MASK,
                                        (current_state & GDK_MOD1_MASK) ?
                                        TRUE : FALSE,
                                        current_state,
                                        shell->gdisp);
    }
}

static void
gimp_display_shell_origin_menu_position (GtkMenu  *menu,
                                         gint     *x,
                                         gint     *y,
                                         gpointer  data)
{
  GtkWidget *origin;
  gint       origin_x;
  gint       origin_y;

  origin = (GtkWidget *) data;

  gdk_window_get_origin (origin->window, &origin_x, &origin_y);

  *x = origin_x + origin->allocation.x + origin->allocation.width - 1;
  *y = origin_y + origin->allocation.y + (origin->allocation.height - 1) / 2;

  if (*x + GTK_WIDGET (menu)->allocation.width > gdk_screen_width ())
    *x -= (GTK_WIDGET (menu)->allocation.width + origin->allocation.width);

  if (*y + GTK_WIDGET (menu)->allocation.height > gdk_screen_height ())
    *y -= (GTK_WIDGET (menu)->allocation.height);
}

static void
gimp_display_shell_origin_menu_popup (GimpDisplayShell *shell,
                                      guint             button,
                                      guint32           time)
{
  GtkItemFactory *factory;
  gint            x, y;

  factory = GTK_ITEM_FACTORY (shell->popup_factory);

  gimp_display_shell_origin_menu_position (GTK_MENU (factory->widget),
                                           &x, &y,
                                           shell->origin);

  gtk_item_factory_popup_with_data (factory,
                                    shell->gdisp->gimage,
                                    NULL,
                                    x, y,
                                    button, time);
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
GdkEvent *
gimp_display_shell_compress_motion (GimpDisplayShell *shell)
{
  GdkEvent *event;
  GList    *requeued_events = NULL;
  GList    *list;
  GdkEvent *last_motion = NULL;

  /*  Move the entire GDK event queue to a private list, filtering
   *  out any motion events for the desired widget.
   */
  while (gdk_events_pending ())
    {
      event = gdk_event_get ();

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
      gdk_event_put ((GdkEvent*) list->data);
      gdk_event_free ((GdkEvent*) list->data);
    }

  g_list_free (requeued_events);

  return last_motion;
}
