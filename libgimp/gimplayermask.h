/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimplayermask.h
 * Copyright (C) Jehan
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

#ifndef __GIMP_LAYER_MASK_H__
#define __GIMP_LAYER_MASK_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#include <libgimp/gimpchannel.h>


#define GIMP_TYPE_LAYER_MASK (gimp_layer_mask_get_type ())
G_DECLARE_FINAL_TYPE (GimpLayerMask, gimp_layer_mask, GIMP, LAYER_MASK, GimpChannel)


GimpLayerMask * gimp_layer_mask_get_by_id (gint32 layer_mask_id);


G_END_DECLS

#endif /* __GIMP_LAYER_MASK_H__ */
