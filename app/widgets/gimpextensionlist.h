/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaextensionlist.h
 * Copyright (C) 2018 Jehan <jehan@ligma.org>
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

#ifndef __LIGMA_EXTENSION_LIST_H__
#define __LIGMA_EXTENSION_LIST_H__


#define LIGMA_TYPE_EXTENSION_LIST            (ligma_extension_list_get_type ())
#define LIGMA_EXTENSION_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_EXTENSION_LIST, LigmaExtensionList))
#define LIGMA_EXTENSION_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_EXTENSION_LIST, LigmaExtensionListClass))
#define LIGMA_IS_EXTENSION_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_EXTENSION_LIST))
#define LIGMA_IS_EXTENSION_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_EXTENSION_LIST))
#define LIGMA_EXTENSION_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_EXTENSION_LIST, LigmaExtensionListClass))


typedef struct _LigmaExtensionListClass    LigmaExtensionListClass;
typedef struct _LigmaExtensionListPrivate  LigmaExtensionListPrivate;

struct _LigmaExtensionList
{
  GtkListBox                parent_instance;

  LigmaExtensionListPrivate *p;
};

struct _LigmaExtensionListClass
{
  GtkListBoxClass           parent_class;

  void         (* extension_activated) (LigmaExtensionList *list,
                                        LigmaExtension     *extension);
};


GType        ligma_extension_list_get_type     (void) G_GNUC_CONST;

GtkWidget  * ligma_extension_list_new          (LigmaExtensionManager *manager);

void         ligma_extension_list_show_system  (LigmaExtensionList *list);
void         ligma_extension_list_show_user    (LigmaExtensionList *list);
void         ligma_extension_list_show_search  (LigmaExtensionList *list,
                                               const gchar       *search_terms);

#endif /* __LIGMA_EXTENSION_LIST_H__ */
