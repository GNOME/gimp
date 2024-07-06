/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpselection.h
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

#ifndef __GIMP_SELECTION_H__
#define __GIMP_SELECTION_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#include <libgimp/gimpchannel.h>


#define GIMP_TYPE_SELECTION (gimp_selection_get_type ())
G_DECLARE_FINAL_TYPE (GimpSelection, gimp_selection, GIMP, SELECTION, GimpChannel)


GimpSelection * gimp_selection_get_by_id (gint32        selection_id);

GimpLayer     * gimp_selection_float     (GimpImage    *image,
                                          gint           n_drawables,
                                          GimpDrawable **drawables,
                                          gint          offx,
                                          gint          offy);


G_END_DECLS

#endif /* __GIMP_SELECTION_H__ */
