/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpimagecombobox.h
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimpui.h> can be included directly."
#endif

#ifndef __GIMP_IMAGE_COMBO_BOX_H__
#define __GIMP_IMAGE_COMBO_BOX_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

/**
 * GimpImageConstraintFunc:
 * @image:
 * @data: (closure):
 */
typedef gboolean (* GimpImageConstraintFunc) (GimpImage *image,
                                              gpointer   data);


#define GIMP_TYPE_IMAGE_COMBO_BOX       (gimp_image_combo_box_get_type ())
G_DECLARE_FINAL_TYPE (GimpImageComboBox, gimp_image_combo_box, GIMP, IMAGE_COMBO_BOX, GimpIntComboBox)


GtkWidget * gimp_image_combo_box_new (GimpImageConstraintFunc  constraint,
                                      gpointer                 data,
                                      GDestroyNotify           data_destroy);


G_END_DECLS

#endif /* __GIMP_IMAGE_COMBO_BOX_H__ */
