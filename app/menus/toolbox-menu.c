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

#include "gui-types.h"

#include "core/gimp.h"

#include "plug-in/plug-ins.h"

#include "widgets/gimpitemfactory.h"

#include "debug-commands.h"
#include "dialogs-commands.h"
#include "file-commands.h"
#include "help-commands.h"
#include "menus.h"
#include "plug-in-menus.h"
#include "toolbox-menu.h"

#include "gimp-intl.h"


GimpItemFactoryEntry toolbox_menu_entries[] =
{
  /*  <Toolbox>/File  */

  MENU_BRANCH (N_("/_File")),

  { { N_("/File/New..."), "<control>N",
      file_new_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_NEW },
    NULL,
    "file/dialogs/file_new.html", NULL },
  { { N_("/File/Open..."), "<control>O",
      file_open_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    NULL,
    "file/dialogs/file_open.html", NULL },

  /*  <Toolbox>/File/Open Recent  */

  MENU_BRANCH (N_("/File/_Open Recent")),

  { { N_("/File/Open Recent/(None)"), NULL, NULL, 0 },
    NULL, NULL, NULL },

  MENU_SEPARATOR ("/File/Open Recent/---"),

  { { N_("/File/Open Recent/Document History..."), "foo",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    "gimp-document-list",
    "file/dialogs/document_index.html", NULL },

  /*  <Toolbox>/File/Acquire  */

  MENU_BRANCH (N_("/File/_Acquire")),

  MENU_SEPARATOR ("/File/---"),

  { { N_("/File/Preferences..."), NULL,
      dialogs_create_toplevel_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PREFERENCES },
    "gimp-preferences-dialog",
    "file/dialogs/preferences/preferences.html", NULL },

  /*  <Toolbox>/File/Dialogs  */

  MENU_BRANCH (N_("/File/_Dialogs")),

  MENU_BRANCH (N_("/File/Dialogs/_Create New Dock")),

  { { N_("/File/Dialogs/Create New Dock/Layers, Channels & Paths..."), NULL,
      dialogs_create_lc_cmd_callback, 0 },
    NULL,
    "file/dialogs/layers_and_channels.html", NULL },
  { { N_("/File/Dialogs/Create New Dock/Brushes, Patterns & Gradients..."), NULL,
      dialogs_create_data_cmd_callback, 0 },
    NULL,
    NULL, NULL },
  { { N_("/File/Dialogs/Create New Dock/Misc. Stuff..."), NULL,
      dialogs_create_stuff_cmd_callback, 0 },
    NULL,
    NULL, NULL },

  { { N_("/File/Dialogs/Tool Options..."), "<control><shift>T",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_OPTIONS },
    "gimp-tool-options",
    "file/dialogs/tool_options.html", NULL },
  { { N_("/File/Dialogs/Device Status..."), NULL,
      dialogs_create_toplevel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_DEVICE_STATUS },
    "gimp-device-status-dialog",
    "file/dialogs/device_status.html", NULL },

  MENU_SEPARATOR ("/File/Dialogs/---"),

  { { N_("/File/Dialogs/Layers..."), "<control>L",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_LAYERS },
    "gimp-layer-list",
    NULL, NULL },
  { { N_("/File/Dialogs/Channels..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_CHANNELS },
    "gimp-channel-list",
    NULL, NULL },
  { { N_("/File/Dialogs/Paths..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_PATHS },
    "gimp-vectors-list",
    NULL, NULL },
  { { N_("/File/Dialogs/Indexed Palette..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_COLOR },
    "gimp-indexed-palette",
    "file/dialogs/indexed_palette.html", NULL },
  { { N_("/File/Dialogs/Selection Editor..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_RECT_SELECT },
    "gimp-selection-editor",
    NULL, NULL },
  { { N_("/File/Dialogs/Navigation..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_NAVIGATION },
    "gimp-navigation-view",
    NULL, NULL },
  { { N_("/File/Dialogs/Undo History..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_UNDO },
    "gimp-undo-history",
    NULL, NULL },

  MENU_SEPARATOR ("/File/Dialogs/---"),

  { { N_("/File/Dialogs/Colors..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_COLOR },
    "gimp-color-editor",
    NULL, NULL },
  { { N_("/File/Dialogs/Brushes..."), "<control><shift>B",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_PAINTBRUSH },
    "gimp-brush-grid",
    "file/dialogs/brush_selection.html", NULL },
  { { N_("/File/Dialogs/Patterns..."), "<control><shift>P",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_BUCKET_FILL },
    "gimp-pattern-grid",
    "file/dialogs/pattern_selection.html", NULL },
  { { N_("/File/Dialogs/Gradients..."), "<control>G",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TOOL_BLEND },
    "gimp-gradient-list",
    "file/dialogs/gradient_selection.html", NULL },
  { { N_("/File/Dialogs/Palettes..."), "<control>P",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_COLOR },
    "gimp-palette-list",
    "file/dialogs/palette_selection.html", NULL },
  { { N_("/File/Dialogs/Fonts..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_SELECT_FONT },
    "gimp-font-list",
    "file/dialogs/font_selection.html", NULL },
  { { N_("/File/Dialogs/Buffers..."), "foo",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_PASTE },
    "gimp-buffer-list",
    NULL, NULL },

  MENU_SEPARATOR ("/File/Dialogs/---"),

  { { N_("/File/Dialogs/Images..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_IMAGES },
    "gimp-image-list",
    NULL, NULL },
  { { N_("/File/Dialogs/Document History..."), "",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_OPEN },
    "gimp-document-list",
    "file/dialogs/document_index.html", NULL },
  { { N_("/File/Dialogs/Templates..."), "",
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_TEMPLATE },
    "gimp-template-list",
    "file/dialogs/templates.html", NULL },
  { { N_("/File/Dialogs/Error Console..."), NULL,
      dialogs_create_dockable_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_WARNING },
    "gimp-error-console",
    "file/dialogs/error_console.html", NULL },

#ifdef ENABLE_DEBUG_ENTRIES
  { { "/File/Debug/Mem Profile", NULL,
      debug_mem_profile_cmd_callback, 0 },
    NULL, NULL, NULL },
  { { "/File/Debug/Dump Items", NULL,
      debug_dump_menus_cmd_callback, 0 },
    NULL, NULL, NULL },
#endif

  MENU_SEPARATOR ("/File/---"),

  { { N_("/File/Quit"), "<control>Q",
      file_quit_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_QUIT },
    NULL,
    "file/quit.html", NULL },

  /*  <Toolbox>/Xtns  */

  MENU_BRANCH (N_("/_Xtns")),

  { { N_("/Xtns/Module Manager..."), NULL,
      dialogs_create_toplevel_cmd_callback, 0 },
    "gimp-module-manager-dialog",
    "dialogs/module_manager.html", NULL },

  MENU_SEPARATOR ("/Xtns/---"),

  /*  <Toolbox>/Help  */

  MENU_BRANCH (N_("/_Help")),

  { { N_("/Help/Help..."), "F1",
      help_help_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_HELP },
    NULL,
    "help/dialogs/help.html", NULL },
  { { N_("/Help/Context Help..."), "<shift>F1",
      help_context_help_cmd_callback, 0,
      "<StockItem>", GTK_STOCK_HELP },
    NULL,
    "help/context_help.html", NULL },
  { { N_("/Help/Tip of the Day..."), NULL,
      dialogs_create_toplevel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_INFO },
    "gimp-tips-dialog",
    "help/dialogs/tip_of_the_day.html", NULL },
  { { N_("/Help/About..."), NULL,
      dialogs_create_toplevel_cmd_callback, 0,
      "<StockItem>", GIMP_STOCK_WILBER },
    "gimp-about-dialog",
    "help/dialogs/about.html", NULL }
};

gint n_toolbox_menu_entries = G_N_ELEMENTS (toolbox_menu_entries);


void
toolbox_menu_setup (GimpItemFactory *factory)
{
  static gchar *reorder_subsubmenus[] = { "/Xtns" };

  GtkWidget    *menu_item;
  GtkWidget    *menu;
  GList        *list;
  gint          i, pos;

  menus_last_opened_add (factory, factory->gimp);

  plug_in_menus_create (factory, factory->gimp->plug_in_proc_defs);

  /*  Move all menu items under "<Toolbox>/Xtns" which are not submenus or
   *  separators to the top of the menu
   */
  pos = 1;
  menu_item = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
                                           "/Xtns/Module Manager...");
  if (menu_item && menu_item->parent && GTK_IS_MENU (menu_item->parent))
    {
      menu = menu_item->parent;

      for (list = g_list_nth (GTK_MENU_SHELL (menu)->children, pos); list;
	   list = g_list_next (list))
	{
	  menu_item = GTK_WIDGET (list->data);

	  if (! GTK_MENU_ITEM (menu_item)->submenu &&
	      GTK_IS_LABEL (GTK_BIN (menu_item)->child))
	    {
	      gtk_menu_reorder_child (GTK_MENU (menu_item->parent),
				      menu_item, pos);
	      list = g_list_nth (GTK_MENU_SHELL (menu)->children, pos);
	      pos++;
	    }
	}
    }

  for (i = 0; i < G_N_ELEMENTS (reorder_subsubmenus); i++)
    {
      menu = gtk_item_factory_get_widget (GTK_ITEM_FACTORY (factory),
					  reorder_subsubmenus[i]);

      if (menu && GTK_IS_MENU (menu))
	{
	  for (list = GTK_MENU_SHELL (menu)->children; list;
	       list = g_list_next (list))
	    {
	      GtkMenuItem *menu_item;

	      menu_item = GTK_MENU_ITEM (list->data);

	      if (menu_item->submenu)
		menus_filters_subdirs_to_top (GTK_MENU (menu_item->submenu));
	    }
	}
    }
}
