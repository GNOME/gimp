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

#ifndef __GIMP_ITEM_FACTORY_H__
#define __GIMP_ITEM_FACTORY_H__

G_BEGIN_DECLS


typedef void (* GimpItemFactoryUpdateFunc) (GtkItemFactory *factory,
                                            gpointer        data);


typedef struct _GimpItemFactoryEntry GimpItemFactoryEntry;

struct _GimpItemFactoryEntry
{
  GtkItemFactoryEntry  entry;

  const gchar *quark_string;

  const gchar *help_page;
  const gchar *description;
};


GtkItemFactory * gimp_item_factory_new (GType                      container_type,
                                        const gchar               *path,
                                        const gchar               *factory_path,
                                        GimpItemFactoryUpdateFunc  update_func,
                                        guint                      n_entries,
                                        GimpItemFactoryEntry      *entries,
                                        gpointer                   callback_data,
                                        gboolean                   create_tearoff);

void   gimp_item_factory_popup_with_data (GtkItemFactory   *item_factory,
                                          gpointer          data,
                                          GtkDestroyNotify  popdown_func);

void   gimp_item_factory_create_item   (GtkItemFactory        *item_factory,
                                        GimpItemFactoryEntry  *entry,
                                        gpointer               callback_data,
                                        guint                  callback_type,
                                        gboolean               create_tearoff,
                                        gboolean               static_entry);
void   gimp_item_factory_create_items  (GtkItemFactory        *item_factory,
                                        guint                  n_entries,
                                        GimpItemFactoryEntry  *entries,
                                        gpointer               callback_data,
                                        guint                  callback_type,
                                        gboolean               create_tearoff,
                                        gboolean               static_entries);

void   gimp_item_factory_set_active    (GtkItemFactory        *factory,
                                        gchar                 *path,
                                        gboolean               state);
void   gimp_item_factory_set_color     (GtkItemFactory        *factory,
                                        gchar                 *path,
                                        const GimpRGB         *color,
                                        gboolean               set_label);
void   gimp_item_factory_set_label     (GtkItemFactory        *factory,
                                        gchar                 *path,
                                        const gchar           *label);
void   gimp_item_factory_set_sensitive (GtkItemFactory        *factory,
                                        gchar                 *path,
                                        gboolean               sensitive);
void   gimp_item_factory_set_visible   (GtkItemFactory        *factory,
                                        gchar                 *path,
                                        gboolean               visible);


void   gimp_item_factory_tearoff_callback (GtkWidget          *widget,
					   gpointer            data,
					   guint               action);


void   gimp_menu_item_create           (GimpItemFactoryEntry  *entry,
                                        gchar                 *domain_name,
                                        gpointer               callback_data);
void   gimp_menu_item_destroy          (gchar                 *path);

void   gimp_menu_item_set_active       (gchar                 *path,
                                        gboolean               state);
void   gimp_menu_item_set_color        (gchar                 *path,
                                       const GimpRGB         *color,
                                        gboolean               set_label);
void   gimp_menu_item_set_label        (gchar                 *path,
                                        const gchar           *label);
void   gimp_menu_item_set_sensitive    (gchar                 *path,
                                        gboolean               sensitive);
void   gimp_menu_item_set_visible      (gchar                 *path,
                                        gboolean               visible);


G_END_DECLS

#endif /* __GIMP_ITEM_FACTORY_H__ */
