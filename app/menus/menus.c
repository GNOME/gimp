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

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpmenufactory.h"

#include "actions/actions.h"
#include "actions/file-commands.h"

#include "brushes-menu.h"
#include "buffers-menu.h"
#include "channels-menu.h"
#include "colormap-editor-menu.h"
#include "dialogs-menu.h"
#include "documents-menu.h"
#include "error-console-menu.h"
#include "file-open-menu.h"
#include "file-save-menu.h"
#include "fonts-menu.h"
#include "gradient-editor-menu.h"
#include "gradients-menu.h"
#include "image-menu.h"
#include "images-menu.h"
#include "layers-menu.h"
#include "menus.h"
#include "palette-editor-menu.h"
#include "palettes-menu.h"
#include "patterns-menu.h"
#include "qmask-menu.h"
#include "templates-menu.h"
#include "tool-options-menu.h"
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

static void   menu_can_change_accels    (GimpGuiConfig   *config);


/*  global variables  */

GimpMenuFactory *global_menu_factory = NULL;


/*  private variables  */

static gboolean menus_initialized = FALSE;


/*  public functions  */

void
menus_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (menus_initialized == FALSE);

  menus_initialized = TRUE;

  actions_init (gimp);

  /* We need to make sure the property is installed before using it */
  g_type_class_ref (GTK_TYPE_MENU);

  menu_can_change_accels (GIMP_GUI_CONFIG (gimp->config));

  g_signal_connect (gimp->config, "notify::can-change-accels",
                    G_CALLBACK (menu_can_change_accels), NULL);

  global_menu_factory = gimp_menu_factory_new (gimp, global_action_factory);

  gimp_menu_factory_menu_register (global_menu_factory, "<Toolbox>",
                                   _("Toolbox Menu"),
                                   GIMP_HELP_TOOLBOX,
                                   toolbox_menu_setup, NULL, FALSE,
                                   n_toolbox_menu_entries,
                                   toolbox_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory, "<Image>",
                                   _("Image Menu"),
                                   GIMP_HELP_IMAGE_WINDOW,
                                   image_menu_setup, image_menu_update, FALSE,
                                   n_image_menu_entries,
                                   image_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<Image>",
                                      "file",
                                      "edit",
                                      "select",
                                      "view",
                                      "image",
                                      "drawable",
                                      "layers",
                                      "vectors",
                                      "tools",
                                      "dialogs",
                                      "plug-in",
                                      NULL,
                                      "/toolbox-menubar", "toolbox-menu.xml",
                                      "/image-menubar",   "image-menu.xml",
                                      "/qmask-popup",     "qmask-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<Load>",
                                   _("Open Menu"),
                                   GIMP_HELP_FILE_OPEN,
                                   file_open_menu_setup, NULL, FALSE,
                                   n_file_open_menu_entries,
                                   file_open_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory, "<Save>",
                                   _("Save Menu"),
                                   GIMP_HELP_FILE_SAVE,
                                   file_save_menu_setup,
                                   file_save_menu_update, FALSE,
                                   n_file_save_menu_entries,
                                   file_save_menu_entries);

  gimp_menu_factory_menu_register (global_menu_factory, "<Layers>",
                                   _("Layers Menu"),
                                   GIMP_HELP_LAYER_DIALOG,
                                   NULL, layers_menu_update, TRUE,
                                   n_layers_menu_entries,
                                   layers_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<Layers>",
                                      "layers",
                                      NULL,
                                      "/layers-popup", "layers-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<Channels>",
                                   _("Channels Menu"),
                                   GIMP_HELP_CHANNEL_DIALOG,
                                   NULL, channels_menu_update, TRUE,
                                   n_channels_menu_entries,
                                   channels_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<Channels>",
                                      "channels",
                                      NULL,
                                      "/channels-popup", "channels-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<Vectors>",
                                   _("Paths Menu"),
                                   GIMP_HELP_PATH_DIALOG,
                                   NULL, vectors_menu_update, TRUE,
                                   n_vectors_menu_entries,
                                   vectors_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<Vectors>",
                                      "vectors",
                                      NULL,
                                      "/vectors-popup", "vectors-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<Dialogs>",
                                   _("Dialogs Menu"),
                                   GIMP_HELP_DOCK,
                                   NULL, dialogs_menu_update, TRUE,
                                   n_dialogs_menu_entries,
                                   dialogs_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<Dialogs>",
                                      "dockable",
                                      NULL,
                                      "/dockable-popup", "dockable-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<Brushes>",
                                   _("Brushes Menu"),
                                   GIMP_HELP_BRUSH_DIALOG,
                                   NULL, brushes_menu_update, TRUE,
                                   n_brushes_menu_entries,
                                   brushes_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<Brushes>",
                                      "brushes",
                                      NULL,
                                      "/brushes-popup", "brushes-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<Patterns>",
                                   _("Patterns Menu"),
                                   GIMP_HELP_PATTERN_DIALOG,
                                   NULL, patterns_menu_update, TRUE,
                                   n_patterns_menu_entries,
                                   patterns_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<Patterns>",
                                      "patterns",
                                      NULL,
                                      "/patterns-popup", "patterns-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<Gradients>",
                                   _("Gradients Menu"),
                                   GIMP_HELP_GRADIENT_DIALOG,
                                   NULL, gradients_menu_update, TRUE,
                                   n_gradients_menu_entries,
                                   gradients_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<Gradients>",
                                      "gradients",
                                      NULL,
                                      "/gradients-popup", "gradients-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<Palettes>",
                                   _("Palettes Menu"),
                                   GIMP_HELP_PALETTE_DIALOG,
                                   NULL, palettes_menu_update, TRUE,
                                   n_palettes_menu_entries,
                                   palettes_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<Palettes>",
                                      "palettes",
                                      NULL,
                                      "/palettes-popup", "palettes-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<Fonts>",
                                   _("Fonts Menu"),
                                   GIMP_HELP_FONT_DIALOG,
                                   NULL, fonts_menu_update, TRUE,
                                   n_fonts_menu_entries,
                                   fonts_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<Fonts>",
                                      "fonts",
                                      NULL,
                                      "/fonts-popup", "fonts-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<Buffers>",
                                   _("Buffers Menu"),
                                   GIMP_HELP_BUFFER_DIALOG,
                                   NULL, buffers_menu_update, TRUE,
                                   n_buffers_menu_entries,
                                   buffers_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<Buffers>",
                                      "buffers",
                                      NULL,
                                      "/buffers-popup", "buffers-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<Documents>",
                                   _("Documents Menu"),
                                   GIMP_HELP_DOCUMENT_DIALOG,
                                   NULL, documents_menu_update, TRUE,
                                   n_documents_menu_entries,
                                   documents_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<Documents>",
                                      "documents",
                                      NULL,
                                      "/documents-popup", "documents-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<Templates>",
                                   _("Templates Menu"),
                                   GIMP_HELP_TEMPLATE_DIALOG,
                                   NULL, templates_menu_update, TRUE,
                                   n_templates_menu_entries,
                                   templates_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<Templates>",
                                      "templates",
                                      NULL,
                                      "/templates-popup", "templates-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<Images>",
                                   _("Images Menu"),
                                   GIMP_HELP_IMAGE_DIALOG,
                                   NULL, images_menu_update, TRUE,
                                   n_images_menu_entries,
                                   images_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<Images>",
                                      "images",
                                      NULL,
                                      "/images-popup", "images-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<GradientEditor>",
                                   _("Gradient Editor Menu"),
                                   GIMP_HELP_GRADIENT_EDITOR_DIALOG,
                                   NULL, gradient_editor_menu_update, TRUE,
                                   n_gradient_editor_menu_entries,
                                   gradient_editor_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<GradientEditor>",
                                      "gradient-editor",
                                      NULL,
                                      "/gradient-editor-popup",
                                      "gradient-editor-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<PaletteEditor>",
                                   _("Palette Editor Menu"),
                                   GIMP_HELP_PALETTE_EDITOR_DIALOG,
                                   NULL, palette_editor_menu_update, TRUE,
                                   n_palette_editor_menu_entries,
                                   palette_editor_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<PaletteEditor>",
                                      "pelette-editor",
                                      NULL,
                                      "/palette-editor-popup",
                                      "palette-editor-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<ColormapEditor>",
                                   _("Indexed Palette Menu"),
                                   GIMP_HELP_INDEXED_PALETTE_DIALOG,
                                   NULL, colormap_editor_menu_update, TRUE,
                                   n_colormap_editor_menu_entries,
                                   colormap_editor_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<ColormapEditor>",
                                      "colormap-editor",
                                      NULL,
                                      "/colormap-editor-popup",
                                      "colormap-editor-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<QMask>",
                                   _("QuickMask Menu"),
                                   GIMP_HELP_QMASK,
                                   NULL, qmask_menu_update, TRUE,
                                   n_qmask_menu_entries,
                                   qmask_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<QMask>",
                                      "qmask",
                                      NULL,
                                      "/qmask-popup",
                                      "qmask-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<ErrorConsole>",
                                   _("Error Console Menu"),
                                   GIMP_HELP_ERRORS_DIALOG,
                                   NULL, error_console_menu_update, TRUE,
                                   n_error_console_menu_entries,
                                   error_console_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<ErrorConsole>",
                                      "error-console",
                                      NULL,
                                      "/error-console-popup",
                                      "error-console-menu.xml",
                                      NULL);

  gimp_menu_factory_menu_register (global_menu_factory, "<ToolOptions>",
                                   _("Tool Options Menu"),
                                   GIMP_HELP_TOOL_OPTIONS_DIALOG,
                                   tool_options_menu_setup,
                                   tool_options_menu_update, TRUE,
                                   n_tool_options_menu_entries,
                                   tool_options_menu_entries);
  gimp_menu_factory_manager_register (global_menu_factory, "<ToolOptions>",
                                      "tool-options",
                                      NULL,
                                      "/tool-options-popup",
                                      "tool-options-menu.xml",
                                      NULL);
}

void
menus_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_object_unref (global_menu_factory);
  global_menu_factory = NULL;

  g_signal_handlers_disconnect_by_func (gimp->config,
                                        menu_can_change_accels,
                                        NULL);

  actions_exit (gimp);
}

void
menus_restore (Gimp *gimp)
{
  gchar *filename;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  filename = gimp_personal_rc_file ("menurc");
  gtk_accel_map_load (filename);
  g_free (filename);
}

void
menus_save (Gimp *gimp)
{
  gchar *filename;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  filename = gimp_personal_rc_file ("menurc");
  gtk_accel_map_save (filename);
  g_free (filename);
}

void
menus_clear (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_print ("TODO: implement menus_clear()\n");
}

void
menus_last_opened_add (GimpItemFactory *item_factory)
{
  GimpItemFactoryEntry *last_opened_entries;
  gint                  n_last_opened_entries;
  gint                  i;

  g_return_if_fail (GIMP_IS_ITEM_FACTORY (item_factory));

  n_last_opened_entries =
    GIMP_GUI_CONFIG (item_factory->gimp->config)->last_opened_size;

  last_opened_entries = g_new (GimpItemFactoryEntry, n_last_opened_entries);

  for (i = 0; i < n_last_opened_entries; i++)
    {
      last_opened_entries[i].entry.path =
        g_strdup_printf ("/File/Open Recent/%02d", i + 1);

      if (i < 9)
        last_opened_entries[i].entry.accelerator =
          g_strdup_printf ("<control>%d", i + 1);
      else if (i == 9)
        last_opened_entries[i].entry.accelerator = "<control>0";
      else
        last_opened_entries[i].entry.accelerator = "";

      last_opened_entries[i].entry.callback        = file_last_opened_cmd_callback;
      last_opened_entries[i].entry.callback_action = i;
      last_opened_entries[i].entry.item_type       = "<StockItem>";
      last_opened_entries[i].entry.extra_data      = GTK_STOCK_OPEN;
      last_opened_entries[i].quark_string          = NULL;
      last_opened_entries[i].help_id               = GIMP_HELP_FILE_OPEN_RECENT;
      last_opened_entries[i].description           = NULL;
    }

  gimp_item_factory_create_items (item_factory,
                                  n_last_opened_entries, last_opened_entries,
                                  item_factory->gimp, 2, FALSE);

  gimp_item_factory_set_sensitive (GTK_ITEM_FACTORY (item_factory),
                                   "/File/Open Recent/(None)",
                                   FALSE);

  for (i = 0; i < n_last_opened_entries; i++)
    {
      GtkWidget *widget;

      widget = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (item_factory),
                                            last_opened_entries[i].entry.path);
      gtk_menu_reorder_child (GTK_MENU (widget->parent), widget, i + 1);

      gimp_item_factory_set_visible (GTK_ITEM_FACTORY (item_factory),
                                     last_opened_entries[i].entry.path,
                                     FALSE);

      g_free (last_opened_entries[i].entry.path);
      if (i < 9)
        g_free (last_opened_entries[i].entry.accelerator);
    }

  g_free (last_opened_entries);

  g_signal_connect_object (item_factory->gimp->documents, "add",
                           G_CALLBACK (menus_last_opened_update),
                           item_factory, 0);
  g_signal_connect_object (item_factory->gimp->documents, "remove",
                           G_CALLBACK (menus_last_opened_update),
                           item_factory, 0);
  g_signal_connect_object (item_factory->gimp->documents, "reorder",
                           G_CALLBACK (menus_last_opened_reorder),
                           item_factory, 0);

  menus_last_opened_update (item_factory->gimp->documents, NULL, item_factory);
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

static void
menu_can_change_accels (GimpGuiConfig *config)
{
  g_object_set (gtk_settings_get_for_screen (gdk_screen_get_default ()),
                "gtk-can-change-accels", config->can_change_accels,
                NULL);
}
