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

#ifndef __GIMP_PALETTE_H__
#define __GIMP_PALETTE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


/*  These will become the default one day  */

gboolean   gimp_palette_set_foreground_rgb (const GimpRGB *rgb);
gboolean   gimp_palette_get_foreground_rgb (GimpRGB       *rgb);
gboolean   gimp_palette_set_background_rgb (const GimpRGB *rgb);
gboolean   gimp_palette_get_background_rgb (GimpRGB       *rgb);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* __GIMP_COLOR_H__ */
