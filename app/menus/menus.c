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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"

#include "file/file-utils.h"

#include "widgets/gimpitemfactory.h"
#include "widgets/gimpmenufactory.h"

#include "brushes-menu.h"
#include "buffers-menu.h"
#include "channels-menu.h"
#include "colormap-editor-menu.h"
#include "dialogs-menu.h"
#include "documents-menu.h"
#include "file-open-menu.h"
#include "file-save-menu.h"
#include "file-commands.h"
#include "gradient-editor-menu.h"
#include "gradients-menu.h"
#include "image-menu.h"
#include "images-menu.h"
#include "layers-menu.h"
#include "menus.h"
#include "palette-editor-menu.h"
#include "palettes-menu.h"
#include "paths-dialog.h"
#include "patterns-menu.h"
#include "qmask-menu.h"
#include "toolbox-menu.h"
#include "vectors-menu.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   menus_last_opened_update  (GimpContainer   *container,
                                         GimpImagefile   *unused,
                                         GimpItemFactory *item_factory);
static void   menus_last_opened_reorder (GimpContainer   *container,
                                         GimpImagefile   *unused1,
                                         gint             unused2,
                                         GimpItemFactory *item_factory);


/*  global variables  */

GimpMenuFactory *global_menu_factory = NULL;


/*  private variables  */

static gboolean menus_initialized = FALSE;


/*  public functions  */

void
menus_init (Gimp *gimp)
{
  gchar *filename;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (menus_initialized == FALSE);

  menus_initialized = TRUE;

  global_menu_factory = gimp_menu_factory_new (gimp);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Toolbox>", "toolbox",
                                   toolbox_menu_setup, NULL, FALSE,
                                   n_toolbox_menu_entries,
                                   toolbox_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Image>", "image",
                                   image_menu_setup, image_menu_update, FALSE,
                                   n_image_menu_entries,
                                   image_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Load>", "open",
                                   file_open_menu_setup, NULL, FALSE,
                                   n_file_open_menu_entries,
                                   file_open_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Save>", "save",
                                   file_save_menu_setup,
                                   file_save_menu_update, FALSE,
                                   n_file_save_menu_entries,
                                   file_save_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Layers>", "layers",
                                   NULL, layers_menu_update, TRUE,
                                   n_layers_menu_entries,
                                   layers_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Channels>", "channels",
                                   NULL, channels_menu_update, TRUE,
                                   n_channels_menu_entries,
                                   channels_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Vectors>", "vectors",
                                   NULL, vectors_menu_update, TRUE,
                                   n_vectors_menu_entries,
                                   vectors_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Paths>", "paths",
                                   NULL, NULL, FALSE,
                                   n_paths_menu_entries,
                                   paths_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Dialogs>", "dialogs",
                                   NULL, dialogs_menu_update, TRUE,
                                   n_dialogs_menu_entries,
                                   dialogs_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Brushes>", "brushes",
                                   NULL, brushes_menu_update, TRUE,
                                   n_brushes_menu_entries,
                                   brushes_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Patterns>", "patterns",
                                   NULL, patterns_menu_update, TRUE,
                                   n_patterns_menu_entries,
                                   patterns_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Gradients>", "gradients",
                                   NULL, gradients_menu_update, TRUE,
                                   n_gradients_menu_entries,
                                   gradients_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Palettes>", "palettes",
                                   NULL, palettes_menu_update, TRUE,
                                   n_palettes_menu_entries,
                                   palettes_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Buffers>", "buffers",
                                   NULL, buffers_menu_update, TRUE,
                                   n_buffers_menu_entries,
                                   buffers_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Documents>", "documents",
                                   NULL, documents_menu_update, TRUE,
                                   n_documents_menu_entries,
                                   documents_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<Images>", "images",
                                   NULL, images_menu_update, TRUE,
                                   n_images_menu_entries,
                                   images_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<GradientEditor>", "gradient_editor",
                                   NULL, gradient_editor_menu_update, TRUE,
                                   n_gradient_editor_menu_entries,
                                   gradient_editor_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<PaletteEditor>", "palette_editor",
                                   NULL, palette_editor_menu_update, TRUE,
                                   n_palette_editor_menu_entries,
                                   palette_editor_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<ColormapEditor>", "colormap_editor",
                                   NULL, colormap_editor_menu_update, TRUE,
                                   n_colormap_editor_menu_entries,
                                   colormap_editor_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory,
                                   "<QMask>", "qmask",
                                   NULL, qmask_menu_update, TRUE,
                                   n_qmask_menu_entries,
                                   qmask_menu_entries);

  filename = gimp_personal_rc_file ("menurc");
  gtk_accel_map_load (filename);
  g_free (filename);
}

void
menus_exit (Gimp *gimp)
{
  gchar *filename;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  filename = gimp_personal_rc_file ("menurc");
  gtk_accel_map_save (filename);
  g_free (filename);

  g_object_unref (global_menu_factory);
  global_menu_factory = NULL;
}

void
menus_last_opened_add (GimpItemFactory *item_factory,
                       Gimp            *gimp)
{
  GimpItemFactoryEntry *last_opened_entries;
  gint                  i;
  gint                  n = GIMP_GUI_CONFIG (gimp->config)->last_opened_size;

  last_opened_entries = g_new (GimpItemFactoryEntry, n);

  for (i = 0; i < n; i++)
    {
      last_opened_entries[i].entry.path = 
        g_strdup_printf ("/File/Open Recent/%02d", i + 1);

      if (i < 9)
        last_opened_entries[i].entry.accelerator =
          g_strdup_printf ("<control>%d", i + 1);
      else
        last_opened_entries[i].entry.accelerator = "";

      last_opened_entries[i].entry.callback        = file_last_opened_cmd_callback;
      last_opened_entries[i].entry.callback_action = i;
      last_opened_entries[i].entry.item_type       = "<StockItem>";
      last_opened_entries[i].entry.extra_data      = GTK_STOCK_OPEN;
      last_opened_entries[i].quark_string          = NULL;
      last_opened_entries[i].help_page             = "file/last_opened.html";
      last_opened_entries[i].description           = NULL;
    }

  gimp_item_factory_create_items (item_factory,
                                  n, last_opened_entries,
                                  gimp, 2, TRUE, FALSE);

  for (i = 0; i < n; i++)
    {
      gimp_item_factory_set_visible (GTK_ITEM_FACTORY (item_factory),
                                     last_opened_entries[i].entry.path,
                                     FALSE);
    }

  gimp_item_factory_set_sensitive (GTK_ITEM_FACTORY (item_factory),
                                   "/File/Open Recent/(None)",
                                   FALSE);

  for (i = 0; i < n; i++)
    {
      g_free (last_opened_entries[i].entry.path);

      if (i < 9)
        g_free (last_opened_entries[i].entry.accelerator);
    }

  g_free (last_opened_entries);

  g_signal_connect_object (gimp->documents, "add",
                           G_CALLBACK (menus_last_opened_update),
                           item_factory, 0);
  g_signal_connect_object (gimp->documents, "remove",
                           G_CALLBACK (menus_last_opened_update),
                           item_factory, 0);
  g_signal_connect_object (gimp->documents, "reorder",
                           G_CALLBACK (menus_last_opened_reorder),
                           item_factory, 0);

  menus_last_opened_update (gimp->documents, NULL, item_factory);
}

void
menus_filters_subdirs_to_top (GtkMenu *menu)
{
  GtkMenuItem *menu_item;
  GList       *list;
  gboolean     submenus_passed = FALSE;
  gint         pos;
  gint         items;

  pos   = 1;
  items = 0;

  for (list = GTK_MENU_SHELL (menu)->children; list; list = g_list_next (list))
    {
      menu_item = GTK_MENU_ITEM (list->data);
      items++;
      
      if (menu_item->submenu)
	{
	  if (submenus_passed)
	    {
	      menus_filters_subdirs_to_top (GTK_MENU (menu_item->submenu));
	      gtk_menu_reorder_child (menu, GTK_WIDGET (menu_item), pos++);
	    }
	}
      else
	{
	  submenus_passed = TRUE;
	}
    }

  if (pos > 1 && items > pos)
    {
      GtkWidget *separator;

      separator = gtk_menu_item_new ();
      gtk_menu_shell_insert (GTK_MENU_SHELL (menu), separator, pos);
      gtk_widget_show (separator);
    }
}


/*  private functions  */

static void
menus_last_opened_update (GimpContainer   *container,
                          GimpImagefile   *unused,
                          GimpItemFactory *item_factory)
{
  GtkWidget *widget;
  gint       num_documents;
  gint       i;
  gint       n = GIMP_GUI_CONFIG (item_factory->gimp->config)->last_opened_size;

  num_documents = gimp_container_num_children (container);

  gimp_item_factory_set_visible (GTK_ITEM_FACTORY (item_factory),
                                 "/File/Open Recent/(None)",
                                 num_documents == 0);

  for (i = 0; i < n; i++)
    {
      gchar *path_str;

      path_str = g_strdup_printf ("/File/Open Recent/%02d", i + 1);

      widget = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (item_factory),
                                            path_str);

      g_free (path_str);

      if (i < num_documents)
        {
          GimpImagefile *imagefile;

          imagefile = (GimpImagefile *)
            gimp_container_get_child_by_index (container, i);

          if (g_object_get_data (G_OBJECT (widget), "gimp-imagefile") !=
              (gpointer) imagefile)
            {
              const gchar *uri;
              gchar       *filename;
              gchar       *basename;

              uri = gimp_object_get_name (GIMP_OBJECT (imagefile));

              filename = file_utils_uri_to_utf8_filename (uri);
              basename = file_utils_uri_to_utf8_basename (uri);

              gtk_label_set_text (GTK_LABEL (GTK_BIN (widget)->child),
				  basename);
              gimp_help_set_help_data (widget, filename, NULL);

              g_free (filename);
              g_free (basename);

              g_object_set_data (G_OBJECT (widget),
				 "gimp-imagefile", imagefile);
              gtk_widget_show (widget);
            }
        }
      else
        {
          g_object_set_data (G_OBJECT (widget), "gimp-imagefile", NULL);
          gtk_widget_hide (widget);
        }
    }
}

static void
menus_last_opened_reorder (GimpContainer   *container,
                           GimpImagefile   *unused1,
                           gint             unused2,
                           GimpItemFactory *item_factory)
{
  menus_last_opened_update (container, unused1, item_factory);
}
