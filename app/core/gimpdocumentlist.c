/* The GIMP -- an image manipulation program
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

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#include "core-types.h"

#include "config/gimpconfig.h"
#include "config/gimpscanner.h"

#include "gimpdocumentlist.h"
#include "gimpimagefile.h"

#include "libgimp/gimpintl.h"


static void     gimp_document_list_config_iface_init (gpointer  iface,
                                                      gpointer  iface_data);
static gboolean gimp_document_list_serialize         (GObject  *list,
                                                      gint      fd,
                                                      gpointer  data);
static gboolean gimp_document_list_deserialize       (GObject  *list,
                                                      GScanner *scanner,
                                                      gpointer  data);


static const gchar *document_symbol = "document";


GType 
gimp_document_list_get_type (void)
{
  static GType document_list_type = 0;

  if (! document_list_type)
    {
      static const GTypeInfo document_list_info =
      {
        sizeof (GimpDocumentListClass),
	NULL,           /* base_init      */
        NULL,           /* base_finalize  */
	NULL,           /* class_init     */
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpDocumentList),
	0,              /* n_preallocs    */
	NULL            /* instance_init  */
      };
      static const GInterfaceInfo document_list_iface_info = 
      { 
        gimp_document_list_config_iface_init,
        NULL,           /* iface_finalize */ 
        NULL            /* iface_data     */
      };

      document_list_type = g_type_register_static (GIMP_TYPE_LIST, 
                                                   "GimpDocumentList",
                                                   &document_list_info, 0);

      g_type_add_interface_static (document_list_type,
                                   GIMP_TYPE_CONFIG_INTERFACE,
                                   &document_list_iface_info);
    }

  return document_list_type;
}

static void
gimp_document_list_config_iface_init (gpointer  iface,
                                      gpointer  iface_data)
{
  GimpConfigInterface *config_iface = (GimpConfigInterface *) iface;

  config_iface->serialize   = gimp_document_list_serialize;
  config_iface->deserialize = gimp_document_list_deserialize;
}

static gboolean
gimp_document_list_serialize (GObject *document_list,
                              gint     fd,
                              gpointer data)
{
  GList   *list;
  GString *str;

  str = g_string_new (NULL);

  for (list = GIMP_LIST (document_list)->list; list; list = list->next)
    {
      gchar *escaped;

      escaped = g_strescape (GIMP_OBJECT (list->data)->name, NULL);
      g_string_printf (str, "(%s \"%s\")\n", document_symbol, escaped); 
      g_free (escaped);

      if (write (fd, str->str, str->len) == -1)
        return FALSE;
    }

  g_string_free (str, TRUE);

  return TRUE;
}

static gboolean
gimp_document_list_deserialize (GObject  *document_list,
                                GScanner *scanner,
                                gpointer  data)
{
  GTokenType  token;
  gint        size;

  size = GPOINTER_TO_INT (data);

  g_scanner_scope_add_symbol (scanner, 0,
                              document_symbol, (gpointer) document_symbol);

  token = G_TOKEN_LEFT_PAREN;

  do
    {
      if (g_scanner_peek_next_token (scanner) != token)
        break;

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
              gchar         *uri;
              GimpImagefile *imagefile;

              if (! gimp_scanner_parse_string (scanner, &uri))
                {
                  token = G_TOKEN_STRING;
                  break;
                }

              imagefile = gimp_imagefile_new (uri);

              if (size > 0)
                gimp_imagefile_update (imagefile, size);

              g_free (uri);

              gimp_container_add (GIMP_CONTAINER (document_list),
                                  GIMP_OBJECT (imagefile));
            }
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }
  while (token != G_TOKEN_EOF);

  GIMP_LIST (document_list)->list = 
    g_list_reverse (GIMP_LIST (document_list)->list);

  if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, document_symbol,
                             _("fatal parse error"), TRUE);
      return FALSE;
    }
  
  return TRUE;
}

GimpContainer *
gimp_document_list_new (void)
{
  GObject *document_list;

  document_list = g_object_new (GIMP_TYPE_DOCUMENT_LIST,
                                "name",          "document_list",
                                "children_type", GIMP_TYPE_IMAGEFILE,
                                "policy",        GIMP_CONTAINER_POLICY_STRONG,
                                NULL);
  
  return GIMP_CONTAINER (document_list);
}

GimpImagefile *
gimp_document_list_add_uri (GimpDocumentList *document_list,
                            const gchar      *uri)
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
      imagefile = gimp_imagefile_new (uri);
      gimp_container_add (container, GIMP_OBJECT (imagefile));
      g_object_unref (G_OBJECT (imagefile));
    }

  return imagefile;
}
