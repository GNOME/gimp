/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatranslationstore.c
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

#include "config.h"

#include <locale.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"

#include "widgets-types.h"

#include "ligmahelp.h"
#include "ligmalanguagestore-parser.h"
#include "ligmatranslationstore.h"

#include "ligma-intl.h"

enum
{
  PROP_0,
  PROP_MANUAL_L18N,
  PROP_EMPTY_LABEL
};

struct _LigmaTranslationStoreClass
{
  LigmaLanguageStoreClass  parent_class;
};

struct _LigmaTranslationStore
{
  LigmaLanguageStore  parent_instance;

  gboolean           manual_l18n;
  gchar             *empty_label;
};


static void   ligma_translation_store_constructed  (GObject           *object);
static void   ligma_translation_store_set_property (GObject           *object,
                                                   guint              property_id,
                                                   const GValue      *value,
                                                   GParamSpec        *pspec);
static void   ligma_translation_store_get_property (GObject           *object,
                                                   guint              property_id,
                                                   GValue            *value,
                                                   GParamSpec        *pspec);


G_DEFINE_TYPE (LigmaTranslationStore, ligma_translation_store,
               LIGMA_TYPE_LANGUAGE_STORE)

#define parent_class ligma_translation_store_parent_class


static void
ligma_translation_store_class_init (LigmaTranslationStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_translation_store_constructed;
  object_class->set_property = ligma_translation_store_set_property;
  object_class->get_property = ligma_translation_store_get_property;

  g_object_class_install_property (object_class, PROP_MANUAL_L18N,
                                   g_param_spec_boolean ("manual-l18n", NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_EMPTY_LABEL,
                                   g_param_spec_string ("empty-label", NULL, NULL,
                                                         NULL,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_translation_store_init (LigmaTranslationStore *store)
{
}

static void
ligma_translation_store_constructed (GObject *object)
{
  GHashTable     *lang_list;
  GHashTableIter  lang_iter;
  gpointer        code;
  gpointer        name;
  GList          *sublist = NULL;

  lang_list = ligma_language_store_parser_get_languages (TRUE);
  g_return_if_fail (lang_list != NULL);

  if (LIGMA_TRANSLATION_STORE (object)->manual_l18n)
    sublist = ligma_help_get_installed_languages ();

  g_hash_table_iter_init (&lang_iter, lang_list);

  if (LIGMA_TRANSLATION_STORE (object)->manual_l18n &&
      LIGMA_TRANSLATION_STORE (object)->empty_label)
    {
      LIGMA_LANGUAGE_STORE_GET_CLASS (object)->add (LIGMA_LANGUAGE_STORE (object),
                                                   LIGMA_TRANSLATION_STORE (object)->empty_label,
                                                   "");
    }
  while (g_hash_table_iter_next (&lang_iter, &code, &name))
    {
      if (! LIGMA_TRANSLATION_STORE (object)->manual_l18n ||
          g_list_find_custom (sublist, code, (GCompareFunc) g_strcmp0))
        {
          LIGMA_LANGUAGE_STORE_GET_CLASS (object)->add (LIGMA_LANGUAGE_STORE (object),
                                                       name, code);
        }
    }
  g_list_free_full (sublist, g_free);
}

static void
ligma_translation_store_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  LigmaTranslationStore *store = LIGMA_TRANSLATION_STORE (object);

  switch (property_id)
    {
    case PROP_MANUAL_L18N:
      store->manual_l18n = g_value_get_boolean (value);
      break;
    case PROP_EMPTY_LABEL:
      store->empty_label = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_translation_store_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  LigmaTranslationStore *store = LIGMA_TRANSLATION_STORE (object);

  switch (property_id)
    {
    case PROP_MANUAL_L18N:
      g_value_set_boolean (value, store->manual_l18n);
      break;
    case PROP_EMPTY_LABEL:
      g_value_set_string (value, store->empty_label);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkListStore *
ligma_translation_store_new (gboolean     manual_l18n,
                            const gchar *empty_label)
{
  return g_object_new (LIGMA_TYPE_TRANSLATION_STORE,
                       "manual-l18n", manual_l18n,
                       "empty-label", empty_label,
                       NULL);
}
