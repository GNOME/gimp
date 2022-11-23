/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaunit.h
 * Copyright (C) 1999-2003 Michael Natterer <mitch@ligma.org>
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

#if !defined (__LIGMA_BASE_H_INSIDE__) && !defined (LIGMA_BASE_COMPILATION)
#error "Only <libligmabase/ligmabase.h> can be included directly."
#endif

#ifndef __LIGMA_UNIT_H__
#define __LIGMA_UNIT_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

/**
 * LIGMA_TYPE_UNIT:
 *
 * #LIGMA_TYPE_UNIT is a #GType derived from #G_TYPE_INT.
 **/

#define LIGMA_TYPE_UNIT               (ligma_unit_get_type ())
#define LIGMA_VALUE_HOLDS_UNIT(value) (G_TYPE_CHECK_VALUE_TYPE ((value), LIGMA_TYPE_UNIT))

GType        ligma_unit_get_type      (void) G_GNUC_CONST;


/*
 * LIGMA_TYPE_PARAM_UNIT
 */

#define LIGMA_TYPE_PARAM_UNIT           (ligma_param_unit_get_type ())
#define LIGMA_PARAM_SPEC_UNIT(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_UNIT, LigmaParamSpecUnit))
#define LIGMA_IS_PARAM_SPEC_UNIT(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_UNIT))

typedef struct _LigmaParamSpecUnit LigmaParamSpecUnit;

struct _LigmaParamSpecUnit
{
  GParamSpecInt parent_instance;

  gboolean      allow_percent;
};

GType        ligma_param_unit_get_type     (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_unit         (const gchar  *name,
                                           const gchar  *nick,
                                           const gchar  *blurb,
                                           gboolean      allow_pixels,
                                           gboolean      allow_percent,
                                           LigmaUnit      default_value,
                                           GParamFlags   flags);



gint          ligma_unit_get_number_of_units          (void);
gint          ligma_unit_get_number_of_built_in_units (void) G_GNUC_CONST;

LigmaUnit      ligma_unit_new                 (gchar       *identifier,
                                             gdouble      factor,
                                             gint         digits,
                                             gchar       *symbol,
                                             gchar       *abbreviation,
                                             gchar       *singular,
                                             gchar       *plural);

gboolean      ligma_unit_get_deletion_flag   (LigmaUnit     unit);
void          ligma_unit_set_deletion_flag   (LigmaUnit     unit,
                                             gboolean     deletion_flag);

gdouble       ligma_unit_get_factor          (LigmaUnit     unit);

gint          ligma_unit_get_digits          (LigmaUnit     unit);
gint          ligma_unit_get_scaled_digits   (LigmaUnit     unit,
                                             gdouble      resolution);

const gchar * ligma_unit_get_identifier      (LigmaUnit     unit);

const gchar * ligma_unit_get_symbol          (LigmaUnit     unit);
const gchar * ligma_unit_get_abbreviation    (LigmaUnit     unit);
const gchar * ligma_unit_get_singular        (LigmaUnit     unit);
const gchar * ligma_unit_get_plural          (LigmaUnit     unit);

gchar       * ligma_unit_format_string       (const gchar *format,
                                             LigmaUnit     unit);

gdouble       ligma_pixels_to_units          (gdouble      pixels,
                                             LigmaUnit     unit,
                                             gdouble      resolution);
gdouble       ligma_units_to_pixels          (gdouble      value,
                                             LigmaUnit     unit,
                                             gdouble      resolution);
gdouble       ligma_units_to_points          (gdouble      value,
                                             LigmaUnit     unit,
                                             gdouble      resolution);

gboolean      ligma_unit_is_metric           (LigmaUnit     unit);


G_END_DECLS

#endif /* __LIGMA_UNIT_H__ */
