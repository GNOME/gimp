/* xmp-model.c - treeview model for XMP metadata
 *
 * Copyright (C) 2004-2005, Raphaël Quinet <raphael@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"

#include "xmp-schemas.h"
#include "xmp-parse.h"
#include "xmp-model.h"

/* The main part of the XMPModel structure is the GtkTreeStore in
 * which all references to XMP properties are stored.  In the tree,
 * the elements at the root level are the schemas (namespaces) and the
 * children of the schemas are the XMP properties.
 *
 * If the XMP file contains a schema that is not part of the XMP
 * specification or a known extension (e.g., IPTC Core), it will be
 * included in the custom_schemas list and the corresponding element
 * in the tree will get a reference to that list element instead of a
 * reference to one of the static schema definitions found in
 * xmp-schemas.c.  Same for custom properties inside a known or custom
 * schema.
 */
struct _XMPModel
{
  GtkTreeStore *treestore;
  GSList       *custom_schemas;
  GSList       *custom_properties;

  XMPSchema    *cached_schema;
  GtkTreeIter   cached_schema_iter;
};

/**
 * xmp_model_new:
 *
 * Return value: a new #XMPModel.
 **/
XMPModel *
xmp_model_new (void)
{
  XMPModel *xmp_model;

  xmp_model = g_new (XMPModel, 1);
  /* columns defined by the XMPModelColumns enum */
  xmp_model->treestore =
    gtk_tree_store_new (XMP_MODEL_NUM_COLUMNS,
                        G_TYPE_STRING,   /* COL_XMP_NAME */
                        G_TYPE_STRING,   /* COL_XMP_VALUE */
                        G_TYPE_POINTER,  /* COL_XMP_VALUE_RAW */
                        G_TYPE_POINTER,  /* COL_XMP_TYPE_XREF */
                        G_TYPE_POINTER,  /* COL_XMP_WIDGET_XREF */
                        G_TYPE_INT,      /* COL_XMP_EDITABLE */
                        GDK_TYPE_PIXBUF, /* COL_XMP_EDIT_ICON */
                        G_TYPE_BOOLEAN,  /* COL_XMP_VISIBLE */
                        G_TYPE_INT,      /* COL_XMP_WEIGHT */
                        G_TYPE_BOOLEAN   /* COL_XMP_WEIGHT_SET */
                        );
  xmp_model->custom_schemas = NULL;
  xmp_model->custom_properties = NULL;
  xmp_model->cached_schema = NULL;
  return xmp_model;
}

/**
 * xmp_model_free:
 * @xmp_model: an #XMPModel
 *
 * Frees an #XMPModel.
 **/
void
xmp_model_free (XMPModel *xmp_model)
{
  GtkTreeModel  *model;
  GtkTreeIter    iter;
  GtkTreeIter    child;
  gchar        **value_array;
  gint           i;

  g_return_if_fail (xmp_model != NULL);
  /* we used XMP_FLAG_DEFER_VALUE_FREE for the parser, so now we must free
     all value arrays */
  model = xmp_model_get_tree_model (xmp_model);
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter))
    {
      do
        {
          if (gtk_tree_model_iter_children (model, &child, &iter))
            {
              gchar **last_value_array = NULL;

              do
                {
                  gtk_tree_model_get (model, &child,
                                      COL_XMP_VALUE_RAW, &value_array,
                                      -1);
                  if (value_array != last_value_array)
                    {
                      /* FIXME: this does not free everything */
                      for (i = 0; value_array[i] != NULL; i++)
                        g_free (value_array[i]);
                      g_free (value_array);
                    }
                  last_value_array = value_array;
                }
              while (gtk_tree_model_iter_next (model, &child));
            }
        }
      while (gtk_tree_model_iter_next (model, &iter));
    }
  g_object_unref (xmp_model->treestore);
  /* FIXME: free custom schemas */
  g_free (xmp_model);
}

/**
 * xmp_model_is_empty:
 * @xmp_model: an #XMPModel
 *
 * Return value: %TRUE if @xmp_model is empty (no shemas, no properties)
 **/
gboolean
xmp_model_is_empty (XMPModel *xmp_model)
{
  GtkTreeIter iter;

  g_return_val_if_fail (xmp_model != NULL, TRUE);
  if ((xmp_model->custom_schemas != NULL)
      || (xmp_model->custom_properties != NULL))
    return FALSE;
  return !gtk_tree_model_get_iter_first (GTK_TREE_MODEL (xmp_model->treestore),
                                         &iter);
}

/* check if the given schema_uri matches a known schema; else return NULL */
static XMPSchema *
find_xmp_schema (XMPModel    *xmp_model,
                 const gchar *schema_uri)
{
  int          i;
  GSList      *list;
  const gchar *c;

  /* check if we know about this schema (exact match for URI) */
  for (i = 0; xmp_schemas[i].uri != NULL; ++i)
    {
      if (! strcmp (xmp_schemas[i].uri, schema_uri))
        {
#ifdef DEBUG_XMP_MODEL
          if (xmp_schemas[i].name != NULL)
            g_print ("%s \t[%s]\n", xmp_schemas[i].name, xmp_schemas[i].uri);
          else
            g_print ("(no name) \t[%s]\n", xmp_schemas[i].uri);
#endif
          return &(xmp_schemas[i]);
        }
    }
  /* this is not a standard shema; now check the custom schemas */
  for (list = xmp_model->custom_schemas; list != NULL; list = list->next)
    {
      if (! strcmp (((XMPSchema *)(list->data))->uri, schema_uri))
        {
#ifdef DEBUG_XMP_MODEL
          g_print ("CUSTOM %s \t[%s]\n",
                  ((XMPSchema *)(list->data))->name,
                  ((XMPSchema *)(list->data))->uri);
#endif
          return (XMPSchema *)(list->data);
        }
    }
  /* now check for some common errors and results of bad encoding: */
  /* - check for "http:" without "//", or missing "http://" */
  for (i = 0; xmp_schemas[i].uri != NULL; ++i)
    {
      if (g_str_has_prefix (xmp_schemas[i].uri, "http://")
          && ((! strcmp (xmp_schemas[i].uri + 7, schema_uri))
              || (g_str_has_prefix (schema_uri, "http:")
                  && ! strcmp (xmp_schemas[i].uri + 7, schema_uri + 5))
              ))
        {
#ifdef DEBUG_XMP_MODEL
          g_print ("%s \t~~~[%s]\n", xmp_schemas[i].name, xmp_schemas[i].uri);
#endif
          return &(xmp_schemas[i]);
        }
    }
  /* - check for errors such as "name (uri)" or "name (prefix, uri)"  */
  for (c = schema_uri; *c; c++)
    if ((*c == '(') || (*c == ' ') || (*c == ','))
      {
        int len;

        c++;
        while (*c == ' ')
          c++;
        if (! *c)
          break;
        for (len = 1; c[len]; len++)
          if ((c[len] == ')') || (c[len] == ' '))
            break;
        for (i = 0; xmp_schemas[i].uri != NULL; ++i)
          {
            if (! strncmp (xmp_schemas[i].uri, c, len))
              {
#ifdef DEBUG_XMP_MODEL
                g_print ("%s \t~~~[%s]\n", xmp_schemas[i].name,
                         xmp_schemas[i].uri);
#endif
                return &(xmp_schemas[i]);
              }
          }
      }
#ifdef DEBUG_XMP_MODEL
  g_print ("Unknown schema URI %s\n", schema_uri);
#endif
  return NULL;
}

/* check if the given prefix matches a known schema; else return NULL */
static XMPSchema *
find_xmp_schema_prefix (XMPModel    *xmp_model,
                        const gchar *prefix)
{
  int     i;
  GSList *list;

  for (i = 0; xmp_schemas[i].uri != NULL; ++i)
    if (! strcmp (xmp_schemas[i].prefix, prefix))
      return &(xmp_schemas[i]);
  for (list = xmp_model->custom_schemas; list != NULL; list = list->next)
    if (! strcmp (((XMPSchema *)(list->data))->prefix, prefix))
      return (XMPSchema *)(list->data);
  return NULL;
}

/* make the next lookup a bit faster if the tree is not modified */
static void
cache_iter_for_schema (XMPModel    *xmp_model,
                      XMPSchema   *schema,
                      GtkTreeIter *iter)
{
  xmp_model->cached_schema = schema;
  if (iter != NULL)
    memcpy (&(xmp_model->cached_schema_iter), iter, sizeof (GtkTreeIter));
}

/* find the GtkTreeIter for the given schema and return TRUE if the schema was
   found in the tree; else return FALSE */
static gboolean
find_iter_for_schema (XMPModel    *xmp_model,
                      XMPSchema   *schema,
                      GtkTreeIter *iter)
{
  XMPSchema *schema_xref;

  /* common case: return the cached iter */
  if (schema == xmp_model->cached_schema)
    {
      memcpy (iter, &(xmp_model->cached_schema_iter), sizeof (GtkTreeIter));
      return TRUE;
    }
  /* find where this schema has been stored in the tree */
  if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (xmp_model->treestore),
                                       iter))
    return FALSE;
  do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (xmp_model->treestore), iter,
                          COL_XMP_TYPE_XREF, &schema_xref,
                          -1);
      if (schema_xref == schema)
        {
          cache_iter_for_schema (xmp_model, schema, iter);
          return TRUE;
        }
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (xmp_model->treestore),
                                   iter));
  return FALSE;
}

/* remove a property from the list of children of schema_iter */
static void
find_and_remove_property (XMPModel    *xmp_model,
                          XMPProperty *property,
                          GtkTreeIter *schema_iter)
{
  GtkTreeIter  child_iter;
  XMPProperty *property_xref;

  if (! gtk_tree_model_iter_children (GTK_TREE_MODEL (xmp_model->treestore),
                                      &child_iter, schema_iter))
    return;
  for (;;)
    {
      gtk_tree_model_get (GTK_TREE_MODEL (xmp_model->treestore), &child_iter,
                          COL_XMP_TYPE_XREF, &property_xref,
                          -1);
      if (property_xref == property)
        {
          if (! gtk_tree_store_remove (GTK_TREE_STORE (xmp_model->treestore),
                                       &child_iter))
            break;
        }
      else
        {
          if (! gtk_tree_model_iter_next (GTK_TREE_MODEL(xmp_model->treestore),
                                          &child_iter))
            break;
        }
    }
}

/* add a schema to the tree */
static void
add_known_schema (XMPModel    *xmp_model,
                  XMPSchema   *schema,
                  GtkTreeIter *iter)
{
  gtk_tree_store_append (xmp_model->treestore, iter, NULL);
  gtk_tree_store_set (xmp_model->treestore, iter,
                      COL_XMP_NAME, schema->name,
                      COL_XMP_VALUE, schema->uri,
                      COL_XMP_VALUE_RAW, NULL,
                      COL_XMP_TYPE_XREF, schema,
                      COL_XMP_WIDGET_XREF, NULL,
                      COL_XMP_EDITABLE, FALSE,
                      COL_XMP_EDIT_ICON, NULL,
                      COL_XMP_VISIBLE, FALSE,
                      COL_XMP_WEIGHT, PANGO_WEIGHT_BOLD,
                      COL_XMP_WEIGHT_SET, TRUE,
                      -1);
  cache_iter_for_schema (xmp_model, schema, iter);
}

/* called by the XMP parser - new schema */
static gpointer
parse_start_schema (XMPParseContext     *context,
                    const gchar         *ns_uri,
                    const gchar         *ns_prefix,
                    gpointer             user_data,
                    GError             **error)
{
  XMPModel    *xmp_model = user_data;
  GtkTreeIter  iter;
  XMPSchema   *schema;

  g_return_val_if_fail (xmp_model != NULL, NULL);
  schema = find_xmp_schema (xmp_model, ns_uri);
  if (schema == NULL)
    {
      /* add schema to custom_schemas */
      schema = g_new (XMPSchema, 1);
      schema->uri = g_strdup (ns_uri);
      schema->prefix = g_strdup (ns_prefix);
      schema->name = schema->uri;
      schema->properties = NULL;
      xmp_model->custom_schemas = g_slist_prepend (xmp_model->custom_schemas,
                                                   schema);
    }
  else if (find_iter_for_schema (xmp_model, schema, &iter))
    {
      /* already in the tree, so no need to add it again */
      return schema;
    }
  /* schemas with NULL names are special and should not go in the tree */
  if (schema->name == NULL)
    {
      cache_iter_for_schema (xmp_model, NULL, NULL);
      return schema;
    }
  /* if the schema is not in the tree yet, add it now */
  add_known_schema (xmp_model, schema, &iter);
  return schema;
}

/* called by the XMP parser - end of schema */
static void
parse_end_schema (XMPParseContext     *context,
                  gpointer             ns_user_data,
                  gpointer             user_data,
                  GError             **error)
{
  XMPModel  *xmp_model = user_data;
  XMPSchema *schema = ns_user_data;

  g_return_if_fail (xmp_model != NULL);
  g_return_if_fail (schema != NULL);
  xmp_model->cached_schema = NULL;
#ifdef DEBUG_XMP_MODEL
  if (schema->name)
    g_print ("End of %s\n", schema->name);
#endif
}

/* called by the XMP parser - new property */
static void
parse_set_property (XMPParseContext     *context,
                    const gchar         *name,
                    XMPParseType         type,
                    const gchar        **value,
                    gpointer             ns_user_data,
                    gpointer             user_data,
                    GError             **error)
{
  XMPModel    *xmp_model = user_data;
  XMPSchema   *schema = ns_user_data;
  int          i;
  const gchar *ns_prefix;
  XMPProperty *property;
  GtkTreeIter  iter;
  GtkTreeIter  child_iter;
  gchar       *tmp_name;
  gchar       *tmp_value;

  g_return_if_fail (xmp_model != NULL);
  g_return_if_fail (schema != NULL);
  if (! find_iter_for_schema (xmp_model, schema, &iter))
    {
      g_warning ("Unable to set XMP property '%s' because its schema is bad",
                 name);
      return;
    }
  ns_prefix = schema->prefix;
  property = NULL;
  if (schema->properties != NULL)
    for (i = 0; schema->properties[i].name != NULL; ++i)
      if (! strcmp (schema->properties[i].name, name))
        {
          property = &(schema->properties[i]);
          break;
        }
  /* if the same property was already present, remove it (replace it) */
  if (property != NULL)
    find_and_remove_property (xmp_model, property, &iter);

  switch (type)
    {
    case XMP_PTYPE_TEXT:
#ifdef DEBUG_XMP_MODEL
      g_print ("\t%s:%s = \"%s\"\n", ns_prefix, name, value[0]);
#endif
      if (property != NULL)
        /* FIXME */;
      else
        {
          property = g_new (XMPProperty, 1);
          property->name = g_strdup (name);
          property->type = XMP_TYPE_TEXT;
          property->editable = TRUE;
          xmp_model->custom_properties =
            g_slist_prepend (xmp_model->custom_properties, property);
        }
      gtk_tree_store_append (xmp_model->treestore, &child_iter, &iter);
      gtk_tree_store_set (xmp_model->treestore, &child_iter,
                          COL_XMP_NAME, name,
                          COL_XMP_VALUE, value[0],
                          COL_XMP_VALUE_RAW, value,
                          COL_XMP_TYPE_XREF, property,
                          COL_XMP_WIDGET_XREF, NULL,
                          COL_XMP_EDITABLE, property->editable,
                          COL_XMP_EDIT_ICON, NULL,
                          COL_XMP_VISIBLE, TRUE,
                          COL_XMP_WEIGHT, PANGO_WEIGHT_NORMAL,
                          COL_XMP_WEIGHT_SET, FALSE,
                          -1);
      break;

    case XMP_PTYPE_RESOURCE:
#ifdef DEBUG_XMP_MODEL
      g_print ("\t%s:%s @ = \"%s\"\n", ns_prefix, name,
              value[0]);
#endif
      if (property != NULL)
        /* FIXME */;
      else
        {
          property = g_new (XMPProperty, 1);
          property->name = g_strdup (name);
          property->type = XMP_TYPE_URI;
          property->editable = TRUE;
          xmp_model->custom_properties =
            g_slist_prepend (xmp_model->custom_properties, property);
        }
      tmp_name = g_strconcat (name, " @", NULL);
      gtk_tree_store_append (xmp_model->treestore, &child_iter, &iter);
      gtk_tree_store_set (xmp_model->treestore, &child_iter,
                          COL_XMP_NAME, tmp_name,
                          COL_XMP_VALUE, value[0],
                          COL_XMP_VALUE_RAW, value,
                          COL_XMP_TYPE_XREF, property,
                          COL_XMP_WIDGET_XREF, NULL,
                          COL_XMP_EDITABLE, property->editable,
                          COL_XMP_EDIT_ICON, NULL,
                          COL_XMP_VISIBLE, TRUE,
                          COL_XMP_WEIGHT, PANGO_WEIGHT_NORMAL,
                          COL_XMP_WEIGHT_SET, FALSE,
                          -1);
      g_free (tmp_name);
      break;

    case XMP_PTYPE_ORDERED_LIST:
    case XMP_PTYPE_UNORDERED_LIST:
#ifdef DEBUG_XMP_MODEL
      g_print ("\t%s:%s [] =", ns_prefix, name);
      for (i = 0; value[i] != NULL; i++)
        if (i == 0)
          g_print (" \"%s\"", value[i]);
        else
          g_print (", \"%s\"", value[i]);
      g_print ("\n");
#endif
      if (property != NULL)
        /* FIXME */;
      else
        {
          property = g_new (XMPProperty, 1);
          property->name = g_strdup (name);
          property->type = ((type == XMP_PTYPE_ORDERED_LIST)
                            ? XMP_TYPE_TEXT_BAG
                            : XMP_TYPE_TEXT_SEQ);
          property->editable = TRUE;
          xmp_model->custom_properties =
            g_slist_prepend (xmp_model->custom_properties, property);
        }

      tmp_name = g_strconcat (name, " []", NULL);
      tmp_value = g_strjoinv ("; ", (gchar **) value);
      gtk_tree_store_append (xmp_model->treestore, &child_iter, &iter);
      gtk_tree_store_set (xmp_model->treestore, &child_iter,
                          COL_XMP_NAME, tmp_name,
                          COL_XMP_VALUE, tmp_value,
                          COL_XMP_VALUE_RAW, value,
                          COL_XMP_TYPE_XREF, property,
                          COL_XMP_WIDGET_XREF, NULL,
                          COL_XMP_EDITABLE, property->editable,
                          COL_XMP_EDIT_ICON, NULL,
                          COL_XMP_VISIBLE, TRUE,
                          COL_XMP_WEIGHT, PANGO_WEIGHT_NORMAL,
                          COL_XMP_WEIGHT_SET, FALSE,
                          -1);
      g_free (tmp_value);
      g_free (tmp_name);
      break;

    case XMP_PTYPE_ALT_THUMBS:
#ifdef DEBUG_XMP_MODEL
      for (i = 0; value[i] != NULL; i += 2)
        g_print ("\t%s:%s [size:%d] = \"...\"\n", ns_prefix, name,
                *(int *)(value[i]));
      g_print ("\n");
#endif
      if (property != NULL)
        /* FIXME */;
      else
        {
          property = g_new (XMPProperty, 1);
          property->name = g_strdup (name);
          property->type = XMP_TYPE_THUMBNAIL_ALT;
          property->editable = TRUE;
          xmp_model->custom_properties =
            g_slist_prepend (xmp_model->custom_properties, property);
        }

      tmp_name = g_strconcat (name, " []", NULL);
      gtk_tree_store_append (xmp_model->treestore, &child_iter, &iter);
      gtk_tree_store_set (xmp_model->treestore, &child_iter,
                          COL_XMP_NAME, tmp_name,
                          COL_XMP_VALUE, "[FIXME: display thumbnails]",
                          COL_XMP_VALUE_RAW, value,
                          COL_XMP_TYPE_XREF, property,
                          COL_XMP_WIDGET_XREF, NULL,
                          COL_XMP_EDITABLE, property->editable,
                          COL_XMP_EDIT_ICON, NULL,
                          COL_XMP_VISIBLE, TRUE,
                          COL_XMP_WEIGHT, PANGO_WEIGHT_NORMAL,
                          COL_XMP_WEIGHT_SET, FALSE,
                          -1);
      g_free (tmp_name);
      break;

    case XMP_PTYPE_ALT_LANG:
#ifdef DEBUG_XMP_MODEL
      for (i = 0; value[i] != NULL; i += 2)
        g_print ("\t%s:%s [lang:%s] = \"%s\"\n", ns_prefix, name,
                value[i], value[i + 1]);
#endif
      if (property != NULL)
        /* FIXME */;
      else
        {
          property = g_new (XMPProperty, 1);
          property->name = g_strdup (name);
          property->type = XMP_TYPE_LANG_ALT;
          property->editable = TRUE;
          xmp_model->custom_properties =
            g_slist_prepend (xmp_model->custom_properties, property);
        }
      for (i = 0; value[i] != NULL; i += 2)
        {
          tmp_name = g_strconcat (name, " [", value[i], "]", NULL);
          gtk_tree_store_append (xmp_model->treestore, &child_iter, &iter);
          gtk_tree_store_set (xmp_model->treestore, &child_iter,
                              COL_XMP_NAME, tmp_name,
                              COL_XMP_VALUE, value[i + 1],
                              COL_XMP_VALUE_RAW, value,
                              COL_XMP_TYPE_XREF, property,
                              COL_XMP_WIDGET_XREF, NULL,
                              COL_XMP_EDITABLE, property->editable,
                              COL_XMP_EDIT_ICON, NULL,
                              COL_XMP_VISIBLE, TRUE,
                              COL_XMP_WEIGHT, PANGO_WEIGHT_NORMAL,
                              COL_XMP_WEIGHT_SET, FALSE,
                              -1);
          g_free (tmp_name);
        }
      break;

    case XMP_PTYPE_STRUCTURE:
#ifdef DEBUG_XMP_MODEL
      for (i = 2; value[i] != NULL; i += 2)
        g_print ("\t%s:%s [%s] = \"%s\"\n", ns_prefix, name,
                value[i], value[i + 1]);
#endif
      if (property != NULL)
        /* FIXME */;
      else
        {
          property = g_new (XMPProperty, 1);
          property->name = g_strdup (name);
          property->type = XMP_TYPE_GENERIC_STRUCTURE;
          property->editable = TRUE;
          xmp_model->custom_properties =
            g_slist_prepend (xmp_model->custom_properties, property);
        }
      for (i = 2; value[i] != NULL; i += 2)
        {
          tmp_name = g_strconcat (name, " [", value[i], "]", NULL);
          gtk_tree_store_append (xmp_model->treestore, &child_iter, &iter);
          gtk_tree_store_set (xmp_model->treestore, &child_iter,
                              COL_XMP_NAME, tmp_name,
                              COL_XMP_VALUE, value[i + 1],
                              COL_XMP_VALUE_RAW, value,
                              COL_XMP_TYPE_XREF, property,
                              COL_XMP_WIDGET_XREF, NULL,
                              COL_XMP_EDITABLE, property->editable,
                              COL_XMP_EDIT_ICON, NULL,
                              COL_XMP_VISIBLE, TRUE,
                              COL_XMP_WEIGHT, PANGO_WEIGHT_NORMAL,
                              COL_XMP_WEIGHT_SET, FALSE,
                              -1);
          g_free (tmp_name);
        }
      break;

    default:
#ifdef DEBUG_XMP_MODEL
      g_print ("\t%s:%s = ?\n", ns_prefix, name);
#endif
      break;
    }
}

/* called by the XMP parser - parse error */
static void
parse_error (XMPParseContext *context,
             GError          *error,
             gpointer         user_data)
{
  g_warning ("While parsing XMP metadata:\n%s\n", error->message);
}

static XMPParser xmp_parser = {
  parse_start_schema,
  parse_end_schema,
  parse_set_property,
  parse_error
};

/**
 * xmp_model_parse_buffer:
 * @xmp_model: pointer to the #XMPModel in which the results will be stored
 * @buffer: buffer to be parsed
 * @buffer_length: length of the @buffer
 * @skip_other_data: if %TRUE, allow arbitrary data before XMP packet marker
 * @error: return location for a #GError
 *
 * Parse a buffer containing XMP metadata and merge the parsed contents into
 * the supplied @xmp_model.  If @skip_other_data is %TRUE, then the parser
 * will try to find the <?xpacket...?> marker in the buffer, skipping any
 * unknown data found before it.
 *
 * Return value: %TRUE on success, %FALSE if an error was set
 *
 * (Note: this calls the functions from xmp_parse.c, which will call the
 *  functions in this file through the xmp_parser structure defined above.)
 **/
gboolean
xmp_model_parse_buffer (XMPModel     *xmp_model,
                        const gchar  *buffer,
                        gssize        buffer_length,
                        gboolean      skip_other_data,
                        GError      **error)
{
  XMPParseFlags    flags;
  XMPParseContext *context;

  flags = XMP_FLAG_DEFER_VALUE_FREE; /* we will free the array ourselves */
  if (skip_other_data)
    flags |= XMP_FLAG_FIND_XPACKET;

  context = xmp_parse_context_new (&xmp_parser, flags, xmp_model, NULL);

  if (! xmp_parse_context_parse (context, buffer, buffer_length, error))
    {
      xmp_parse_context_free (context);
      return FALSE;
    }

  if (! xmp_parse_context_end_parse (context, error))
    {
      xmp_parse_context_free (context);
      return FALSE;
    }

  xmp_parse_context_free (context);
  return TRUE;
}

/**
 * xmp_model_parse_file:
 * @xmp_model: pointer to the #XMPModel in which the results will be stored
 * @filename: name of the file containing XMP metadata to parse
 * @error: return location for a #GError
  *
 * Try to find XMP metadata in a file and merge its contents into the supplied
 * @xmp_model.
 *
 * Return value: %TRUE on success, %FALSE if an error was set
 **/
gboolean
xmp_model_parse_file (XMPModel     *xmp_model,
                      const gchar  *filename,
                      GError      **error)
{
  gchar *buffer;
  gsize  buffer_length;

  g_return_val_if_fail (filename != NULL, FALSE);
  if (! g_file_get_contents (filename, &buffer, &buffer_length, error))
    return FALSE;
  if (! xmp_model_parse_buffer (xmp_model, buffer, buffer_length, TRUE, error))
    return FALSE;
  g_free (buffer);
  return TRUE;
}

/**
 * xmp_model_get_tree_model:
 * @xmp_model: pointer to an #XMPModel
 *
 * Return a pointer to the #GtkTreeModel contained in the #XMPModel.
 **/
GtkTreeModel *
xmp_model_get_tree_model (XMPModel *xmp_model)
{
  g_return_val_if_fail (xmp_model != NULL, NULL);
  return GTK_TREE_MODEL (xmp_model->treestore);
}

/**
 * xmp_model_get_scalar_property:
 * @xmp_model: pointer to an #XMPModel
 * @schema_name: full URI or usual prefix of the schema
 * @property_name: name of the property to store
 *
 * Store a new value for the specified XMP property.
 *
 * Return value: string representation of the value of that property, or %NULL if the property does not exist
 **/
const gchar *
xmp_model_get_scalar_property (XMPModel    *xmp_model,
                               const gchar *schema_name,
                               const gchar *property_name)
{
  XMPSchema    *schema;
  GtkTreeIter   iter;
  XMPProperty  *property = NULL;
  GtkTreeIter   child_iter;
  int           i;
  XMPProperty  *property_xref;
  const gchar  *value;

  g_return_val_if_fail (xmp_model != NULL, NULL);
  g_return_val_if_fail (schema_name != NULL, NULL);
  g_return_val_if_fail (property_name != NULL, NULL);
  schema = find_xmp_schema (xmp_model, schema_name);
  if (! schema)
    schema = find_xmp_schema_prefix (xmp_model, schema_name);
  if (! schema)
    return NULL;
  if (! find_iter_for_schema (xmp_model, schema, &iter))
    return NULL;
 if (schema->properties != NULL)
    for (i = 0; schema->properties[i].name != NULL; ++i)
      if (! strcmp (schema->properties[i].name, property_name))
        {
          property = &(schema->properties[i]);
          break;
        }
  if (property == NULL)
    return NULL;
  if (! gtk_tree_model_iter_children (GTK_TREE_MODEL (xmp_model->treestore),
                                      &child_iter, &iter))
    return NULL;
  do
    {
      gtk_tree_model_get (GTK_TREE_MODEL (xmp_model->treestore), &child_iter,
                          COL_XMP_TYPE_XREF, &property_xref,
                          COL_XMP_VALUE, &value,
                          -1);
      if (property_xref == property)
        return value;
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL(xmp_model->treestore),
                                   &child_iter));
  return NULL;
}

/**
 * xmp_model_set_scalar_property:
 * @xmp_model: pointer to an #XMPModel
 * @schema_name: full URI or usual prefix of the schema
 * @property_name: name of the property to store
 * @property_value: value to store
 *
 * Store a new value for the specified XMP property.
 *
 * Return value: %TRUE if the property was set, %FALSE if an error occurred (for example, the @schema_name is invalid)
 **/
gboolean
xmp_model_set_scalar_property (XMPModel    *xmp_model,
                               const gchar *schema_name,
                               const gchar *property_name,
                               const gchar *property_value)
{
  XMPSchema    *schema;
  GtkTreeIter   iter;
  XMPProperty  *property = NULL;
  GtkTreeIter   child_iter;
  int           i;
  gchar       **value;

  g_return_val_if_fail (xmp_model != NULL, FALSE);
  g_return_val_if_fail (schema_name != NULL, FALSE);
  g_return_val_if_fail (property_name != NULL, FALSE);
  g_return_val_if_fail (property_value != NULL, FALSE);
  schema = find_xmp_schema (xmp_model, schema_name);
  if (! schema)
    schema = find_xmp_schema_prefix (xmp_model, schema_name);
  if (! schema)
    return FALSE;

  if (! find_iter_for_schema (xmp_model, schema, &iter))
    add_known_schema (xmp_model, schema, &iter);

 if (schema->properties != NULL)
    for (i = 0; schema->properties[i].name != NULL; ++i)
      if (! strcmp (schema->properties[i].name, property_name))
        {
          property = &(schema->properties[i]);
          break;
        }
  if (property != NULL)
    find_and_remove_property (xmp_model, property, &iter);
  else
    {
      property = g_new (XMPProperty, 1);
      property->name = g_strdup (property_name);
      property->type = XMP_TYPE_TEXT;
      property->editable = TRUE;
      xmp_model->custom_properties =
        g_slist_prepend (xmp_model->custom_properties, property);
    }

  value = g_new (gchar *, 2);
  value[0] = g_strdup (property_value);
  value[1] = NULL;
  gtk_tree_store_append (xmp_model->treestore, &child_iter, &iter);
  gtk_tree_store_set (xmp_model->treestore, &child_iter,
                      COL_XMP_NAME, g_strdup (property_name),
                      COL_XMP_VALUE, value[0],
                      COL_XMP_VALUE_RAW, value,
                      COL_XMP_TYPE_XREF, property,
                      COL_XMP_WIDGET_XREF, NULL,
                      COL_XMP_EDITABLE, property->editable,
                      COL_XMP_EDIT_ICON, NULL,
                      COL_XMP_VISIBLE, TRUE,
                      COL_XMP_WEIGHT, PANGO_WEIGHT_NORMAL,
                      COL_XMP_WEIGHT_SET, FALSE,
                      -1);
  return TRUE;
}
