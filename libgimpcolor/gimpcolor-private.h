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
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_COLOR_PRIVATE_H__
#define __GIMP_COLOR_PRIVATE_H__

/* Legacy definition to calculate luminance from sRGB.
 *
 * These values and macro are specific to a "RGB float" to "Y float"
 * conversion within sRGB space. These should not be used in new code,
 * where converting to "Y float", space aware, may often be what you
 * wanted to do.
 * Once we got rid of all remaining usages in plug-ins and app, we can
 * delete this file as it's not distributed.
 */
#define GIMP_RGB_LUMINANCE_RED    (0.22248840)
#define GIMP_RGB_LUMINANCE_GREEN  (0.71690369)
#define GIMP_RGB_LUMINANCE_BLUE   (0.06060791)

#define GIMP_RGB_LUMINANCE(r,g,b) ((r) * GIMP_RGB_LUMINANCE_RED   + \
                                   (g) * GIMP_RGB_LUMINANCE_GREEN + \
                                   (b) * GIMP_RGB_LUMINANCE_BLUE)


#endif  /* __GIMP_COLOR_PRIVATE_H__ */
