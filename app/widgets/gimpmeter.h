/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmeter.h
 * Copyright (C) 2017 Ell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


#define GIMP_TYPE_METER            (gimp_meter_get_type ())
#define GIMP_METER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_METER, GimpMeter))
#define GIMP_METER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_METER, GimpMeterClass))
#define GIMP_IS_METER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_METER))
#define GIMP_IS_METER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_METER))
#define GIMP_METER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_METER, GimpMeterClass))


typedef struct _GimpMeterPrivate GimpMeterPrivate;
typedef struct _GimpMeterClass   GimpMeterClass;

struct _GimpMeter
{
  GtkWidget         parent_instance;

  GimpMeterPrivate *priv;
};

struct _GimpMeterClass
{
  GtkWidgetClass  parent_class;
};


GType                   gimp_meter_get_type                  (void) G_GNUC_CONST;

GtkWidget             * gimp_meter_new                       (Gimp                  *gimp,
                                                              gint                   n_values);

void                    gimp_meter_set_size                  (GimpMeter             *meter,
                                                              gint                   size);
gint                    gimp_meter_get_size                  (GimpMeter             *meter);

void                    gimp_meter_set_refresh_rate          (GimpMeter             *meter,
                                                              gdouble                rate);
gdouble                 gimp_meter_get_refresh_rate          (GimpMeter             *meter);

void                    gimp_meter_set_range                 (GimpMeter             *meter,
                                                              gdouble                min,
                                                              gdouble                max);
gdouble                 gimp_meter_get_range_min             (GimpMeter             *meter);
gdouble                 gimp_meter_get_range_max             (GimpMeter             *meter);

void                    gimp_meter_set_n_values              (GimpMeter             *meter,
                                                              gint                   n_values);
gint                    gimp_meter_get_n_values              (GimpMeter             *meter);

void                    gimp_meter_set_value_active          (GimpMeter             *meter,
                                                              gint                   value,
                                                              gboolean               active);
gboolean                gimp_meter_get_value_active          (GimpMeter             *meter,
                                                              gint                   value);

void                    gimp_meter_set_value_show_in_gauge    (GimpMeter            *meter,
                                                               gint                  value,
                                                               gboolean              show);
gboolean                gimp_meter_get_value_show_in_gauge    (GimpMeter            *meter,
                                                               gint                  value);

void                    gimp_meter_set_value_show_in_history (GimpMeter             *meter,
                                                              gint                   value,
                                                              gboolean               show);
gboolean                gimp_meter_get_value_show_in_history (GimpMeter             *meter,
                                                              gint                   value);

void                    gimp_meter_set_value_color           (GimpMeter             *meter,
                                                              gint                   value,
                                                              GeglColor             *color);
GeglColor             * gimp_meter_get_value_color           (GimpMeter             *meter,
                                                              gint                   value);

void                    gimp_meter_set_value_interpolation   (GimpMeter             *meter,
                                                              gint                   value,
                                                              GimpInterpolationType  interpolation);
GimpInterpolationType   gimp_meter_get_value_interpolation   (GimpMeter             *meter,
                                                              gint                   value);

void                    gimp_meter_set_history_visible       (GimpMeter             *meter,
                                                              gboolean               visible);
gboolean                gimp_meter_get_history_visible       (GimpMeter             *meter);

void                    gimp_meter_set_history_duration      (GimpMeter             *meter,
                                                              gdouble                duration);
gdouble                 gimp_meter_get_history_duration      (GimpMeter             *meter);

void                    gimp_meter_set_history_resolution    (GimpMeter             *meter,
                                                              gdouble                resolution);
gdouble                 gimp_meter_get_history_resolution    (GimpMeter             *meter);

void                    gimp_meter_clear_history             (GimpMeter             *meter);

void                    gimp_meter_add_sample                (GimpMeter             *meter,
                                                              const gdouble         *sample);

void                    gimp_meter_set_led_active            (GimpMeter             *meter,
                                                              gboolean               active);
gboolean                gimp_meter_get_led_active            (GimpMeter             *meter);

void                    gimp_meter_set_led_color             (GimpMeter             *meter,
                                                              GeglColor             *color);
GeglColor             * gimp_meter_get_led_color             (GimpMeter             *meter);
