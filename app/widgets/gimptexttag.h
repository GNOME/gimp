/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaTextTag
 * Copyright (C) 2010  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TEXT_TAG_H__
#define __LIGMA_TEXT_TAG_H__


/*  GtkTextTag property names  */

#define LIGMA_TEXT_PROP_NAME_SIZE     "size"
#define LIGMA_TEXT_PROP_NAME_BASELINE "rise"
#define LIGMA_TEXT_PROP_NAME_KERNING  "rise" /* FIXME */
#define LIGMA_TEXT_PROP_NAME_FONT     "font"
#define LIGMA_TEXT_PROP_NAME_FG_COLOR "foreground-rgba"
#define LIGMA_TEXT_PROP_NAME_BG_COLOR "background-rgba"


gint       ligma_text_tag_get_size     (GtkTextTag *tag);
gint       ligma_text_tag_get_baseline (GtkTextTag *tag);
gint       ligma_text_tag_get_kerning  (GtkTextTag *tag);
gchar    * ligma_text_tag_get_font     (GtkTextTag *tag);
gboolean   ligma_text_tag_get_fg_color (GtkTextTag *tag,
                                       LigmaRGB    *color);
gboolean   ligma_text_tag_get_bg_color (GtkTextTag *tag,
                                       LigmaRGB    *color);


#endif /* __LIGMA_TEXT_TAG_H__ */
