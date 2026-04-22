/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagesaveview.c
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

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpviewable.h"

#include "gimpimagesaveview.h"
#include "gimprow-utils.h"
#include "gimpviewrenderer.h"


G_DEFINE_TYPE (GimpImageSaveView,
               gimp_image_save_view,
               GIMP_TYPE_CONTAINER_LIST_VIEW)

#define parent_class gimp_image_save_view_parent_class


static void
gimp_image_save_view_class_init (GimpImageSaveViewClass *klass)
{
  GimpContainerListViewClass *list_class =
    GIMP_CONTAINER_LIST_VIEW_CLASS (klass);

  list_class->create_row_func = gimp_row_create_for_save_view;
}

static void
gimp_image_save_view_init (GimpImageSaveView *view)
{
}

GtkWidget *
gimp_image_save_view_new (GimpContainer *container,
                          GimpContext   *context,
                          gint           view_size,
                          gint           view_border_width)
{
  g_return_val_if_fail (container == NULL || GIMP_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  return g_object_new (GIMP_TYPE_IMAGE_SAVE_VIEW,
                       "view-size",         view_size,
                       "view-border-width", view_border_width,
                       "context",           context,
                       "container",         container,
                       NULL);
}
