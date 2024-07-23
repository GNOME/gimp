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
#include "imap_icons.h"
#include "imap_main.h"
#include "imap_menu.h"
#include "imap_menu_funcs.h"
#include "imap_polygon.h"
#include "imap_preferences.h"
#include "imap_rectangle.h"
#include "imap_settings.h"
#include "imap_source.h"

#include "libgimp/stdplugins-intl.h"

void
menu_set_zoom_sensitivity (gpointer data,
                           gint     factor)
{
  GAction  *action;
  GimpImap *imap = GIMP_IMAP (data);

  action = g_action_map_lookup_action (G_ACTION_MAP (imap->app), "zoom-in");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), factor < 8);

  action = g_action_map_lookup_action (G_ACTION_MAP (imap->app), "zoom-out");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), factor > -6);
}

void
menu_set_zoom (gpointer data,
               gint     factor)
{
  menu_set_zoom_sensitivity (data, factor);
}

void
menu_shapes_selected (gint     count,
                      gpointer data)
{
  GimpImap *imap     = GIMP_IMAP (data);
  gboolean sensitive = (count > 0);
  GAction  *action;

  action = g_action_map_lookup_action (G_ACTION_MAP (imap->app), "cut");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), sensitive);
  action = g_action_map_lookup_action (G_ACTION_MAP (imap->app), "copy");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), sensitive);
  action = g_action_map_lookup_action (G_ACTION_MAP (imap->app), "clear");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), sensitive);
  action = g_action_map_lookup_action (G_ACTION_MAP (imap->app), "edit-area-info");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), sensitive);
  action = g_action_map_lookup_action (G_ACTION_MAP (imap->app), "deselect-all");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), sensitive);
}

static void
command_list_changed (Command_t *command,
                      gpointer   data)
{
  GAction  *action;
  GimpImap *imap;
  gchar    *label;

  imap = GIMP_IMAP (data);
  action = g_action_map_lookup_action (G_ACTION_MAP (imap->app), "undo");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), command != NULL);

  label = g_strdup_printf (_("_Undo %s"),
                           command && command->name ? command->name : "");
  /* TODO: Find a way to change GAction label in menu for undo */
  g_free (label);

  command = command_list_get_redo_command ();

  action = g_action_map_lookup_action (G_ACTION_MAP (imap->app), "redo");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), command != NULL);

  label = g_strdup_printf (_("_Redo %s"),
                           command && command->name ? command->name : "");
    /* TODO: Find a way to change GAction label in menu for redo */
  g_free (label);
}

static void
paste_buffer_added (Object_t *obj, gpointer data)
{
  GAction  *action;
  GimpImap *imap = GIMP_IMAP (data);

  action = g_action_map_lookup_action (G_ACTION_MAP (imap->app), "paste");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), TRUE);
}

static void
paste_buffer_removed (Object_t *obj, gpointer data)
{
  GAction  *action;
  GimpImap *imap = GIMP_IMAP (data);

  action = g_action_map_lookup_action (G_ACTION_MAP (imap->app), "paste");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
}

Menu_t*
make_menu (GimpImap *imap)
{
  GAction *action;

  paste_buffer_add_add_cb (paste_buffer_added, imap);
  paste_buffer_add_remove_cb (paste_buffer_removed, imap);

  action = g_action_map_lookup_action (G_ACTION_MAP (imap->app), "paste");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

  menu_shapes_selected (0, imap);

  menu_set_zoom_sensitivity (imap, 1);

  command_list_add_update_cb (command_list_changed, imap);

  command_list_changed (NULL, imap);

  return NULL;
}

void
do_main_popup_menu (GdkEventButton *event,
                    gpointer        data)
{
  GtkWidget  *menu;
  GMenuModel *model;
  GimpImap   *imap = GIMP_IMAP (data);

  model = G_MENU_MODEL (gtk_builder_get_object (imap->builder, "imap-main-popup"));
  menu = gtk_menu_new_from_model (model);

  gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (imap->dlg), NULL);
  gtk_menu_popup_at_pointer (GTK_MENU (menu), (GdkEvent *) event);
}

GtkWidget*
make_selection_toolbar (GimpImap *imap)
{
  GtkWidget *toolbar = gtk_toolbar_new ();

  add_tool_button (toolbar, "app.move-up", GIMP_ICON_GO_UP,
                   _("Move Up"), _("Move Up"));
  add_tool_button (toolbar, "app.move-down", GIMP_ICON_GO_DOWN,
                   _("Move Down"), _("Move Down"));
  add_tool_button (toolbar, "app.edit-area-info", GIMP_ICON_EDIT,
                   _("Edit Area Info..."), _("Edit selected area info"));
  add_tool_button (toolbar, "app.clear", GIMP_ICON_EDIT_DELETE,
                   _("Delete"), _("Delete"));

  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (toolbar),
                                  GTK_ORIENTATION_VERTICAL);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 0);

  gtk_widget_show (toolbar);
  return toolbar;
}
