/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpitem.h
 * Copyright (C) Jehan
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

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_ITEM_H__
#define __GIMP_ITEM_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_ITEM (gimp_item_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpItem, gimp_item, GIMP, ITEM, GObject)


struct _GimpItemClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_gimp_reserved0) (void);
  void (*_gimp_reserved1) (void);
  void (*_gimp_reserved2) (void);
  void (*_gimp_reserved3) (void);
  void (*_gimp_reserved4) (void);
  void (*_gimp_reserved5) (void);
  void (*_gimp_reserved6) (void);
  void (*_gimp_reserved7) (void);
  void (*_gimp_reserved8) (void);
  void (*_gimp_reserved9) (void);
};


gint32      gimp_item_get_id          (GimpItem *item);
GimpItem  * gimp_item_get_by_id       (gint32    item_id);

gboolean    gimp_item_is_valid        (GimpItem *item);
gboolean    gimp_item_is_drawable     (GimpItem *item);
gboolean    gimp_item_is_layer        (GimpItem *item);
gboolean    gimp_item_is_text_layer   (GimpItem *item);
gboolean    gimp_item_is_vector_layer (GimpItem *item);
gboolean    gimp_item_is_link_layer   (GimpItem *item);
gboolean    gimp_item_is_group_layer  (GimpItem *item);
gboolean    gimp_item_is_channel      (GimpItem *item);
gboolean    gimp_item_is_layer_mask   (GimpItem *item);
gboolean    gimp_item_is_selection    (GimpItem *item);
gboolean    gimp_item_is_path         (GimpItem *item);

GList     * gimp_item_list_children   (GimpItem *item);


G_END_DECLS

#endif /* __GIMP_ITEM_H__ */
