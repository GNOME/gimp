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
#include "core/gimp-edit.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpcontainerview-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "buffers-commands.h"

#include "gimp-intl.h"


/*  public functions */

void
buffers_paste_cmd_callback (GimpAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContainer       *container;
  GimpContext         *context;
  GimpBuffer          *buffer;
  GimpPasteType        paste_type = (GimpPasteType) g_variant_get_int32 (value);

  container = gimp_container_view_get_container (editor->view);
  context   = gimp_container_view_get_context (editor->view);

  buffer = gimp_context_get_buffer (context);

  if (buffer && gimp_container_have (container, GIMP_OBJECT (buffer)))
    {
      GimpDisplay *display = gimp_context_get_display (context);
      GimpImage   *image   = NULL;
      gint         x       = -1;
      gint         y       = -1;
      gint         width   = -1;
      gint         height  = -1;

      if (display)
        {
          GimpDisplayShell *shell = gimp_display_get_shell (display);

          gimp_display_shell_untransform_viewport (
            shell,
            ! gimp_display_shell_get_infinite_canvas (shell),
            &x, &y, &width, &height);

          image = gimp_display_get_image (display);
        }
      else
        {
          image = gimp_context_get_image (context);
        }

      if (image)
        {
          gimp_edit_paste (image, gimp_image_get_active_drawable (image),
                           GIMP_OBJECT (buffer), paste_type,
                           x, y, width, height);

          gimp_image_flush (image);
        }
    }
}

void
buffers_paste_as_new_image_cmd_callback (GimpAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);
  GimpContainer       *container;
  GimpContext         *context;
  GimpBuffer          *buffer;

  container = gimp_container_view_get_container (editor->view);
  context   = gimp_container_view_get_context (editor->view);

  buffer = gimp_context_get_buffer (context);

  if (buffer && gimp_container_have (container, GIMP_OBJECT (buffer)))
    {
      GtkWidget *widget = GTK_WIDGET (editor);
      GimpImage *new_image;

      new_image = gimp_edit_paste_as_new_image (context->gimp,
                                                GIMP_OBJECT (buffer));
      gimp_create_display (context->gimp, new_image,
                           GIMP_UNIT_PIXEL, 1.0,
                           G_OBJECT (gtk_widget_get_screen (widget)),
                           gimp_widget_get_monitor (widget));
      g_object_unref (new_image);
    }
}

void
buffers_delete_cmd_callback (GimpAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);

  gimp_container_view_remove_active (editor->view);
}
