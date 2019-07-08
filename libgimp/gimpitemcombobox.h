/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpitemcombobox.h
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
 * Copyright (C) 2006 Simon Budig <simon@gimp.org>
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

#ifndef __GIMP_ITEM_COMBO_BOX_H__
#define __GIMP_ITEM_COMBO_BOX_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_DRAWABLE_COMBO_BOX    (gimp_drawable_combo_box_get_type ())
#define GIMP_DRAWABLE_COMBO_BOX(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DRAWABLE_COMBO_BOX, GimpDrawableComboBox))
#define GIMP_IS_DRAWABLE_COMBO_BOX(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DRAWABLE_COMBO_BOX))

#define GIMP_TYPE_CHANNEL_COMBO_BOX     (gimp_channel_combo_box_get_type ())
#define GIMP_CHANNEL_COMBO_BOX(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CHANNEL_COMBO_BOX, GimpChannelComboBox))
#define GIMP_IS_CHANNEL_COMBO_BOX(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CHANNEL_COMBO_BOX))

#define GIMP_TYPE_LAYER_COMBO_BOX       (gimp_layer_combo_box_get_type ())
#define GIMP_LAYER_COMBO_BOX(obj)       (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LAYER_COMBO_BOX, GimpLayerComboBox))
#define GIMP_IS_LAYER_COMBO_BOX(obj)    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LAYER_COMBO_BOX))

#define GIMP_TYPE_VECTORS_COMBO_BOX     (gimp_vectors_combo_box_get_type ())
#define GIMP_VECTORS_COMBO_BOX(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_VECTORS_COMBO_BOX, GimpVectorsComboBox))
#define GIMP_IS_VECTORS_COMBO_BOX(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_VECTORS_COMBO_BOX))


typedef gboolean (* GimpItemConstraintFunc) (gint32   image_id,
                                             gint32   item_id,
                                             gpointer data);

typedef GimpItemConstraintFunc GimpVectorsConstraintFunc;
typedef GimpItemConstraintFunc GimpDrawableConstraintFunc;


GType       gimp_drawable_combo_box_get_type (void) G_GNUC_CONST;
GType       gimp_channel_combo_box_get_type  (void) G_GNUC_CONST;
GType       gimp_layer_combo_box_get_type    (void) G_GNUC_CONST;
GType       gimp_vectors_combo_box_get_type  (void) G_GNUC_CONST;

GtkWidget * gimp_drawable_combo_box_new (GimpDrawableConstraintFunc constraint,
                                         gpointer                   data);
GtkWidget * gimp_channel_combo_box_new  (GimpDrawableConstraintFunc constraint,
                                         gpointer                   data);
GtkWidget * gimp_layer_combo_box_new    (GimpDrawableConstraintFunc constraint,
                                         gpointer                   data);
GtkWidget * gimp_vectors_combo_box_new  (GimpVectorsConstraintFunc  constraint,
                                         gpointer                   data);


G_END_DECLS

#endif /* __GIMP_ITEM_COMBO_BOX_H__ */
