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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "xmp-schemas.h"
#include "xmp-model.h"

#include "gimpxmpmodelentry.h"


static void         gimp_xmp_model_entry_init           (GimpXmpModelEntry *entry);

static void         gimp_entry_xmp_model_changed        (XMPModel     *xmp_model,
                                                         GtkTreeIter  *iter,
                                                         gpointer     *user_data);

static void         gimp_xmp_model_entry_entry_changed  (GimpXmpModelEntry *widget);

const gchar*        find_schema_prefix                  (const gchar *schema_uri);

static void         set_property_edit_icon              (GimpXmpModelEntry *entry,
                                                         GtkTreeIter       *iter);


G_DEFINE_TYPE (GimpXmpModelEntry, gimp_xmp_model_entry, GTK_TYPE_ENTRY)

#define parent_class gimp_xmp_model_entry_parent_class
#define GIMP_XMP_MODEL_ENTRY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIMP_TYPE_XMP_MODEL_ENTRY, GimpXmpModelEntryPrivate))

static void
gimp_xmp_model_entry_class_init (GimpXmpModelEntryClass *klass)
{
  g_type_class_add_private (klass, sizeof (GimpXmpModelEntryPrivate));
}

static void
gimp_xmp_model_entry_init (GimpXmpModelEntry *entry)
{
  entry->p = GIMP_XMP_MODEL_ENTRY_GET_PRIVATE (entry);

  entry->p->schema_uri     = NULL;
  entry->p->property_name  = NULL;
  entry->p->xmp_model      = NULL;

  g_signal_connect (entry, "changed",
                    G_CALLBACK (gimp_xmp_model_entry_entry_changed), NULL);
}

/**
 * gimp_xmp_model_entry_new:
 * @schema_uri: the XMP schema_uri this entry belongs to
 * @property_name: the property name this entry changes in the XMP model
 * @xmp_model: the xmp_model for itself
 *
 * Returns: a new #GimpXmpModelEntry widget
 *
 **/
GtkWidget*
gimp_xmp_model_entry_new (const gchar *schema_uri,
                          const gchar *property_name,
                          XMPModel    *xmp_model)
{
  GimpXmpModelEntry *entry;
  const gchar       *value;
  const gchar       *signal;
  const gchar       *signal_detail;
  const gchar       *schema_prefix;

  entry = g_object_new (GIMP_TYPE_XMP_MODEL_ENTRY, NULL);
  entry->p->schema_uri     = schema_uri;
  entry->p->property_name  = property_name;
  entry->p->xmp_model      = xmp_model;

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
  GimpXmpModelEntry *entry = GIMP_XMP_MODEL_ENTRY (user_data);
  const gchar       *tree_value;
  const gchar       *property_name;
  GdkPixbuf         *icon;

  gtk_tree_model_get (GTK_TREE_MODEL (xmp_model), iter,
                      COL_XMP_NAME, &property_name,
                      COL_XMP_VALUE, &tree_value,
                      COL_XMP_EDIT_ICON, &icon,
                      -1);
  if (! strcmp (entry->p->property_name, property_name))
    gtk_entry_set_text (GTK_ENTRY (entry), tree_value);

  if (icon == NULL)
    set_property_edit_icon (entry, iter);

  return;
}


static void
gimp_xmp_model_entry_entry_changed (GimpXmpModelEntry *entry)
{
  xmp_model_set_scalar_property (entry->p->xmp_model,
                                 entry->p->schema_uri,
                                 entry->p->property_name,
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

static void
set_property_edit_icon (GimpXmpModelEntry *entry,
                        GtkTreeIter       *iter)
{
  GdkPixbuf    *icon;
  gboolean      editable;

  gtk_tree_model_get (GTK_TREE_MODEL (entry->p->xmp_model), iter,
                      COL_XMP_EDITABLE, &editable,
                      COL_XMP_EDIT_ICON, &icon,
                      -1);

  if (editable == XMP_AUTO_UPDATE)
    {
      icon = gtk_widget_render_icon (GTK_WIDGET (entry), GIMP_STOCK_WILBER,
                                     GTK_ICON_SIZE_MENU, NULL);
      gtk_tree_store_set (GTK_TREE_STORE (entry->p->xmp_model), iter,
                          COL_XMP_EDIT_ICON, icon,
                          -1);
    }
  else if (editable == TRUE)
    {
      icon = gtk_widget_render_icon (GTK_WIDGET (entry), GTK_STOCK_EDIT,
                                     GTK_ICON_SIZE_MENU, NULL);
      gtk_tree_store_set (GTK_TREE_STORE (entry->p->xmp_model), iter,
                          COL_XMP_EDIT_ICON, icon,
                          -1);
    }

  return;
}
