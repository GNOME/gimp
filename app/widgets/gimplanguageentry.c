/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplanguageentry.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* GimpLanguageEntry is an entry widget that provides completion on
 * translated language names. It is suited for specifying the language
 * a text is written in.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "gimplanguageentry.h"
#include "gimplanguagestore.h"


enum
{
  PROP_0,
  PROP_MODEL
};

struct _GimpLanguageEntry
{
  GtkEntry       parent_instance;

  GtkListStore  *store;
  gchar         *code;  /*  ISO 639-1 language code  */
};


static void      gimp_language_entry_constructed  (GObject      *object);
static void      gimp_language_entry_finalize     (GObject      *object);
static void      gimp_language_entry_set_property (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void      gimp_language_entry_get_property (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);

static gboolean  gimp_language_entry_language_selected (GtkEntryCompletion *completion,
                                                        GtkTreeModel       *model,
                                                        GtkTreeIter        *iter,
                                                        GimpLanguageEntry  *entry);


G_DEFINE_TYPE (GimpLanguageEntry, gimp_language_entry, GTK_TYPE_ENTRY)

#define parent_class gimp_language_entry_parent_class


static void
gimp_language_entry_class_init (GimpLanguageEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_language_entry_constructed;
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

static void
gimp_language_entry_constructed (GObject *object)
{
  GimpLanguageEntry *entry = GIMP_LANGUAGE_ENTRY (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  if (entry->store)
    {
      GtkEntryCompletion *completion;

      completion = g_object_new (GTK_TYPE_ENTRY_COMPLETION,
                                 "model",             entry->store,
                                 "inline-selection",  TRUE,
                                 NULL);

      /* Note that we must use this function to set the text column,
       * otherwise we won't get a cell renderer for free.
       */
      gtk_entry_completion_set_text_column (completion,
                                            GIMP_LANGUAGE_STORE_LABEL);

      gtk_entry_set_completion (GTK_ENTRY (entry), completion);
      g_object_unref (completion);

      g_signal_connect (completion, "match-selected",
                        G_CALLBACK (gimp_language_entry_language_selected),
                        entry);
    }
}

static void
gimp_language_entry_finalize (GObject *object)
{
  GimpLanguageEntry *entry = GIMP_LANGUAGE_ENTRY (object);

  g_clear_object (&entry->store);

  g_clear_pointer (&entry->code, g_free);

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
      g_return_if_fail (entry->store == NULL);
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

static gboolean
gimp_language_entry_language_selected (GtkEntryCompletion *completion,
                                       GtkTreeModel       *model,
                                       GtkTreeIter        *iter,
                                       GimpLanguageEntry  *entry)
{
  g_free (entry->code);

  gtk_tree_model_get (model, iter,
                      GIMP_LANGUAGE_STORE_CODE, &entry->code,
                      -1);

  return FALSE;
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

const gchar *
gimp_language_entry_get_code (GimpLanguageEntry *entry)
{
  g_return_val_if_fail (GIMP_IS_LANGUAGE_ENTRY (entry), NULL);

  return entry->code;
}

gboolean
gimp_language_entry_set_code (GimpLanguageEntry *entry,
                              const gchar       *code)
{
  GtkTreeIter  iter;

  g_return_val_if_fail (GIMP_IS_LANGUAGE_ENTRY (entry), FALSE);

  g_clear_pointer (&entry->code, g_free);

  if (! code || ! strlen (code))
    {
      gtk_entry_set_text (GTK_ENTRY (entry), "");

      return TRUE;
    }

  if (gimp_language_store_lookup (GIMP_LANGUAGE_STORE (entry->store),
                                  code, &iter))
    {
      gchar *label;

      gtk_tree_model_get (GTK_TREE_MODEL (entry->store), &iter,
                          GIMP_LANGUAGE_STORE_LABEL, &label,
                          GIMP_LANGUAGE_STORE_CODE,  &entry->code,
                          -1);

      gtk_entry_set_text (GTK_ENTRY (entry), label);
      g_free (label);

      return TRUE;
    }

  return FALSE;
}
