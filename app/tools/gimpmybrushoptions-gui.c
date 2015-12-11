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

#ifdef HAVE_LIBMYPAINT

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"

#include "paint/gimpmybrushoptions.h"

#include "widgets/gimppropwidgets.h"

#include "gimpmybrushoptions-gui.h"
#include "gimppaintoptions-gui.h"

#include "gimp-intl.h"


static void
gimp_mybrush_options_load_brush (GFile        *file,
                                 GtkTreeModel *model)
{
  GtkListStore *store  = GTK_LIST_STORE (model);
  GtkTreeIter   iter   = { 0, };
  GdkPixbuf    *pixbuf = NULL;
  gchar        *filename;
  gchar        *basename;
  gchar        *preview_filename;

  filename = g_file_get_path (file);
  g_object_weak_ref (G_OBJECT (store), (GWeakNotify) g_free, filename);

  basename = g_strndup (filename, strlen (filename) - 4);
  preview_filename = g_strconcat (basename, "_prev.png", NULL);
  g_free (basename);

  pixbuf = gdk_pixbuf_new_from_file_at_size (preview_filename,
                                             48, 48, NULL);
  g_free (preview_filename);

  basename = g_file_get_basename (file);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      GIMP_INT_STORE_LABEL,     gimp_filename_to_utf8 (basename),
                      GIMP_INT_STORE_PIXBUF,    pixbuf,
                      GIMP_INT_STORE_USER_DATA, filename,
                      -1);

  g_free (basename);

  if (pixbuf)
    g_object_unref (pixbuf);
}

static void
gimp_mybrush_options_load_recursive (GFile        *dir,
                                     GtkTreeModel *model)
{
  GFileEnumerator *enumerator;

  enumerator = g_file_enumerate_children (dir,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                          G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL, NULL);

  if (enumerator)
    {
      GFileInfo *info;

      while ((info = g_file_enumerator_next_file (enumerator,
                                                  NULL, NULL)))
        {
          if (! g_file_info_get_is_hidden (info))
            {
              GFile *file = g_file_enumerator_get_child (enumerator, info);

              switch (g_file_info_get_file_type (info))
                {
                case G_FILE_TYPE_DIRECTORY:
                  gimp_mybrush_options_load_recursive (file, model);
                  break;

                case G_FILE_TYPE_REGULAR:
                  if (gimp_file_has_extension (file, ".myb"))
                    gimp_mybrush_options_load_brush (file, model);
                  break;

                default:
                  break;
                }

              g_object_unref (file);
            }

          g_object_unref (info);
        }

      g_object_unref (enumerator);
    }
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
  GList        *path;
  GList        *list;

  /* radius */
  scale = gimp_prop_spin_scale_new (config, "radius",
                                    _("Radius"),
                                    0.1, 1.0, 2);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* opaque */
  scale = gimp_prop_spin_scale_new (config, "opaque",
                                    _("Base Opacity"),
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

  path = gimp_config_path_expand_to_files (GIMP_CONTEXT (config)->gimp->config->mypaint_brush_path, NULL);

  for (list = path; list; list = g_list_next (list))
    {
      GFile *dir = list->data;

      gimp_mybrush_options_load_recursive (dir, model);
    }

  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_mybrush_options_brush_changed),
                    config);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), 0);

  return vbox;
}

#endif
