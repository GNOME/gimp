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

#include <string.h>

#include <gtk/gtk.h>

#include "gimpprocbox.h"


/*  public functions  */

GtkWidget *
gimp_proc_box_new (void)
{
  GtkWidget *scrolled_window;
  GtkWidget *vbox;

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_ALWAYS);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window),
                                         vbox);
  gtk_widget_show (vbox);

  g_object_set_data (G_OBJECT (scrolled_window), "vbox", vbox);

  return scrolled_window;
}

void
gimp_proc_box_set_widget (GtkWidget *proc_box,
                          GtkWidget *widget)
{
  GtkWidget *vbox;
  GtkWidget *child;

  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (proc_box));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  vbox = g_object_get_data (G_OBJECT (proc_box), "vbox");

  g_return_if_fail (GTK_IS_VBOX (vbox));

  child = g_object_get_data (G_OBJECT (vbox), "child");

  if (child)
    gtk_container_remove (GTK_CONTAINER (vbox), child);

  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  g_object_set_data (G_OBJECT (vbox), "child", widget);
}

void
gimp_proc_box_show_message (GtkWidget   *proc_box,
                            const gchar *message)
{
  GtkWidget *vbox;
  GtkWidget *child;

  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (proc_box));
  g_return_if_fail (message != NULL);

  vbox = g_object_get_data (G_OBJECT (proc_box), "vbox");

  g_return_if_fail (GTK_IS_VBOX (vbox));

  child = g_object_get_data (G_OBJECT (vbox), "child");

  if (GTK_IS_LABEL (child))
    {
      gtk_label_set_text (GTK_LABEL (child), message);
    }
  else
    {
      if (child)
        gtk_container_remove (GTK_CONTAINER (vbox), child);

      child = gtk_label_new (message);
      gtk_box_pack_start (GTK_BOX (vbox), child, FALSE, FALSE, 0);
      gtk_widget_show (child);

      g_object_set_data (G_OBJECT (vbox), "child", child);
    }

  while (gtk_events_pending ())
    gtk_main_iteration ();
}
