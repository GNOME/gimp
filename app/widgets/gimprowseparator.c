/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimprowseparator.c
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "gimprowseparator.h"


struct _GimpRowSeparator
{
  GimpRow  parent_instance;
};


static void   gimp_row_separator_constructed (GObject *object);


G_DEFINE_TYPE (GimpRowSeparator,
               gimp_row_separator,
               GIMP_TYPE_ROW)

#define parent_class gimp_row_separator_parent_class


static void
gimp_row_separator_class_init (GimpRowSeparatorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = gimp_row_separator_constructed;
}

static void
gimp_row_separator_init (GimpRowSeparator *row)
{
  gtk_list_box_row_set_selectable (GTK_LIST_BOX_ROW (row), FALSE);
}

static void
gimp_row_separator_constructed (GObject *object)
{
  GimpRow   *row = GIMP_ROW (object);
  GtkWidget *sep;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_widget_set_visible (_gimp_row_get_icon  (row), FALSE);
  gtk_widget_set_visible (_gimp_row_get_view  (row), FALSE);
  gtk_widget_set_visible (_gimp_row_get_label (row), FALSE);

  sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_container_add (GTK_CONTAINER (_gimp_row_get_box (row)), sep);
  gtk_widget_show (sep);
}
