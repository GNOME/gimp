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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-colormap.h"

#include "widgets/gimpcolordialog.h"
#include "widgets/gimpcolormapeditor.h"
#include "widgets/gimpdialogfactory.h"

#include "actions.h"
#include "colormap-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   colormap_edit_color_update (GimpColorDialog      *dialog,
                                          const GimpRGB        *color,
                                          GimpColorDialogState  state,
                                          GimpColormapEditor   *editor);


/*  public functions  */

void
colormap_edit_color_cmd_callback (GtkAction *action,
                                  gpointer   data)
{
  GimpColormapEditor *editor;
  GimpImage          *image;
  const guchar       *colormap;
  GimpRGB             color;
  gchar              *desc;
  return_if_no_image (image, data);

  editor = GIMP_COLORMAP_EDITOR (data);

  colormap = gimp_image_get_colormap (image);

  gimp_rgba_set_uchar (&color,
                       colormap[editor->col_index * 3],
                       colormap[editor->col_index * 3 + 1],
                       colormap[editor->col_index * 3 + 2],
                       OPAQUE_OPACITY);

  desc = g_strdup_printf (_("Edit colormap entry #%d"), editor->col_index);

  if (! editor->color_dialog)
    {
      editor->color_dialog =
        gimp_color_dialog_new (GIMP_VIEWABLE (image),
                               action_data_get_context (data),
                               _("Edit Colormap Entry"),
                               GIMP_STOCK_COLORMAP,
                               desc,
                               GTK_WIDGET (editor),
                               gimp_dialog_factory_get_singleton (),
                               "gimp-colormap-editor-color-dialog",
                               (const GimpRGB *) &color,
                               FALSE, FALSE);

      g_signal_connect (editor->color_dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &editor->color_dialog);

      g_signal_connect (editor->color_dialog, "update",
                        G_CALLBACK (colormap_edit_color_update),
                        editor);
    }
  else
    {
      gimp_viewable_dialog_set_viewable (GIMP_VIEWABLE_DIALOG (editor->color_dialog),
                                         GIMP_VIEWABLE (image),
                                         action_data_get_context (data));
      g_object_set (editor->color_dialog, "description", desc, NULL);
      gimp_color_dialog_set_color (GIMP_COLOR_DIALOG (editor->color_dialog),
                                   &color);
    }

  g_free (desc);

  gtk_window_present (GTK_WINDOW (editor->color_dialog));
}

void
colormap_add_color_cmd_callback (GtkAction *action,
                                 gint       value,
                                 gpointer   data)
{
  GimpContext *context;
  GimpImage   *image;
  return_if_no_context (context, data);
  return_if_no_image (image, data);

  if (gimp_image_get_colormap_size (image) < 256)
    {
      GimpRGB color;

      if (value)
        gimp_context_get_background (context, &color);
      else
        gimp_context_get_foreground (context, &color);

      gimp_image_add_colormap_entry (image, &color);
      gimp_image_flush (image);
    }
}


/*  private functions  */

static void
colormap_edit_color_update (GimpColorDialog      *dialog,
                            const GimpRGB        *color,
                            GimpColorDialogState  state,
                            GimpColormapEditor   *editor)
{
  GimpImage *image = GIMP_IMAGE_EDITOR (editor)->image;

  switch (state)
    {
    case GIMP_COLOR_DIALOG_UPDATE:
      break;

    case GIMP_COLOR_DIALOG_OK:
      gimp_image_set_colormap_entry (image, editor->col_index, color, TRUE);
      gimp_image_flush (image);
      /* Fall through */

    case GIMP_COLOR_DIALOG_CANCEL:
      gtk_widget_hide (editor->color_dialog);
      break;
    }
}
