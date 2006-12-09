/* GIMP - The GNU Image Manipulation Program
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

#include <gtk/gtk.h>

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpimageview.h"

#include "display/gimpdisplay.h"

#include "images-commands.h"


/*  public functionss */

void
images_raise_views_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContainer       *container;
  GimpContext         *context;
  GimpImage           *image;

  container = gimp_container_view_get_container (editor->view);
  context   = gimp_container_view_get_context (editor->view);

  image = gimp_context_get_image (context);

  if (image && gimp_container_have (container, GIMP_OBJECT (image)))
    {
      GList *list;

      for (list = GIMP_LIST (image->gimp->displays)->list;
           list;
           list = g_list_next (list))
        {
          GimpDisplay *display = list->data;

          if (display->image == image)
            gtk_window_present (GTK_WINDOW (display->shell));
        }
    }
}

void
images_new_view_cmd_callback (GtkAction *action,
                              gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContainer       *container;
  GimpContext         *context;
  GimpImage           *image;

  container = gimp_container_view_get_container (editor->view);
  context   = gimp_container_view_get_context (editor->view);

  image = gimp_context_get_image (context);

  if (image && gimp_container_have (container, GIMP_OBJECT (image)))
    {
      gimp_create_display (image->gimp, image, GIMP_UNIT_PIXEL, 1.0);
    }
}

void
images_delete_image_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContainer       *container;
  GimpContext         *context;
  GimpImage           *image;

  container = gimp_container_view_get_container (editor->view);
  context   = gimp_container_view_get_context (editor->view);

  image = gimp_context_get_image (context);

  if (image && gimp_container_have (container, GIMP_OBJECT (image)))
    {
      if (image->disp_count == 0)
        g_object_unref (image);
    }
}
