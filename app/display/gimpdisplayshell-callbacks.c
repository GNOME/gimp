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

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-qmask.h"
#include "core/gimplayer.h"
#include "core/gimptoolinfo.h"

#include "libgimptool/gimptooltypes.h"
#include "libgimptool/gimptoolcontrol.h"

#include "tools/gimpmovetool.h"
#include "tools/tool_manager.h"

#include "widgets/gimpcursor.h"
#include "widgets/gimpdevices.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpitemfactory.h"

#include "gui/dialogs.h"

#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-layer-select.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-selection.h"
#include "gimpstatusbar.h"

#include "gimprc.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void     gimp_display_shell_vscrollbar_update (GtkAdjustment    *adjustment,
                                                      GimpDisplayShell *shell);
static void     gimp_display_shell_hscrollbar_update (GtkAdjustment    *adjustment,
                                                      GimpDisplayShell *shell);

static gboolean gimp_display_shell_get_state         (GimpDisplayShell *shell,
                                                      GdkEvent         *event,
                                                      GdkDevice        *device,
                                                      GdkModifierType  *state);
static void     gimp_display_shell_get_device_state  (GimpDisplayShell *shell,
                                                      GdkDevice        *device,
                                                      GdkModifierType  *state);
static gboolean gimp_display_shell_get_coords        (GimpDisplayShell *shell,
                                                      GdkEvent         *event,
                                                      GdkDevice        *device,
                                                      GimpCoords       *coords);
static void     gimp_display_shell_get_device_coords (GimpDisplayShell *shell,
                                                      GdkDevice        *device,
                                                      GimpCoords       *coords);

static GdkModifierType
                gimp_display_shell_key_to_state      (gint              key);

static void  gimp_display_shell_origin_menu_position (GtkMenu          *menu,
                                                      gint             *x,
                                                      gint             *y,
                                                      gpointer          data);

GdkEvent *      gimp_display_shell_compress_motion   (GimpDisplayShell *shell);


/*  public functions  */

gboolean
gimp_display_shell_events (GtkWidget        *widget,
                           GdkEvent         *event,
                           GimpDisplayShell *shell)
{
  switch (event->type)
    {
    case GDK_KEY_PRESS:
    case GDK_BUTTON_PRESS:
    case GDK_SCROLL:
      /*  Setting the context's display automatically sets the image, too  */
      gimp_context_set_display (gimp_get_user_context (shell->gdisp->gimage->gimp),
				shell->gdisp);

      break;
    default:
      break;
    }

  return FALSE;
}

void
gimp_display_shell_canvas_realize (GtkWidget        *canvas,
                                   GimpDisplayShell *shell)
{
  GimpDisplay *gdisp;
  GdkColor     color;
  guchar       r, g, b;

  gdisp = shell->gdisp;

  gtk_widget_grab_focus (shell->canvas);

  if (TRUE /* gimprc.use_style_padding_color */)
    {
      r = shell->canvas->style->bg[GTK_STATE_NORMAL].red   >> 8;
      g = shell->canvas->style->bg[GTK_STATE_NORMAL].green >> 8;
      b = shell->canvas->style->bg[GTK_STATE_NORMAL].blue  >> 8;

      gimp_rgb_set_uchar (&shell->padding_color, r, g, b);

      g_signal_handlers_block_by_func (G_OBJECT (shell->padding_button),
                                       gimp_display_shell_color_button_changed,
                                       shell);

      gimp_color_button_set_color (GIMP_COLOR_BUTTON (shell->padding_button),
                                   &shell->padding_color);

      g_signal_handlers_unblock_by_func (G_OBJECT (shell->padding_button),
                                         gimp_display_shell_color_button_changed,
                                         shell);
    }

  gimp_rgb_get_uchar (&shell->padding_color, &r, &g, &b);

  shell->padding_gc = gdk_gc_new (canvas->window);

  color.red   = (r << 8) | r;
  color.green = (g << 8) | g;
  color.blue  = (b << 8) | b;

  gdk_gc_set_rgb_fg_color (shell->padding_gc, &color);

  gdk_window_set_back_pixmap (shell->canvas->window, NULL, FALSE);

  gimp_statusbar_resize_cursor (GIMP_STATUSBAR (shell->statusbar));

  gimp_display_shell_update_title (shell);

  /*  create the selection object  */
  shell->select = gimp_display_shell_selection_create (canvas->window,
                                                       shell,
                                                       gdisp->gimage->height,
                                                       gdisp->gimage->width, 
                                                       gimprc.marching_speed);

  shell->disp_width  = canvas->allocation.width;
  shell->disp_height = canvas->allocation.height;

  /*  create GC for rendering  */
  shell->render_gc = gdk_gc_new (shell->canvas->window);
  gdk_gc_set_exposures (shell->render_gc, TRUE);

  /*  set up the scrollbar observers  */
  g_signal_connect (G_OBJECT (shell->hsbdata), "value_changed",
                    G_CALLBACK (gimp_display_shell_hscrollbar_update),
                    shell);
  g_signal_connect (G_OBJECT (shell->vsbdata), "value_changed",
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

gboolean
gimp_display_shell_canvas_focus_in (GtkWidget        *widget,
                                    GdkEventFocus    *fevent,
                                    GimpDisplayShell *shell)
{
  GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);

  /*  stop the signal because otherwise gtk+ exposes the whole
   *  canvas to get the non-existant focus indicator drawn
   */
  return TRUE;
}

gboolean
gimp_display_shell_canvas_focus_out (GtkWidget        *widget,
                                     GdkEventFocus    *fevent,
                                     GimpDisplayShell *shell)
{
  GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

  /*  stop the signal because otherwise gtk+ exposes the whole
   *  canvas to get the non-existant focus indicator drawn
   */
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
  GimpCoords       display_coords;
  GimpCoords       image_coords;
  GdkModifierType  state;
  guint32          time;
  gboolean         return_val    = FALSE;
  gboolean         update_cursor = FALSE;

  static GdkModifierType  press_state;

  static gboolean  scrolling      = FALSE;
  static gint      scroll_start_x = 0;
  static gint      scroll_start_y = 0;

  if (! canvas->window)
    {
      g_warning ("%s: called unrealized", G_STRLOC);
      return FALSE;
    }

  gdisp = shell->gdisp;

  gimage = gdisp->gimage;

  /*  Find out what device the event occurred upon  */
  if (! gimage->gimp->busy && gimp_devices_check_change (gimage->gimp, event))
    {
      gimp_display_shell_check_device_cursor (shell);
    }

  gimp_display_shell_get_coords (shell, event,
                                 gimp_devices_get_current (gimage->gimp),
                                 &display_coords);
  gimp_display_shell_get_state (shell, event,
                                gimp_devices_get_current (gimage->gimp),
                                &state);
  time = gdk_event_get_time (event);

  image_coords = display_coords;

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

        /*  press modifier keys when entering the canvas  */
        if (state)
          {
            if (state & GDK_SHIFT_MASK)
              tool_manager_modifier_key_active (gimage->gimp,
                                                GDK_SHIFT_MASK, TRUE, state,
                                                gdisp);
            if (state & GDK_CONTROL_MASK)
              tool_manager_modifier_key_active (gimage->gimp,
                                                GDK_CONTROL_MASK, TRUE, state,
                                                gdisp);
            if (state & GDK_MOD1_MASK)
              tool_manager_modifier_key_active (gimage->gimp,
                                                GDK_MOD1_MASK, TRUE, state,
                                                gdisp);

            tool_manager_oper_update_active (gimage->gimp,
                                             &image_coords, state,
                                             gdisp);
          }
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

        /*  release modifier keys when leaving the canvas  */
        if (state)
          {
            if (state & GDK_MOD1_MASK)
              tool_manager_modifier_key_active (gimage->gimp,
                                                GDK_MOD1_MASK, FALSE, 0,
                                                gdisp);
            if (state & GDK_CONTROL_MASK)
              tool_manager_modifier_key_active (gimage->gimp,
                                                GDK_CONTROL_MASK, FALSE, 0,
                                                gdisp);
            if (state & GDK_SHIFT_MASK)
              tool_manager_modifier_key_active (gimage->gimp,
                                                GDK_SHIFT_MASK, FALSE, 0,
                                                gdisp);

            tool_manager_oper_update_active (gimage->gimp,
                                             &image_coords, 0,
                                             gdisp);
          }
      }
      break;

    case GDK_PROXIMITY_IN:
      break;

    case GDK_PROXIMITY_OUT:
      shell->proximity = FALSE;
      break;

    case GDK_BUTTON_PRESS:
      {
        GdkEventButton *bevent;
        GimpTool       *active_tool;

        bevent = (GdkEventButton *) event;

        /*  ignore new mouse events  */
        if (gimage->gimp->busy)
          return TRUE;

        active_tool = tool_manager_get_active (gimage->gimp);

        switch (bevent->button)
          {
          case 1:
            state |= GDK_BUTTON1_MASK;

            if (((gimp_tool_control_motion_mode(active_tool->control) == GIMP_MOTION_MODE_EXACT) &&
                 gimprc.perfectmouse) ||

                /*  don't request motion hins for XInput devices because
                 *  the wacom driver is known to report crappy hints
                 *  (#6901) --mitch
                 */
                (gimp_devices_get_current (gimage->gimp) !=
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
            press_state = state;

            if (active_tool && (! gimp_image_is_empty (gimage) ||
                                gimp_tool_control_handles_empty_image(active_tool->control)))
              {
                if (gimp_tool_control_auto_snap_to(active_tool->control))
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
                    tool_manager_initialize_active (gimage->gimp, gdisp);
                  }
                else if ((active_tool->drawable !=
                          gimp_image_active_drawable (gimage)) &&
                         ! gimp_tool_control_preserve(active_tool->control))
                  {
                    /*  create a new one, deleting the current
                     */
                    gimp_context_tool_changed (gimp_get_user_context (gimage->gimp));

                    tool_manager_initialize_active (gimage->gimp, gdisp);
                  }

                tool_manager_button_press_active (gimage->gimp,
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
            gimp_item_factory_popup_with_data (shell->item_factory,
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

        active_tool = tool_manager_get_active (gimage->gimp);

        if (gimage->gimp->busy)
          return TRUE;

        switch (bevent->button)
          {
          case 1:
            state &= ~GDK_BUTTON1_MASK;

            if (active_tool && (! gimp_image_is_empty (gimage) ||
                                gimp_tool_control_handles_empty_image(active_tool->control)))
              {
                if (gimp_tool_control_is_active(active_tool->control))
                  {
                    if (gimp_tool_control_auto_snap_to(active_tool->control))
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

                    tool_manager_button_release_active (gimage->gimp,
                                                        &image_coords, time, state,
                                                        gdisp);
                  }
              }

            gdk_pointer_ungrab (time);

            /*  restore the tool's modifier state because it didn't get
             *  key events while BUTTON1 was down
             */
            if ((press_state & GDK_SHIFT_MASK) != (state & GDK_SHIFT_MASK))
              {
                tool_manager_modifier_key_active (gimage->gimp,
                                                  GDK_SHIFT_MASK,
                                                  (state & GDK_SHIFT_MASK) ?
                                                  TRUE : FALSE,
                                                  state,
                                                  gdisp);
              }

            if ((press_state & GDK_CONTROL_MASK) != (state & GDK_CONTROL_MASK))
              {
                tool_manager_modifier_key_active (gimage->gimp,
                                                  GDK_CONTROL_MASK,
                                                  (state & GDK_CONTROL_MASK) ?
                                                  TRUE : FALSE,
                                                  state,
                                                  gdisp);
              }

            if ((press_state & GDK_MOD1_MASK) != (state & GDK_MOD1_MASK))
              {
                tool_manager_modifier_key_active (gimage->gimp,
                                                  GDK_MOD1_MASK,
                                                  (state & GDK_MOD1_MASK) ?
                                                  TRUE : FALSE,
                                                  state,
                                                  gdisp);
              }

            tool_manager_oper_update_active (gimage->gimp,
                                             &image_coords, state,
                                             gdisp);
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

        tool_manager_oper_update_active (gimage->gimp,
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

        if (gimage->gimp->busy)
          return TRUE;

        active_tool = tool_manager_get_active (gimage->gimp);

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
                                           gimp_devices_get_current (gimage->gimp),
                                           &display_coords);
            gimp_display_shell_get_state (shell, compressed_motion,
                                          gimp_devices_get_current (gimage->gimp),
                                          &state);
            time = gdk_event_get_time (event);

            image_coords = display_coords;

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
            if (active_tool                  &&
                gimp_tool_control_is_active(active_tool->control) &&
                (! gimp_image_is_empty (gimage) ||
                 gimp_tool_control_handles_empty_image(active_tool->control)))
              {
                /*  if the first mouse button is down, check for automatic
                 *  scrolling...
                 */
                if ((mevent->x < 0                 ||
                     mevent->y < 0                 ||
                     mevent->x > shell->disp_width ||
                     mevent->y > shell->disp_height) &&
                    ! gimp_tool_control_scroll_lock(active_tool->control))
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

                if (gimp_tool_control_auto_snap_to(active_tool->control))
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

                tool_manager_motion_active (gimage->gimp,
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
            tool_manager_oper_update_active (gimage->gimp,
                                             &image_coords, state,
                                             gdisp);
          }
      }
      break;

    case GDK_KEY_PRESS:
      {
        GdkEventKey *kevent;

        kevent = (GdkEventKey *) event;

        /*  ignore any key presses  */
        if (gimage->gimp->busy)
          return TRUE;

        /*  do not process any key event while BUTTON1 is down. We do this
         *  so tools keep the modifier state they were in when BUTTON1 was
         *  pressed and to prevent accelerators from being invoked.
         */
        if (state & GDK_BUTTON1_MASK)
          return TRUE;

        switch (kevent->keyval)
          {
            GdkModifierType key;

          case GDK_Left: case GDK_Right:
          case GDK_Up: case GDK_Down:
            if (! gimp_image_is_empty (gimage))
              {
                tool_manager_arrow_key_active (gimage->gimp,
                                               kevent,
                                               gdisp);
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
            key = gimp_display_shell_key_to_state (kevent->keyval);
            state |= key;

            /*  For all modifier keys: call the tools modifier_key *and*
             *  oper_update method so tools can choose if they are interested
             *  in the release itself or only in the resulting state
             */
            if (! gimp_image_is_empty (gimage))
              {
                tool_manager_modifier_key_active (gimage->gimp,
                                                  key, TRUE, state,
                                                  gdisp);
              }
            break;
          }

        tool_manager_oper_update_active (gimage->gimp,
                                         &image_coords, state,
                                         gdisp);
      }
      break;

    case GDK_KEY_RELEASE:
      {
        GdkEventKey *kevent;

        kevent = (GdkEventKey *) event;

        /*  ignore any key releases  */
        if (gimage->gimp->busy)
          return TRUE;

        /*  do not process any key event while BUTTON1 is down. We do this
         *  so tools keep the modifier state they were in when BUTTON1 was
         *  pressed and to prevent accelerators from being invoked.
         */
        if (state & GDK_BUTTON1_MASK)
          return TRUE;

        switch (kevent->keyval)
          {
            GdkModifierType key;

            /*  Update the state based on modifiers being pressed  */
          case GDK_Alt_L: case GDK_Alt_R:
          case GDK_Shift_L: case GDK_Shift_R:
          case GDK_Control_L: case GDK_Control_R:
            key = gimp_display_shell_key_to_state (kevent->keyval);
            state &= ~key;

            /*  For all modifier keys: call the tools modifier_key *and*
             *  oper_update method so tools can choose if they are interested
             *  in the press itself or only in the resulting state
             */
            if (! gimp_image_is_empty (gimage))
              {
                tool_manager_modifier_key_active (gimage->gimp,
                                                  key, FALSE, state,
                                                  gdisp);
              }
            break;
          }

        tool_manager_oper_update_active (gdisp->gimage->gimp,
                                         &image_coords, state,
                                         gdisp);
      }
      break;

    default:
      break;
    }

  /*  if we reached this point in gimp_busy mode, return now  */
  if (gimage->gimp->busy)
    return TRUE;

  /*  cursor update support  */

  if (! gimprc.no_cursor_updating)
    {
      GimpTool *active_tool;

      active_tool = tool_manager_get_active (gimage->gimp);

      if (active_tool)
        {
          if ((! gimp_image_is_empty (gimage) ||
               gimp_tool_control_handles_empty_image(active_tool->control)) &&
              ! (state & (GDK_BUTTON1_MASK |
                          GDK_BUTTON2_MASK |
                          GDK_BUTTON3_MASK)))
            {
              tool_manager_cursor_update_active (gimage->gimp,
                                                 &image_coords, state,
                                                 gdisp);
            }
          else if (gimp_image_is_empty (gimage))
            {
              gimp_display_shell_set_cursor (shell,
                                             GIMP_BAD_CURSOR,
                                             gimp_tool_control_get_tool_cursor(active_tool->control),
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
  GimpDisplay *gdisp;

  gdisp = shell->gdisp;

  if (! gdisp->gimage->gimp->busy && event->button == 1)
    {
      GtkItemFactory *factory;
      gint            x, y;

      factory = GTK_ITEM_FACTORY (shell->item_factory);

      gimp_display_shell_origin_menu_position (GTK_MENU (factory->widget),
                                               &x, &y,
                                               widget);

      gtk_item_factory_popup_with_data (factory,
					gdisp->gimage,
                                        NULL,
					x, y,
					1, event->time);
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
                                   "/Default Color", &color, FALSE);
    }

  return FALSE;
}

void
gimp_display_shell_color_button_changed (GtkWidget        *widget,
                                         GimpDisplayShell *shell)
{
  GdkColor gdk_color;
  guchar   r, g, b;

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (widget),
                               &shell->padding_color);

  gimp_rgb_get_uchar (&shell->padding_color, &r, &g, &b);

  gdk_color.red   = r + r * 256;
  gdk_color.green = g + g * 256;
  gdk_color.blue  = b + b * 256;

  gdk_gc_set_rgb_fg_color (shell->padding_gc, &gdk_color);

  gimp_display_shell_expose_full (shell);
  gimp_display_shell_flush (shell);
}

void  
gimp_display_shell_color_button_default (gpointer   callback_data, 
                                         guint      callback_action, 
                                         GtkWidget *widget)
{
  GimpDisplayShell *shell;
  guchar            r, g, b;
  GimpRGB           color;

  shell = GIMP_DISPLAY_SHELL (callback_data);

  r = shell->canvas->style->bg[GTK_STATE_NORMAL].red   >> 8;
  g = shell->canvas->style->bg[GTK_STATE_NORMAL].green >> 8;
  b = shell->canvas->style->bg[GTK_STATE_NORMAL].blue  >> 8;

  gimp_rgb_set_uchar (&color, r, g, b);

  gimp_color_button_set_color (GIMP_COLOR_BUTTON (shell->padding_button),
                               &color);
}

gboolean
gimp_display_shell_qmask_button_press (GtkWidget        *widget,
                                       GdkEventButton   *event,
                                       GimpDisplayShell *shell)
{
  GimpDisplay *gdisp;

  gdisp = shell->gdisp;

  if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3))
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

  gdisplays_flush ();
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
