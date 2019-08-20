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

#define GIMP_TYPE_ITEM            (gimp_item_get_type ())
#define GIMP_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ITEM, GimpItem))
#define GIMP_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ITEM, GimpItemClass))
#define GIMP_IS_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ITEM))
#define GIMP_IS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ITEM))
#define GIMP_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ITEM, GimpItemClass))


typedef struct _GimpItemClass   GimpItemClass;
typedef struct _GimpItemPrivate GimpItemPrivate;

struct _GimpItem
{
  GObject           parent_instance;

  GimpItemPrivate *priv;
};

struct _GimpItemClass
{
  GObjectClass parent_class;

  /* Signals. */
  void (* destroyed) (GimpItem *image);

  /* Padding for future expansion */
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

GType         gimp_item_get_type         (void) G_GNUC_CONST;

gint32        gimp_item_get_id           (GimpItem       *item);
GimpItem    * gimp_item_get_by_id        (gint32          item_id);


G_GNUC_INTERNAL
void          _gimp_item_process_signal  (gint32          item_id,
                                          const gchar    *name,
                                          GimpValueArray *params);


#ifndef GIMP_DEPRECATED_REPLACE_NEW_API

GList       * gimp_item_get_children (GimpItem     *item);

#else /* GIMP_DEPRECATED_REPLACE_NEW_API */

#define gimp_item_get_children gimp_item_get_children_deprecated

#endif /* GIMP_DEPRECATED_REPLACE_NEW_API */


gint        * gimp_item_get_children_deprecated (gint32  item_id,
                                                 gint   *num_children);


G_END_DECLS

#endif /* __GIMP_ITEM_H__ */
