/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_BASE_TYPES_H__
#define __GIMP_BASE_TYPES_H__


#include <libgimpcolor/gimpcolortypes.h>
#include <libgimpmath/gimpmathtypes.h>

#include <libgimpbase/gimpbaseenums.h>

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


typedef struct _GimpParasite     GimpParasite;
typedef struct _GimpDatafileData GimpDatafileData;


typedef void (* GimpDatafileLoaderFunc) (const GimpDatafileData *file_data,
                                         gpointer                user_data);


void           gimp_type_set_translation_domain (GType        type,
                                                 const gchar *domain);
const gchar *  gimp_type_get_translation_domain (GType        type);


G_END_DECLS

#endif  /* __GIMP_BASE_TYPES_H__ */
