/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaeevl.h
 * Copyright (C) 2008-2009 Fredrik Alstromer <roe@excu.se>
 * Copyright (C) 2008-2009 Martin Nordholts <martinn@svn.gnome.org>
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

#ifndef __LIGMA_EEVL_H__
#define __LIGMA_EEVL_H__

G_BEGIN_DECLS


/**
 * LigmaEevlQuantity:
 * @value:     In reference units.
 * @dimension: in has a dimension of 1, in^2 has a dimension of 2 etc
 */
typedef struct
{
  gdouble value;
  gint dimension;
} LigmaEevlQuantity;


/**
 * LigmaEevlUnitResolverProc:
 * @identifier: Identifier of unit to resolve or %NULL if default unit
 *              should be resolved.
 * @factor:     Units per reference unit. For example, in LIGMA the
 *              reference unit is inches so resolving "mm" should
 *              return 25.4 since there are 25.4 millimeters per inch.
 * @offset:     Offset to apply after scaling the value according to @factor.
 * @data:       Data given to ligma_eevl_evaluate().
 *
 * Returns: If the unit was successfully resolved or not.
 *
 */
typedef gboolean (* LigmaEevlUnitResolverProc) (const gchar      *identifier,
                                               LigmaEevlQuantity *factor,
                                               gdouble          *offset,
                                               gpointer          data);


/**
 * LigmaEevlOptions:
 * @unit_resolver_proc: Unit resolver callback.
 * @data:               Data passed to unit resolver.
 * @ratio_expressions:  Allow ratio expressions
 * @ratio_invert:       Invert ratios
 * @ratio_quantity:     Quantity to multiply ratios by
 */
typedef struct
{
  LigmaEevlUnitResolverProc unit_resolver_proc;
  gpointer                 data;

  gboolean                 ratio_expressions;
  gboolean                 ratio_invert;
  LigmaEevlQuantity         ratio_quantity;
} LigmaEevlOptions;

#define LIGMA_EEVL_OPTIONS_INIT                                                 \
  ((const LigmaEevlOptions)                                                     \
  {                                                                            \
    .unit_resolver_proc = NULL,                                                \
    .data               = NULL,                                                \
                                                                               \
    .ratio_expressions  = FALSE,                                               \
    .ratio_invert       = FALSE,                                               \
    .ratio_quantity     = {0.0, 0}                                             \
  })


gboolean ligma_eevl_evaluate (const gchar            *string,
                             const LigmaEevlOptions  *options,
                             LigmaEevlQuantity       *result,
                             const gchar           **error_pos,
                             GError                **error);


G_END_DECLS

#endif /* __LIGMA_EEVL_H__ */
