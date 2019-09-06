/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2006 Maurits Rijk  m.rijk@chello.nl
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
 *
 */

#include "config.h"

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "imap_about.h"
#include "imap_circle.h"
#include "imap_file.h"
#include "imap_grid.h"
#include "imap_main.h"
#include "imap_menu.h"
#include "imap_menu_funcs.h"
#include "imap_polygon.h"
#include "imap_preferences.h"
#include "imap_rectangle.h"
#include "imap_settings.h"
#include "imap_stock.h"
#include "imap_source.h"

#include "libgimp/stdplugins-intl.h"

static Menu_t _menu;
static GtkUIManager *ui_manager;

GtkWidget*
menu_get_widget(const gchar *path)
{
  return gtk_ui_manager_get_widget (ui_manager, path);
}

static void
set_sensitive (const gchar *path, gboolean sensitive)
{
  GtkAction *action = gtk_ui_manager_get_action (ui_manager, path);
  g_object_set (action, "sensitive", sensitive, NULL);
}

static void
menu_mru(GtkWidget *widget, gpointer data)
{
   MRU_t *mru = get_mru();
   char *filename = (char*) data;

   if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
      load(filename);
   } else {
      do_file_error_dialog(_("Error opening file"), filename);
      mru_remove(mru, filename);
      menu_build_mru_items(mru);
   }
}

void
menu_set_zoom_sensitivity(gint factor)
{
  set_sensitive ("/MainMenu/ViewMenu/ZoomIn", factor < 8);
  set_sensitive ("/MainMenu/ViewMenu/ZoomOut", factor > 1);
}

void
menu_set_zoom(gint factor)
{
  menu_set_zoom_sensitivity (factor);
}

void
menu_shapes_selected(gint count)
{
  gboolean sensitive = (count > 0);

  set_sensitive ("/MainMenu/EditMenu/Cut", sensitive);
  set_sensitive ("/MainMenu/EditMenu/Copy", sensitive);
  set_sensitive ("/MainMenu/EditMenu/Clear", sensitive);
  set_sensitive ("/MainMenu/EditMenu/EditAreaInfo", sensitive);
  set_sensitive ("/MainMenu/EditMenu/DeselectAll", sensitive);
}

static void
command_list_changed(Command_t *command, gpointer data)
{
  GtkAction *action;
  gchar     *label;

  action = gtk_ui_manager_get_action (ui_manager, "/MainMenu/EditMenu/Undo");

  label = g_strdup_printf (_("_Undo %s"),
                           command && command->name ? command->name : "");

  g_object_set (action,
                "label",     label,
                "sensitive", command != NULL,
                NULL);
  g_free (label);

  command = command_list_get_redo_command();

  action = gtk_ui_manager_get_action (ui_manager, "/MainMenu/EditMenu/Redo");

  label = g_strdup_printf (_("_Redo %s"),
                           command && command->name ? command->name : "");
  g_object_set (action,
                "label",     label,
                "sensitive", command != NULL,
                NULL);
  g_free (label);
}

static void
paste_buffer_added(Object_t *obj, gpointer data)
{
  set_sensitive("/MainMenu/EditMenu/Paste", TRUE);
}

static void
paste_buffer_removed(Object_t *obj, gpointer data)
{
  set_sensitive("/MainMenu/EditMenu/Paste", FALSE);
}

/* Normal items */
static const GtkActionEntry entries[] =
{
  { "FileMenu", NULL,
    N_("_File") },
  { "Open", GIMP_ICON_DOCUMENT_OPEN,
    N_("_Open..."), NULL, N_("Open"),
    do_file_open_dialog},
  { "Save", GIMP_ICON_DOCUMENT_SAVE,
    N_("_Save..."), NULL, N_("Save"),
    save},
  { "SaveAs", GIMP_ICON_DOCUMENT_SAVE_AS,
    N_("Save _As..."), "<shift><control>S", NULL,
    do_file_save_as_dialog},
  { "Close", GIMP_ICON_CLOSE, N_("_Close"), "<primary>w", NULL, do_close},
  { "Quit", GIMP_ICON_APPLICATION_EXIT,
    N_("_Quit"), "<primary>q", NULL, do_quit},

  { "EditMenu", NULL, N_("_Edit") },
  { "Undo", GIMP_ICON_EDIT_UNDO,
    N_("_Undo"), NULL, N_("Undo"), do_undo},
  { "Redo", GIMP_ICON_EDIT_REDO,
    N_("_Redo"), NULL, N_("Redo"), do_redo},
  { "Cut", GIMP_ICON_EDIT_CUT,
    N_("Cu_t"), "<primary>x", N_("Cut"), do_cut},
  { "Copy", GIMP_ICON_EDIT_COPY,
    N_("_Copy"), "<primary>c", N_("Copy"), do_copy},
  { "Paste", GIMP_ICON_EDIT_PASTE,
    N_("_Paste"), "<primary>v", N_("Paste"), do_paste},
  { "Clear", GIMP_ICON_EDIT_DELETE,
    N_("_Delete"), NULL, N_("Delete"), do_clear},
  { "SelectAll", NULL,
    N_("Select _All"), "<primary>A", NULL, do_select_all},
  { "DeselectAll", NULL,
    N_("D_eselect All"), "<shift><primary>A", NULL,
    do_deselect_all},
  { "EditAreaInfo", GIMP_ICON_EDIT
    , N_("Edit Area _Info..."), NULL,
    N_("Edit selected area info"), do_edit_selected_shape},
  { "Preferences", GIMP_ICON_PREFERENCES_SYSTEM,
    N_("_Preferences"), NULL, N_("Preferences"),
    do_preferences_dialog},
  { "MoveToFront", IMAP_STOCK_TO_FRONT, "", NULL, N_("Move Area to Front"),
    do_move_to_front},
  { "SendToBack", IMAP_STOCK_TO_BACK, "", NULL, N_("Move Area to Bottom"),
    do_send_to_back},
  { "DeleteArea", NULL, N_("Delete Area"), NULL, NULL, NULL},
  { "MoveUp", GIMP_ICON_GO_UP, N_("Move Up"), NULL, NULL, NULL},
  { "MoveDown", GIMP_ICON_GO_DOWN, N_("Move Down"), NULL, NULL, NULL},

  { "InsertPoint", NULL, N_("Insert Point"), NULL, NULL, polygon_insert_point},
  { "DeletePoint", NULL, N_("Delete Point"), NULL, NULL, polygon_delete_point},

  { "ViewMenu", NULL, N_("_View") },
  { "Source", NULL, N_("Source..."), NULL, NULL, do_source_dialog},
  { "ZoomIn", GIMP_ICON_ZOOM_IN, N_("Zoom _In"), "plus", N_("Zoom in"), do_zoom_in},
  { "ZoomOut", GIMP_ICON_ZOOM_OUT, N_("Zoom _Out"), "minus", N_("Zoom out"), do_zoom_out},
  { "ZoomToMenu", NULL, N_("_Zoom To") },

  { "MappingMenu", NULL, N_("_Mapping") },
  { "EditMapInfo", GIMP_ICON_DIALOG_INFORMATION, N_("Edit Map Info..."), NULL,
    N_("Edit Map Info"), do_settings_dialog},

  { "ToolsMenu", NULL, N_("_Tools") },
  { "GridSettings", NULL, N_("Grid Settings..."), NULL, NULL,
    do_grid_settings_dialog},
  { "UseGimpGuides", NULL, N_("Use GIMP Guides..."), NULL, NULL,
    do_use_gimp_guides_dialog},
  { "CreateGuides", NULL, N_("Create Guides..."), NULL, NULL,
    do_create_guides_dialog},

  { "HelpMenu", NULL, N_("_Help") },
  { "Contents", GIMP_ICON_HELP, N_("_Contents"), NULL, NULL, imap_help},
  { "About", GIMP_ICON_HELP_ABOUT, N_("_About"), NULL, NULL, do_about_dialog},

  { "ZoomMenu", NULL, N_("_Zoom") },
};

/* Toggle items */
static const GtkToggleActionEntry toggle_entries[] = {
  { "AreaList", NULL, N_("Area List"), NULL, NULL, NULL, TRUE },
  { "Grid", GIMP_ICON_GRID, N_("_Grid"), NULL, N_("Grid"), toggle_grid, FALSE }
};

static const GtkRadioActionEntry color_entries[] = {
  { "Color", NULL, N_("Color"), NULL, NULL, 0},
  { "Gray", NULL, N_("Gray"), NULL, NULL, 1},
};

static const GtkRadioActionEntry mapping_entries[] = {
  { "Arrow", GIMP_ICON_CURSOR, N_("Arrow"), NULL,
    N_("Select existing area"), 0},
  { "Rectangle", IMAP_STOCK_RECTANGLE, N_("Rectangle"), NULL,
    N_("Define Rectangle area"), 1},
  { "Circle", IMAP_STOCK_CIRCLE, N_("Circle"), NULL,
    N_("Define Circle/Oval area"), 2},
  { "Polygon", IMAP_STOCK_POLYGON, N_("Polygon"), NULL,
    N_("Define Polygon area"), 3},
};

static const GtkRadioActionEntry zoom_entries[] = {
  { "Zoom1:1", NULL, "1:1", NULL, NULL, 0},
  { "Zoom1:2", NULL, "1:2", NULL, NULL, 1},
  { "Zoom1:3", NULL, "1:3", NULL, NULL, 2},
  { "Zoom1:4", NULL, "1:4", NULL, NULL, 3},
  { "Zoom1:5", NULL, "1:5", NULL, NULL, 4},
  { "Zoom1:6", NULL, "1:6", NULL, NULL, 5},
  { "Zoom1:7", NULL, "1:7", NULL, NULL, 6},
  { "Zoom1:8", NULL, "1:8", NULL, NULL, 7},
};

static const gchar ui_description[] =
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='FileMenu'>"
"      <menuitem action='Open'/>"
"      <menuitem action='Save'/>"
"      <menuitem action='SaveAs'/>"
"      <separator/>"
"      <menuitem action='Close'/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='EditMenu'>"
"      <menuitem action='Undo'/>"
"      <menuitem action='Redo'/>"
"      <menuitem action='Cut'/>"
"      <menuitem action='Copy'/>"
"      <menuitem action='Paste'/>"
"      <menuitem action='Clear'/>"
"      <separator/>"
"      <menuitem action='SelectAll'/>"
"      <menuitem action='DeselectAll'/>"
"      <separator/>"
"      <menuitem action='EditAreaInfo'/>"
"      <separator/>"
"      <menuitem action='Preferences'/>"
"    </menu>"
"    <menu action='ViewMenu'>"
"      <menuitem action='AreaList'/>"
"      <menuitem action='Source'/>"
"      <separator/>"
"      <menuitem action='Color'/>"
"      <menuitem action='Gray'/>"
"      <separator/>"
"      <menuitem action='ZoomIn'/>"
"      <menuitem action='ZoomOut'/>"
"      <menu action='ZoomToMenu'>"
"        <menuitem action='Zoom1:1'/>"
"        <menuitem action='Zoom1:2'/>"
"        <menuitem action='Zoom1:3'/>"
"        <menuitem action='Zoom1:4'/>"
"        <menuitem action='Zoom1:5'/>"
"        <menuitem action='Zoom1:6'/>"
"        <menuitem action='Zoom1:7'/>"
"        <menuitem action='Zoom1:8'/>"
"      </menu>"
"    </menu>"
"    <menu action='MappingMenu'>"
"      <menuitem action='Arrow'/>"
"      <menuitem action='Rectangle'/>"
"      <menuitem action='Circle'/>"
"      <menuitem action='Polygon'/>"
"      <separator/>"
"      <menuitem action='EditMapInfo'/>"
"    </menu>"
"    <menu action='ToolsMenu'>"
"      <menuitem action='Grid'/>"
"      <menuitem action='GridSettings'/>"
"      <separator/>"
"      <menuitem action='UseGimpGuides'/>"
"      <menuitem action='CreateGuides'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='Contents'/>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
""
"  <popup name='PopupMenu'>"
"    <menuitem action='EditMapInfo'/>"
"    <menu action='ToolsMenu'>"
"      <menuitem action='Arrow'/>"
"      <menuitem action='Rectangle'/>"
"      <menuitem action='Circle'/>"
"      <menuitem action='Polygon'/>"
"    </menu>"
"    <menu action='ZoomMenu'>"
"      <menuitem action='ZoomIn'/>"
"      <menuitem action='ZoomOut'/>"
"    </menu>"
"    <menuitem action='Grid'/>"
"    <menuitem action='GridSettings'/>"
"    <menuitem action='CreateGuides'/>"
"    <menuitem action='Paste'/>"
"  </popup>"
""
"  <popup name='ObjectPopupMenu'>"
"    <menuitem action='EditAreaInfo'/>"
"    <menuitem action='DeleteArea'/>"
"    <menuitem action='MoveUp'/>"
"    <menuitem action='MoveDown'/>"
"    <menuitem action='Cut'/>"
"    <menuitem action='Copy'/>"
"  </popup>"
""
"  <popup name='PolygonPopupMenu'>"
"    <menuitem action='InsertPoint'/>"
"    <menuitem action='DeletePoint'/>"
"    <menuitem action='EditAreaInfo'/>"
"    <menuitem action='DeleteArea'/>"
"    <menuitem action='MoveUp'/>"
"    <menuitem action='MoveDown'/>"
"    <menuitem action='Cut'/>"
"    <menuitem action='Copy'/>"
"  </popup>"
""
"  <toolbar name='Toolbar'>"
"    <toolitem action='Open'/>"
"    <toolitem action='Save'/>"
"    <separator/>"
"    <toolitem action='Preferences'/>"
"    <separator/>"
"    <toolitem action='Undo'/>"
"    <toolitem action='Redo'/>"
"    <separator/>"
"    <toolitem action='Cut'/>"
"    <toolitem action='Copy'/>"
"    <toolitem action='Paste'/>"
"    <separator/>"
"    <toolitem action='ZoomIn'/>"
"    <toolitem action='ZoomOut'/>"
"    <separator/>"
"    <toolitem action='EditMapInfo'/>"
"    <separator/>"
"    <toolitem action='MoveToFront'/>"
"    <toolitem action='SendToBack'/>"
"    <separator/>"
"    <toolitem action='Grid'/>"
"  </toolbar>"
""
"  <toolbar name='Tools'>"
"    <toolitem action='Arrow'/>"
"    <toolitem action='Rectangle'/>"
"    <toolitem action='Circle'/>"
"    <toolitem action='Polygon'/>"
"    <separator/>"
"    <toolitem action='EditAreaInfo'/>"
"  </toolbar>"
""
"  <toolbar name='Selection'>"
"    <toolitem action='MoveUp'/>"
"    <toolitem action='MoveDown'/>"
"    <toolitem action='EditAreaInfo'/>"
"    <toolitem action='Clear'/>"
"  </toolbar>"
"</ui>";

Menu_t*
make_menu(GtkWidget *main_vbox, GtkWidget *window)
{
  GtkWidget *menubar;
  GtkActionGroup *action_group;
  GtkAccelGroup *accel_group;
  GError *error;

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, NULL);

  gtk_action_group_add_actions (action_group, entries, G_N_ELEMENTS (entries),
                                window);
  gtk_action_group_add_toggle_actions (action_group, toggle_entries,
                                       G_N_ELEMENTS (toggle_entries), window);

  gtk_action_group_add_radio_actions (action_group, color_entries,
                                      G_N_ELEMENTS (color_entries), 0,
                                      G_CALLBACK (set_preview_color), NULL);
  gtk_action_group_add_radio_actions (action_group, zoom_entries,
                                      G_N_ELEMENTS (zoom_entries), 0,
                                      G_CALLBACK (set_zoom_factor), NULL);
  gtk_action_group_add_radio_actions (action_group, mapping_entries,
                                      G_N_ELEMENTS (mapping_entries), 0,
                                      G_CALLBACK (set_func), window);

  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

  accel_group = gtk_ui_manager_get_accel_group (ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

  error = NULL;
  if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1,
                                          &error))
    {
      g_warning ("building menus failed: %s", error->message);
      g_error_free (error);
      /* exit (EXIT_FAILURE); */
    }

  menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
  gtk_widget_show (menubar);
  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, FALSE, 0);

  paste_buffer_add_add_cb(paste_buffer_added, NULL);
  paste_buffer_add_remove_cb(paste_buffer_removed, NULL);

  set_sensitive ("/MainMenu/EditMenu/Paste", FALSE);
  menu_shapes_selected (0);

  menu_set_zoom_sensitivity (1);

  command_list_add_update_cb (command_list_changed, NULL);

  command_list_changed (NULL, NULL);

  return &_menu;
}

void
menu_build_mru_items(MRU_t *mru)
{
   GList *p;
   gint position = 0;
   int i;

  return;

   if (_menu.nr_off_mru_items) {
      GList *children;

      children = gtk_container_get_children(GTK_CONTAINER(_menu.open_recent));
      p = g_list_nth(children, position);
      for (i = 0; i < _menu.nr_off_mru_items; i++, p = p->next) {
         gtk_widget_destroy((GtkWidget*) p->data);
      }
      g_list_free(children);
   }

   i = 0;
   for (p = mru->list; p; p = p->next, i++) {
      GtkWidget *item = insert_item_with_label(_menu.open_recent, position++,
                                               (gchar*) p->data,
                                               menu_mru, p->data);
      if (i < 9) {
         guchar accelerator_key = '1' + i;
         add_accelerator(item, accelerator_key, GDK_CONTROL_MASK);
      }
   }
   _menu.nr_off_mru_items = i;
}

void
do_main_popup_menu(GdkEventButton *event)
{
  GtkWidget *popup = gtk_ui_manager_get_widget (ui_manager, "/PopupMenu");
  gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL,
                  event->button, event->time);
}

void
menu_check_grid(gboolean check)
{
  GtkAction *action = gtk_ui_manager_get_action (ui_manager,
                                                 "/MainMenu/ToolsMenu/Grid");
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), check);
}

GtkWidget*
make_toolbar(GtkWidget *main_vbox, GtkWidget *window)
{
  GtkWidget *toolbar;

  toolbar = gtk_ui_manager_get_widget (ui_manager, "/Toolbar");
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), toolbar, FALSE, FALSE, 0);
  gtk_widget_show (toolbar);

  return toolbar;
}

GtkWidget*
make_tools(GtkWidget *window)
{
  GtkWidget *toolbar;

  toolbar = gtk_ui_manager_get_widget (ui_manager, "/Tools");
  gtk_orientable_set_orientation (GTK_ORIENTABLE (toolbar),
                                  GTK_ORIENTATION_VERTICAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 0);
  gtk_widget_show (toolbar);

  return toolbar;
}

GtkWidget*
make_selection_toolbar(void)
{
  GtkWidget *toolbar;

  toolbar = gtk_ui_manager_get_widget (ui_manager, "/Selection");

  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (toolbar),
                                  GTK_ORIENTATION_VERTICAL);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 0);

  gtk_widget_show (toolbar);
  return toolbar;
}
