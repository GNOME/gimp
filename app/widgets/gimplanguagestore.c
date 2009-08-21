/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplanguagestore.c
 * Copyright (C) 2008  Sven Neumann <sven@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

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

gboolean
gimp_language_store_lookup (GimpLanguageStore *store,
                            const gchar       *code,
                            GtkTreeIter       *iter)
{
  GtkTreeModel *model;
  const gchar  *hyphen;
  gint          len;
  gboolean      iter_valid;

  g_return_val_if_fail (GIMP_IS_LANGUAGE_STORE (store), FALSE);
  g_return_val_if_fail (code != NULL, FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  /*  We accept the code in RFC-3066 format here and only look at what's
   *  before the first hyphen.
   */
  hyphen = strchr (code, '-');

  if (hyphen)
    len = hyphen - code;
  else
    len = strlen (code);

  model = GTK_TREE_MODEL (store);

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      gchar *value;

      gtk_tree_model_get (model, iter,
                          GIMP_LANGUAGE_STORE_ISO_639_1, &value,
                          -1);

      if (strncmp (code, value, len) == 0)
        {
          g_free (value);
          break;
        }

      g_free (value);
    }

  return iter_valid;
}
