/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * http://triq.net/~pearl/thumbnail-spec/
 *
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
 *                          Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_THUMB_ERROR_H__
#define __GIMP_THUMB_ERROR_H__

G_BEGIN_DECLS


typedef enum
{
  GIMP_THUMB_ERROR_OPEN,         /*  open failed                            */
  GIMP_THUMB_ERROR_OPEN_ENOENT,  /*  file does not exist                    */
  GIMP_THUMB_ERROR_MKDIR         /*  mkdir failed                           */
} GimpThumbError;


#define GIMP_THUMB_ERROR (gimp_thumb_error_quark ())

GQuark  gimp_thumb_error_quark (void) G_GNUC_CONST;


G_END_DECLS

#endif  /* __GIMP_THUMB_ERROR_H__ */
