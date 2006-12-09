/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_IMAGE_PREVIEW_H__
#define __GIMP_IMAGE_PREVIEW_H__


/*
 *  virtual functions of GimpImage -- dont't call directly
 */

void      gimp_image_get_preview_size (GimpViewable *viewable,
                                       gint          size,
                                       gboolean      is_popup,
                                       gboolean      dot_for_dot,
                                       gint         *width,
                                       gint         *height);
gboolean  gimp_image_get_popup_size   (GimpViewable *viewable,
                                       gint          width,
                                       gint          height,
                                       gboolean      dot_for_dot,
                                       gint         *popup_width,
                                       gint         *popup_height);
TempBuf * gimp_image_get_preview      (GimpViewable *viewable,
                                       GimpContext  *context,
                                       gint          width,
                                       gint          height);
TempBuf * gimp_image_get_new_preview  (GimpViewable *viewable,
                                       GimpContext  *context,
                                       gint          width,
                                       gint          height);


#endif /* __GIMP_IMAGE_PREVIEW_H__ */
