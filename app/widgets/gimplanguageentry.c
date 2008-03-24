/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplanguageentry.c
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
#include "gimplanguageentry.h"


enum
{
  PROP_0,
  PROP_MODEL
};


static GObject * gimp_language_entry_constructor  (GType                  type,
                                                   guint                  n_params,
                                                   GObjectConstructParam *params);

static void      gimp_language_entry_finalize     (GObject      *object);
static void      gimp_language_entry_set_property (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void      gimp_language_entry_get_property (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);


G_DEFINE_TYPE (GimpLanguageEntry, gimp_language_entry, GTK_TYPE_ENTRY)

#define parent_class gimp_language_entry_parent_class


static void
gimp_language_entry_class_init (GimpLanguageEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor  = gimp_language_entry_constructor;
  object_class->finalize     = gimp_language_entry_finalize;
  object_class->set_property = gimp_language_entry_set_property;
  object_class->get_property = gimp_language_entry_get_property;

  g_object_class_install_property (object_class, PROP_MODEL,
                                   g_param_spec_object ("model", NULL, NULL,
                                                        GIMP_TYPE_LANGUAGE_STORE,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        GIMP_PARAM_READWRITE));
}

static void
gimp_language_entry_init (GimpLanguageEntry *entry)
{
}

static GObject *
gimp_language_entry_constructor (GType                  type,
                                 guint                  n_params,
                                 GObjectConstructParam *params)
{
  GimpLanguageEntry *entry;
  GObject           *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  entry  = GIMP_LANGUAGE_ENTRY (object);

  if (entry->store)
    {
      GtkEntryCompletion *completion;

      completion = g_object_new (GTK_TYPE_ENTRY_COMPLETION,
                                 "model",             entry->store,
                                 "text-column",       GIMP_LANGUAGE_STORE_LANGUAGE,
                                 "inline-selection",  TRUE,
                                 NULL);

      gtk_entry_set_completion (GTK_ENTRY (entry), completion);
      g_object_unref (completion);
    }

  return object;
}

static void
gimp_language_entry_finalize (GObject *object)
{
  GimpLanguageEntry *entry = GIMP_LANGUAGE_ENTRY (object);

  if (entry->store)
    {
      g_object_unref (entry->store);
      entry->store = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_language_entry_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpLanguageEntry *entry = GIMP_LANGUAGE_ENTRY (object);

  switch (property_id)
    {
    case PROP_MODEL:
      if (entry->store)
        g_object_unref (entry->store);
      entry->store = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_language_entry_get_property (GObject      *object,
                                  guint         property_id,
                                  GValue       *value,
                                  GParamSpec   *pspec)
{
  GimpLanguageEntry *entry = GIMP_LANGUAGE_ENTRY (object);

  switch (property_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, entry->store);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_language_entry_new (void)
{
  GtkWidget    *entry;
  GtkListStore *store;

  store = gimp_language_store_new ();

  entry = g_object_new (GIMP_TYPE_LANGUAGE_ENTRY,
                        "model", store,
                        NULL);

  g_object_unref (store);

  return entry;
}
