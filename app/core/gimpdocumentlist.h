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

#pragma once

#include "gimplist.h"


#define GIMP_TYPE_DOCUMENT_LIST           (gimp_document_list_get_type ())
#define GIMP_DOCUMENT_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DOCUMENT_LIST, GimpDocumentList))
#define GIMP_DOCUMENT_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DOCUMENT_LIST, GimpDocumentListClass))
#define GIMP_IS_DOCUMENT_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DOCUMENT_LIST))
#define GIMP_IS_DOCUMENT_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DOCUMENT_LIST))


typedef struct _GimpDocumentListClass GimpDocumentListClass;

struct _GimpDocumentList
{
  GimpList  parent_instance;

  Gimp     *gimp;
};

struct _GimpDocumentListClass
{
  GimpListClass  parent_class;
};


GType           gimp_document_list_get_type (void) G_GNUC_CONST;
GimpContainer * gimp_document_list_new      (Gimp             *gimp);

GimpImagefile * gimp_document_list_add_file (GimpDocumentList *document_list,
                                             GFile            *file,
                                             const gchar      *mime_type);
