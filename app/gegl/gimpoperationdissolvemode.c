/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationdissolvemode.c
 * Copyright (C) 2012 Ville Sokk <ville.sokk@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gegl-plugin.h>

#include "gimp-gegl-types.h"

#include "gimpoperationdissolvemode.h"


#define RANDOM_TABLE_SIZE 4096


static gboolean gimp_operation_dissolve_mode_process (GeglOperation       *operation,
                                                      void                *in_buf,
                                                      void                *aux_buf,
                                                      void                *out_buf,
                                                      glong                samples,
                                                      const GeglRectangle *roi,
                                                      gint                 level);


G_DEFINE_TYPE (GimpOperationDissolveMode, gimp_operation_dissolve_mode,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)

static gint32  random_table[RANDOM_TABLE_SIZE];


static void
gimp_operation_dissolve_mode_class_init (GimpOperationDissolveModeClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_class;
  GRand                           *gr;
  gint                             i;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
           "name"       , "gimp:dissolve-mode",
           "description", "GIMP dissolve mode operation",
           NULL);

  point_class->process         = gimp_operation_dissolve_mode_process;

#define RANDOM_SEED 314159265

  /* generate a table of random seeds */
  gr = g_rand_new_with_seed (RANDOM_SEED);

  for (i = 0; i < RANDOM_TABLE_SIZE; i++)
    {
      random_table[i] = g_rand_int (gr);
    }

  g_rand_free (gr);
}

static void
gimp_operation_dissolve_mode_init (GimpOperationDissolveMode *self)
{
}

static gboolean
gimp_operation_dissolve_mode_process (GeglOperation       *operation,
                                      void                *in_buf,
                                      void                *aux_buf,
                                      void                *out_buf,
                                      glong                samples,
                                      const GeglRectangle *roi,
                                      gint                 level)
{
  gfloat *in     = in_buf;
  gfloat *layer  = aux_buf;
  gfloat *out    = out_buf;
  gint    x      = roi->x;
  gint    y      = roi->y;
  gint    width  = roi->width;
  gint    height = roi->height;
  gint    row;

  for (row = 0; row < height; row++)
    {
      GRand *gr;
      gint   pixel;
      gint   i;

      gr = g_rand_new_with_seed (random_table[(y + row) % RANDOM_TABLE_SIZE]);
      for (i = 0; i < x; i++)
        {
          g_rand_int (gr);
        }

      for (pixel = 0; pixel < width; pixel++)
        {
          gdouble rand_val;

          /* dissolve if random value is >= opacity */
          rand_val = g_rand_double_range (gr, 0.0, 1.0);

          if (layer[ALPHA] >= rand_val)
            {
              out[ALPHA] = 1.0;

              for (i = RED; i < ALPHA; i++)
                {
                  out[i] = layer[ALPHA] ? layer[i] / layer[ALPHA] : 0.0;
                }
            }
          else
            {
              out[ALPHA] = in[ALPHA];

              for (i = RED; i < ALPHA; i++)
                {
                  out[i] = in[i];
                }
            }

          in += 4;
          layer += 4;
          out += 4;
        }

      g_rand_free (gr);
    }

  return TRUE;
}
