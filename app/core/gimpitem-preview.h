/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_ITEM__PREVIEW_H__
#define __GIMP_ITEM__PREVIEW_H__


/*
 *  virtual functions of GimpItem -- don't call directly
 */

void      gimp_item_get_preview_size (GimpViewable *viewable,
                                      gint          size,
                                      gboolean      is_popup,
                                      gboolean      dot_for_dot,
                                      gint         *width,
                                      gint         *height);
gboolean  gimp_item_get_popup_size   (GimpViewable *viewable,
                                      gint          width,
                                      gint          height,
                                      gboolean      dot_for_dot,
                                      gint         *popup_width,
                                      gint         *popup_height);


#endif /* __GIMP_ITEM__PREVIEW_H__ */
