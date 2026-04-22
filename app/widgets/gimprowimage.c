/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimprowimage.c
 * Copyright (C) 2026 Michael Natterer <mitch@gimp.org>
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

#include "core/gimpimage.h"

#include "gimpdnd.h"
#include "gimprowimage.h"


static void   gimp_row_image_constructed   (GObject      *object);

static void   gimp_row_image_set_viewable  (GimpRow      *row,
                                            GimpViewable *viewable);
static void   gimp_row_image_set_view_size (GimpRow      *row);


G_DEFINE_TYPE (GimpRowImage,
               gimp_row_image,
               GIMP_TYPE_ROW)

#define parent_class gimp_row_image_parent_class


static void
gimp_row_image_class_init (GimpRowImageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GimpRowClass *row_class    = GIMP_ROW_CLASS (klass);

  object_class->constructed = gimp_row_image_constructed;

  row_class->set_viewable   = gimp_row_image_set_viewable;
  row_class->set_view_size  = gimp_row_image_set_view_size;
}

static void
gimp_row_image_init (GimpRowImage *row)
{
}

static void
gimp_row_image_constructed (GObject *object)
{
  GtkWidget *view;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  view = _gimp_row_get_view (GIMP_ROW (object));

  if (view)
    {
      gint view_size = gimp_row_get_view_size (GIMP_ROW (object), NULL);

      gtk_widget_set_size_request (view, view_size, view_size);
    }
}

static void
gimp_row_image_set_viewable (GimpRow      *row,
                             GimpViewable *viewable)
{
  if (gimp_row_get_viewable (row))
    {
      gimp_dnd_xds_source_remove (GTK_WIDGET (row));
    }

  GIMP_ROW_CLASS (parent_class)->set_viewable (row, viewable);

  if (viewable)
    {
      gimp_dnd_xds_source_add (GTK_WIDGET (row),
                               (GimpDndDragViewableFunc)
                               gimp_dnd_get_drag_viewable,
                               NULL);
    }
}

static void
gimp_row_image_set_view_size (GimpRow *row)
{
  GtkWidget *view;

  GIMP_ROW_CLASS (parent_class)->set_view_size (row);

  view = _gimp_row_get_view (row);

  if (view)
    {
      gint view_size = gimp_row_get_view_size (row, NULL);

      gtk_widget_set_size_request (view, view_size, view_size);
    }
}
