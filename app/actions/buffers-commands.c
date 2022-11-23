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
#include "core/ligma-edit.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"

#include "widgets/ligmacontainereditor.h"
#include "widgets/ligmacontainerview.h"
#include "widgets/ligmacontainerview-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-transform.h"

#include "buffers-commands.h"

#include "ligma-intl.h"


/*  public functions */

void
buffers_paste_cmd_callback (LigmaAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContainer       *container;
  LigmaContext         *context;
  LigmaBuffer          *buffer;
  LigmaPasteType        paste_type = (LigmaPasteType) g_variant_get_int32 (value);

  container = ligma_container_view_get_container (editor->view);
  context   = ligma_container_view_get_context (editor->view);

  buffer = ligma_context_get_buffer (context);

  if (buffer && ligma_container_have (container, LIGMA_OBJECT (buffer)))
    {
      LigmaDisplay *display = ligma_context_get_display (context);
      LigmaImage   *image   = NULL;
      gint         x       = -1;
      gint         y       = -1;
      gint         width   = -1;
      gint         height  = -1;

      if (display)
        {
          LigmaDisplayShell *shell = ligma_display_get_shell (display);

          ligma_display_shell_untransform_viewport (
            shell,
            ! ligma_display_shell_get_infinite_canvas (shell),
            &x, &y, &width, &height);

          image = ligma_display_get_image (display);
        }
      else
        {
          image = ligma_context_get_image (context);
        }

      if (image)
        {
          GList *drawables = ligma_image_get_selected_drawables (image);

          g_list_free (ligma_edit_paste (image,
                                        g_list_length (drawables) == 1 ? drawables->data : NULL,
                                        LIGMA_OBJECT (buffer), paste_type,
                                        context, FALSE,
                                        x, y, width, height));

          ligma_image_flush (image);
          g_list_free (drawables);
        }
    }
}

void
buffers_paste_as_new_image_cmd_callback (LigmaAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);
  LigmaContainer       *container;
  LigmaContext         *context;
  LigmaBuffer          *buffer;

  container = ligma_container_view_get_container (editor->view);
  context   = ligma_container_view_get_context (editor->view);

  buffer = ligma_context_get_buffer (context);

  if (buffer && ligma_container_have (container, LIGMA_OBJECT (buffer)))
    {
      GtkWidget *widget = GTK_WIDGET (editor);
      LigmaImage *new_image;

      new_image = ligma_edit_paste_as_new_image (context->ligma,
                                                LIGMA_OBJECT (buffer));
      ligma_create_display (context->ligma, new_image,
                           LIGMA_UNIT_PIXEL, 1.0,
                           G_OBJECT (ligma_widget_get_monitor (widget)));
      g_object_unref (new_image);
    }
}

void
buffers_delete_cmd_callback (LigmaAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  LigmaContainerEditor *editor = LIGMA_CONTAINER_EDITOR (data);

  ligma_container_view_remove_active (editor->view);
}
