/* The GIMP -- an image manipulation program
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

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"

#include "widgets/gimpcolormapeditor.h"

#include "actions.h"
#include "colormap-editor-commands.h"


/*  public functions  */

void
colormap_editor_edit_color_cmd_callback (GtkAction *action,
                                         gpointer   data)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (data);

  if (GTK_WIDGET_SENSITIVE (editor->edit_button))
    gtk_button_clicked (GTK_BUTTON (editor->edit_button));
}

void
colormap_editor_add_color_cmd_callback (GtkAction *action,
                                        gint       value,
                                        gpointer   data)
{
  GimpContext *context;
  GimpImage   *gimage;
  return_if_no_context (context, data);
  return_if_no_image (gimage, data);

  if (gimage->num_cols < 256)
    {
      GimpRGB color;

      if (value)
        gimp_context_get_background (context, &color);
      else
        gimp_context_get_foreground (context, &color);

      gimp_image_add_colormap_entry (gimage, &color);
      gimp_image_flush (gimage);
    }
}
