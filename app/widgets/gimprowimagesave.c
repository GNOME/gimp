/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimprowimagesave.c
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display/display-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpimagewindow.h"

#include "gimpaction.h"
#include "gimprowimagesave.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void   gimp_row_image_save_constructed  (GObject         *object);

static void   gimp_row_image_save_name_changed (GimpRow         *row);

static void   gimp_row_image_save_clicked      (GtkWidget       *button,
                                                GimpRow         *row);
static void   gimp_row_image_save_ext_clicked  (GtkWidget       *button,
                                                GdkModifierType  state,
                                                GimpRow         *row);


G_DEFINE_TYPE (GimpRowImageSave,
               gimp_row_image_save,
               GIMP_TYPE_ROW_IMAGE)

#define parent_class gimp_row_image_save_parent_class


static void
gimp_row_image_save_class_init (GimpRowImageSaveClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GimpRowClass *row_class    = GIMP_ROW_CLASS (klass);

  object_class->constructed = gimp_row_image_save_constructed;

  row_class->name_changed   = gimp_row_image_save_name_changed;
}

static void
gimp_row_image_save_init (GimpRowImageSave *row)
{
  GtkWidget *button;
  GtkWidget *image;
  gchar     *tip;

  button = gimp_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_box_pack_end (GTK_BOX (_gimp_row_get_box (GIMP_ROW (row))), button,
                    FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (_gimp_row_get_box (GIMP_ROW (row))),
                         button, 1);
  gtk_widget_set_visible (button, TRUE);

  tip = g_strconcat (_("Save this image"), "\n<b>",
                     gimp_get_mod_string (GDK_SHIFT_MASK),
                     "</b>  ", _("Save as"),
                     NULL);
  gtk_widget_set_tooltip_markup (button, tip);
  g_free (tip);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_row_image_save_clicked),
                    row);
  g_signal_connect (button, "extended-clicked",
                    G_CALLBACK (gimp_row_image_save_ext_clicked),
                    row);

  image = gtk_image_new_from_icon_name ("document-save",
                                        GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_visible (image, TRUE);
}

static void
gimp_row_image_save_constructed (GObject *object)
{
  GtkWidget *view;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  view = _gimp_row_get_view (GIMP_ROW (object));

  if (view)
    gimp_row_image_save_name_changed (GIMP_ROW (object));
}

static void
gimp_row_image_save_name_changed (GimpRow *row)
{
  GimpViewable *viewable;
  gchar        *desc;

  viewable = gimp_row_get_viewable (row);

  if (! viewable)
    {
      GIMP_ROW_CLASS (parent_class)->name_changed (row);

      return;
    }

  desc = gimp_viewable_get_description (viewable, NULL);

  if (gimp_image_is_export_dirty (GIMP_IMAGE (viewable)))
    {
      gtk_label_set_text (GTK_LABEL (_gimp_row_get_label (row)), desc);
    }
  else
    {
      GFile       *file;
      const gchar *filename;
      gchar       *escaped_desc;
      gchar       *escaped_filename;
      gchar       *exported;
      gchar       *markup;

      file = gimp_image_get_exported_file (GIMP_IMAGE (viewable));
      if (! file)
        file = gimp_image_get_imported_file (GIMP_IMAGE (viewable));

      filename = gimp_file_get_utf8_name (file);

      escaped_desc     = g_markup_escape_text (desc, -1);
      escaped_filename = g_markup_escape_text (filename, -1);

      exported = g_strdup_printf (_("Exported to %s"), escaped_filename);
      markup = g_strdup_printf ("%s\n<i>%s</i>", escaped_desc, exported);
      g_free (exported);

      g_free (escaped_desc);
      g_free (escaped_filename);

      gtk_label_set_markup (GTK_LABEL (_gimp_row_get_label (row)), markup);

      g_free (markup);
    }

  g_free (desc);
}

static void
gimp_row_image_save_clicked (GtkWidget *button,
                             GimpRow   *row)
{
  gimp_row_image_save_ext_clicked (button, 0, row);
}

static void
gimp_row_image_save_ext_clicked (GtkWidget       *button,
                                 GdkModifierType  state,
                                 GimpRow         *row)
{
  GimpImage *image = GIMP_IMAGE (gimp_row_get_viewable (row));
  GList     *list;

  for (list = gimp_get_display_iter (image->gimp);
       list;
       list = g_list_next (list))
    {
      GimpDisplay *display = list->data;

      if (gimp_display_get_image (display) == image)
        {
          GimpDisplayShell *shell  = gimp_display_get_shell (display);
          GimpImageWindow  *window = gimp_display_shell_get_window (shell);

          if (window)
            {
              GAction *action;

              gimp_display_shell_present (shell);
              /* Make sure the quit dialog kept keyboard focus when
               * the save dialog will exit.
               */
              gtk_window_present (GTK_WINDOW (gtk_widget_get_toplevel (button)));

              if (state & GDK_SHIFT_MASK)
                action = g_action_map_lookup_action (G_ACTION_MAP (image->gimp->app),
                                                     "file-save-as");
              else
                action = g_action_map_lookup_action (G_ACTION_MAP (image->gimp->app),
                                                     "file-save");

              g_return_if_fail (GIMP_IS_ACTION (action));
              gimp_action_activate (GIMP_ACTION (action));
            }

          break;
        }
    }
}
