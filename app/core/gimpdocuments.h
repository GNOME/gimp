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

#ifndef __GIMP_DOCUMENTS_H__
#define __GIMP_DOCUMENTS_H__

#include "core/gimplist.h"


#define GIMP_TYPE_DOCUMENTS            (gimp_documents_get_type ())
#define GIMP_DOCUMENTS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DOCUMENTS, GimpDocuments))
#define GIMP_DOCUMENTS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DOCUMENTS, GimpDocumentsClass))
#define GIMP_IS_DOCUMENTS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DOCUMENTS))
#define GIMP_IS_DOCUMENTS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DOCUMENTS))


typedef struct _GimpListClass GimpDocumentsClass;


GType           gimp_documents_get_type (void) G_GNUC_CONST; 
GimpContainer * gimp_documents_new      (void);

void            gimp_documents_load     (GimpDocuments *documents,
                                         gint           thumbnail_size);
void            gimp_documents_save     (GimpDocuments *documents);

GimpImagefile * gimp_documents_add      (GimpDocuments *documents,
                                         const gchar   *uri);


#endif  /*  __GIMP_DOCUMENTS_H__  */
