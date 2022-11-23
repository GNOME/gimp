/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaintstore.c
 * Copyright (C) 2004  Sven Neumann <sven@ligma.org>
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

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_INT_STORE_H__
#define __LIGMA_INT_STORE_H__

G_BEGIN_DECLS


/**
 * LigmaIntStoreColumns:
 * @LIGMA_INT_STORE_VALUE:       the integer value
 * @LIGMA_INT_STORE_LABEL:       a human-readable label
 * @LIGMA_INT_STORE_ABBREV:      an abbreviated label
 * @LIGMA_INT_STORE_ICON_NAME:   an icon name
 * @LIGMA_INT_STORE_PIXBUF:      a #GdkPixbuf
 * @LIGMA_INT_STORE_USER_DATA:   arbitrary user data
 * @LIGMA_INT_STORE_NUM_COLUMNS: the number of columns
 *
 * The column types of #LigmaIntStore.
 **/
typedef enum
{
  LIGMA_INT_STORE_VALUE,
  LIGMA_INT_STORE_LABEL,
  LIGMA_INT_STORE_ABBREV,
  LIGMA_INT_STORE_ICON_NAME,
  LIGMA_INT_STORE_PIXBUF,
  LIGMA_INT_STORE_USER_DATA,
  LIGMA_INT_STORE_NUM_COLUMNS
} LigmaIntStoreColumns;


#define LIGMA_TYPE_INT_STORE            (ligma_int_store_get_type ())
#define LIGMA_INT_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_INT_STORE, LigmaIntStore))
#define LIGMA_INT_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_INT_STORE, LigmaIntStoreClass))
#define LIGMA_IS_INT_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_INT_STORE))
#define LIGMA_IS_INT_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_INT_STORE))
#define LIGMA_INT_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_INT_STORE, LigmaIntStoreClass))


typedef struct _LigmaIntStorePrivate LigmaIntStorePrivate;
typedef struct _LigmaIntStoreClass   LigmaIntStoreClass;

struct _LigmaIntStore
{
  GtkListStore         parent_instance;

  LigmaIntStorePrivate *priv;
};

struct _LigmaIntStoreClass
{
  GtkListStoreClass  parent_class;

  /* Padding for future expansion */
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GType          ligma_int_store_get_type        (void) G_GNUC_CONST;

GtkListStore * ligma_int_store_new             (const gchar   *first_label,
                                               gint           first_value,
                                               ...) G_GNUC_NULL_TERMINATED;
GtkListStore * ligma_int_store_new_valist      (const gchar   *first_label,
                                               gint           first_value,
                                               va_list        values);

gboolean       ligma_int_store_lookup_by_value (GtkTreeModel  *model,
                                               gint           value,
                                               GtkTreeIter   *iter);
gboolean   ligma_int_store_lookup_by_user_data (GtkTreeModel  *model,
                                               gpointer       user_data,
                                               GtkTreeIter   *iter);


G_END_DECLS

#endif  /* __LIGMA_INT_STORE_H__ */
