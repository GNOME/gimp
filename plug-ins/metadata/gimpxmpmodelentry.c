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


enum
{
  PROP_0,
  PROP_SCHEMA_URI,
  PROP_PROPERTY_NAME,
  PROP_XMPMODEL
};

typedef struct
{
  const gchar *schema_uri;
  const gchar *property_name;
  XMPModel    *xmp_model;
} GimpXmpModelEntryPrivate;


static void   gimp_xmp_model_entry_set_property  (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void   gimp_xmp_model_entry_get_property  (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
static void   gimp_entry_xmp_model_changed       (XMPModel     *xmp_model,
                                                  GtkTreeIter  *iter,
                                                  gpointer     *user_data);

static void   gimp_xmp_model_entry_entry_changed (GimpXmpModelEntry *widget);

const gchar * find_schema_prefix                 (const gchar       *schema_uri);
static void   set_property_edit_icon             (GimpXmpModelEntry *entry,
                                                  GtkTreeIter       *iter);


G_DEFINE_TYPE (GimpXmpModelEntry, gimp_xmp_model_entry, GTK_TYPE_ENTRY)

#define parent_class gimp_xmp_model_entry_parent_class

#define GIMP_XMP_MODEL_ENTRY_GET_PRIVATE(obj) \
  ((GimpXmpModelEntryPrivate *) GIMP_XMP_MODEL_ENTRY (obj)->priv)


static GObject *
gimp_xmp_model_entry_constructor (GType                  type,
                                  guint                  n_params,
                                  GObjectConstructParam *params)
{
  GObject                  *obj;
  GimpXmpModelEntry        *entry;
  GimpXmpModelEntryPrivate *priv;
  gchar                    *signal;

  obj = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  entry = GIMP_XMP_MODEL_ENTRY (obj);

  priv = GIMP_XMP_MODEL_ENTRY_GET_PRIVATE (entry);

  signal = g_strdup_printf ("property-changed::%s:%s",
                            find_schema_prefix (priv->schema_uri),
                            priv->property_name);

  g_signal_connect (priv->xmp_model, signal,
                    G_CALLBACK (gimp_entry_xmp_model_changed),
                    entry);

  g_free (signal);

  return obj;
}

static void
gimp_xmp_model_entry_class_init (GimpXmpModelEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor  = gimp_xmp_model_entry_constructor;
  object_class->set_property = gimp_xmp_model_entry_set_property;
  object_class->get_property = gimp_xmp_model_entry_get_property;

  g_object_class_install_property (object_class, PROP_SCHEMA_URI,
                                   g_param_spec_string ("schema-uri", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PROPERTY_NAME,
                                   g_param_spec_string ("property-name", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_XMPMODEL,
                                   g_param_spec_object ("xmp-model", NULL, NULL,
                                                        GIMP_TYPE_XMP_MODEL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        GIMP_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GimpXmpModelEntryPrivate));
}

static void
gimp_xmp_model_entry_init (GimpXmpModelEntry *entry)
{
  entry->priv = G_TYPE_INSTANCE_GET_PRIVATE (entry,
                                             GIMP_TYPE_XMP_MODEL_ENTRY,
                                             GimpXmpModelEntryPrivate);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (gimp_xmp_model_entry_entry_changed),
                    NULL);
}

static void
gimp_xmp_model_entry_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpXmpModelEntryPrivate *priv = GIMP_XMP_MODEL_ENTRY_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_SCHEMA_URI:
      priv->schema_uri = g_value_dup_string (value);
      break;

    case PROP_PROPERTY_NAME:
      priv->property_name = g_value_dup_string (value);
      break;

    case PROP_XMPMODEL:
      priv->xmp_model = g_value_dup_object (value);
      break;

    default:
       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
       break;
    }
}

static void
gimp_xmp_model_entry_get_property (GObject      *object,
                                   guint         property_id,
                                   GValue       *value,
                                   GParamSpec   *pspec)
{
  GimpXmpModelEntryPrivate *priv = GIMP_XMP_MODEL_ENTRY_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_SCHEMA_URI:
      g_value_set_string (value, priv->schema_uri);
      break;

    case PROP_PROPERTY_NAME:
      g_value_set_string (value, priv->property_name);
      break;

    case PROP_XMPMODEL:
      g_value_set_object (value, priv->xmp_model);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_entry_xmp_model_changed (XMPModel     *xmp_model,
                              GtkTreeIter  *iter,
                              gpointer     *user_data)
{
  GimpXmpModelEntry        *entry = GIMP_XMP_MODEL_ENTRY (user_data);
  GimpXmpModelEntryPrivate *priv  = GIMP_XMP_MODEL_ENTRY_GET_PRIVATE (entry);
  const gchar              *tree_value;
  const gchar              *property_name;
  GdkPixbuf                *icon;

  gtk_tree_model_get (GTK_TREE_MODEL (xmp_model), iter,
                      COL_XMP_NAME,      &property_name,
                      COL_XMP_VALUE,     &tree_value,
                      COL_XMP_EDIT_ICON, &icon,
                      -1);

  if (! strcmp (priv->property_name, property_name))
    gtk_entry_set_text (GTK_ENTRY (entry), tree_value);

  if (icon == NULL)
    set_property_edit_icon (entry, iter);

  return;
}


static void
gimp_xmp_model_entry_entry_changed (GimpXmpModelEntry *entry)
{
  GimpXmpModelEntryPrivate *priv = GIMP_XMP_MODEL_ENTRY_GET_PRIVATE (entry);

  xmp_model_set_scalar_property (priv->xmp_model,
                                 priv->schema_uri,
                                 priv->property_name,
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
  GimpXmpModelEntryPrivate *priv  = GIMP_XMP_MODEL_ENTRY_GET_PRIVATE (entry);
  GdkPixbuf                *icon;
  gboolean                  editable;

  gtk_tree_model_get (GTK_TREE_MODEL (priv->xmp_model), iter,
                      COL_XMP_EDITABLE, &editable,
                      COL_XMP_EDIT_ICON, &icon,
                      -1);

  if (editable == XMP_AUTO_UPDATE)
    {
      icon = gtk_widget_render_icon (GTK_WIDGET (entry), GIMP_STOCK_WILBER,
                                     GTK_ICON_SIZE_MENU, NULL);
      gtk_tree_store_set (GTK_TREE_STORE (priv->xmp_model), iter,
                          COL_XMP_EDIT_ICON, icon,
                          -1);
    }
  else if (editable == TRUE)
    {
      icon = gtk_widget_render_icon (GTK_WIDGET (entry), GTK_STOCK_EDIT,
                                     GTK_ICON_SIZE_MENU, NULL);
      gtk_tree_store_set (GTK_TREE_STORE (priv->xmp_model), iter,
                          COL_XMP_EDIT_ICON, icon,
                          -1);
    }

  return;
}
