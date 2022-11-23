/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmalanguagestore.h
 * Copyright (C) 2008, 2009  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_LANGUAGE_STORE_H__
#define __LIGMA_LANGUAGE_STORE_H__


enum
{
  LIGMA_LANGUAGE_STORE_LABEL,
  LIGMA_LANGUAGE_STORE_CODE
};


#define LIGMA_TYPE_LANGUAGE_STORE            (ligma_language_store_get_type ())
#define LIGMA_LANGUAGE_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LANGUAGE_STORE, LigmaLanguageStore))
#define LIGMA_LANGUAGE_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_LANGUAGE_STORE, LigmaLanguageStoreClass))
#define LIGMA_IS_LANGUAGE_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LANGUAGE_STORE))
#define LIGMA_IS_LANGUAGE_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_LANGUAGE_STORE))
#define LIGMA_LANGUAGE_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_LANGUAGE_STORE, LigmaLanguageStoreClass))


typedef struct _LigmaLanguageStoreClass  LigmaLanguageStoreClass;

struct _LigmaLanguageStoreClass
{
  GtkListStoreClass  parent_class;

  void (* add) (LigmaLanguageStore *store,
                const gchar       *label,
                const gchar       *code);
};

struct _LigmaLanguageStore
{
  GtkListStore     parent_instance;
};


GType          ligma_language_store_get_type (void) G_GNUC_CONST;

GtkListStore * ligma_language_store_new      (void);

gboolean       ligma_language_store_lookup   (LigmaLanguageStore *store,
                                             const gchar       *code,
                                             GtkTreeIter       *iter);

#endif  /* __LIGMA_LANGUAGE_STORE_H__ */
