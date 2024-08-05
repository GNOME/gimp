/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunit.h
 * Copyright (C) 1999-2003 Michael Natterer <mitch@gimp.org>
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

#if !defined (__GIMP_BASE_H_INSIDE__) && !defined (GIMP_BASE_COMPILATION)
#error "Only <libgimpbase/gimpbase.h> can be included directly."
#endif

#ifndef __GIMP_UNIT_H__
#define __GIMP_UNIT_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

/**
 * GIMP_TYPE_UNIT:
 *
 * #GIMP_TYPE_UNIT is a class representing units used for measuring
 * dimensions in GIMP.
 **/

#define GIMP_TYPE_UNIT (gimp_unit_get_type ())
G_DECLARE_FINAL_TYPE (GimpUnit, gimp_unit, GIMP, UNIT, GObject)


gint32        gimp_unit_get_id                       (GimpUnit    *unit);

const gchar * gimp_unit_get_name                     (GimpUnit    *unit);
gdouble       gimp_unit_get_factor                   (GimpUnit    *unit);
gint          gimp_unit_get_digits                   (GimpUnit    *unit);
gint          gimp_unit_get_scaled_digits            (GimpUnit    *unit,
                                                      gdouble      resolution);
const gchar * gimp_unit_get_symbol                   (GimpUnit    *unit);
const gchar * gimp_unit_get_abbreviation             (GimpUnit    *unit);

gboolean      gimp_unit_get_deletion_flag            (GimpUnit    *unit);
void          gimp_unit_set_deletion_flag            (GimpUnit    *unit,
                                                      gboolean     deletion_flag);

GimpUnit    * gimp_unit_get_by_id                    (gint         unit_id);
GimpUnit    * gimp_unit_pixel                        (void);
GimpUnit    * gimp_unit_mm                           (void);
GimpUnit    * gimp_unit_inch                         (void);
GimpUnit    * gimp_unit_point                        (void);
GimpUnit    * gimp_unit_pica                         (void);
GimpUnit    * gimp_unit_percent                      (void);

gboolean      gimp_unit_is_built_in                  (GimpUnit    *unit);
gboolean      gimp_unit_is_metric                    (GimpUnit    *unit);


gchar       * gimp_unit_format_string                (const gchar *format,
                                                      GimpUnit    *unit);

gdouble       gimp_pixels_to_units                   (gdouble      pixels,
                                                      GimpUnit    *unit,
                                                      gdouble      resolution);
gdouble       gimp_units_to_pixels                   (gdouble      value,
                                                      GimpUnit    *unit,
                                                      gdouble      resolution);
gdouble       gimp_units_to_points                   (gdouble      value,
                                                      GimpUnit    *unit,
                                                      gdouble       resolution);

/*
 * GIMP_TYPE_PARAM_UNIT
 */

#define GIMP_VALUE_HOLDS_UNIT(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_UNIT))

#define GIMP_TYPE_PARAM_UNIT           (gimp_param_unit_get_type ())
#define GIMP_PARAM_SPEC_UNIT(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_UNIT, GimpParamSpecUnit))
#define GIMP_IS_PARAM_SPEC_UNIT(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_UNIT))

typedef struct _GimpParamSpecUnit GimpParamSpecUnit;

struct _GimpParamSpecUnit
{
  GParamSpecObject  parent_instance;

  gboolean          allow_pixel;
  gboolean          allow_percent;
  GimpUnit         *default_value;
};

GType        gimp_param_unit_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_unit     (const gchar *name,
                                       const gchar *nick,
                                       const gchar *blurb,
                                       gboolean     allow_pixel,
                                       gboolean     allow_percent,
                                       GimpUnit    *default_value,
                                       GParamFlags  flags);


G_END_DECLS

#endif /* __GIMP_UNIT_H__ */
