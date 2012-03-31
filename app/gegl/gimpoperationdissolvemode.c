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

enum
{
  PROP_0,
  PROP_PREMULTIPLIED
};


static void          gimp_operation_dissolve_mode_set_property            (GObject                   *object,
                                                                           guint                      property_id,
                                                                           const GValue              *value,
                                                                           GParamSpec                *pspec);
static void          gimp_operation_dissolve_mode_get_property            (GObject                   *object,
                                                                           guint                      property_id,
                                                                           GValue                    *value,
                                                                           GParamSpec                *pspec);
static void          gimp_operation_dissolve_mode_prepare                 (GeglOperation             *operation);
static GeglRectangle gimp_operation_dissolve_mode_get_required_for_output (GeglOperation             *operation,
                                                                           const gchar               *input_pad,
                                                                           const GeglRectangle       *roi);
static gboolean gimp_operation_dissolve_mode_process                      (GeglOperation             *operation,
                                                                           GeglBuffer                *input,
                                                                           GeglBuffer                *aux,
                                                                           GeglBuffer                *output,
                                                                           const GeglRectangle       *result,
                                                                           gint                       level);


G_DEFINE_TYPE (GimpOperationDissolveMode, gimp_operation_dissolve_mode,
               GEGL_TYPE_OPERATION_COMPOSER)

static gint32 random_table[RANDOM_TABLE_SIZE];


static void
gimp_operation_dissolve_mode_class_init (GimpOperationDissolveModeClass *klass)
{
  GObjectClass               *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass         *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationComposerClass *composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);
  GRand                      *gr;
  gint                        i;

  object_class->set_property = gimp_operation_dissolve_mode_set_property;
  object_class->get_property = gimp_operation_dissolve_mode_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:dissolve-mode",
                                 "description", "GIMP dissolve mode operation",
                                 "categories",  "compositors",
                                 NULL);

  operation_class->prepare                 = gimp_operation_dissolve_mode_prepare;
  operation_class->get_required_for_output = gimp_operation_dissolve_mode_get_required_for_output;
  composer_class->process                  = gimp_operation_dissolve_mode_process;

  g_object_class_install_property (object_class, PROP_PREMULTIPLIED,
                                   g_param_spec_boolean ("premultiplied",
                                                         NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

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

static void
gimp_operation_dissolve_mode_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  GimpOperationDissolveMode *self = GIMP_OPERATION_DISSOLVE_MODE (object);

  switch (property_id)
    {
    case PROP_PREMULTIPLIED:
      self->premultiplied = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_dissolve_mode_get_property (GObject    *object,
                                           guint       property_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  GimpOperationDissolveMode *self = GIMP_OPERATION_DISSOLVE_MODE (object);

  switch (property_id)
    {
    case PROP_PREMULTIPLIED:
      g_value_set_boolean (value, self->premultiplied);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_dissolve_mode_prepare (GeglOperation *operation)
{
  GimpOperationDissolveMode *self = GIMP_OPERATION_DISSOLVE_MODE (operation);
  const Babl                *format;

  if (self->premultiplied)
    format = babl_format ("RaGaBaA float");
  else
    format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "aux", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
gimp_operation_dissolve_mode_get_required_for_output (GeglOperation       *operation,
                                                      const gchar         *input_pad,
                                                      const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation,
                                                                  "input");

  return result;
}

static gboolean
gimp_operation_dissolve_mode_process (GeglOperation       *operation,
                                      GeglBuffer          *input,
                                      GeglBuffer          *aux,
                                      GeglBuffer          *output,
                                      const GeglRectangle *result,
                                      gint                 level)
{
  GimpOperationDissolveMode *self = GIMP_OPERATION_DISSOLVE_MODE (operation);
  GeglBufferIterator        *it;
  const Babl                *format;
  gint                       index_in, index_out, index_layer;

  if (self->premultiplied)
    format = babl_format ("RaGaBaA float");
  else
    format = babl_format ("RGBA float");

  it          = gegl_buffer_iterator_new (output, result, level,
                                          format, GEGL_BUFFER_WRITE,
                                          GEGL_ABYSS_NONE);
  index_in    = gegl_buffer_iterator_add (it, input, result,
                                          level, format, GEGL_BUFFER_READ,
                                          GEGL_ABYSS_NONE);
  index_layer = gegl_buffer_iterator_add (it, aux, result,
                                          level, format, GEGL_BUFFER_READ,
                                          GEGL_ABYSS_NONE);
  index_out   = 0;

  while (gegl_buffer_iterator_next (it))
    {
      gint    row;
      gint    width  = it->roi->width;
      gint    height = it->roi->height;
      gfloat *in     = it->data[index_in];
      gfloat *out    = it->data[index_out];
      gfloat *layer  = it->data[index_layer];

      for (row = 0; row < height; row++)
        {
          gint   pixel, i;
          gint   x  = gegl_buffer_get_x (aux) + 4096 + it->roi->x;
          gint   y  = gegl_buffer_get_y (aux) + 4096 + it->roi->y;
          GRand *gr = g_rand_new_with_seed (random_table[(y + row) % RANDOM_TABLE_SIZE]);

          for (i = 0; i < x; i++)
            {
              g_rand_double_range (gr, 0.0, 0.1);
            }

          for (pixel = 0; pixel < width; pixel++)
            {
              gdouble  rand_val;

              /* dissolve if random value is >= opacity */
              rand_val = g_rand_double_range (gr, 0.0, 1.0);

              if (layer[ALPHA] >= rand_val)
                {
                  out[ALPHA] = 1.0;

                  if (self->premultiplied)
                    {
                      for (i = RED; i < ALPHA; i++)
                        {
                          out[i] = layer[i] / layer[ALPHA];
                        }
                    }
                  else
                    {
                      for (i = RED; i < ALPHA; i++)
                        {
                          out[i] = layer[i];
                        }
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

              in    += 4;
              layer += 4;
              out   += 4;
            }

          g_rand_free (gr);
        }
    }

  return TRUE;
}
