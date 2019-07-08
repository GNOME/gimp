/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "gegl/gimp-gegl-loops.h"

#include "core/gimppickable.h"

#include "gimptilehandleriscissors.h"


enum
{
  PROP_0,
  PROP_PICKABLE
};


static void   gimp_tile_handler_iscissors_finalize     (GObject         *object);
static void   gimp_tile_handler_iscissors_set_property (GObject         *object,
                                                        guint            property_id,
                                                        const GValue    *value,
                                                        GParamSpec      *pspec);
static void   gimp_tile_handler_iscissors_get_property (GObject         *object,
                                                        guint            property_id,
                                                        GValue          *value,
                                                        GParamSpec      *pspec);

static void   gimp_tile_handler_iscissors_validate     (GimpTileHandlerValidate *validate,
                                                        const GeglRectangle     *rect,
                                                        const Babl              *format,
                                                        gpointer                 dest_buf,
                                                        gint                     dest_stride);


G_DEFINE_TYPE (GimpTileHandlerIscissors, gimp_tile_handler_iscissors,
               GIMP_TYPE_TILE_HANDLER_VALIDATE)

#define parent_class gimp_tile_handler_iscissors_parent_class


static void
gimp_tile_handler_iscissors_class_init (GimpTileHandlerIscissorsClass *klass)
{
  GObjectClass                 *object_class = G_OBJECT_CLASS (klass);
  GimpTileHandlerValidateClass *validate_class;

  validate_class = GIMP_TILE_HANDLER_VALIDATE_CLASS (klass);

  object_class->finalize     = gimp_tile_handler_iscissors_finalize;
  object_class->set_property = gimp_tile_handler_iscissors_set_property;
  object_class->get_property = gimp_tile_handler_iscissors_get_property;

  validate_class->validate   = gimp_tile_handler_iscissors_validate;

  g_object_class_install_property (object_class, PROP_PICKABLE,
                                   g_param_spec_object ("pickable", NULL, NULL,
                                                        GIMP_TYPE_PICKABLE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_tile_handler_iscissors_init (GimpTileHandlerIscissors *iscissors)
{
}

static void
gimp_tile_handler_iscissors_finalize (GObject *object)
{
  GimpTileHandlerIscissors *iscissors = GIMP_TILE_HANDLER_ISCISSORS (object);

  if (iscissors->pickable)
    {
      g_object_unref (iscissors->pickable);
      iscissors->pickable = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tile_handler_iscissors_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  GimpTileHandlerIscissors *iscissors = GIMP_TILE_HANDLER_ISCISSORS (object);

  switch (property_id)
    {
    case PROP_PICKABLE:
      iscissors->pickable = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tile_handler_iscissors_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  GimpTileHandlerIscissors *iscissors = GIMP_TILE_HANDLER_ISCISSORS (object);

  switch (property_id)
    {
    case PROP_PICKABLE:
      g_value_set_object (value, iscissors->pickable);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static const gfloat horz_deriv[9] =
{
  1,  0, -1,
  2,  0, -2,
  1,  0, -1,
};

static const gfloat vert_deriv[9] =
{
  1,  2,  1,
  0,  0,  0,
  -1, -2, -1,
};

static const gfloat blur_32[9] =
{
  1,  1,  1,
  1, 24,  1,
  1,  1,  1,
};

#define  MAX_GRADIENT 179.606  /* == sqrt (127^2 + 127^2) */
#define  MIN_GRADIENT  63      /* gradients < this are directionless */
#define  COST_WIDTH     2      /* number of bytes for each pixel in cost map */

static void
gimp_tile_handler_iscissors_validate (GimpTileHandlerValidate *validate,
                                      const GeglRectangle     *rect,
                                      const Babl              *format,
                                      gpointer                 dest_buf,
                                      gint                     dest_stride)
{
  GimpTileHandlerIscissors *iscissors = GIMP_TILE_HANDLER_ISCISSORS (validate);
  GeglBuffer               *src;
  GeglBuffer               *temp0;
  GeglBuffer               *temp1;
  GeglBuffer               *temp2;
  gint                      stride1;
  gint                      stride2;
  gint                      i, j;

  /*  temporary convolution buffers --  */
  guchar *maxgrad_conv1;
  guchar *maxgrad_conv2;

#if 0
  g_printerr ("validating at %d %d %d %d\n",
              rect->x,
              rect->y,
              rect->width,
              rect->height);
#endif

  gimp_pickable_flush (iscissors->pickable);

  src = gimp_pickable_get_buffer (iscissors->pickable);

  temp0 = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                           rect->width,
                                           rect->height),
                           babl_format ("R'G'B'A u8"));

  temp1 = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                           rect->width,
                                           rect->height),
                           babl_format ("R'G'B'A u8"));

  temp2 = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                           rect->width,
                                           rect->height),
                           babl_format ("R'G'B'A u8"));

  /* XXX tile edges? */

  /*  Blur the source to get rid of noise  */
  gimp_gegl_convolve (src,   rect,
                      temp0, GEGL_RECTANGLE (0, 0, rect->width, rect->height),
                      blur_32, 3, 32, GIMP_NORMAL_CONVOL, FALSE);

  /*  Use this blurred region as the new source  */

  /*  Get the horizontal derivative  */
  gimp_gegl_convolve (temp0, GEGL_RECTANGLE (0, 0, rect->width, rect->height),
                      temp1, GEGL_RECTANGLE (0, 0, rect->width, rect->height),
                      horz_deriv, 3, 1, GIMP_NEGATIVE_CONVOL, FALSE);

  /*  Get the vertical derivative  */
  gimp_gegl_convolve (temp0, GEGL_RECTANGLE (0, 0, rect->width, rect->height),
                      temp2, GEGL_RECTANGLE (0, 0, rect->width, rect->height),
                      vert_deriv, 3, 1, GIMP_NEGATIVE_CONVOL, FALSE);

  maxgrad_conv1 =
    (guchar *) gegl_buffer_linear_open (temp1,
                                        GEGL_RECTANGLE (0, 0,
                                                        rect->width,
                                                        rect->height),
                                        &stride1, NULL);

  maxgrad_conv2 =
    (guchar *) gegl_buffer_linear_open (temp2,
                                        GEGL_RECTANGLE (0, 0,
                                                        rect->width,
                                                        rect->height),
                                        &stride2, NULL);

  /* calculate overall gradient */

  for (i = 0; i < rect->height; i++)
    {
      const guint8 *datah   = maxgrad_conv1 + stride1 * i;
      const guint8 *datav   = maxgrad_conv2 + stride2 * i;
      guint8       *gradmap = (guint8 *) dest_buf + dest_stride * i;

      for (j = 0; j < rect->width; j++)
        {
          gint8  hmax = datah[0] - 128;
          gint8  vmax = datav[0] - 128;
          gfloat gradient;
          gint   b;

          for (b = 1; b < 4; b++)
            {
              if (abs (datah[b] - 128) > abs (hmax))
                hmax = datah[b] - 128;

              if (abs (datav[b] - 128) > abs (vmax))
                vmax = datav[b] - 128;
            }

          if (i == 0 || j == 0 || i == rect->height - 1 || j == rect->width - 1)
            {
              gradmap[j * COST_WIDTH + 0] = 0;
              gradmap[j * COST_WIDTH + 1] = 255;
              goto contin;
            }

          /* 1 byte absolute magnitude first */
          gradient = sqrt (SQR (hmax) + SQR (vmax));
          gradmap[j * COST_WIDTH] = gradient * 255 / MAX_GRADIENT;

          /* then 1 byte direction */
          if (gradient > MIN_GRADIENT)
            {
              gfloat direction;

              if (! hmax)
                direction = (vmax > 0) ? G_PI_2 : -G_PI_2;
              else
                direction = atan ((gdouble) vmax / (gdouble) hmax);

              /* Scale the direction from between 0 and 254,
               *  corresponding to -PI/2, PI/2 255 is reserved for
               *  directionless pixels
               */
              gradmap[j * COST_WIDTH + 1] =
                (guint8) (254 * (direction + G_PI_2) / G_PI);
            }
          else
            {
              gradmap[j * COST_WIDTH + 1] = 255; /* reserved for weak gradient */
            }

        contin:
          datah += 4;
          datav += 4;
        }
    }

  gegl_buffer_linear_close (temp1, maxgrad_conv1);
  gegl_buffer_linear_close (temp2, maxgrad_conv2);

  g_object_unref (temp0);
  g_object_unref (temp1);
  g_object_unref (temp2);
}

GeglTileHandler *
gimp_tile_handler_iscissors_new (GimpPickable *pickable)
{
  g_return_val_if_fail (GIMP_IS_PICKABLE (pickable), NULL);

  return g_object_new (GIMP_TYPE_TILE_HANDLER_ISCISSORS,
                       "whole-tile", TRUE,
                       "pickable",   pickable,
                       NULL);
}
