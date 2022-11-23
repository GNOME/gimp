/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaitemcombobox.h
 * Copyright (C) 2004 Sven Neumann <sven@ligma.org>
 * Copyright (C) 2006 Simon Budig <simon@ligma.org>
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

#ifndef __LIGMA_ITEM_COMBO_BOX_H__
#define __LIGMA_ITEM_COMBO_BOX_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_TYPE_DRAWABLE_COMBO_BOX    (ligma_drawable_combo_box_get_type ())
#define LIGMA_DRAWABLE_COMBO_BOX(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DRAWABLE_COMBO_BOX, LigmaDrawableComboBox))
#define LIGMA_IS_DRAWABLE_COMBO_BOX(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DRAWABLE_COMBO_BOX))

#define LIGMA_TYPE_CHANNEL_COMBO_BOX     (ligma_channel_combo_box_get_type ())
#define LIGMA_CHANNEL_COMBO_BOX(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CHANNEL_COMBO_BOX, LigmaChannelComboBox))
#define LIGMA_IS_CHANNEL_COMBO_BOX(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CHANNEL_COMBO_BOX))

#define LIGMA_TYPE_LAYER_COMBO_BOX       (ligma_layer_combo_box_get_type ())
#define LIGMA_LAYER_COMBO_BOX(obj)       (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LAYER_COMBO_BOX, LigmaLayerComboBox))
#define LIGMA_IS_LAYER_COMBO_BOX(obj)    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LAYER_COMBO_BOX))

#define LIGMA_TYPE_VECTORS_COMBO_BOX     (ligma_vectors_combo_box_get_type ())
#define LIGMA_VECTORS_COMBO_BOX(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_VECTORS_COMBO_BOX, LigmaVectorsComboBox))
#define LIGMA_IS_VECTORS_COMBO_BOX(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_VECTORS_COMBO_BOX))


GType       ligma_drawable_combo_box_get_type (void) G_GNUC_CONST;
GType       ligma_channel_combo_box_get_type  (void) G_GNUC_CONST;
GType       ligma_layer_combo_box_get_type    (void) G_GNUC_CONST;
GType       ligma_vectors_combo_box_get_type  (void) G_GNUC_CONST;

/**
 * LigmaItemConstraintFunc:
 * @image:
 * @item:
 * @data: (closure):
 */
typedef gboolean (* LigmaItemConstraintFunc) (LigmaImage *image,
                                             LigmaItem  *item,
                                             gpointer   data);

GtkWidget * ligma_drawable_combo_box_new (LigmaItemConstraintFunc constraint,
                                         gpointer               data,
                                         GDestroyNotify         data_destroy);
GtkWidget * ligma_channel_combo_box_new  (LigmaItemConstraintFunc constraint,
                                         gpointer               data,
                                         GDestroyNotify         data_destroy);
GtkWidget * ligma_layer_combo_box_new    (LigmaItemConstraintFunc constraint,
                                         gpointer               data,
                                         GDestroyNotify         data_destroy);
GtkWidget * ligma_vectors_combo_box_new  (LigmaItemConstraintFunc constraint,
                                         gpointer               data,
                                         GDestroyNotify         data_destroy);


G_END_DECLS

#endif /* __LIGMA_ITEM_COMBO_BOX_H__ */
