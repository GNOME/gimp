/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpextensiondetails.c
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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

#include "core/gimpextension.h"
#include "core/gimpextensionmanager.h"

#include "gimpextensiondetails.h"

#include "gimp-intl.h"

struct _GimpExtensionDetailsPrivate
{
  GimpExtension *extension;

  gint           screenshot_width;
  gint           screenshot_height;
};

static void gimp_extension_details_size_allocate (GimpExtensionDetails *details,
                                                  GdkRectangle         *allocation,
                                                  GtkGrid              *data);

G_DEFINE_TYPE_WITH_PRIVATE (GimpExtensionDetails, gimp_extension_details,
                            GTK_TYPE_FRAME)

#define parent_class gimp_extension_details_parent_class


static void
gimp_extension_details_class_init (GimpExtensionDetailsClass *klass)
{
}

static void
gimp_extension_details_init (GimpExtensionDetails *details)
{
  gtk_frame_set_label_align (GTK_FRAME (details), 0.5, 1.0);
  details->p = gimp_extension_details_get_instance_private (details);
}

GtkWidget *
gimp_extension_details_new (void)
{
  return g_object_new (GIMP_TYPE_EXTENSION_DETAILS, NULL);
}

void
gimp_extension_details_set (GimpExtensionDetails *details,
                            GimpExtension        *extension)
{
  GtkWidget     *grid;
  GtkWidget     *widget;
  GtkTextBuffer *buffer;

  g_return_if_fail (GIMP_IS_EXTENSION (extension));

  if (details->p->extension)
    g_object_unref (details->p->extension);
  details->p->extension  = g_object_ref (extension);
  details->p->screenshot_width = details->p->screenshot_height = 0;

  gtk_container_foreach (GTK_CONTAINER (details),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  gtk_frame_set_label (GTK_FRAME (details),
                       extension ? gimp_extension_get_name (extension) : NULL);

  grid = gtk_grid_new ();
  gtk_grid_set_column_homogeneous (GTK_GRID (grid), FALSE);
  gtk_grid_set_row_homogeneous (GTK_GRID (grid), FALSE);
  gtk_container_add (GTK_CONTAINER (details), grid);
  gtk_widget_show (grid);

  if (extension)
    {
      gchar       *desc;
      GtkTextIter  iter;

      /* Screenshots. */
      if (gtk_widget_get_realized (GTK_WIDGET (details)))
        {
          GtkAllocation allocation;
          gint          baseline   = 0;

          gtk_widget_get_allocated_size (GTK_WIDGET (details),
                                         &allocation, &baseline);
          gimp_extension_details_size_allocate (details, &allocation, GTK_GRID (grid));
        }
      g_signal_connect (details, "size-allocate",
                        G_CALLBACK (gimp_extension_details_size_allocate),
                        grid);

      /* Description. */
      desc = gimp_extension_get_markup_description (extension);
      widget = gtk_text_view_new ();
      gtk_widget_set_vexpand (widget, TRUE);
      gtk_widget_set_hexpand (widget, TRUE);
      gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (widget), GTK_WRAP_WORD_CHAR);
      gtk_text_view_set_editable (GTK_TEXT_VIEW (widget), FALSE);
      gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (widget), FALSE);
      gtk_text_view_set_justification (GTK_TEXT_VIEW (widget), GTK_JUSTIFY_FILL);
      gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 3, 3);
      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
      gtk_text_buffer_get_start_iter (buffer, &iter);
      if (desc)
        {
          gtk_text_buffer_insert_markup (buffer, &iter, desc, -1);
          g_free (desc);
        }
      gtk_widget_show (widget);
    }
}

/* Private functions. */

static void
gimp_extension_details_size_allocate (GimpExtensionDetails *details,
                                      GdkRectangle         *allocation,
                                      GtkGrid              *grid)
{
  GtkWidget *widget;

  if (allocation->width > 0 && allocation->height > 0 &&
      allocation->width * 2 / 3 != details->p->screenshot_width &&
      allocation->height * 2 / 3 != details->p->screenshot_height)
    {
      GdkPixbuf *pixbuf;

      details->p->screenshot_width  = allocation->width * 2 / 3;
      details->p->screenshot_height = allocation->height * 2 / 3;

      pixbuf = gimp_extension_get_screenshot (details->p->extension,
                                              details->p->screenshot_width, details->p->screenshot_height,
                                              NULL);
      if (pixbuf)
        {
          widget = gtk_image_new_from_pixbuf (pixbuf);
          gtk_widget_set_vexpand (widget, FALSE);
          gtk_widget_set_hexpand (widget, FALSE);
          gtk_grid_attach (grid, widget, 1, 0, 1, 1);
          gtk_widget_show (widget);

          g_object_unref (pixbuf);
        }
    }
}
