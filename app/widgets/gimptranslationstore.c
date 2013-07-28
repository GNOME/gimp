/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptranslationstore.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <locale.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "gimplanguagestore-parser.h"
#include "gimptranslationstore.h"


struct _GimpTranslationStoreClass
{
  GimpLanguageStoreClass  parent_class;
};

struct _GimpTranslationStore
{
  GimpLanguageStore  parent_instance;
};


static void   gimp_translation_store_constructed (GObject              *object);


G_DEFINE_TYPE (GimpTranslationStore, gimp_translation_store,
               GIMP_TYPE_LANGUAGE_STORE)

#define parent_class gimp_translation_store_parent_class


static void
gimp_translation_store_class_init (GimpTranslationStoreClass *klass)
{
  GObjectClass           *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = gimp_translation_store_constructed;
}

static void
gimp_translation_store_init (GimpTranslationStore *store)
{
}

static void
gimp_translation_store_constructed (GObject *object)
{
  GHashTable     *lang_list;
  GHashTableIter  lang_iter;
  gpointer        code;
  gpointer        name;

  lang_list = gimp_language_store_parser_get_languages (TRUE);
  g_return_if_fail (lang_list != NULL);

  g_hash_table_iter_init (&lang_iter, lang_list);

  while (g_hash_table_iter_next (&lang_iter, &code, &name))
    GIMP_LANGUAGE_STORE_GET_CLASS (object)->add (GIMP_LANGUAGE_STORE (object),
                                                 name, code);
}

GtkListStore *
gimp_translation_store_new (void)
{
  return g_object_new (GIMP_TYPE_TRANSLATION_STORE, NULL);
}
