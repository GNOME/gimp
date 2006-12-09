/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * color-history.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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

#ifndef __COLOR_HISTORY_H__
#define __COLOR_HISTORY_H__


#define COLOR_HISTORY_SIZE 12

void   color_history_save        (Gimp             *gimp);
void   color_history_restore     (Gimp             *gimp);

gint   color_history_add         (const GimpRGB    *rgb);
void   color_history_set         (gint              index,
                                  const GimpRGB    *rgb);
void   color_history_get         (gint              index,
                                  GimpRGB          *rgb);


#endif /* __COLOR_HISTORY_H__ */
