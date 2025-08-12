/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplanguagestore.h
 * Copyright (C) 2008, 2009  Sven Neumann <sven@gimp.org>
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

#pragma once


enum
{
  GIMP_LANGUAGE_STORE_LABEL,
  GIMP_LANGUAGE_STORE_CODE
};


#define GIMP_TYPE_LANGUAGE_STORE            (gimp_language_store_get_type ())
#define GIMP_LANGUAGE_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LANGUAGE_STORE, GimpLanguageStore))
#define GIMP_LANGUAGE_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LANGUAGE_STORE, GimpLanguageStoreClass))
#define GIMP_IS_LANGUAGE_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LANGUAGE_STORE))
#define GIMP_IS_LANGUAGE_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LANGUAGE_STORE))
#define GIMP_LANGUAGE_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LANGUAGE_STORE, GimpLanguageStoreClass))


typedef struct _GimpLanguageStoreClass  GimpLanguageStoreClass;

struct _GimpLanguageStoreClass
{
  GtkListStoreClass  parent_class;

  void (* add) (GimpLanguageStore *store,
                const gchar       *label,
                const gchar       *code);
};

struct _GimpLanguageStore
{
  GtkListStore     parent_instance;
};


GType          gimp_language_store_get_type (void) G_GNUC_CONST;

GtkListStore * gimp_language_store_new      (void);

gboolean       gimp_language_store_lookup   (GimpLanguageStore *store,
                                             const gchar       *code,
                                             GtkTreeIter       *iter);
