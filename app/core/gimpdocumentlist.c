/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "config/ligmacoreconfig.h"

#include "ligma.h"
#include "ligmadocumentlist.h"
#include "ligmaimagefile.h"


G_DEFINE_TYPE (LigmaDocumentList, ligma_document_list, LIGMA_TYPE_LIST)


static void
ligma_document_list_class_init (LigmaDocumentListClass *klass)
{
}

static void
ligma_document_list_init (LigmaDocumentList *list)
{
}

LigmaContainer *
ligma_document_list_new (Ligma *ligma)
{
  LigmaDocumentList *document_list;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  document_list = g_object_new (LIGMA_TYPE_DOCUMENT_LIST,
                                "name",          "document-list",
                                "children-type", LIGMA_TYPE_IMAGEFILE,
                                "policy",        LIGMA_CONTAINER_POLICY_STRONG,
                                NULL);

  document_list->ligma = ligma;

  return LIGMA_CONTAINER (document_list);
}

LigmaImagefile *
ligma_document_list_add_file (LigmaDocumentList *document_list,
                             GFile            *file,
                             const gchar      *mime_type)
{
  Ligma          *ligma;
  LigmaImagefile *imagefile;
  LigmaContainer *container;
  gchar         *uri;

  g_return_val_if_fail (LIGMA_IS_DOCUMENT_LIST (document_list), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  container = LIGMA_CONTAINER (document_list);

  ligma = document_list->ligma;

  uri = g_file_get_uri (file);

  imagefile = (LigmaImagefile *) ligma_container_get_child_by_name (container,
                                                                  uri);

  g_free (uri);

  if (imagefile)
    {
      ligma_container_reorder (container, LIGMA_OBJECT (imagefile), 0);
    }
  else
    {
      imagefile = ligma_imagefile_new (ligma, file);
      ligma_container_add (container, LIGMA_OBJECT (imagefile));
      g_object_unref (imagefile);
    }

  ligma_imagefile_set_mime_type (imagefile, mime_type);

  if (ligma->config->save_document_history)
    ligma_recent_list_add_file (ligma, file, mime_type);

  return imagefile;
}
