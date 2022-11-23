/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacurvesconfig.h
 * Copyright (C) 2007 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CURVES_CONFIG_H__
#define __LIGMA_CURVES_CONFIG_H__


#include "ligmaoperationsettings.h"


#define LIGMA_TYPE_CURVES_CONFIG            (ligma_curves_config_get_type ())
#define LIGMA_CURVES_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CURVES_CONFIG, LigmaCurvesConfig))
#define LIGMA_CURVES_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_CURVES_CONFIG, LigmaCurvesConfigClass))
#define LIGMA_IS_CURVES_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CURVES_CONFIG))
#define LIGMA_IS_CURVES_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_CURVES_CONFIG))
#define LIGMA_CURVES_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_CURVES_CONFIG, LigmaCurvesConfigClass))


typedef struct _LigmaCurvesConfigClass LigmaCurvesConfigClass;

struct _LigmaCurvesConfig
{
  LigmaOperationSettings  parent_instance;

  LigmaTRCType            trc;

  LigmaHistogramChannel   channel;

  LigmaCurve              *curve[5];
};

struct _LigmaCurvesConfigClass
{
  LigmaOperationSettingsClass  parent_class;
};


GType      ligma_curves_config_get_type            (void) G_GNUC_CONST;

GObject  * ligma_curves_config_new_spline          (gint32             channel,
                                                   const gdouble     *points,
                                                   gint               n_points);
GObject *  ligma_curves_config_new_explicit        (gint32             channel,
                                                   const gdouble     *samples,
                                                   gint               n_samples);

GObject  * ligma_curves_config_new_spline_cruft    (gint32             channel,
                                                   const guint8      *points,
                                                   gint               n_points);
GObject *  ligma_curves_config_new_explicit_cruft  (gint32             channel,
                                                   const guint8      *samples,
                                                   gint               n_samples);

void       ligma_curves_config_reset_channel       (LigmaCurvesConfig  *config);

gboolean   ligma_curves_config_load_cruft          (LigmaCurvesConfig  *config,
                                                   GInputStream      *input,
                                                   GError           **error);
gboolean   ligma_curves_config_save_cruft          (LigmaCurvesConfig  *config,
                                                   GOutputStream     *output,
                                                   GError           **error);


#endif /* __LIGMA_CURVES_CONFIG_H__ */
