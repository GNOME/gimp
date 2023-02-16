/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationdissolve.c
 * Copyright (C) 2012 Ville Sokk <ville.sokk@gmail.com>
 *               2012 Øyvind Kolås <pippin@gimp.org>
 *               2003 Helvetix Victorinox
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

#include <gegl-plugin.h>

#include "../operations-types.h"

#include "gimpoperationdissolve.h"


#define RANDOM_TABLE_SIZE 4096


static gboolean                   gimp_operation_dissolve_process             (GeglOperation          *op,
                                                                               void                   *in,
                                                                               void                   *layer,
                                                                               void                   *mask,
                                                                               void                   *out,
                                                                               glong                   samples,
                                                                               const GeglRectangle    *result,
                                                                               gint                    level);
static GimpLayerCompositeRegion   gimp_operation_dissolve_get_affected_region (GimpOperationLayerMode *layer_mode);


G_DEFINE_TYPE (GimpOperationDissolve, gimp_operation_dissolve,
               GIMP_TYPE_OPERATION_LAYER_MODE)


static gint32 random_table[RANDOM_TABLE_SIZE];


static void
gimp_operation_dissolve_class_init (GimpOperationDissolveClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  GimpOperationLayerModeClass *layer_mode_class = GIMP_OPERATION_LAYER_MODE_CLASS (klass);
  GRand                       *gr;
  gint                         i;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:dissolve",
                                 "description", "GIMP dissolve mode operation",
                                 "categories",  "compositors",
                                 NULL);

  layer_mode_class->process             = gimp_operation_dissolve_process;
  layer_mode_class->get_affected_region = gimp_operation_dissolve_get_affected_region;

  /* generate a table of random seeds */
  gr = g_rand_new_with_seed (314159265);
  for (i = 0; i < RANDOM_TABLE_SIZE; i++)
    random_table[i] = g_rand_int (gr);

  g_rand_free (gr);
}

static void
gimp_operation_dissolve_init (GimpOperationDissolve *self)
{
}

static gboolean
gimp_operation_dissolve_process (GeglOperation       *op,
                                 void                *in_p,
                                 void                *layer_p,
                                 void                *mask_p,
                                 void                *out_p,
                                 glong                samples,
                                 const GeglRectangle *result,
                                 gint                 level)
{
  GimpOperationLayerMode *layer_mode = (gpointer) op;
  gfloat                 *in         = in_p;
  gfloat                 *out        = out_p;
  gfloat                 *layer      = layer_p;
  gfloat                 *mask       = mask_p;
  gfloat                  opacity    = layer_mode->opacity;
  const gboolean          has_mask   = mask != NULL;
  gint                    x, y;

  for (y = result->y; y < result->y + result->height; y++)
    {
      GRand *gr;

      /* The offset can be negative. I could just abs() the result, but we
       * probably prefer to use different indexes of the table when possible for
       * nicer randomization, so let's cycle the modulo so that -1 is the last
       * table index.
       */
      gr = g_rand_new_with_seed (random_table[((y % RANDOM_TABLE_SIZE) + RANDOM_TABLE_SIZE) % RANDOM_TABLE_SIZE]);

      /* fast forward through the rows pseudo random sequence */
      for (x = 0; x < result->x; x++)
        g_rand_int (gr);

      for (x = result->x; x < result->x + result->width; x++)
        {
          gfloat value = layer[ALPHA] * opacity * 255;

          if (has_mask)
            value *= *mask;

          if (g_rand_int_range (gr, 0, 255) >= value)
            {
              out[0] = in[0];
              out[1] = in[1];
              out[2] = in[2];

              if (layer_mode->composite_mode == GIMP_LAYER_COMPOSITE_UNION ||
                  layer_mode->composite_mode == GIMP_LAYER_COMPOSITE_CLIP_TO_BACKDROP)
                {
                  out[3] = in[3];
                }
              else
                {
                  out[3] = 0.0f;
                }
            }
          else
            {
              out[0] = layer[0];
              out[1] = layer[1];
              out[2] = layer[2];

              if (layer_mode->composite_mode == GIMP_LAYER_COMPOSITE_UNION ||
                  layer_mode->composite_mode == GIMP_LAYER_COMPOSITE_CLIP_TO_LAYER)
                {
                  out[3] = 1.0f;
                }
              else
                {
                  out[3] = in[3];
                }
            }

          in    += 4;
          layer += 4;
          out   += 4;

          if (has_mask)
            mask++;
        }

      g_rand_free (gr);
    }

  return TRUE;
}

static GimpLayerCompositeRegion
gimp_operation_dissolve_get_affected_region (GimpOperationLayerMode *layer_mode)
{
  return GIMP_LAYER_COMPOSITE_REGION_SOURCE;
}
