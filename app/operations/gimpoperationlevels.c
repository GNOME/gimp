/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationlevels.c
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

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmamath/ligmamath.h"

#include "operations-types.h"

#include "ligmalevelsconfig.h"
#include "ligmaoperationlevels.h"

#include "ligma-intl.h"


static gboolean ligma_operation_levels_process (GeglOperation       *operation,
                                               void                *in_buf,
                                               void                *out_buf,
                                               glong                samples,
                                               const GeglRectangle *roi,
                                               gint                 level);


G_DEFINE_TYPE (LigmaOperationLevels, ligma_operation_levels,
               LIGMA_TYPE_OPERATION_POINT_FILTER)

#define parent_class ligma_operation_levels_parent_class


static void
ligma_operation_levels_class_init (LigmaOperationLevelsClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property   = ligma_operation_point_filter_set_property;
  object_class->get_property   = ligma_operation_point_filter_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "ligma:levels",
                                 "categories",  "color",
                                 "description", _("Adjust color levels"),
                                 NULL);

  point_class->process = ligma_operation_levels_process;

  g_object_class_install_property (object_class,
                                   LIGMA_OPERATION_POINT_FILTER_PROP_TRC,
                                   g_param_spec_enum ("trc",
                                                      "Linear/Percptual",
                                                      "What TRC to operate on",
                                                      LIGMA_TYPE_TRC_TYPE,
                                                      LIGMA_TRC_NON_LINEAR,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   LIGMA_OPERATION_POINT_FILTER_PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        "Config",
                                                        "The config object",
                                                        LIGMA_TYPE_LEVELS_CONFIG,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
ligma_operation_levels_init (LigmaOperationLevels *self)
{
}

static inline gdouble
ligma_operation_levels_map (gdouble  value,
                           gdouble  low_input,
                           gdouble  high_input,
                           gboolean clamp_input,
                           gdouble  inv_gamma,
                           gdouble  low_output,
                           gdouble  high_output,
                           gboolean clamp_output)
{
  /*  determine input intensity  */
  if (high_input != low_input)
    value = (value - low_input) / (high_input - low_input);
  else
    value = (value - low_input);

  if (clamp_input)
    value = CLAMP (value, 0.0, 1.0);

  if (inv_gamma != 1.0 && value > 0)
    value =  pow (value, inv_gamma);

  /*  determine the output intensity  */
  if (high_output >= low_output)
    value = value * (high_output - low_output) + low_output;
  else if (high_output < low_output)
    value = low_output - value * (low_output - high_output);

  if (clamp_output)
    value = CLAMP (value, 0.0, 1.0);

  return value;
}

static gboolean
ligma_operation_levels_process (GeglOperation       *operation,
                               void                *in_buf,
                               void                *out_buf,
                               glong                samples,
                               const GeglRectangle *roi,
                               gint                 level)
{
  LigmaOperationPointFilter *point  = LIGMA_OPERATION_POINT_FILTER (operation);
  LigmaLevelsConfig         *config = LIGMA_LEVELS_CONFIG (point->config);
  gfloat                   *src    = in_buf;
  gfloat                   *dest   = out_buf;
  gfloat                    inv_gamma[5];
  gint                      channel;

  if (! config)
    return FALSE;

  for (channel = 0; channel < 5; channel++)
    {
      g_return_val_if_fail (config->gamma[channel] != 0.0, FALSE);

      inv_gamma[channel] = 1.0 / config->gamma[channel];
    }

  while (samples--)
    {
      for (channel = 0; channel < 4; channel++)
        {
          gdouble value;

          value = ligma_operation_levels_map (src[channel],
                                             config->low_input[channel + 1],
                                             config->high_input[channel + 1],
                                             config->clamp_input,
                                             inv_gamma[channel + 1],
                                             config->low_output[channel + 1],
                                             config->high_output[channel + 1],
                                             config->clamp_output);

          /* don't apply the overall curve to the alpha channel */
          if (channel != ALPHA)
            value = ligma_operation_levels_map (value,
                                               config->low_input[0],
                                               config->high_input[0],
                                               config->clamp_input,
                                               inv_gamma[0],
                                               config->low_output[0],
                                               config->high_output[0],
                                               config->clamp_output);

          dest[channel] = value;
        }

      src  += 4;
      dest += 4;
    }

  return TRUE;
}


/*  public functions  */

gdouble
ligma_operation_levels_map_input (LigmaLevelsConfig     *config,
                                 LigmaHistogramChannel  channel,
                                 gdouble               value)
{
  g_return_val_if_fail (LIGMA_IS_LEVELS_CONFIG (config), 0.0);

  /*  determine input intensity  */
  if (config->high_input[channel] != config->low_input[channel])
    value = ((value - config->low_input[channel]) /
             (config->high_input[channel] - config->low_input[channel]));
  else
    value = (value - config->low_input[channel]);

  if (config->gamma[channel] != 0.0 && value > 0.0)
    value = pow (value, 1.0 / config->gamma[channel]);

  return value;
}
