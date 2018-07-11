/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpview-popup.c
 * Copyright (C) 2003-2006 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpcontext.h"
#include "core/gimpviewable.h"

#include "gimpview.h"
#include "gimpviewrenderer.h"
#include "gimpview-popup.h"


#define VIEW_POPUP_DELAY 150


typedef struct _GimpViewPopup GimpViewPopup;

struct _GimpViewPopup
{
  GtkWidget    *widget;
  GimpContext  *context;
  GimpViewable *viewable;

  gint          popup_width;
  gint          popup_height;
  gboolean      dot_for_dot;
  gint          button;
  gint          button_x;
  gint          button_y;

  guint         timeout_id;
  GtkWidget    *popup;
};


/*  local function prototypes  */

static void       gimp_view_popup_hide           (GimpViewPopup  *popup);
static gboolean   gimp_view_popup_button_release (GtkWidget      *widget,
                                                  GdkEventButton *bevent,
                                                  GimpViewPopup  *popup);
static void       gimp_view_popup_unmap          (GtkWidget      *widget,
                                                  GimpViewPopup  *popup);
static void       gimp_view_popup_drag_begin     (GtkWidget      *widget,
                                                  GdkDragContext *context,
                                                  GimpViewPopup  *popup);
static gboolean   gimp_view_popup_timeout        (GimpViewPopup  *popup);


/*  public functions  */

gboolean
gimp_view_popup_show (GtkWidget      *widget,
                      GdkEventButton *bevent,
                      GimpContext    *context,
                      GimpViewable   *viewable,
                      gint            view_width,
                      gint            view_height,
                      gboolean        dot_for_dot)
{
  GimpViewPopup *popup;
  gint           popup_width;
  gint           popup_height;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (bevent != NULL, FALSE);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), FALSE);

  if (! gimp_viewable_get_popup_size (viewable,
                                      view_width,
                                      view_height,
                                      dot_for_dot,
                                      &popup_width,
                                      &popup_height))
    return FALSE;

  popup = g_slice_new0 (GimpViewPopup);

  popup->widget       = widget;
  popup->context      = context;
  popup->viewable     = viewable;
  popup->popup_width  = popup_width;
  popup->popup_height = popup_height;
  popup->dot_for_dot  = dot_for_dot;
  popup->button       = bevent->button;
  popup->button_x     = bevent->x_root;
  popup->button_y     = bevent->y_root;

  g_signal_connect (widget, "button-release-event",
                    G_CALLBACK (gimp_view_popup_button_release),
                    popup);
  g_signal_connect (widget, "unmap",
                    G_CALLBACK (gimp_view_popup_unmap),
                    popup);
  g_signal_connect (widget, "drag-begin",
                    G_CALLBACK (gimp_view_popup_drag_begin),
                    popup);

  popup->timeout_id = g_timeout_add (VIEW_POPUP_DELAY,
                                     (GSourceFunc) gimp_view_popup_timeout,
                                     popup);

  g_object_set_data_full (G_OBJECT (widget), "gimp-view-popup", popup,
                          (GDestroyNotify) gimp_view_popup_hide);

  gtk_grab_add (widget);

  return TRUE;
}


/*  private functions  */

static void
gimp_view_popup_hide (GimpViewPopup *popup)
{
  if (popup->timeout_id)
    g_source_remove (popup->timeout_id);

  if (popup->popup)
    gtk_widget_destroy (popup->popup);

  g_signal_handlers_disconnect_by_func (popup->widget,
                                        gimp_view_popup_button_release,
                                        popup);
  g_signal_handlers_disconnect_by_func (popup->widget,
                                        gimp_view_popup_unmap,
                                        popup);
  g_signal_handlers_disconnect_by_func (popup->widget,
                                        gimp_view_popup_drag_begin,
                                        popup);

  gtk_grab_remove (popup->widget);

  g_slice_free (GimpViewPopup, popup);
}

static gboolean
gimp_view_popup_button_release (GtkWidget      *widget,
                                GdkEventButton *bevent,
                                GimpViewPopup  *popup)
{
  if (bevent->button == popup->button)
    g_object_set_data (G_OBJECT (popup->widget), "gimp-view-popup", NULL);

  return FALSE;
}

static void
gimp_view_popup_unmap (GtkWidget     *widget,
                       GimpViewPopup *popup)
{
  g_object_set_data (G_OBJECT (popup->widget), "gimp-view-popup", NULL);
}

static void
gimp_view_popup_drag_begin (GtkWidget      *widget,
                            GdkDragContext *context,
                            GimpViewPopup  *popup)
{
  g_object_set_data (G_OBJECT (popup->widget), "gimp-view-popup", NULL);
}

static gboolean
gimp_view_popup_timeout (GimpViewPopup *popup)
{
  GtkWidget    *window;
  GtkWidget    *frame;
  GtkWidget    *view;
  GdkScreen    *screen;
  GdkRectangle  rect;
  gint          monitor;
  gint          x;
  gint          y;

  popup->timeout_id = 0;

  screen = gtk_widget_get_screen (popup->widget);

  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

  gtk_window_set_screen (GTK_WINDOW (window), screen);
  gtk_window_set_transient_for (GTK_WINDOW (window),
                                GTK_WINDOW (gtk_widget_get_toplevel (popup->widget)));

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (window), frame);
  gtk_widget_show (frame);

  view = gimp_view_new_full (popup->context,
                             popup->viewable,
                             popup->popup_width,
                             popup->popup_height,
                             0, TRUE, FALSE, FALSE);
  gimp_view_renderer_set_dot_for_dot (GIMP_VIEW (view)->renderer,
                                      popup->dot_for_dot);
  gtk_container_add (GTK_CONTAINER (frame), view);
  gtk_widget_show (view);

  x = popup->button_x - (popup->popup_width  / 2);
  y = popup->button_y - (popup->popup_height / 2);

  monitor = gdk_screen_get_monitor_at_point (screen, x, y);
  gdk_screen_get_monitor_workarea (screen, monitor, &rect);

  x = CLAMP (x, rect.x, rect.x + rect.width  - popup->popup_width);
  y = CLAMP (y, rect.y, rect.y + rect.height - popup->popup_height);

  gtk_window_move (GTK_WINDOW (window), x, y);
  gtk_widget_show (window);

  popup->popup = window;

  return FALSE;
}
