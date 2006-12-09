/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_TEXT_LAYOUT_H__
#define __GIMP_TEXT_LAYOUT_H__


#define GIMP_TYPE_TEXT_LAYOUT    (gimp_text_layout_get_type ())
#define GIMP_TEXT_LAYOUT(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TEXT_LAYOUT, GimpTextLayout))
#define GIMP_IS_TEXT_LAYOUT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TEXT_LAYOUT))


typedef struct _GimpTextLayoutClass GimpTextLayoutClass;


GType            gimp_text_layout_get_type    (void) G_GNUC_CONST;

GimpTextLayout * gimp_text_layout_new         (GimpText       *text,
                                               GimpImage      *image);
gboolean         gimp_text_layout_get_size    (GimpTextLayout *layout,
                                               gint           *width,
                                               gint           *heigth);
void             gimp_text_layout_get_offsets (GimpTextLayout *layout,
                                               gint           *x,
                                               gint           *y);


#endif /* __GIMP_TEXT_LAYOUT_H__ */
