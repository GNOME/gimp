/* xmp-gen.c - generate XMP metadata from the tree model
 *
 * Copyright (C) 2005, Raphaël Quinet <raphael@gimp.org>
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

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "xmp-encode.h"
#include "xmp-model.h"

static gssize
size_schema (GtkTreeModel     *model,
             GtkTreeIter      *iter,
             const XMPSchema **schema_r)
{
  gtk_tree_model_get (model, iter,
                      COL_XMP_TYPE_XREF, schema_r,
                      -1);
  return (sizeof (" <rdf:Description xmlns:%s='%s'>\n") - 5
          + strlen ((*schema_r)->prefix)
          + strlen ((*schema_r)->uri)
          + sizeof (" </rdf:Description>\n\n") - 1);
}

static gssize
size_property (GtkTreeModel    *model,
               GtkTreeIter     *iter,
               const XMPSchema *schema)
{
  const XMPProperty  *property;
  const gchar       **value_array;
  gssize              length;
  gint                i;

  gtk_tree_model_get (model, iter,
                      COL_XMP_TYPE_XREF, &property,
                      COL_XMP_VALUE_RAW, &value_array,
                      -1);

  switch (property->type)
    {
    case XMP_TYPE_BOOLEAN:
    case XMP_TYPE_DATE:
    case XMP_TYPE_INTEGER:
    case XMP_TYPE_REAL:
    case XMP_TYPE_MIME_TYPE:
    case XMP_TYPE_TEXT:
    case XMP_TYPE_RATIONAL:
      return (sizeof ("  <%s:%s>%s</%s:%s>\n") - 11
              + 2 * strlen (schema->prefix)
              + 2 * strlen (property->name)
              + strlen (value_array[0]));

    case XMP_TYPE_LOCALE_BAG:
    case XMP_TYPE_TEXT_BAG:
    case XMP_TYPE_XPATH_BAG:
    case XMP_TYPE_JOB_BAG:
    case XMP_TYPE_INTEGER_SEQ:
    case XMP_TYPE_TEXT_SEQ:
    case XMP_TYPE_RESOURCE_EVENT_SEQ:
    case XMP_TYPE_RATIONAL_SEQ:
      length = (sizeof ("  <%s:%s>\n   <rdf:Bag>\n") - 5
                + sizeof ("   </rdf:Bag>\n  </%s:%s>\n") - 5
                + 2 * strlen (schema->prefix)
                + 2 * strlen (property->name));
      for (i = 0; value_array[i] != NULL; i++)
        length += (sizeof ("    <rdf:li>%s</rdf:li>\n") - 3
                   + strlen (value_array[i]));
      return length;

    case XMP_TYPE_LANG_ALT:
      length = (sizeof ("  <%s:%s>\n   <rdf:Alt>\n") - 5
                + sizeof ("   </rdf:Alt>\n  </%s:%s>\n") - 5
                + 2 * strlen (schema->prefix)
                + 2 * strlen (property->name));
      for (i = 0; value_array[i] != NULL; i += 2)
        length += (sizeof ("    <rdf:li xml:lang='%s'>%s</rdf:li>\n") - 5
                   + strlen (value_array[i])
                   + strlen (value_array[i + 1]));
      return length;

    case XMP_TYPE_URI:
      return (sizeof ("  <%s:%s rdf:resource='%s' />\n") - 7
              + strlen (schema->prefix)
              + strlen (property->name)
              + strlen (value_array[0]));

    case XMP_TYPE_RESOURCE_REF:
    case XMP_TYPE_DIMENSIONS:
    case XMP_TYPE_THUMBNAIL_ALT:
    case XMP_TYPE_GPS_COORDINATE:
    case XMP_TYPE_FLASH:
    case XMP_TYPE_OECF_SFR:
    case XMP_TYPE_CFA_PATTERN:
    case XMP_TYPE_DEVICE_SETTINGS:
      return 100; /* FIXME */

    case XMP_TYPE_UNKNOWN:
      return 0;
    }
  return 0;
}

/**
 * xmp_estimate_size:
 * @xmp_model: An #XMPModel
 *
 * Return value: estimated size (upper bound) of the XMP (RDF) encoding of
 * the given model.
 **/
gssize
xmp_estimate_size (XMPModel *xmp_model)
{
  GtkTreeModel    *model;
  GtkTreeIter      iter;
  GtkTreeIter      child;
  gssize           buffer_size;
  const XMPSchema *schema;

  model = xmp_model_get_tree_model (xmp_model);
  buffer_size = 158 + 44 + 1;
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter))
    {
      do
        {
          buffer_size += size_schema (model, &iter, &schema);
          if (gtk_tree_model_iter_children (model, &child, &iter))
            {
              do
                {
                  buffer_size += size_property (model, &child, schema);
                }
              while (gtk_tree_model_iter_next (model, &child));
            }
        }
      while (gtk_tree_model_iter_next (model, &iter));
    }
  return buffer_size;
}

static gint
gen_schema_start (GtkTreeModel     *model,
                  GtkTreeIter      *iter,
                  gchar            *buffer,
                  const XMPSchema **schema_r)
{
  gtk_tree_model_get (model, iter,
                      COL_XMP_TYPE_XREF, schema_r,
                      -1);
  return sprintf (buffer, " <rdf:Description xmlns:%s='%s'>\n",
                  (*schema_r)->prefix, (*schema_r)->uri);
}

static gint
gen_schema_end (GtkTreeModel *model,
                GtkTreeIter  *iter,
                gchar        *buffer)
{
  return sprintf (buffer, " </rdf:Description>\n\n");
}

static gint
gen_property (GtkTreeModel    *model,
              GtkTreeIter     *iter,
              gchar           *buffer,
              const XMPSchema *schema)
{
  const XMPProperty  *property;
  const gchar       **value_array;
  gssize              length;
  gint                i;

  gtk_tree_model_get (model, iter,
                      COL_XMP_TYPE_XREF, &property,
                      COL_XMP_VALUE_RAW, &value_array,
                      -1);
  g_return_val_if_fail (property->name != NULL, 0);
  switch (property->type)
    {
    case XMP_TYPE_BOOLEAN:
    case XMP_TYPE_DATE:
    case XMP_TYPE_INTEGER:
    case XMP_TYPE_REAL:
    case XMP_TYPE_MIME_TYPE:
    case XMP_TYPE_TEXT:
    case XMP_TYPE_RATIONAL:
      return sprintf (buffer, "  <%s:%s>%s</%s:%s>\n",
                      schema->prefix, property->name,
                      value_array[0],
                      schema->prefix, property->name);

    case XMP_TYPE_LOCALE_BAG:
    case XMP_TYPE_TEXT_BAG:
    case XMP_TYPE_XPATH_BAG:
    case XMP_TYPE_JOB_BAG:
      length = sprintf (buffer, "  <%s:%s>\n   <rdf:Bag>\n",
                        schema->prefix, property->name);
      for (i = 0; value_array[i] != NULL; i++)
        length += sprintf (buffer + length, "    <rdf:li>%s</rdf:li>\n",
                           value_array[i]);
      length += sprintf (buffer + length, "   </rdf:Bag>\n  </%s:%s>\n",
                         schema->prefix, property->name);
      return length;

    case XMP_TYPE_INTEGER_SEQ:
    case XMP_TYPE_TEXT_SEQ:
    case XMP_TYPE_RESOURCE_EVENT_SEQ:
    case XMP_TYPE_RATIONAL_SEQ:
      length = sprintf (buffer, "  <%s:%s>\n   <rdf:Seq>\n",
                        schema->prefix, property->name);
      for (i = 0; value_array[i] != NULL; i++)
        length += sprintf (buffer + length, "    <rdf:li>%s</rdf:li>\n",
                           value_array[i]);
      length += sprintf (buffer + length, "   </rdf:Seq>\n  </%s:%s>\n",
                         schema->prefix, property->name);
      return length;

    case XMP_TYPE_LANG_ALT:
      length = sprintf (buffer, "  <%s:%s>\n   <rdf:Alt>\n",
                        schema->prefix, property->name);
      for (i = 0; value_array[i] != NULL; i += 2)
        length += sprintf (buffer + length,
                           "    <rdf:li xml:lang='%s'>%s</rdf:li>\n",
                           value_array[i], value_array[i + 1]);
      length += sprintf (buffer + length, "   </rdf:Alt>\n  </%s:%s>\n",
                         schema->prefix, property->name);
      return length;

    case XMP_TYPE_URI:
      return sprintf (buffer, "  <%s:%s rdf:resource='%s' />\n",
                      schema->prefix, property->name, value_array[0]);

    case XMP_TYPE_RESOURCE_REF:
    case XMP_TYPE_DIMENSIONS:
    case XMP_TYPE_THUMBNAIL_ALT:
    case XMP_TYPE_GPS_COORDINATE:
    case XMP_TYPE_FLASH:
    case XMP_TYPE_OECF_SFR:
    case XMP_TYPE_CFA_PATTERN:
    case XMP_TYPE_DEVICE_SETTINGS:
      g_warning ("FIXME: output not implemented yet (%s)", property->name);
      break;

    case XMP_TYPE_UNKNOWN:
      g_warning ("Unknown property type for %s", property->name);
      break;
    }
  return 0;
}

/**
 * xmp_generate_block:
 * @xmp_model: An #XMPModel
 * @buffer: buffer in which the XMP block will be written
 * @buffer_size: maximum size of the buffer
 *
 * Generate XMP block from xmp_model.
 *
 * Return value: number of characters stored in the buffer (not including the terminating NUL).
 */
gssize
xmp_generate_block (XMPModel *xmp_model,
                    gchar    *buffer,
                    gssize    buffer_size)
{
  GtkTreeModel    *model;
  GtkTreeIter      iter;
  GtkTreeIter      child;
  gssize           n;
  const XMPSchema *schema;

  model = xmp_model_get_tree_model (xmp_model);

  strcpy (buffer,
          "<?xpacket begin='\357\273\277' id='W5M0MpCehiHzreSzNTczkc9d'?>\n"
          "<x:xmpmeta xmlns:x='adobe:ns:meta/'>\n"
          "<rdf:RDF xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'>\n"
          "\n");
  n = sizeof (
          "<?xpacket begin='\357\273\277' id='W5M0MpCehiHzreSzNTczkc9d'?>\n"
          "<x:xmpmeta xmlns:x='adobe:ns:meta/'>\n"
          "<rdf:RDF xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'>\n"
          "\n") - 1;
  /* generate the contents of the XMP block */
  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter))
    {
      do
        {
          n += gen_schema_start (model, &iter, buffer + n, &schema);
          if (gtk_tree_model_iter_children (model, &child, &iter))
            {
              do
                {
                  n += gen_property (model, &child, buffer + n, schema);
                }
              while (gtk_tree_model_iter_next (model, &child));
            }
          n += gen_schema_end (model, &iter, buffer + n);
        }
      while (gtk_tree_model_iter_next (model, &iter));
    }
  n = strlen (buffer);
  strcpy (buffer + n, "</rdf:RDF>\n</x:xmpmeta>\n<?xpacket end='r'?>\n");
  n += sizeof ("</rdf:RDF>\n</x:xmpmeta>\n<?xpacket end='r'?>\n") - 1;

  return n;
}
