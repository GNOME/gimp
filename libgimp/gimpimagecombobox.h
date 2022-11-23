/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaimagecombobox.h
 * Copyright (C) 2004 Sven Neumann <sven@ligma.org>
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

#if !defined (__LIGMA_UI_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligmaui.h> can be included directly."
#endif

#ifndef __LIGMA_IMAGE_COMBO_BOX_H__
#define __LIGMA_IMAGE_COMBO_BOX_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

/**
 * LigmaImageConstraintFunc:
 * @image:
 * @data: (closure):
 */
typedef gboolean (* LigmaImageConstraintFunc) (LigmaImage *image,
                                              gpointer   data);


#define LIGMA_TYPE_IMAGE_COMBO_BOX       (ligma_image_combo_box_get_type ())
#define LIGMA_IMAGE_COMBO_BOX(obj)       (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_IMAGE_COMBO_BOX, LigmaImageComboBox))
#define LIGMA_IS_IMAGE_COMBO_BOX(obj)    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_IMAGE_COMBO_BOX)


GType       ligma_image_combo_box_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_image_combo_box_new      (LigmaImageConstraintFunc  constraint,
                                           gpointer                 data,
                                           GDestroyNotify           data_destroy);


G_END_DECLS

#endif /* __LIGMA_IMAGE_COMBO_BOX_H__ */
