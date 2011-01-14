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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "gimptranslationstore.h"

#include "gimp-intl.h"


struct _GimpTranslationStoreClass
{
  GimpLanguageStoreClass  parent_class;
};

struct _GimpTranslationStore
{
  GimpLanguageStore  parent_instance;

  GHashTable        *map;
};


static void   gimp_translation_store_constructed (GObject              *object);

static void   gimp_translation_store_add         (GimpLanguageStore    *store,
                                                  const gchar          *lang,
                                                  const gchar          *code);

static void   gimp_translation_store_populate    (GimpTranslationStore *store);


G_DEFINE_TYPE (GimpTranslationStore, gimp_translation_store,
               GIMP_TYPE_LANGUAGE_STORE)

#define parent_class gimp_translation_store_parent_class


static void
gimp_translation_store_class_init (GimpTranslationStoreClass *klass)
{
  GObjectClass           *object_class = G_OBJECT_CLASS (klass);
  GimpLanguageStoreClass *store_class  = GIMP_LANGUAGE_STORE_CLASS (klass);

  object_class->constructed = gimp_translation_store_constructed;

  store_class->add          = gimp_translation_store_add;
}

static void
gimp_translation_store_init (GimpTranslationStore *store)
{
  store->map = g_hash_table_new_full (g_str_hash, g_str_equal,
                                      (GDestroyNotify) g_free,
                                      (GDestroyNotify) g_free);
}

static void
gimp_translation_store_constructed (GObject *object)
{
  GimpTranslationStore *store = GIMP_TRANSLATION_STORE (object);
  gchar                *label;

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_translation_store_populate (store);

  /*  we don't need the map any longer  */
  g_hash_table_unref (store->map);
  store->map = NULL;

  /*  add special entries for system locale and for "C"  */
  GIMP_LANGUAGE_STORE_CLASS (parent_class)->add (GIMP_LANGUAGE_STORE (store),
                                                 _("System Language"),
                                                 NULL);
  label = g_strdup_printf ("%s [%s]", _("English"), "en_US");
  GIMP_LANGUAGE_STORE_CLASS (parent_class)->add (GIMP_LANGUAGE_STORE (store),
                                                 label, "en_US");
  g_free (label);
}

static const gchar *
gimp_translation_store_map (GimpTranslationStore *store,
                            const gchar          *locale)
{
  const gchar *lang;

  /*  A locale directory name is typically of the form language[_territory]  */
  lang = g_hash_table_lookup (store->map, locale);

  if (! lang)
    {
      /*  strip off the territory suffix  */
      const gchar *delimiter = strchr (locale, '_');

      if (delimiter)
        {
          gchar *copy;

          copy = g_strndup (locale, delimiter - locale);
          lang = g_hash_table_lookup (store->map, copy);
          g_free (copy);
        }
    }

  return lang;
}

static void
gimp_translation_store_populate (GimpTranslationStore *store)
{
  /*  FIXME: this should better be done asynchronously  */
  GDir        *dir = g_dir_open (gimp_locale_directory (), 0, NULL);
  const gchar *dirname;

  if (! dir)
    return;

  while ((dirname = g_dir_read_name (dir)) != NULL)
    {
      gchar *filename = g_build_filename (gimp_locale_directory (),
                                          dirname,
                                          "LC_MESSAGES",
                                          GETTEXT_PACKAGE ".mo",
                                          NULL);
      if (g_file_test (filename, G_FILE_TEST_EXISTS))
        {
          const gchar *lang = gimp_translation_store_map (store, dirname);

          if (lang)
            {
              GimpLanguageStore *language_store = GIMP_LANGUAGE_STORE (store);
              gchar             *label;

              label = g_strdup_printf ("%s [%s]", lang, dirname);

              GIMP_LANGUAGE_STORE_CLASS (parent_class)->add (language_store,
                                                             label, dirname);
              g_free (label);
            }
        }

      g_free (filename);
    }

  g_dir_close (dir);
}

static void
gimp_translation_store_add (GimpLanguageStore *store,
                            const gchar       *lang,
                            const gchar       *code)
{
  g_hash_table_replace (GIMP_TRANSLATION_STORE (store)->map,
                        g_strdup (code),
                        g_strdup (lang));
}

GtkListStore *
gimp_translation_store_new (void)
{
  return g_object_new (GIMP_TYPE_TRANSLATION_STORE, NULL);
}
