/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpbrushes.h
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

#ifndef __GIMP_BRUSHES_H__
#define __GIMP_BRUSHES_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

GIMP_DEPRECATED_FOR(gimp_context_get_opacity)
gdouble         gimp_brushes_get_opacity    (void);
GIMP_DEPRECATED_FOR(gimp_context_set_opacity)
gboolean        gimp_brushes_set_opacity    (gdouble        opacity);
GIMP_DEPRECATED_FOR(gimp_context_get_paint_mode)
GimpLayerMode   gimp_brushes_get_paint_mode (void);
GIMP_DEPRECATED_FOR(gimp_context_set_paint_mode)
gboolean        gimp_brushes_set_paint_mode (GimpLayerMode  paint_mode);
GIMP_DEPRECATED_FOR(gimp_context_set_brush)
gboolean        gimp_brushes_set_brush      (const gchar   *name);

G_END_DECLS

#endif /* __GIMP_BRUSHES_H__ */
