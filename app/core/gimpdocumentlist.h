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

#ifndef __LIGMA_DOCUMENT_LIST_H__
#define __LIGMA_DOCUMENT_LIST_H__

#include "core/ligmalist.h"


#define LIGMA_TYPE_DOCUMENT_LIST           (ligma_document_list_get_type ())
#define LIGMA_DOCUMENT_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DOCUMENT_LIST, LigmaDocumentList))
#define LIGMA_DOCUMENT_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DOCUMENT_LIST, LigmaDocumentListClass))
#define LIGMA_IS_DOCUMENT_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DOCUMENT_LIST))
#define LIGMA_IS_DOCUMENT_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DOCUMENT_LIST))


typedef struct _LigmaDocumentListClass LigmaDocumentListClass;

struct _LigmaDocumentList
{
  LigmaList  parent_instance;

  Ligma     *ligma;
};

struct _LigmaDocumentListClass
{
  LigmaListClass  parent_class;
};


GType           ligma_document_list_get_type (void) G_GNUC_CONST;
LigmaContainer * ligma_document_list_new      (Ligma             *ligma);

LigmaImagefile * ligma_document_list_add_file (LigmaDocumentList *document_list,
                                             GFile            *file,
                                             const gchar      *mime_type);


#endif  /*  __LIGMA_DOCUMENT_LIST_H__  */
