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
#ifndef __MENUS_H__
#define __MENUS_H__

#include "gtk/gtk.h"

typedef struct _GimpItemFactoryEntry GimpItemFactoryEntry;

struct _GimpItemFactoryEntry
{
  GtkItemFactoryEntry  entry;

  gchar *help_page;
  gchar *description;
};

void menus_get_toolbox_menubar (GtkWidget            **menubar,
				GtkAccelGroup        **accel_group);
void menus_get_image_menu      (GtkWidget            **menu,
				GtkAccelGroup        **accel_group);
void menus_get_load_menu       (GtkWidget            **menu,
				GtkAccelGroup        **accel_group);
void menus_get_save_menu       (GtkWidget            **menu,
				GtkAccelGroup        **accel_group);
void menus_get_layers_menu     (GtkWidget            **menu,
				GtkAccelGroup        **accel_group);
void menus_get_channels_menu   (GtkWidget            **menu,
				GtkAccelGroup        **accel_group);
void menus_get_paths_menu      (GtkWidget            **menu,
				GtkAccelGroup        **accel_group);

void menus_create_item_from_full_path (GimpItemFactoryEntry  *entry,
				       gpointer               callback_data);

void menus_destroy             (gchar                 *path);

void menus_quit                (void);

void menus_set_sensitive       (gchar                 *path,
				gboolean               sensitive);
void menus_set_sensitive_glue  (gchar                 *prepath,
				gchar                 *path,
				gboolean               sensitive);

void menus_set_state           (gchar                 *path,
				gboolean               state);
void menus_set_state_glue      (gchar                 *prepath,
				gchar                 *path,
				gboolean               state);

void menus_last_opened_add     (gchar                 *filename);

#endif /* __MENUS_H__ */
