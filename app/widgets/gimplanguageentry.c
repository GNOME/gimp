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


static void   gimp_language_entry_set_property (GObject      *object,
                                                guint         property_id,
                                                const GValue *value,
                                                GParamSpec   *pspec);
static void   gimp_language_entry_get_property (GObject      *object,
                                                guint         property_id,
                                                GValue       *value,
                                                GParamSpec   *pspec);


G_DEFINE_TYPE (GimpLanguageEntry, gimp_language_entry, GTK_TYPE_ENTRY)

static void
gimp_language_entry_class_init (GimpLanguageEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_language_entry_set_property;
  object_class->get_property = gimp_language_entry_get_property;
}

static void
gimp_language_entry_init (GimpLanguageEntry *entry)
{
  GtkListStore       *store;
  GtkEntryCompletion *completion;

  store = gimp_language_store_new (FALSE);

  completion = g_object_new (GTK_TYPE_ENTRY_COMPLETION,
                             "model",             store,
                             "text-column",       GIMP_LANGUAGE_STORE_LANGUAGE,
                             "inline-completion", TRUE,
                             NULL);

  gtk_entry_set_completion (GTK_ENTRY (entry), completion);

  g_object_unref (completion);
  g_object_unref (store);
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
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_language_entry_new (void)
{
  return g_object_new (GIMP_TYPE_LANGUAGE_ENTRY, NULL);
}
