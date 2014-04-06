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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "paint/gimpmybrushoptions.h"

#include "widgets/gimppropwidgets.h"

#include "gimpmybrushoptions-gui.h"
#include "gimppaintoptions-gui.h"

#include "gimp-intl.h"


static void
gimp_mybrush_options_load_brush (const GimpDatafileData *file_data,
                                 gpointer                user_data)
{
  if (gimp_datafiles_check_extension (file_data->basename, ".myb"))
    {
      GtkListStore *store  = user_data;
      GtkTreeIter   iter   = { 0, };
      GdkPixbuf    *pixbuf = NULL;
      gchar        *filename;
      gchar        *basename;
      gchar        *preview_filename;

      filename = g_strdup (file_data->filename);
      g_object_weak_ref (G_OBJECT (store), (GWeakNotify) g_free, filename);

      basename = g_strndup (filename, strlen (filename) - 4);
      preview_filename = g_strconcat (basename, "_prev.png", NULL);
      g_free (basename);

      pixbuf = gdk_pixbuf_new_from_file (preview_filename, NULL);
      g_free (preview_filename);

      if (pixbuf)
        {
          GdkPixbuf *scaled;
          gint       width  = gdk_pixbuf_get_width (pixbuf);
          gint       height = gdk_pixbuf_get_height (pixbuf);
          gdouble    factor = 48.0 / height;

          scaled = gdk_pixbuf_scale_simple (pixbuf,
                                            width * factor,
                                            height * factor,
                                            GDK_INTERP_NEAREST);

          g_object_unref (pixbuf);
          pixbuf = scaled;
        }

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          GIMP_INT_STORE_LABEL,     file_data->basename,
                          GIMP_INT_STORE_PIXBUF,    pixbuf,
                          GIMP_INT_STORE_USER_DATA, filename,
                          -1);

      if (pixbuf)
        g_object_unref (pixbuf);
    }
}

static void
gimp_mybrush_options_load_recursive (const GimpDatafileData *file_data,
                                     gpointer                user_data)
{
  gimp_datafiles_read_directories (file_data->filename,
                                   G_FILE_TEST_IS_REGULAR,
                                   gimp_mybrush_options_load_brush,
                                   user_data);
}

static void
gimp_mybrush_options_brush_changed (GtkComboBox *combo,
                                    GObject     *config)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (combo, &iter))
    {
      GtkTreeModel *model = gtk_combo_box_get_model (combo);
      const gchar  *brush;

      gtk_tree_model_get (model, &iter,
                          GIMP_INT_STORE_USER_DATA, &brush,
                          -1);

      if (brush)
        g_object_set (config,
                      "mybrush", brush,
                      NULL);
    }
}

GtkWidget *
gimp_mybrush_options_gui (GimpToolOptions *tool_options)
{
  GObject      *config = G_OBJECT (tool_options);
  GtkWidget    *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget    *scale;
  GtkWidget    *combo;
  GtkTreeModel *model;

  /* radius */
  scale = gimp_prop_spin_scale_new (config, "radius",
                                    _("Radius"),
                                    0.1, 1.0, 2);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* hardness */
  scale = gimp_prop_spin_scale_new (config, "hardness",
                                    _("Hardness"),
                                    0.1, 1.0, 2);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* brushes */
  combo = g_object_new (GIMP_TYPE_INT_COMBO_BOX,
                        "label",     _("Brush"),
                        "ellipsize", PANGO_ELLIPSIZE_END,
                        NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  gimp_datafiles_read_directories ("/usr/share/mypaint/brushes",
                                   G_FILE_TEST_IS_DIR,
                                   gimp_mybrush_options_load_recursive,
                                   model);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_mybrush_options_brush_changed),
                    config);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), 0);

  return vbox;
}
