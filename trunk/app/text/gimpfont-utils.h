/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfont-utils.h
 * Copyright (C) 2005 Manish Singh <yosh@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_FONT_UTILS_H__
#define __GIMP_FONT_UTILS_H__


/* This is solely to workaround pango bug #166540, by tacking on a ',' to
 * font names that end in numbers, so pango_font_description_from_string
 * doesn't interpret it as a size. Note that this doesn't fully workaround
 * problems pango has with font name serialization, just only the bad size
 * interpretation. Family names that end with style names are still
 * processed wrongly.
 */
gchar * gimp_font_util_pango_font_description_to_string (const PangoFontDescription *desc);


#endif  /*  __GIMP_FONT_UTILS_H__  */
