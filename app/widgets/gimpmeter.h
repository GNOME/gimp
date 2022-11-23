/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmameter.h
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

#ifndef __LIGMA_METER_H__
#define __LIGMA_METER_H__


#define LIGMA_TYPE_METER            (ligma_meter_get_type ())
#define LIGMA_METER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_METER, LigmaMeter))
#define LIGMA_METER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_METER, LigmaMeterClass))
#define LIGMA_IS_METER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, LIGMA_TYPE_METER))
#define LIGMA_IS_METER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_METER))
#define LIGMA_METER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_METER, LigmaMeterClass))


typedef struct _LigmaMeterPrivate LigmaMeterPrivate;
typedef struct _LigmaMeterClass   LigmaMeterClass;

struct _LigmaMeter
{
  GtkWidget         parent_instance;

  LigmaMeterPrivate *priv;
};

struct _LigmaMeterClass
{
  GtkWidgetClass  parent_class;
};


GType                   ligma_meter_get_type                  (void) G_GNUC_CONST;

GtkWidget             * ligma_meter_new                       (gint                   n_values);

void                    ligma_meter_set_size                  (LigmaMeter             *meter,
                                                              gint                   size);
gint                    ligma_meter_get_size                  (LigmaMeter             *meter);

void                    ligma_meter_set_refresh_rate          (LigmaMeter             *meter,
                                                              gdouble                rate);
gdouble                 ligma_meter_get_refresh_rate          (LigmaMeter             *meter);

void                    ligma_meter_set_range                 (LigmaMeter             *meter,
                                                              gdouble                min,
                                                              gdouble                max);
gdouble                 ligma_meter_get_range_min             (LigmaMeter             *meter);
gdouble                 ligma_meter_get_range_max             (LigmaMeter             *meter);

void                    ligma_meter_set_n_values              (LigmaMeter             *meter,
                                                              gint                   n_values);
gint                    ligma_meter_get_n_values              (LigmaMeter             *meter);

void                    ligma_meter_set_value_active          (LigmaMeter             *meter,
                                                              gint                   value,
                                                              gboolean               active);
gboolean                ligma_meter_get_value_active          (LigmaMeter             *meter,
                                                              gint                   value);

void                    ligma_meter_set_value_show_in_gauge    (LigmaMeter            *meter,
                                                               gint                  value,
                                                               gboolean              show);
gboolean                ligma_meter_get_value_show_in_gauge    (LigmaMeter            *meter,
                                                               gint                  value);

void                    ligma_meter_set_value_show_in_history (LigmaMeter             *meter,
                                                              gint                   value,
                                                              gboolean               show);
gboolean                ligma_meter_get_value_show_in_history (LigmaMeter             *meter,
                                                              gint                   value);

void                    ligma_meter_set_value_color           (LigmaMeter             *meter,
                                                              gint                   value,
                                                              const LigmaRGB         *color);
const LigmaRGB         * ligma_meter_get_value_color           (LigmaMeter             *meter,
                                                              gint                   value);

void                    ligma_meter_set_value_interpolation   (LigmaMeter             *meter,
                                                              gint                   value,
                                                              LigmaInterpolationType  interpolation);
LigmaInterpolationType   ligma_meter_get_value_interpolation   (LigmaMeter             *meter,
                                                              gint                   value);

void                    ligma_meter_set_history_visible       (LigmaMeter             *meter,
                                                              gboolean               visible);
gboolean                ligma_meter_get_history_visible       (LigmaMeter             *meter);

void                    ligma_meter_set_history_duration      (LigmaMeter             *meter,
                                                              gdouble                duration);
gdouble                 ligma_meter_get_history_duration      (LigmaMeter             *meter);

void                    ligma_meter_set_history_resolution    (LigmaMeter             *meter,
                                                              gdouble                resolution);
gdouble                 ligma_meter_get_history_resolution    (LigmaMeter             *meter);

void                    ligma_meter_clear_history             (LigmaMeter             *meter);

void                    ligma_meter_add_sample                (LigmaMeter             *meter,
                                                              const gdouble         *sample);

void                    ligma_meter_set_led_active            (LigmaMeter             *meter,
                                                              gboolean               active);
gboolean                ligma_meter_get_led_active            (LigmaMeter             *meter);

void                    ligma_meter_set_led_color             (LigmaMeter             *meter,
                                                              const LigmaRGB         *color);
const LigmaRGB         * ligma_meter_get_led_color             (LigmaMeter             *meter);


#endif /* __LIGMA_METER_H__ */
