/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpimageview.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "images-commands.h"


/*  public functions */

void
images_raise_views_cmd_callback (GimpAction *action,
                                 GVariant   *value,
                                 gpointer    data)
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

      for (list = gimp_get_display_iter (image->gimp);
           list;
           list = g_list_next (list))
        {
          GimpDisplay *display = list->data;

          if (gimp_display_get_image (display) == image)
            gimp_display_shell_present (gimp_display_get_shell (display));
        }
    }
}

void
images_new_view_cmd_callback (GimpAction *action,
                              GVariant   *value,
                              gpointer    data)
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
      gimp_create_display (image->gimp, image, GIMP_UNIT_PIXEL, 1.0,
                           G_OBJECT (gtk_widget_get_screen (GTK_WIDGET (editor))),
                           gimp_widget_get_monitor (GTK_WIDGET (editor)));
    }
}

void
images_delete_image_cmd_callback (GimpAction *action,
                                  GVariant   *value,
                                  gpointer    data)
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
      if (gimp_image_get_display_count (image) == 0)
        g_object_unref (image);
    }
}
