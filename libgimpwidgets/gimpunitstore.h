/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaunitstore.h
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

#ifndef __LIGMA_UNIT_STORE_H__
#define __LIGMA_UNIT_STORE_H__

G_BEGIN_DECLS


enum
{
  LIGMA_UNIT_STORE_UNIT,
  LIGMA_UNIT_STORE_UNIT_FACTOR,
  LIGMA_UNIT_STORE_UNIT_DIGITS,
  LIGMA_UNIT_STORE_UNIT_IDENTIFIER,
  LIGMA_UNIT_STORE_UNIT_SYMBOL,
  LIGMA_UNIT_STORE_UNIT_ABBREVIATION,
  LIGMA_UNIT_STORE_UNIT_SINGULAR,
  LIGMA_UNIT_STORE_UNIT_PLURAL,
  LIGMA_UNIT_STORE_UNIT_SHORT_FORMAT,
  LIGMA_UNIT_STORE_UNIT_LONG_FORMAT,
  LIGMA_UNIT_STORE_UNIT_COLUMNS,
  LIGMA_UNIT_STORE_FIRST_VALUE = LIGMA_UNIT_STORE_UNIT_COLUMNS
};


#define LIGMA_TYPE_UNIT_STORE            (ligma_unit_store_get_type ())
#define LIGMA_UNIT_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_UNIT_STORE, LigmaUnitStore))
#define LIGMA_UNIT_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_UNIT_STORE, LigmaUnitStoreClass))
#define LIGMA_IS_UNIT_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_UNIT_STORE))
#define LIGMA_IS_UNIT_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_UNIT_STORE))
#define LIGMA_UNIT_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_UNIT_STORE, LigmaUnitStoreClass))


typedef struct _LigmaUnitStorePrivate LigmaUnitStorePrivate;
typedef struct _LigmaUnitStoreClass   LigmaUnitStoreClass;

struct _LigmaUnitStore
{
  GObject               parent_instance;

  LigmaUnitStorePrivate *priv;
};

struct _LigmaUnitStoreClass
{
  GObjectClass  parent_class;

  /* Padding for future expansion */
  void (*_ligma_reserved1) (void);
  void (*_ligma_reserved2) (void);
  void (*_ligma_reserved3) (void);
  void (*_ligma_reserved4) (void);
  void (*_ligma_reserved5) (void);
  void (*_ligma_reserved6) (void);
  void (*_ligma_reserved7) (void);
  void (*_ligma_reserved8) (void);
};


GType           ligma_unit_store_get_type         (void) G_GNUC_CONST;

LigmaUnitStore * ligma_unit_store_new              (gint           num_values);

void            ligma_unit_store_set_has_pixels   (LigmaUnitStore *store,
                                                  gboolean       has_pixels);
gboolean        ligma_unit_store_get_has_pixels   (LigmaUnitStore *store);

void            ligma_unit_store_set_has_percent  (LigmaUnitStore *store,
                                                  gboolean       has_percent);
gboolean        ligma_unit_store_get_has_percent  (LigmaUnitStore *store);

void            ligma_unit_store_set_pixel_value  (LigmaUnitStore *store,
                                                  gint           index,
                                                  gdouble        value);
void            ligma_unit_store_set_pixel_values (LigmaUnitStore *store,
                                                  gdouble        first_value,
                                                  ...);
void            ligma_unit_store_set_resolution   (LigmaUnitStore *store,
                                                  gint           index,
                                                  gdouble        resolution);
void            ligma_unit_store_set_resolutions  (LigmaUnitStore *store,
                                                  gdouble        first_resolution,
                                                  ...);
gdouble         ligma_unit_store_get_nth_value    (LigmaUnitStore *store,
                                                  LigmaUnit       unit,
                                                  gint           index);
void            ligma_unit_store_get_values       (LigmaUnitStore *store,
                                                  LigmaUnit       unit,
                                                  gdouble       *first_value,
                                                  ...);

void            _ligma_unit_store_sync_units      (LigmaUnitStore *store);


G_END_DECLS

#endif  /* __LIGMA_UNIT_STORE_H__ */
