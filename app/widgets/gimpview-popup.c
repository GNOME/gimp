/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaview-popup.c
 * Copyright (C) 2003-2006 Michael Natterer <mitch@ligma.org>
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

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmacontext.h"
#include "core/ligmaviewable.h"

#include "ligmaview.h"
#include "ligmaviewrenderer.h"
#include "ligmaview-popup.h"


#define VIEW_POPUP_DELAY 150


typedef struct _LigmaViewPopup LigmaViewPopup;

struct _LigmaViewPopup
{
  GtkWidget    *widget;
  LigmaContext  *context;
  LigmaViewable *viewable;

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

static void       ligma_view_popup_hide           (LigmaViewPopup  *popup);
static gboolean   ligma_view_popup_button_release (GtkWidget      *widget,
                                                  GdkEventButton *bevent,
                                                  LigmaViewPopup  *popup);
static void       ligma_view_popup_unmap          (GtkWidget      *widget,
                                                  LigmaViewPopup  *popup);
static void       ligma_view_popup_drag_begin     (GtkWidget      *widget,
                                                  GdkDragContext *context,
                                                  LigmaViewPopup  *popup);
static gboolean   ligma_view_popup_timeout        (LigmaViewPopup  *popup);


/*  public functions  */

gboolean
ligma_view_popup_show (GtkWidget      *widget,
                      GdkEventButton *bevent,
                      LigmaContext    *context,
                      LigmaViewable   *viewable,
                      gint            view_width,
                      gint            view_height,
                      gboolean        dot_for_dot)
{
  LigmaViewPopup *popup;
  gint           popup_width;
  gint           popup_height;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (bevent != NULL, FALSE);
  g_return_val_if_fail (context == NULL || LIGMA_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (LIGMA_IS_VIEWABLE (viewable), FALSE);

  if (! ligma_viewable_get_popup_size (viewable,
                                      view_width,
                                      view_height,
                                      dot_for_dot,
                                      &popup_width,
                                      &popup_height))
    return FALSE;

  popup = g_slice_new0 (LigmaViewPopup);

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
                    G_CALLBACK (ligma_view_popup_button_release),
                    popup);
  g_signal_connect (widget, "unmap",
                    G_CALLBACK (ligma_view_popup_unmap),
                    popup);
  g_signal_connect (widget, "drag-begin",
                    G_CALLBACK (ligma_view_popup_drag_begin),
                    popup);

  popup->timeout_id = g_timeout_add (VIEW_POPUP_DELAY,
                                     (GSourceFunc) ligma_view_popup_timeout,
                                     popup);

  g_object_set_data_full (G_OBJECT (widget), "ligma-view-popup", popup,
                          (GDestroyNotify) ligma_view_popup_hide);

  gtk_grab_add (widget);

  return TRUE;
}


/*  private functions  */

static void
ligma_view_popup_hide (LigmaViewPopup *popup)
{
  if (popup->timeout_id)
    g_source_remove (popup->timeout_id);

  if (popup->popup)
    gtk_widget_destroy (popup->popup);

  g_signal_handlers_disconnect_by_func (popup->widget,
                                        ligma_view_popup_button_release,
                                        popup);
  g_signal_handlers_disconnect_by_func (popup->widget,
                                        ligma_view_popup_unmap,
                                        popup);
  g_signal_handlers_disconnect_by_func (popup->widget,
                                        ligma_view_popup_drag_begin,
                                        popup);

  gtk_grab_remove (popup->widget);

  g_slice_free (LigmaViewPopup, popup);
}

static gboolean
ligma_view_popup_button_release (GtkWidget      *widget,
                                GdkEventButton *bevent,
                                LigmaViewPopup  *popup)
{
  if (bevent->button == popup->button)
    g_object_set_data (G_OBJECT (popup->widget), "ligma-view-popup", NULL);

  return FALSE;
}

static void
ligma_view_popup_unmap (GtkWidget     *widget,
                       LigmaViewPopup *popup)
{
  g_object_set_data (G_OBJECT (popup->widget), "ligma-view-popup", NULL);
}

static void
ligma_view_popup_drag_begin (GtkWidget      *widget,
                            GdkDragContext *context,
                            LigmaViewPopup  *popup)
{
  g_object_set_data (G_OBJECT (popup->widget), "ligma-view-popup", NULL);
}

static gboolean
ligma_view_popup_timeout (LigmaViewPopup *popup)
{
  GtkWidget    *window;
  GtkWidget    *frame;
  GtkWidget    *view;
  GdkDisplay   *display;
  GdkRectangle  workarea;
  gint          x;
  gint          y;

  popup->timeout_id = 0;

  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

  gtk_window_set_screen (GTK_WINDOW (window),
                         gtk_widget_get_screen (popup->widget));
  gtk_window_set_transient_for (GTK_WINDOW (window),
                                GTK_WINDOW (gtk_widget_get_toplevel (popup->widget)));

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (window), frame);
  gtk_widget_show (frame);

  view = ligma_view_new_full (popup->context,
                             popup->viewable,
                             popup->popup_width,
                             popup->popup_height,
                             0, TRUE, FALSE, FALSE);
  ligma_view_renderer_set_dot_for_dot (LIGMA_VIEW (view)->renderer,
                                      popup->dot_for_dot);
  gtk_container_add (GTK_CONTAINER (frame), view);
  gtk_widget_show (view);

  display = gtk_widget_get_display (popup->widget);

  gdk_monitor_get_workarea (gdk_display_get_monitor_at_point (display,
                                                              popup->button_x,
                                                              popup->button_y),
                            &workarea);

  x = popup->button_x - (popup->popup_width  / 2);
  y = popup->button_y - (popup->popup_height / 2);

  x = CLAMP (x, workarea.x, workarea.x + workarea.width  - popup->popup_width);
  y = CLAMP (y, workarea.y, workarea.y + workarea.height - popup->popup_height);

  gtk_window_move (GTK_WINDOW (window), x, y);
  gtk_widget_show (window);

  popup->popup = window;

  return FALSE;
}
