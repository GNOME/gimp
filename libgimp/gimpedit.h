/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpedit.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_EDIT_H__
#define __GIMP_EDIT_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


GIMP_DEPRECATED_FOR(gimp_edit_paste_as_new_image)
gint32   gimp_edit_paste_as_new       (void);

GIMP_DEPRECATED_FOR(gimp_edit_named_paste_as_new_image)
gint32   gimp_edit_named_paste_as_new (const gchar *buffer_name);

G_END_DECLS

#endif /* __GIMP_EDIT_H__ */
