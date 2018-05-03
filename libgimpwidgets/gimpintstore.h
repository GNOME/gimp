/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpintstore.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_INT_STORE_H__
#define __GIMP_INT_STORE_H__

G_BEGIN_DECLS


/**
 * GimpIntStoreColumns:
 * @GIMP_INT_STORE_VALUE:       the integer value
 * @GIMP_INT_STORE_LABEL:       a human-readable label
 * @GIMP_INT_STORE_ICON_NAME:   an icon name
 * @GIMP_INT_STORE_PIXBUF:      a #GdkPixbuf
 * @GIMP_INT_STORE_USER_DATA:   arbitrary user data
 * @GIMP_INT_STORE_ABBREV:      an abbreviated label
 * @GIMP_INT_STORE_NUM_COLUMNS: the number of columns
 *
 * The column types of #GimpIntStore.
 **/
typedef enum
{
  GIMP_INT_STORE_VALUE,
  GIMP_INT_STORE_LABEL,
  GIMP_INT_STORE_ICON_NAME,
  GIMP_INT_STORE_PIXBUF,
  GIMP_INT_STORE_USER_DATA,
  GIMP_INT_STORE_ABBREV,
  GIMP_INT_STORE_NUM_COLUMNS
} GimpIntStoreColumns;


#define GIMP_TYPE_INT_STORE            (gimp_int_store_get_type ())
#define GIMP_INT_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_INT_STORE, GimpIntStore))
#define GIMP_INT_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_INT_STORE, GimpIntStoreClass))
#define GIMP_IS_INT_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_INT_STORE))
#define GIMP_IS_INT_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_INT_STORE))
#define GIMP_INT_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_INT_STORE, GimpIntStoreClass))


typedef struct _GimpIntStorePrivate GimpIntStorePrivate;
typedef struct _GimpIntStoreClass   GimpIntStoreClass;

struct _GimpIntStore
{
  GtkListStore         parent_instance;

  GimpIntStorePrivate *priv;

  /* FIXME MOVE TO PRIVATE */
  GtkTreeIter  *empty_iter;
};

struct _GimpIntStoreClass
{
  GtkListStoreClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


GType          gimp_int_store_get_type        (void) G_GNUC_CONST;

GtkListStore * gimp_int_store_new             (void);

gboolean       gimp_int_store_lookup_by_value (GtkTreeModel  *model,
                                               gint           value,
                                               GtkTreeIter   *iter);
gboolean   gimp_int_store_lookup_by_user_data (GtkTreeModel  *model,
                                               gpointer       user_data,
                                               GtkTreeIter   *iter);


G_END_DECLS

#endif  /* __GIMP_INT_STORE_H__ */
