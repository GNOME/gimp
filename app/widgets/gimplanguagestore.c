/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplanguagestore.c
 * Copyright (C) 2008  Sven Neumann <sven@gimp.org>
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

#include "widgets-types.h"

#include "gimplanguagestore.h"
#include "gimplanguagestore-parser.h"


G_DEFINE_TYPE (GimpLanguageStore, gimp_language_store, GTK_TYPE_LIST_STORE)

static void
gimp_language_store_class_init (GimpLanguageStoreClass *klass)
{
}

static void
gimp_language_store_init (GimpLanguageStore *store)
{
  GType column_types[2] = { G_TYPE_STRING, G_TYPE_STRING };

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
                                   G_N_ELEMENTS (column_types), column_types);

  gimp_language_store_populate (store, NULL);
}

GtkListStore *
gimp_language_store_new (void)
{
  return g_object_new (GIMP_TYPE_LANGUAGE_STORE, NULL);
}

void
gimp_language_store_add (GimpLanguageStore *store,
                         const gchar       *lang,
                         const gchar       *code)
{
  GtkTreeIter  iter;

  g_return_if_fail (GIMP_IS_LANGUAGE_STORE (store));
  g_return_if_fail (lang != NULL && code != NULL);

  gtk_list_store_append (GTK_LIST_STORE (store), &iter);

  gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                      GIMP_LANGUAGE_STORE_LANGUAGE,  lang,
                      GIMP_LANGUAGE_STORE_ISO_639_1, code,
                      -1);
}
