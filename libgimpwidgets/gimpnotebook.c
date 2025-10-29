/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpnotebook.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpnotebook.h"


/**
 * SECTION: gimpnotebook
 * @title: GimpNotebook
 * @short_description: A #GtkNotebook with a little extra functionality.
 *
 * #GimpNotebook restores the GTK2 behavior of the scroll wheel scrolling
 * through the notebook tabs.
 **/


static gboolean    gimp_notebook_scrolled     (GtkWidget      *widget,
                                               GdkEventScroll *event);


G_DEFINE_TYPE (GimpNotebook, gimp_notebook, GTK_TYPE_NOTEBOOK)

#define parent_class gimp_notebook_parent_class


static void
gimp_notebook_class_init (GimpNotebookClass *klass)
{
}

static void
gimp_notebook_init (GimpNotebook *notebook)
{
  gtk_widget_add_events (GTK_WIDGET (notebook), GDK_SCROLL_MASK);

  g_signal_connect_object (notebook, "scroll-event",
                           G_CALLBACK (gimp_notebook_scrolled),
                           NULL, 0);
}

/* Restore GTK2 behavior of mouse-scrolling to switch between
 * notebook tabs. References Geany's notebook_tab_bar_click_cb ()
 * at https://github.com/geany/geany/blob/master/src/notebook.c
 */
static gboolean
gimp_notebook_scrolled (GtkWidget      *widget,
                        GdkEventScroll *event)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (widget);
  GtkWidget   *page     = NULL;

  page = gtk_notebook_get_nth_page (notebook,
                                    gtk_notebook_get_current_page (notebook));
  if (! page)
    return FALSE;

  switch (event->direction)
    {
    case GDK_SCROLL_RIGHT:
    case GDK_SCROLL_DOWN:
      gtk_notebook_next_page (notebook);
      break;

    case GDK_SCROLL_LEFT:
    case GDK_SCROLL_UP:
      gtk_notebook_prev_page (notebook);
      break;

    default:
      break;
    }

  return TRUE;
}

/**
 * gimp_notebook_new:
 *
 * Creates a new #GimpNotebook widget.
 *
 * Returns: A pointer to the new #GimpNotebook widget.
 **/
GtkWidget *
gimp_notebook_new (void)
{
  return g_object_new (GIMP_TYPE_NOTEBOOK, NULL);
}

/**
 * gimp_notebook_append_page
 *
 * Wrapper for gtk_notebook_append_page () that ensures
 * the @tag_label can also respond to scroll requests.
 *
 * Returns: The index (starting from 0) of the appended page
 * in the notebook, or -1 if function fails.
 **/
gint
gimp_notebook_append_page (GimpNotebook *notebook,
                           GtkWidget    *child,
                           GtkWidget    *tab_label)
{
  g_return_val_if_fail (GIMP_IS_NOTEBOOK (notebook), -1);

  if (tab_label)
    gtk_widget_add_events (tab_label, GDK_SCROLL_MASK);

  return gtk_notebook_append_page (GTK_NOTEBOOK (notebook), child, tab_label);
}
