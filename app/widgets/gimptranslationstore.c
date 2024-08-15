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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <locale.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "gimphelp.h"
#ifdef HAVE_ISO_CODES
#include "gimplanguagestore-data.h"
#endif
#include "gimptranslationstore.h"

#include "gimp-intl.h"

static const gchar *system_lang = NULL;

enum
{
  PROP_0,
  PROP_MANUAL_L18N,
  PROP_EMPTY_LABEL
};

struct _GimpTranslationStoreClass
{
  GimpLanguageStoreClass  parent_class;
};

struct _GimpTranslationStore
{
  GimpLanguageStore  parent_instance;

  gboolean           manual_l18n;
  gchar             *empty_label;
};


static void   gimp_translation_store_constructed  (GObject           *object);
static void   gimp_translation_store_set_property (GObject           *object,
                                                   guint              property_id,
                                                   const GValue      *value,
                                                   GParamSpec        *pspec);
static void   gimp_translation_store_get_property (GObject           *object,
                                                   guint              property_id,
                                                   GValue            *value,
                                                   GParamSpec        *pspec);


G_DEFINE_TYPE (GimpTranslationStore, gimp_translation_store,
               GIMP_TYPE_LANGUAGE_STORE)

#define parent_class gimp_translation_store_parent_class


static void
gimp_translation_store_class_init (GimpTranslationStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_translation_store_constructed;
  object_class->set_property = gimp_translation_store_set_property;
  object_class->get_property = gimp_translation_store_get_property;

  g_object_class_install_property (object_class, PROP_MANUAL_L18N,
                                   g_param_spec_boolean ("manual-l18n", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_EMPTY_LABEL,
                                   g_param_spec_string ("empty-label", NULL, NULL,
                                                         NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_translation_store_init (GimpTranslationStore *store)
{
}

static void
gimp_translation_store_constructed (GObject *object)
{
  GList *sublist = NULL;

  if (GIMP_TRANSLATION_STORE (object)->manual_l18n)
    sublist = gimp_help_get_installed_languages ();

  if (GIMP_TRANSLATION_STORE (object)->manual_l18n &&
      GIMP_TRANSLATION_STORE (object)->empty_label)
    {
      GIMP_LANGUAGE_STORE_GET_CLASS (object)->add (GIMP_LANGUAGE_STORE (object),
                                                   GIMP_TRANSLATION_STORE (object)->empty_label,
                                                   "");
    }
#ifdef HAVE_ISO_CODES
  for (gint i = 0; i < GIMP_L10N_LANGS_SIZE; i++)
    {
      GimpLanguageDef def = GimpL10nLanguages[i];

      if (! GIMP_TRANSLATION_STORE (object)->manual_l18n ||
          g_list_find_custom (sublist, def.code, (GCompareFunc) g_strcmp0))
        GIMP_LANGUAGE_STORE_GET_CLASS (object)->add (GIMP_LANGUAGE_STORE (object),
                                                     def.name, def.code);
    }

  g_return_if_fail (system_lang != NULL);

  GIMP_LANGUAGE_STORE_GET_CLASS (object)->add (GIMP_LANGUAGE_STORE (object),
                                               system_lang, "");
#endif

  g_list_free_full (sublist, g_free);
}

static void
gimp_translation_store_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpTranslationStore *store = GIMP_TRANSLATION_STORE (object);

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
gimp_translation_store_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpTranslationStore *store = GIMP_TRANSLATION_STORE (object);

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

void
gimp_translation_store_initialize (const gchar *system_lang_l10n)
{
  g_return_if_fail (system_lang_l10n != NULL);

  if (system_lang != NULL)
    {
      g_warning ("gimp_translation_store_initialize() must be run only once.");
      return;
    }

  system_lang = system_lang_l10n;
}

GtkListStore *
gimp_translation_store_new (gboolean     manual_l18n,
                            const gchar *empty_label)
{
  return g_object_new (GIMP_TYPE_TRANSLATION_STORE,
                       "manual-l18n", manual_l18n,
                       "empty-label", empty_label,
                       NULL);
}
