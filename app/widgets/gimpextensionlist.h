/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpextensionlist.h
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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
 */

#ifndef __GIMP_EXTENSION_LIST_H__
#define __GIMP_EXTENSION_LIST_H__


#define GIMP_TYPE_EXTENSION_LIST            (gimp_extension_list_get_type ())
#define GIMP_EXTENSION_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_EXTENSION_LIST, GimpExtensionList))
#define GIMP_EXTENSION_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_EXTENSION_LIST, GimpExtensionListClass))
#define GIMP_IS_EXTENSION_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_EXTENSION_LIST))
#define GIMP_IS_EXTENSION_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_EXTENSION_LIST))
#define GIMP_EXTENSION_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_EXTENSION_LIST, GimpExtensionListClass))


typedef struct _GimpExtensionListClass    GimpExtensionListClass;
typedef struct _GimpExtensionListPrivate  GimpExtensionListPrivate;

struct _GimpExtensionList
{
  GtkListBox                parent_instance;

  GimpExtensionListPrivate *p;
};

struct _GimpExtensionListClass
{
  GtkListBoxClass           parent_class;

  void         (* extension_activated) (GimpExtensionList *list,
                                        GimpExtension     *extension);
};


GType        gimp_extension_list_get_type     (void) G_GNUC_CONST;

GtkWidget  * gimp_extension_list_new          (GimpExtensionManager *manager);

void         gimp_extension_list_show_system  (GimpExtensionList *list);
void         gimp_extension_list_show_user    (GimpExtensionList *list);
void         gimp_extension_list_show_search  (GimpExtensionList *list,
                                               const gchar       *search_terms);

#endif /* __GIMP_EXTENSION_LIST_H__ */
