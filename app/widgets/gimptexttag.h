/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTextTag
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_TEXT_TAG_H__
#define __GIMP_TEXT_TAG_H__


/*  GtkTextTag property names  */

#define GIMP_TEXT_PROP_NAME_SIZE     "size"
#define GIMP_TEXT_PROP_NAME_BASELINE "rise"
#define GIMP_TEXT_PROP_NAME_KERNING  "rise" /* FIXME */
#define GIMP_TEXT_PROP_NAME_FONT     "font"
#define GIMP_TEXT_PROP_NAME_FG_COLOR "foreground-rgba"
#define GIMP_TEXT_PROP_NAME_BG_COLOR "background-rgba"


gint       gimp_text_tag_get_size     (GtkTextTag *tag);
gint       gimp_text_tag_get_baseline (GtkTextTag *tag);
gint       gimp_text_tag_get_kerning  (GtkTextTag *tag);
gchar    * gimp_text_tag_get_font     (GtkTextTag *tag);
gboolean   gimp_text_tag_get_fg_color (GtkTextTag *tag,
                                       GimpRGB    *color);
gboolean   gimp_text_tag_get_bg_color (GtkTextTag *tag,
                                       GimpRGB    *color);


#endif /* __GIMP_TEXT_TAG_H__ */
