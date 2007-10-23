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
#include "core/gimp-edit.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "widgets/gimpbufferview.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpcontainerview-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "buffers-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   buffers_paste (GimpBufferView *view,
                             gboolean        paste_into);


/*  public functionss */

void
buffers_paste_cmd_callback (GtkAction *action,
                            gpointer   data)
{
  buffers_paste (GIMP_BUFFER_VIEW (data), FALSE);
}

void
buffers_paste_into_cmd_callback (GtkAction *action,
                                 gpointer   data)
{
  buffers_paste (GIMP_BUFFER_VIEW (data), TRUE);
}

void
buffers_paste_as_new_cmd_callback (GtkAction *action,
                                   gpointer   data)
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
      GimpImage *image = gimp_context_get_image (context);

      if (image)
        {
          GimpImage *new_image;

          new_image = gimp_edit_paste_as_new (image->gimp, image, buffer);

          if (new_image)
            {
              gimp_create_display (image->gimp, new_image,
                                   GIMP_UNIT_PIXEL, 1.0);
              g_object_unref (new_image);
            }
        }
    }
}

void
buffers_delete_cmd_callback (GtkAction *action,
                             gpointer   data)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (data);

  gimp_container_view_remove_active (editor->view);
}


/*  private functions  */

static void
buffers_paste (GimpBufferView *view,
               gboolean        paste_into)
{
  GimpContainerEditor *editor = GIMP_CONTAINER_EDITOR (view);
  GimpContainer       *container;
  GimpContext         *context;
  GimpBuffer          *buffer;

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
          GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (display->shell);

          gimp_display_shell_untransform_viewport (shell,
                                                   &x, &y, &width, &height);

          image = display->image;
        }
      else
        {
          image = gimp_context_get_image (context);
        }

      if (image)
        {
          gimp_edit_paste (image, gimp_image_get_active_drawable (image),
                           buffer, paste_into, x, y, width, height);

          gimp_image_flush (image);
        }
    }
}
