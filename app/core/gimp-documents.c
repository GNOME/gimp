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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpconfig.h"
#include "config/gimpscanner.h"

#include "gimpdocuments.h"
#include "gimpimagefile.h"
#include "gimplist.h"

#include "libgimp/gimpintl.h"


static void     gimp_documents_config_iface_init (gpointer  iface,
                                                  gpointer  iface_data);
static void     gimp_documents_serialize         (GObject  *object,
                                                  gint      fd);
static gboolean gimp_documents_deserialize       (GObject  *object,
                                                  GScanner *scanner);


GType 
gimp_documents_get_type (void)
{
  static GType documents_type = 0;

  if (! documents_type)
    {
      static const GTypeInfo documents_info =
      {
        sizeof (GimpDocumentsClass),
	NULL,           /* base_init      */
        NULL,           /* base_finalize  */
	NULL,           /* class_init     */
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpDocuments),
	0,              /* n_preallocs    */
	NULL            /* instance_init  */
      };
      static const GInterfaceInfo documents_iface_info = 
      { 
        gimp_documents_config_iface_init,
        NULL,           /* iface_finalize */ 
        NULL            /* iface_data     */
      };

      documents_type = g_type_register_static (GIMP_TYPE_LIST, 
                                               "GimpDocuments",
                                               &documents_info, 0);

      g_type_add_interface_static (documents_type,
                                   GIMP_TYPE_CONFIG_INTERFACE,
                                   &documents_iface_info);
    }

  return documents_type;
}

static void
gimp_documents_config_iface_init (gpointer  iface,
                                  gpointer  iface_data)
{
  GimpConfigInterface *config_iface = (GimpConfigInterface *) iface;

  config_iface->serialize   = gimp_documents_serialize;
  config_iface->deserialize = gimp_documents_deserialize;
}

static void
gimp_documents_serialize (GObject *documents,
                          gint     fd)
{
  GList   *list;
  GString *str;

  const gchar *header =
    "# GIMP documents\n"
    "#\n"
    "# This file will be entirely rewritten every time you\n"
    "# quit the gimp.\n\n";

  write (fd, header, strlen (header));

  str = g_string_new (NULL);

  for (list = GIMP_LIST (documents)->list; list; list = list->next)
    {
      gchar *escaped;

      escaped = g_strescape (GIMP_OBJECT (list->data)->name, NULL);

      g_string_assign (str, "(document \"");
      g_string_append (str, escaped);
      g_string_append (str, "\")\n");

      g_free (escaped);

      write (fd, str->str, str->len);
    }

  g_string_free (str, TRUE);
}

static gboolean
gimp_documents_deserialize (GObject  *documents,
                            GScanner *scanner)
{
  GTokenType  token;
  gint        size;

  size = GPOINTER_TO_INT (g_object_get_data (documents, "thumbnail_size"));

  g_scanner_scope_add_symbol (scanner, 0,
                              "document", GINT_TO_POINTER (1));

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
          if (scanner->value.v_symbol == GINT_TO_POINTER (1))
            {
              gchar         *uri;
              GimpImagefile *imagefile;

              if (! gimp_scanner_parse_string (scanner, &uri))
                {
                  token = G_TOKEN_STRING;
                  break;
                }

              imagefile = gimp_imagefile_new (uri);
              gimp_imagefile_update (imagefile, size);

              g_free (uri);

              gimp_container_add (GIMP_CONTAINER (documents),
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

  GIMP_LIST (documents)->list = g_list_reverse (GIMP_LIST (documents)->list);

  if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
      return FALSE;
    }
  
  return TRUE;
}

GimpContainer *
gimp_documents_new (void)
{
  GObject *documents;

  documents = g_object_new (GIMP_TYPE_DOCUMENTS,
                            "name",          "documents",
                            "children_type", GIMP_TYPE_IMAGEFILE,
                            "policy",        GIMP_CONTAINER_POLICY_STRONG,
                            NULL);
  
  return GIMP_CONTAINER (documents);
}

void
gimp_documents_load (GimpDocuments *documents,
                     gint           thumbnail_size)
{
  gchar  *filename;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_DOCUMENTS (documents));

  g_object_set_data (G_OBJECT (documents), "thumbnail_size",
                     GINT_TO_POINTER (thumbnail_size));

  filename = gimp_personal_rc_file ("documents");

  if (! gimp_config_deserialize (G_OBJECT (documents), filename, &error))
    {
      g_message (error->message);
      g_error_free (error);
    }

  g_free (filename);
}

void
gimp_documents_save (GimpDocuments *documents)
{
  gchar  *filename;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_DOCUMENTS (documents));

  filename = gimp_personal_rc_file ("documents");

  if (! gimp_config_serialize (G_OBJECT (documents), filename, &error))
    {
      g_message (error->message);
      g_error_free (error);
    }

  g_free (filename);
}

GimpImagefile *
gimp_documents_add (GimpDocuments *documents,
                    const gchar   *uri)
{
  GimpImagefile *imagefile;
  GimpContainer *container;

  g_return_val_if_fail (GIMP_IS_DOCUMENTS (documents), NULL);
  g_return_val_if_fail (uri != NULL, NULL);

  container = GIMP_CONTAINER (documents);

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
