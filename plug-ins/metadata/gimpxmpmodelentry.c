/* gimpxmpmodelentry.c - custom entry widget linked to the xmp model
 *
 * Copyright (C) 2009, Róman Joost <romanofski@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include <gtk/gtk.h>

#include "xmp-schemas.h"
#include "xmp-model.h"

#include "gimpxmpmodelentry.h"


static void         gimp_xmp_model_entry_init       (GimpXMPModelEntry *entry);

static void         gimp_entry_xmp_model_changed    (XMPModel     *xmp_model,
                                                     GtkTreeIter  *iter,
                                                     gpointer     *user_data);

static void         entry_changed                   (GimpXMPModelEntry *widget);

const gchar*        find_schema_prefix              (const gchar *schema_uri);



G_DEFINE_TYPE (GimpXMPModelEntry, gimp_xmp_model_entry, GTK_TYPE_ENTRY)

#define parent_class gimp_xmp_model_entry_parent_class


static void
gimp_xmp_model_entry_class_init (GimpXMPModelEntryClass *klass)
{
}

static void
gimp_xmp_model_entry_init (GimpXMPModelEntry *entry)
{
  entry->schema_uri     = NULL;
  entry->property_name  = NULL;
  entry->xmp_model      = NULL;

  g_signal_connect (entry, "changed",
                    G_CALLBACK (entry_changed), NULL);
}

/**
 * gimp_xmp_model_entry_new:
 * @schema_uri: the XMP schema_uri this entry belongs to
 * @property_name: the property name this entry changes in the XMP model
 * @xmp_model: the xmp_model for itself
 *
 * Returns: a new #GimpXMPModelEntry widget
 *
 **/
GtkWidget*
gimp_xmp_model_entry_new (const gchar *schema_uri,
                          const gchar *property_name,
                          XMPModel    *xmp_model)
{
  GimpXMPModelEntry *entry;
  const gchar       *value;
  const gchar       *signal;
  const gchar       *signal_detail;
  const gchar       *schema_prefix;

  entry = g_object_new (GIMP_TYPE_XMP_MODEL_ENTRY, NULL);
  entry->schema_uri     = schema_uri;
  entry->property_name  = property_name;
  entry->xmp_model      = xmp_model;

  value = xmp_model_get_scalar_property (xmp_model, schema_uri, property_name);
  if (value != NULL)
    gtk_entry_set_text (GTK_ENTRY (entry), value);
  else
    gtk_entry_set_text (GTK_ENTRY (entry), "");

  schema_prefix = find_schema_prefix (schema_uri);
  signal_detail = g_strjoin (":", schema_prefix, property_name, NULL);
  signal = g_strjoin ("::", "property-changed", signal_detail, NULL);

  g_signal_connect (xmp_model, signal,
                    G_CALLBACK (gimp_entry_xmp_model_changed),
                    entry);
  return GTK_WIDGET (entry);
}

static void
gimp_entry_xmp_model_changed (XMPModel     *xmp_model,
                              GtkTreeIter  *iter,
                              gpointer     *user_data)
{
  GimpXMPModelEntry *entry = GIMP_XMP_MODEL_ENTRY (user_data);
  const gchar       *tree_value;
  const gchar       *property_name;

  gtk_tree_model_get (GTK_TREE_MODEL (xmp_model), iter,
                      COL_XMP_NAME, &property_name,
                      COL_XMP_VALUE, &tree_value,
                      -1);
  if (! strcmp (entry->property_name, property_name))
    gtk_entry_set_text (GTK_ENTRY (entry), tree_value);

  return;
}


static void
entry_changed (GimpXMPModelEntry *entry)
{
  xmp_model_set_scalar_property (entry->xmp_model,
                                 entry->schema_uri,
                                 entry->property_name,
                                 gtk_entry_get_text (GTK_ENTRY (entry)));

}

/* find the schema prefix for the given URI */
const gchar*
find_schema_prefix (const gchar *schema_uri)
{
  int i;

  for (i = 0; xmp_schemas[i].uri != NULL; ++i)
  {
    if (! strcmp (xmp_schemas[i].uri, schema_uri))
      return xmp_schemas[i].prefix;
  }
  return NULL;
}
