/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_COMPAT_ENUMS_H__
#define __GIMP_COMPAT_ENUMS_H__


G_BEGIN_DECLS

/*  These enums exist only for compatibility, their nicks are needed
 *  for config file parsing; they are registered in gimp_base_init().
 */


#if 0
/*  Leave one enum here for documentation purposes, remove it as
 *  soon as we need a real compat enum again, also don't have
 *  that "skip" on the compat enum...
 */
#define GIMP_TYPE_ADD_MASK_TYPE_COMPAT (gimp_add_mask_type_compat_get_type ())

GType gimp_add_mask_type_compat_get_type (void) G_GNUC_CONST;

typedef enum /*< skip >*/
{
  GIMP_ADD_WHITE_MASK          = GIMP_ADD_MASK_WHITE,
  GIMP_ADD_BLACK_MASK          = GIMP_ADD_MASK_BLACK,
  GIMP_ADD_ALPHA_MASK          = GIMP_ADD_MASK_ALPHA,
  GIMP_ADD_ALPHA_TRANSFER_MASK = GIMP_ADD_MASK_ALPHA_TRANSFER,
  GIMP_ADD_SELECTION_MASK      = GIMP_ADD_MASK_SELECTION,
  GIMP_ADD_COPY_MASK           = GIMP_ADD_MASK_COPY,
  GIMP_ADD_CHANNEL_MASK        = GIMP_ADD_MASK_CHANNEL
} GimpAddMaskTypeCompat;
#endif


G_END_DECLS

#endif  /* __GIMP_COMPAT_ENUMS_H__ */
