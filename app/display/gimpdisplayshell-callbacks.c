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
#include "core/gimpimage-guides.h"
#include "core/gimpimage-qmask.h"
#include "core/gimplayer.h"
#include "core/gimptoolinfo.h"

#include "tools/gimpmovetool.h"
#include "tools/gimptoolcontrol.h"
#include "tools/tool_manager.h"

#include "widgets/gimpcursor.h"
#include "widgets/gimpdevices.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvas.h"
#include "gimpdisplay.h"
#include "gimpdisplayoptions.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-cursor.h"
#include "gimpdisplayshell-draw.h"
#include "gimpdisplayshell-layer-select.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-selection.h"
#include "gimpdisplayshell-title.h"
#include "gimpdisplayshell-transform.h"
#include "gimpnavigationview.h"
#include "gimpstatusbar.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void     gimp_display_shell_vscrollbar_update (GtkAdjustment    *adjustment,
                                                      GimpDisplayShell *shell);
static void     gimp_display_shell_hscrollbar_update (GtkAdjustment    *adjustment,
                                                      GimpDisplayShell *shell);

static gboolean gimp_display_shell_get_event_coords  (GimpDisplayShell *shell,
                                                      GdkEvent         *event,
                                                      GdkDevice        *device,
                                                      GimpCoords       *coords);
static void     gimp_display_shell_get_device_coords (GimpDisplayShell *shell,
                                                      GdkDevice        *device,
                                                      GimpCoords       *coords);
static gboolean gimp_display_shell_get_event_state   (GimpDisplayShell *shell,
                                                      GdkEvent         *event,
                                                      GdkDevice        *device,
                                                      GdkModifierType  *state);
static void     gimp_display_shell_get_device_state  (GimpDisplayShell *shell,
                                                      GdkDevice        *device,
                                                      GdkModifierType  *state);

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
  Gimp        *gimp;
  gboolean     set_display = FALSE;

  /*  are we in destruction?  */
  if (! shell->gdisp || ! shell->gdisp->shell)
    return TRUE;

  gimp = shell->gdisp->gimage->gimp;

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
          case GDK_Left:      case GDK_Right:
          case GDK_Up:        case GDK_Down:
          case GDK_space:
          case GDK_Tab:
          case GDK_ISO_Left_Tab:
          case GDK_Alt_L:     case GDK_Alt_R:
          case GDK_Shift_L:   case GDK_Shift_R:
          case GDK_Control_L: case GDK_Control_R:
            break;

          case GDK_Escape:
            if (event->type == GDK_KEY_PRESS)
              gimp_display_shell_set_fullscreen (shell, FALSE);
            break;

          default:
            if (shell->space_pressed)
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

        if (fevent->in && GIMP_DISPLAY_CONFIG (gimp->config)->activate_on_focus)
          set_display = TRUE;
      }
      break;

    case GDK_WINDOW_STATE:
      {
	GdkEventWindowState *sevent = (GdkEventWindowState *) event;
        GimpDisplayOptions  *options;
        gboolean             fullscreen;

	shell->window_state = sevent->new_window_state;

        if (! (sevent->changed_mask & GDK_WINDOW_STATE_FULLSCREEN))
          break;

        fullscreen = gimp_display_shell_get_fullscreen (shell);

        options = fullscreen ? shell->fullscreen_options : shell->options;

        gimp_display_shell_set_show_menubar    (shell,
                                                options->show_menubar);
        gimp_display_shell_set_show_rulers     (shell,
                                                options->show_rulers);
        gimp_display_shell_set_show_scrollbars (shell,
                                                options->show_scrollbars);
        gimp_display_shell_set_show_statusbar  (shell,
                                                options->show_statusbar);
        gimp_display_shell_set_show_selection  (shell,
                                                options->show_selection);
        gimp_display_shell_set_show_layer      (shell,
                                                options->show_layer_boundary);
        gimp_display_shell_set_show_guides     (shell,
                                                options->show_guides);
        gimp_display_shell_set_show_grid       (shell,
                                                options->show_grid);
        gimp_display_shell_set_padding         (shell,
                                                options->padding_mode,
                                                &options->padding_color);

        gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->menubar_factory),
				      "/View/Fullscreen", fullscreen);

        if (shell->gdisp ==
            gimp_context_get_display (gimp_get_user_context (gimp)))
	gimp_item_factory_set_active (GTK_ITEM_FACTORY (shell->popup_factory),
				      "/View/Fullscreen", fullscreen);
      }
      break;

    default:
      break;
    }

  if (set_display)
    {
      Gimp *gimp = shell->gdisp->gimage->gimp;

      /*  Setting the context's display automatically sets the image, too  */
      gimp_context_set_display (gimp_get_user_context (gimp), shell->gdisp);
    }

  return FALSE;
}

void
gimp_display_shell_canvas_realize (GtkWidget        *canvas,
                                   GimpDisplayShell *shell)
{
  GimpDisplayConfig     *config;
  GimpDisplay           *gdisp;
  GimpCanvasPaddingMode  padding_mode;
  GimpRGB                padding_color;

  gdisp  = shell->gdisp;
  config = GIMP_DISPLAY_CONFIG (gdisp->gimage->gimp->config);

  gtk_widget_grab_focus (shell->canvas);

  gimp_display_shell_get_padding (shell, &padding_mode, &padding_color);
  gimp_display_shell_set_padding (shell, padding_mode, &padding_color);

  gimp_statusbar_resize_cursor (GIMP_STATUSBAR (shell->statusbar));

  gimp_display_shell_update_title (shell);

  shell->disp_width  = canvas->allocation.width;
  shell->disp_height = canvas->allocation.height;

  /*  set up the scrollbar observers  */
  g_signal_connect (shell->hsbdata, "value_changed",
                    G_CALLBACK (gimp_display_shell_hscrollbar_update),
                    shell);
  g_signal_connect (shell->vsbdata, "value_changed",
                    G_CALLBACK (gimp_display_shell_vscrollbar_update),
                    shell);

  /*  set the initial cursor  */
  gimp_display_shell_set_cursor (shell,
                                 GDK_TOP_LEFT_ARROW,
                                 GIMP_TOOL_CURSOR_NONE,
                                 GIMP_CURSOR_MODIFIER_NONE);

  /*  allow shrinking  */
  gtk_widget_set_size_request (GTK_WIDGET (shell), 0, 0);

  gimp_display_shell_draw_vectors (shell);
}

gboolean
gimp_display_shell_canvas_configure (GtkWidget         *widget,
                                     GdkEventConfigure *cevent,
                                     GimpDisplayShell  *shell)
{
  /*  are we in destruction?  */
  if (! shell->gdisp || ! shell->gdisp->shell)
    return TRUE;

  if ((shell->disp_width  != shell->canvas->allocation.width) ||
      (shell->disp_height != shell->canvas->allocation.height))
    {
      shell->disp_width  = shell->canvas->allocation.width;
      shell->disp_height = shell->canvas->allocation.height;

      gimp_display_shell_scroll_clamp_offsets (shell);
      gimp_display_shell_scale_setup (shell);
      gimp_display_shell_scaled (shell);
    }

  return TRUE;
}

gboolean
gimp_display_shell_canvas_expose (GtkWidget        *widget,
                                  GdkEventExpose   *eevent,
                                  GimpDisplayShell *shell)
{
  GdkRegion    *region = NULL;
  GdkRectangle *rects;
  gint          n_rects;
  gint          i;

  /*  are we in destruction?  */
  if (! shell->gdisp || ! shell->gdisp->shell)
    return TRUE;

  /*  If the call to gimp_display_shell_pause() would cause a redraw,
   *  we need to make sure that no XOR drawing happens on areas that
   *  have already been cleared by the windowing system.
   */
  if (shell->paused_count == 0)
    {
      GdkRectangle  area;

      area.x      = 0;
      area.y      = 0;
      area.width  = shell->disp_width;
      area.height = shell->disp_height;

      region = gdk_region_rectangle (&area);

      gdk_region_subtract (region, eevent->region);

      gimp_canvas_set_clip_region (GIMP_CANVAS (shell->canvas),
                                   GIMP_CANVAS_STYLE_XOR, region);
      gimp_canvas_set_clip_region (GIMP_CANVAS (shell->canvas),
                                   GIMP_CANVAS_STYLE_XOR_DASHED, region);
    }

  gimp_display_shell_pause (shell);

  if (region)
    {
      gimp_canvas_set_clip_region (GIMP_CANVAS (shell->canvas),
                                   GIMP_CANVAS_STYLE_XOR, NULL);
      gimp_canvas_set_clip_region (GIMP_CANVAS (shell->canvas),
                                   GIMP_CANVAS_STYLE_XOR_DASHED, NULL);
      gdk_region_destroy (region);
    }

  gdk_region_get_rectangles (eevent->region, &rects, &n_rects);

  for (i = 0; i < n_rects; i++)
    gimp_display_shell_draw_area (shell,
                                  rects[i].x,
                                  rects[i].y,
                                  rects[i].width,
                                  rects[i].height);

  g_free (rects);

  /* draw the guides */
  gimp_display_shell_draw_guides (shell);

  /* draw the grid */
  gimp_display_shell_draw_grid (shell);

  /* and the cursor (if we have a software cursor) */
  gimp_display_shell_draw_cursor (shell);

  /* restart (and recalculate) the selection boundaries */
  gimp_display_shell_selection_start (shell->select, TRUE);

  gimp_display_shell_resume (shell);

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
gimp_display_shell_popup_menu (GtkWidget *widget)
{
  GimpDisplayShell *shell;

  shell = GIMP_DISPLAY_SHELL (widget);

  gimp_context_set_display (gimp_get_user_context (shell->gdisp->gimage->gimp),
                            shell->gdisp);

  gimp_item_factory_popup_with_data (shell->popup_factory,
                                     shell->gdisp,
                                     GTK_WIDGET (shell),
                                     gimp_display_shell_origin_menu_position,
                                     shell->origin,
                                     NULL);

  return TRUE;
}

gboolean
gimp_display_shell_canvas_tool_events (GtkWidget        *canvas,
                                       GdkEvent         *event,
                                       GimpDisplayShell *shell)
{
  GimpDisplay     *gdisp;
  GimpImage       *gimage;
  Gimp            *gimp;
  GdkDisplay      *gdk_display;
  GimpCoords       display_coords;
  GimpCoords       image_coords;
  GdkModifierType  state;
  guint32          time;
  gboolean         return_val    = FALSE;
  gboolean         update_cursor = FALSE;

  static GimpToolInfo *space_shaded_tool  = NULL;

  if (! canvas->window)
    {
      g_warning ("%s: called unrealized", G_STRLOC);
      return FALSE;
    }

  /*  are we in destruction?  */
  if (! shell->gdisp || ! shell->gdisp->shell)
    return TRUE;

  gdisp  = shell->gdisp;
  gimage = gdisp->gimage;
  gimp   = gimage->gimp;

  /*  Find out what device the event occurred upon  */
  if (! gimp->busy && gimp_devices_check_change (gimp, event))
    {
      gimp_display_shell_check_device_cursor (shell);
    }

  gimp_display_shell_get_event_coords (shell, event,
                                       gimp_devices_get_current (gimp),
                                       &display_coords);
  gimp_display_shell_get_event_state (shell, event,
                                      gimp_devices_get_current (gimp),
                                      &state);
  time = gdk_event_get_time (event);

  /*  GimpCoords passed to tools are ALWAYS in image coordinates  */
  gimp_display_shell_untransform_coords (shell,
                                         &display_coords,
                                         &image_coords);

  gdk_display = gtk_widget_get_display (canvas);

  switch (event->type)
    {
    case GDK_ENTER_NOTIFY:
      {
        GdkEventCrossing *cevent = (GdkEventCrossing *) event;

        if (cevent->mode != GDK_CROSSING_NORMAL)
          return TRUE;

        update_cursor = TRUE;

        tool_manager_oper_update_active (gimp,
                                         &image_coords, state,
                                         gdisp);
      }
      break;

    case GDK_LEAVE_NOTIFY:
      {
        GdkEventCrossing *cevent = (GdkEventCrossing *) event;

        if (cevent->mode != GDK_CROSSING_NORMAL)
          return TRUE;

        shell->proximity = FALSE;
        gimp_display_shell_update_cursor (shell, -1, -1);

        tool_manager_oper_update_active (gimp,
                                         &image_coords, state,
                                         gdisp);
      }
      break;

    case GDK_PROXIMITY_IN:
      tool_manager_oper_update_active (gimp,
                                       &image_coords, state,
                                       gdisp);
      break;

    case GDK_PROXIMITY_OUT:
      shell->proximity = FALSE;
      gimp_display_shell_update_cursor (shell, -1, -1);

      tool_manager_oper_update_active (gimp,
                                       &image_coords, state,
                                       gdisp);
      break;

    case GDK_FOCUS_CHANGE:
      {
        GdkEventFocus *fevent = (GdkEventFocus *) event;

        if (fevent->in)
          {
            GTK_WIDGET_SET_FLAGS (canvas, GTK_HAS_FOCUS);

            /*  press modifier keys when the canvas gets the focus
             *
             *  in "click to focus" mode, we did this on BUTTON_PRESS, so
             *  do it here only if button_press_before_focus is FALSE
             */
            if (! shell->button_press_before_focus)
              {
                tool_manager_focus_display_active (gimp, gdisp);
                tool_manager_modifier_state_active (gimp, state, gdisp);

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
            shell->button_press_before_focus = FALSE;

            /*  release modifier keys when the canvas loses the focus  */
            tool_manager_focus_display_active (gimp, NULL);

            tool_manager_oper_update_active (gimp,
                                             &image_coords, 0,
                                             gdisp);
          }

        /*  stop the signal because otherwise gtk+ exposes the whole
         *  canvas to get the non-existant focus indicator drawn
         */
        return_val = TRUE;
      }
      break;

    case GDK_BUTTON_PRESS:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;
        GdkEventMask    event_mask;
        GimpTool       *active_tool;

        if (! GTK_WIDGET_HAS_FOCUS (canvas))
          {
            /*  in "click to focus" mode, the BUTTON_PRESS arrives before
             *  FOCUS_IN, so we have to update the tool's modifier state here
             */
            tool_manager_focus_display_active (gimp, gdisp);
            tool_manager_modifier_state_active (gimp, state, gdisp);

            tool_manager_oper_update_active (gimp,
                                             &image_coords, state,
                                             gdisp);

            shell->button_press_before_focus = TRUE;

            /*  we expect a FOCUS_IN event to follow, but can't rely
             *  on it, so force one
             */
            gdk_window_focus (canvas->window, time);
          }

        /*  ignore new mouse events  */
        if (gimp->busy)
          return TRUE;

        active_tool = tool_manager_get_active (gimp);

        switch (bevent->button)
          {
          case 1:
            state |= GDK_BUTTON1_MASK;

            event_mask = (GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);

            if (! GIMP_DISPLAY_CONFIG (gimp->config)->perfect_mouse &&
                (gimp_tool_control_motion_mode (active_tool->control) !=
                 GIMP_MOTION_MODE_EXACT))
              {
                /*  don't request motion hins for XInput devices because
                 *  the wacom driver is known to report crappy hints
                 *  (#6901) --mitch
                 */
                if (gimp_devices_get_current (gimp) ==
                    gdk_display_get_core_pointer (gdk_display))
                  {
                    event_mask |= (GDK_POINTER_MOTION_HINT_MASK);
                  }
              }

            gdk_pointer_grab (canvas->window,
                              FALSE, event_mask, NULL, NULL, time);

            if (! shell->space_pressed && ! shell->space_release_pending)
              gdk_keyboard_grab (canvas->window, FALSE, time);

            if (active_tool &&
                (! gimp_image_is_empty (gimage) ||
                 gimp_tool_control_handles_empty_image (active_tool->control)))
              {
                gboolean initialized = TRUE;

                if (gimp_tool_control_auto_snap_to (active_tool->control))
                  {
                    gint x, y, width, height;

                    gimp_tool_control_snap_offsets (active_tool->control,
                                                    &x, &y, &width, &height);

                    gimp_display_shell_snap_coords (shell,
                                                    &image_coords,
                                                    &image_coords,
                                                    x, y, width, height);

                    update_cursor = TRUE;
                  }

                /*  initialize the current tool if it has no drawable
                 */
                if (! active_tool->drawable)
                  {
                    initialized = tool_manager_initialize_active (gimp, gdisp);
                  }
                else if ((active_tool->drawable !=
                          gimp_image_active_drawable (gimage)) &&
                         ! gimp_tool_control_preserve (active_tool->control))
                  {
                    /*  create a new one, deleting the current
                     */
                    gimp_context_tool_changed (gimp_get_user_context (gimp));

                    initialized = tool_manager_initialize_active (gimp, gdisp);
                  }

                if (initialized)
                  tool_manager_button_press_active (gimp,
                                                    &image_coords, time, state,
                                                    gdisp);
              }
            break;

          case 2:
            state |= GDK_BUTTON2_MASK;

            shell->scrolling      = TRUE;
            shell->scroll_start_x = bevent->x + shell->offset_x;
            shell->scroll_start_y = bevent->y + shell->offset_y;

            gtk_grab_add (canvas);

            gimp_display_shell_set_override_cursor (shell, GDK_FLEUR);
            break;

          case 3:
            state |= GDK_BUTTON3_MASK;
            gimp_item_factory_popup_with_data (shell->popup_factory,
                                               gdisp,
                                               GTK_WIDGET (shell),
                                               NULL, NULL, NULL);
            return_val = TRUE;
            break;

          default:
            break;
          }
      }
      break;

    case GDK_BUTTON_RELEASE:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;
        GimpTool       *active_tool;

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
                        gint x, y, width, height;

                        gimp_tool_control_snap_offsets (active_tool->control,
                                                        &x, &y, &width, &height);

                        gimp_display_shell_snap_coords (shell,
                                                        &image_coords,
                                                        &image_coords,
                                                        x, y, width, height);

                        update_cursor = TRUE;
                      }

                    tool_manager_button_release_active (gimp,
                                                        &image_coords,
                                                        time, state,
                                                        gdisp);
                  }
              }

            /*  update the tool's modifier state because it didn't get
             *  key events while BUTTON1 was down
             */
            tool_manager_focus_display_active (gimp, gdisp);
            tool_manager_modifier_state_active (gimp, state, gdisp);

            tool_manager_oper_update_active (gimp,
                                             &image_coords, state,
                                             gdisp);

            if (! shell->space_pressed && ! shell->space_release_pending)
              gdk_display_keyboard_ungrab (gdk_display, time);

            gdk_display_pointer_ungrab (gdk_display, time);

            if (shell->space_release_pending)
              {
                g_print ("%s: popping move tool\n", G_GNUC_FUNCTION);

                gimp_context_set_tool (gimp_get_user_context (gimp),
                                       space_shaded_tool);
                space_shaded_tool = NULL;

                tool_manager_focus_display_active (gimp, gdisp);
                tool_manager_modifier_state_active (gimp, state, gdisp);

                tool_manager_oper_update_active (gimp,
                                                 &image_coords, state,
                                                 gdisp);

                shell->space_release_pending = FALSE;

                gdk_display_keyboard_ungrab (gdk_display, time);
              }
            break;

          case 2:
            state &= ~GDK_BUTTON2_MASK;

            shell->scrolling      = FALSE;
            shell->scroll_start_x = 0;
            shell->scroll_start_y = 0;

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
        GdkEventScroll     *sevent = (GdkEventScroll *) event;
        GdkScrollDirection  direction;

        direction = sevent->direction;

       if (state & GDK_SHIFT_MASK)
          {
            switch (direction)
              {
              case GDK_SCROLL_UP:
                gimp_display_shell_scale (shell, GIMP_ZOOM_IN, 0.0);
                break;

              case GDK_SCROLL_DOWN:
                gimp_display_shell_scale (shell, GIMP_ZOOM_OUT, 0.0);
                break;

              default:
                break;
              }
          }
        else
          {
            GtkAdjustment *adj = NULL;
            gdouble        value;

            if (state & GDK_CONTROL_MASK)
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

            value = adj->value + ((direction == GDK_SCROLL_UP ||
                                   direction == GDK_SCROLL_LEFT) ?
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
        GdkEventMotion *mevent            = (GdkEventMotion *) event;
        GdkEvent       *compressed_motion = NULL;
        GimpTool       *active_tool;

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
            GdkDevice *device = gimp_devices_get_current (gimp);

            gimp_display_shell_get_event_coords (shell, compressed_motion,
                                                 device, &display_coords);
            gimp_display_shell_get_event_state (shell, compressed_motion,
                                                device, &state);
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
                    gint x, y, width, height;

                    gimp_tool_control_snap_offsets (active_tool->control,
                                                    &x, &y, &width, &height);

                    gimp_display_shell_snap_coords (shell,
                                                    &image_coords,
                                                    &image_coords,
                                                    x, y, width, height);

                    update_cursor = TRUE;
                  }

                tool_manager_motion_active (gimp,
                                            &image_coords, time, state,
                                            gdisp);
              }
          }
        else if (state & GDK_BUTTON2_MASK)
          {
            if (shell->scrolling)
              {
                gimp_display_shell_scroll (shell,
                                           (shell->scroll_start_x - mevent->x -
                                            shell->offset_x),
                                           (shell->scroll_start_y - mevent->y -
                                            shell->offset_y));
              }
          }

        if (! (state &
               (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
          {
            tool_manager_oper_update_active (gimp,
                                             &image_coords, state,
                                             gdisp);
          }
      }
      break;

    case GDK_KEY_PRESS:
      {
        GdkEventKey *kevent = (GdkEventKey *) event;

        tool_manager_focus_display_active (gimp, gdisp);

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
              GimpTool *active_tool = tool_manager_get_active (gimp);

              if (! shell->space_pressed && ! GIMP_IS_MOVE_TOOL (active_tool))
                {
                  GimpToolInfo *move_tool_info;

                  move_tool_info = (GimpToolInfo *)
                    gimp_container_get_child_by_name (gimp->tool_info_list,
                                                      "gimp-move-tool");

                  if (GIMP_IS_TOOL_INFO (move_tool_info))
                    {
                      g_print ("%s: pushing move tool\n", G_GNUC_FUNCTION);

                      space_shaded_tool = active_tool->tool_info;

                      gdk_keyboard_grab (canvas->window, FALSE, time);

                      gimp_context_set_tool (gimp_get_user_context (gimp),
                                             move_tool_info);

                      tool_manager_focus_display_active (gimp, gdisp);
                      tool_manager_modifier_state_active (gimp, state, gdisp);

                      shell->space_pressed = TRUE;
                    }
                }
            }

            return_val = TRUE;
            break;

          case GDK_Tab:
          case GDK_ISO_Left_Tab:
            if (! state)
              {
                GimpDialogFactory *dialog_factory;

                dialog_factory = gimp_dialog_factory_from_name ("toolbox");

                /*  Hide or show all dialogs  */
                gimp_dialog_factories_toggle (dialog_factory);
              }
            else if (! gimp_image_is_empty (gimage))
              {
                if (state & GDK_CONTROL_MASK)
                  {
                    if (kevent->keyval == GDK_Tab)
                      gimp_display_shell_layer_select_init (shell,
                                                            1, kevent->time);
                    else
                      gimp_display_shell_layer_select_init (shell,
                                                            -1, kevent->time);
                  }
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

              /*  For all modifier keys: call the tools modifier_state *and*
               *  oper_update method so tools can choose if they are interested
               *  in the release itself or only in the resulting state
               */
              if (! gimp_image_is_empty (gimage))
                tool_manager_modifier_state_active (gimp, state, gdisp);
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
        GdkEventKey *kevent = (GdkEventKey *) event;

        tool_manager_focus_display_active (gimp, gdisp);

        switch (kevent->keyval)
          {
          case GDK_space:
            if (shell->space_pressed)
              {
                g_print ("%s: popping move tool\n", G_GNUC_FUNCTION);

                gimp_context_set_tool (gimp_get_user_context (gimp),
                                       space_shaded_tool);
                space_shaded_tool = NULL;

                tool_manager_focus_display_active (gimp, gdisp);
                tool_manager_modifier_state_active (gimp, state, gdisp);

                shell->space_pressed = FALSE;

                gdk_display_keyboard_ungrab (gdk_display, time);
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

              /*  For all modifier keys: call the tools modifier_state *and*
               *  oper_update method so tools can choose if they are interested
               *  in the press itself or only in the resulting state
               */
              if (! gimp_image_is_empty (gimage))
                tool_manager_modifier_state_active (gimp, state, gdisp);
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
    return return_val;

  /*  cursor update support  */

  if (GIMP_DISPLAY_CONFIG (gimp->config)->cursor_updating)
    {
      GimpTool *active_tool = tool_manager_get_active (gimp);

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
  GimpDisplay *gdisp = shell->gdisp;

  if (gdisp->gimage->gimp->busy)
    return TRUE;

  if (event->type == GDK_BUTTON_PRESS && event->button == 1)
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

              gdk_keyboard_grab (shell->canvas->window, FALSE, event->time);

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
  GimpDisplay *gdisp = shell->gdisp;

  if (gdisp->gimage->gimp->busy)
    return TRUE;

  if (event->type == GDK_BUTTON_PRESS && event->button == 1)
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

              gdk_keyboard_grab (shell->canvas->window, FALSE, event->time);

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
  if (! shell->gdisp->gimage->gimp->busy)
    {
      if (event->button == 1)
        gimp_display_shell_popup_menu (GTK_WIDGET (shell));
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
      GtkItemFactory  *item_factory;
      guchar           r, g, b;
      GimpRGB          color;

      color_button = GIMP_COLOR_BUTTON (widget);
      item_factory = GTK_ITEM_FACTORY (color_button->popup_menu);

      r = shell->canvas->style->bg[GTK_STATE_NORMAL].red   >> 8;
      g = shell->canvas->style->bg[GTK_STATE_NORMAL].green >> 8;
      b = shell->canvas->style->bg[GTK_STATE_NORMAL].blue  >> 8;

      gimp_rgba_set_uchar (&color, r, g, b, 255);
      gimp_item_factory_set_color (item_factory,
                                   "/From Theme", &color, FALSE);

      gimp_rgba_set_uchar (&color,
                           render_blend_light_check[0],
                           render_blend_light_check[1],
                           render_blend_light_check[2],
                           255);
      gimp_item_factory_set_color (item_factory,
                                   "/Light Check Color", &color, FALSE);

      gimp_rgba_set_uchar (&color,
                           render_blend_dark_check[0],
                           render_blend_dark_check[1],
                           render_blend_dark_check[2],
                           255);
      gimp_item_factory_set_color (item_factory,
                                   "/Dark Check Color", &color, FALSE);
    }

  return FALSE;
}

void
gimp_display_shell_color_button_changed (GtkWidget        *widget,
                                         GimpDisplayShell *shell)
{
  GimpRGB color;

  if (gimp_display_shell_get_fullscreen (shell))
    shell->fullscreen_options->padding_mode_set = TRUE;
  else
    shell->options->padding_mode_set = TRUE;

  gimp_color_button_get_color (GIMP_COLOR_BUTTON (widget), &color);

  gimp_display_shell_set_padding (shell,
                                  GIMP_CANVAS_PADDING_MODE_CUSTOM, &color);
}

void
gimp_display_shell_color_button_menu_callback (gpointer   callback_data,
                                               guint      callback_action,
                                               GtkWidget *widget)
{
  GimpDisplayOptions *options;
  GimpDisplayShell   *shell;
  gboolean            fullscreen;

  shell = GIMP_DISPLAY_SHELL (callback_data);

  fullscreen = gimp_display_shell_get_fullscreen (shell);

  if (fullscreen)
    options = shell->fullscreen_options;
  else
    options = shell->options;

  if (callback_action == GIMP_CANVAS_PADDING_MODE_CUSTOM)
    {
      gtk_button_clicked (GTK_BUTTON (shell->padding_button));
    }
  else if (callback_action == 0xffff)
    {
      GimpDisplayConfig  *config;
      GimpDisplayOptions *default_options;

      config = GIMP_DISPLAY_CONFIG (shell->gdisp->gimage->gimp->config);

      options->padding_mode_set = FALSE;

      if (fullscreen)
        default_options = config->default_fullscreen_view;
      else
        default_options = config->default_view;

      gimp_display_shell_set_padding (shell,
                                      default_options->padding_mode,
                                      &default_options->padding_color);
    }
  else
    {
      options->padding_mode_set = TRUE;

      gimp_display_shell_set_padding (shell, callback_action,
                                      &options->padding_color);
    }
}

gboolean
gimp_display_shell_qmask_button_press (GtkWidget        *widget,
                                       GdkEventButton   *bevent,
                                       GimpDisplayShell *shell)
{
  if ((bevent->type == GDK_BUTTON_PRESS) && (bevent->button == 3))
    {
      gimp_item_factory_popup_with_data (shell->qmask_factory, shell,
                                         GTK_WIDGET (shell),
                                         NULL, NULL, NULL);

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
gimp_display_shell_get_event_coords (GimpDisplayShell *shell,
                                     GdkEvent         *event,
                                     GdkDevice        *device,
                                     GimpCoords       *coords)
{
  if (gdk_event_get_axis (event, GDK_AXIS_X, &coords->x))
    {
      gdk_event_get_axis (event, GDK_AXIS_Y, &coords->y);

      /*  CLAMP() the return value of each *_get_axis() call to be safe
       *  against buggy XInput drivers. Provide default values if the
       *  requested axis does not exist.
       */

      if (gdk_event_get_axis (event, GDK_AXIS_PRESSURE, &coords->pressure))
        coords->pressure = CLAMP (coords->pressure, 0.0, 1.0);
      else
        coords->pressure = 1.0;

      if (gdk_event_get_axis (event, GDK_AXIS_XTILT, &coords->xtilt))
        coords->xtilt = CLAMP (coords->xtilt, 0.0, 1.0);
      else
        coords->xtilt = 0.5;

      if (gdk_event_get_axis (event, GDK_AXIS_YTILT, &coords->ytilt))
        coords->ytilt = CLAMP (coords->ytilt, 0.0, 1.0);
      else
        coords->ytilt = 0.5;

      if (gdk_event_get_axis (event, GDK_AXIS_WHEEL, &coords->wheel))
        coords->wheel = CLAMP (coords->wheel, 0.0, 1.0);
      else
        coords->wheel = 0.5;

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

  gdk_device_get_axis (device, axes, GDK_AXIS_X, &coords->x);
  gdk_device_get_axis (device, axes, GDK_AXIS_Y, &coords->y);

  /*  CLAMP() the return value of each *_get_axis() call to be safe
   *  against buggy XInput drivers. Provide default values if the
   *  requested axis does not exist.
   */

  if (gdk_device_get_axis (device, axes, GDK_AXIS_PRESSURE, &coords->pressure))
    coords->pressure = CLAMP (coords->pressure, 0.0, 1.0);
  else
    coords->pressure = 1.0;

  if (gdk_device_get_axis (device, axes, GDK_AXIS_XTILT, &coords->xtilt))
    coords->xtilt = CLAMP (coords->xtilt, 0.0, 1.0);
  else
    coords->xtilt = 0.5;

  if (gdk_device_get_axis (device, axes, GDK_AXIS_YTILT, &coords->ytilt))
    coords->ytilt = CLAMP (coords->ytilt, 0.0, 1.0);
  else
    coords->ytilt = 0.5;

  if (gdk_device_get_axis (device, axes, GDK_AXIS_WHEEL, &coords->wheel))
    coords->wheel = CLAMP (coords->wheel, 0.0, 1.0);
  else
    coords->wheel = 0.5;
}

static gboolean
gimp_display_shell_get_event_state (GimpDisplayShell *shell,
                                    GdkEvent         *event,
                                    GdkDevice        *device,
                                    GdkModifierType  *state)
{
  if (gdk_event_get_state (event, state))
    return TRUE;

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
  gimp_button_menu_position (GTK_WIDGET (data), menu, GTK_POS_RIGHT, x, y);
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
      gdk_event_put ((GdkEvent *) list->data);
      gdk_event_free ((GdkEvent *) list->data);
    }

  g_list_free (requeued_events);

  return last_motion;
}
