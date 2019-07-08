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


#define GIMP_TYPE_IMAGE_COMBO_BOX       (gimp_image_combo_box_get_type ())
#define GIMP_IMAGE_COMBO_BOX(obj)       (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_IMAGE_COMBO_BOX, GimpImageComboBox))
#define GIMP_IS_IMAGE_COMBO_BOX(obj)    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_IMAGE_COMBO_BOX)


typedef gboolean (* GimpImageConstraintFunc) (gint32   image_id,
                                              gpointer data);


GType       gimp_image_combo_box_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_image_combo_box_new (GimpImageConstraintFunc  constraint,
                                      gpointer                 data);


G_END_DECLS

#endif /* __GIMP_IMAGE_COMBO_BOX_H__ */
