/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <glib-object.h>

#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpdocumentlist.h"
#include "gimpimagefile.h"


static void     gimp_document_list_config_iface_init (GimpConfigInterface *iface);
static gboolean gimp_document_list_serialize   (GimpConfig       *config,
                                                GimpConfigWriter *writer,
                                                gpointer          data);
static gboolean gimp_document_list_deserialize (GimpConfig       *config,
                                                GScanner         *scanner,
                                                gint              nest_level,
                                                gpointer          data);


G_DEFINE_TYPE_WITH_CODE (GimpDocumentList, gimp_document_list, GIMP_TYPE_LIST,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_document_list_config_iface_init))

static const gchar document_symbol[] = "document";


static void
gimp_document_list_class_init (GimpDocumentListClass *klass)
{
}

static void
gimp_document_list_init (GimpDocumentList *list)
{
}

static void
gimp_document_list_config_iface_init (GimpConfigInterface *iface)
{
  iface->serialize   = gimp_document_list_serialize;
  iface->deserialize = gimp_document_list_deserialize;
}

static gboolean
gimp_document_list_serialize (GimpConfig       *config,
                              GimpConfigWriter *writer,
                              gpointer          data)
{
  GList *list;

  for (list = GIMP_LIST (config)->list; list; list = list->next)
    {
      gimp_config_writer_open (writer, document_symbol);
      gimp_config_writer_string (writer, GIMP_OBJECT (list->data)->name);
      gimp_config_writer_close (writer);
    }

  return TRUE;
}

static gboolean
gimp_document_list_deserialize (GimpConfig *config,
                                GScanner   *scanner,
                                gint        nest_level,
                                gpointer    data)
{
  GimpDocumentList *document_list = GIMP_DOCUMENT_LIST (config);
  GTokenType        token;
  gint              size;

  size = GPOINTER_TO_INT (data);

  g_scanner_scope_add_symbol (scanner, 0,
                              document_symbol, (gpointer) document_symbol);

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          token = G_TOKEN_RIGHT_PAREN;
          if (scanner->value.v_symbol == document_symbol)
            {
              gchar *uri = NULL;

              if (! gimp_scanner_parse_string (scanner, &uri))
                {
                  token = G_TOKEN_STRING;
                  break;
                }

              if (uri)
                {
                  GimpImagefile *imagefile;

                  imagefile = gimp_imagefile_new (document_list->gimp, uri);

                  g_free (uri);

                  gimp_container_add (GIMP_CONTAINER (document_list),
                                      GIMP_OBJECT (imagefile));

                  g_object_unref (imagefile);
                }
            }
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }

  gimp_list_reverse (GIMP_LIST (document_list));

  return gimp_config_deserialize_return (scanner, token, nest_level);
}

GimpContainer *
gimp_document_list_new (Gimp *gimp)
{
  GimpDocumentList *document_list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  document_list = g_object_new (GIMP_TYPE_DOCUMENT_LIST,
                                "name",          "document_list",
                                "children_type", GIMP_TYPE_IMAGEFILE,
                                "policy",        GIMP_CONTAINER_POLICY_STRONG,
                                NULL);

  document_list->gimp = gimp;

  return GIMP_CONTAINER (document_list);
}

GimpImagefile *
gimp_document_list_add_uri (GimpDocumentList *document_list,
                            const gchar      *uri,
                            const gchar      *mime_type)
{
  GimpImagefile *imagefile;
  GimpContainer *container;

  g_return_val_if_fail (GIMP_IS_DOCUMENT_LIST (document_list), NULL);
  g_return_val_if_fail (uri != NULL, NULL);

  container = GIMP_CONTAINER (document_list);

  imagefile = (GimpImagefile *) gimp_container_get_child_by_name (container,
                                                                  uri);

  if (imagefile)
    {
      gimp_container_reorder (container, GIMP_OBJECT (imagefile), 0);
    }
  else
    {
      imagefile = gimp_imagefile_new (document_list->gimp, uri);
      gimp_container_add (container, GIMP_OBJECT (imagefile));
      g_object_unref (imagefile);
    }

  g_object_set (imagefile->thumbnail,
                "image-mimetype", mime_type,
                NULL);

  return imagefile;
}
