/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"

#include "widgets/ligmacontainerview.h"
#include "widgets/ligmaimageview.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"

#include "images-commands.h"


/*  public functions */

void
images_raise_views_cmd_callback (LigmaAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContainer       *container;
  LigmaContext         *context;
  LigmaImage           *image;

  container = ligma_container_view_get_container (editor->view);
  context   = ligma_container_view_get_context (editor->view);

  image = ligma_context_get_image (context);

  if (image && ligma_container_have (container, LIGMA_OBJECT (image)))
    {
      GList *list;

      for (list = ligma_get_display_iter (image->ligma);
           list;
           list = g_list_next (list))
        {
          LigmaDisplay *display = list->data;

          if (ligma_display_get_image (display) == image)
            ligma_display_shell_present (ligma_display_get_shell (display));
        }
    }
}

void
images_new_view_cmd_callback (LigmaAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContainer       *container;
  LigmaContext         *context;
  LigmaImage           *image;

  container = ligma_container_view_get_container (editor->view);
  context   = ligma_container_view_get_context (editor->view);

  image = ligma_context_get_image (context);

  if (image && ligma_container_have (container, LIGMA_OBJECT (image)))
    {
      ligma_create_display (image->ligma, image, LIGMA_UNIT_PIXEL, 1.0,
                           G_OBJECT (ligma_widget_get_monitor (GTK_WIDGET (editor))));
    }
}

void
images_delete_image_cmd_callback (LigmaAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContainer       *container;
  LigmaContext         *context;
  LigmaImage           *image;

  container = ligma_container_view_get_container (editor->view);
  context   = ligma_container_view_get_context (editor->view);

  image = ligma_context_get_image (context);

  if (image && ligma_container_have (container, LIGMA_OBJECT (image)))
    {
      if (ligma_image_get_display_count (image) == 0)
        g_object_unref (image);
    }
}
