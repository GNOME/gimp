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


struct _GimpItemFactoryEntry
{
  GtkItemFactoryEntry  entry;

  const gchar *quark_string;

  const gchar *help_id;
  const gchar *description;
};


#define GIMP_TYPE_ITEM_FACTORY            (gimp_item_factory_get_type ())
#define GIMP_ITEM_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ITEM_FACTORY, GimpItemFactory))
#define GIMP_ITEM_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ITEM_FACTORY, GimpItemFactoryClass))
#define GIMP_IS_ITEM_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ITEM_FACTORY))
#define GIMP_IS_ITEM_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ITEM_FACTORY))
#define GIMP_ITEM_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ITEM_FACTORY, GimpItemFactoryClass))


typedef struct _GimpItemFactoryClass  GimpItemFactoryClass;

struct _GimpItemFactory
{
  GtkItemFactory             parent_instance;

  Gimp                      *gimp;
  GimpItemFactoryUpdateFunc  update_func;
  gboolean                   update_on_popup;
  gchar                     *title;
  gchar                     *help_id;

  GList                     *translation_trash;
};

struct _GimpItemFactoryClass
{
  GtkItemFactoryClass  parent_class;

  GHashTable          *factories;
};


GType   gimp_item_factory_get_type        (void) G_GNUC_CONST;

GimpItemFactory * gimp_item_factory_new   (Gimp                 *gimp,
                                           GType                 container_type,
                                           const gchar          *factory_path,
                                           const gchar          *title,
                                           const gchar          *help_id,
                                           GimpItemFactoryUpdateFunc  update_func,
                                           gboolean              update_on_popup,
                                           guint                 n_entries,
                                           GimpItemFactoryEntry *entries,
                                           gpointer              callback_data,
                                           gboolean              create_tearoff);

GimpItemFactory * gimp_item_factory_from_path   (const gchar     *path);
GList           * gimp_item_factories_from_path (const gchar     *path);

void   gimp_item_factory_create_item      (GimpItemFactory       *factory,
                                           GimpItemFactoryEntry  *entry,
                                           const gchar           *textdomain,
                                           gpointer               callback_data,
                                           guint                  callback_type,
                                           gboolean               create_tearoff,
                                           gboolean               static_entry);
void   gimp_item_factory_create_items     (GimpItemFactory       *factory,
                                           guint                  n_entries,
                                           GimpItemFactoryEntry  *entries,
                                           gpointer               callback_data,
                                           guint                  callback_type,
                                           gboolean               create_tearoff,
                                           gboolean               static_entries);

void   gimp_item_factory_update           (GimpItemFactory       *item_factory,
                                           gpointer               popup_data);
void   gimp_item_factory_popup_with_data  (GimpItemFactory       *factory,
                                           gpointer               popup_data,
                                           GtkWidget             *parent,
                                           GimpMenuPositionFunc   position_func,
                                           gpointer               position_data,
                                           GtkDestroyNotify       popdown_func);

void   gimp_item_factory_set_active       (GtkItemFactory        *factory,
                                           const gchar           *path,
                                           gboolean               state);
void   gimp_item_factories_set_active     (const gchar           *factory_path,
                                           const gchar           *path,
                                           gboolean               state);

void   gimp_item_factory_set_color        (GtkItemFactory        *factory,
                                           const gchar           *path,
                                           const GimpRGB         *color,
                                           gboolean               set_label);
void   gimp_item_factories_set_color      (const gchar           *factory_path,
                                           const gchar           *path,
                                           const GimpRGB         *color,
                                           gboolean               set_label);

void   gimp_item_factory_set_label        (GtkItemFactory        *factory,
                                           const gchar           *path,
                                           const gchar           *label);
void   gimp_item_factories_set_label      (const gchar           *factory_path,
                                           const gchar           *path,
                                           const gchar           *label);

void   gimp_item_factory_set_sensitive    (GtkItemFactory        *factory,
                                           const gchar           *path,
                                           gboolean               sensitive);
void   gimp_item_factories_set_sensitive  (const gchar           *factory_path,
                                           const gchar           *path,
                                           gboolean               sensitive);

void   gimp_item_factory_set_visible      (GtkItemFactory        *factory,
                                           const gchar           *path,
                                           gboolean               visible);
void   gimp_item_factories_set_visible    (const gchar           *factory_path,
                                           const gchar           *path,
                                           gboolean               visible);

void   gimp_item_factory_tearoff_callback (GtkWidget             *widget,
                                           gpointer               data,
                                           guint                  action);

G_END_DECLS

#endif /* __GIMP_ITEM_FACTORY_H__ */
