/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpbrushes.h
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_BRUSHES_H__
#define __GIMP_BRUSHES_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

#ifndef GIMP_DISABLE_DEPRECATED

gdouble              gimp_brushes_get_opacity    (void);
gboolean             gimp_brushes_set_opacity    (gdouble               opacity);
GimpLayerModeEffects gimp_brushes_get_paint_mode (void);
gboolean             gimp_brushes_set_paint_mode (GimpLayerModeEffects  paint_mode);
gboolean             gimp_brushes_set_brush      (const gchar          *name);

#endif /* GIMP_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* __GIMP_BRUSHES_H__ */
