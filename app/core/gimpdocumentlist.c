/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimpdocumentlist.h"
#include "gimpimagefile.h"


G_DEFINE_TYPE (GimpDocumentList, gimp_document_list, GIMP_TYPE_LIST)


static void
gimp_document_list_class_init (GimpDocumentListClass *klass)
{
}

static void
gimp_document_list_init (GimpDocumentList *list)
{
}

GimpContainer *
gimp_document_list_new (Gimp *gimp)
{
  GimpDocumentList *document_list;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  document_list = g_object_new (GIMP_TYPE_DOCUMENT_LIST,
                                "name",          "document-list",
                                "children-type", GIMP_TYPE_IMAGEFILE,
                                "policy",        GIMP_CONTAINER_POLICY_STRONG,
                                NULL);

  document_list->gimp = gimp;

  return GIMP_CONTAINER (document_list);
}

GimpImagefile *
gimp_document_list_add_file (GimpDocumentList *document_list,
                             GFile            *file,
                             const gchar      *mime_type)
{
  Gimp          *gimp;
  GimpImagefile *imagefile;
  GimpContainer *container;
  gchar         *uri;

  g_return_val_if_fail (GIMP_IS_DOCUMENT_LIST (document_list), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  container = GIMP_CONTAINER (document_list);

  gimp = document_list->gimp;

  uri = g_file_get_uri (file);

  imagefile = (GimpImagefile *) gimp_container_get_child_by_name (container,
                                                                  uri);

  g_free (uri);

  if (imagefile)
    {
      gimp_container_reorder (container, GIMP_OBJECT (imagefile), 0);
    }
  else
    {
      imagefile = gimp_imagefile_new (gimp, file);
      gimp_container_add (container, GIMP_OBJECT (imagefile));
      g_object_unref (imagefile);
    }

  gimp_imagefile_set_mime_type (imagefile, mime_type);

  if (gimp->config->save_document_history)
    gimp_recent_list_add_file (gimp, file, mime_type);

  return imagefile;
}
