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


enum
{
  PROP_0,
  PROP_TRANSLATIONS
};


static void   gimp_language_store_set_property (GObject      *object,
                                                guint         property_id,
                                                const GValue *value,
                                                GParamSpec   *pspec);
static void   gimp_language_store_get_property (GObject      *object,
                                                guint         property_id,
                                                GValue       *value,
                                                GParamSpec   *pspec);

G_DEFINE_TYPE (GimpLanguageStore, gimp_language_store, GTK_TYPE_LIST_STORE)

static void
gimp_language_store_class_init (GimpLanguageStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_language_store_set_property;
  object_class->get_property = gimp_language_store_get_property;

  g_object_class_install_property (object_class, PROP_TRANSLATIONS,
                                   g_param_spec_boolean ("translations",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_language_store_init (GimpLanguageStore *store)
{
  GType column_types[2] = { G_TYPE_STRING, G_TYPE_STRING };

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
                                   G_N_ELEMENTS (column_types), column_types);
}

static void
gimp_language_store_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpLanguageStore *store = GIMP_LANGUAGE_STORE (object);

  switch (property_id)
    {
    case PROP_TRANSLATIONS:
      store->translations = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_language_store_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  GimpLanguageStore *store = GIMP_LANGUAGE_STORE (object);

  switch (property_id)
    {
    case PROP_TRANSLATIONS:
      g_value_set_boolean (value, store->translations);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkListStore *
gimp_language_store_new (gboolean translations)
{
  return g_object_new (GIMP_TYPE_LANGUAGE_STORE,
                       "translations", translations,
                       NULL);
}
