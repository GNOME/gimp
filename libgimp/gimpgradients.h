/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpgradients.h
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

#ifndef __GIMP_GRADIENTS_H__
#define __GIMP_GRADIENTS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

GIMP_DEPRECATED_FOR(gimp_context_get_gradient)
gchar    * gimp_gradients_get_gradient (void);
GIMP_DEPRECATED_FOR(gimp_context_set_gradient)
gboolean   gimp_gradients_set_gradient (const gchar *name);

G_END_DECLS

#endif /* __GIMP_GRADIENTS_H__ */
