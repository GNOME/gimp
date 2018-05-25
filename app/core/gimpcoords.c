/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcoords.c
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimpcoords.h"


#define INPUT_RESOLUTION 256


/*   amul * a + bmul * b = ret_val  */

void
gimp_coords_mix (const gdouble     amul,
                 const GimpCoords *a,
                 const gdouble     bmul,
                 const GimpCoords *b,
                 GimpCoords       *ret_val)
{
  if (b)
    {
      ret_val->x         = amul * a->x         + bmul * b->x;
      ret_val->y         = amul * a->y         + bmul * b->y;
      ret_val->pressure  = amul * a->pressure  + bmul * b->pressure;
      ret_val->xtilt     = amul * a->xtilt     + bmul * b->xtilt;
      ret_val->ytilt     = amul * a->ytilt     + bmul * b->ytilt;
      ret_val->wheel     = amul * a->wheel     + bmul * b->wheel;
      ret_val->distance  = amul * a->distance  + bmul * b->distance;
      ret_val->rotation  = amul * a->rotation  + bmul * b->rotation;
      ret_val->slider    = amul * a->slider    + bmul * b->slider;
      ret_val->velocity  = amul * a->velocity  + bmul * b->velocity;
      ret_val->direction = amul * a->direction + bmul * b->direction;
    }
  else
    {
      ret_val->x         = amul * a->x;
      ret_val->y         = amul * a->y;
      ret_val->pressure  = amul * a->pressure;
      ret_val->xtilt     = amul * a->xtilt;
      ret_val->ytilt     = amul * a->ytilt;
      ret_val->wheel     = amul * a->wheel;
      ret_val->distance  = amul * a->distance;
      ret_val->rotation  = amul * a->rotation;
      ret_val->slider    = amul * a->slider;
      ret_val->velocity  = amul * a->velocity;
      ret_val->direction = amul * a->direction;
    }
}


/*    (a+b)/2 = ret_average  */

void
gimp_coords_average (const GimpCoords *a,
                     const GimpCoords *b,
                     GimpCoords       *ret_average)
{
  gimp_coords_mix (0.5, a, 0.5, b, ret_average);
}


/* a + b = ret_add  */

void
gimp_coords_add (const GimpCoords *a,
                 const GimpCoords *b,
                 GimpCoords       *ret_add)
{
  gimp_coords_mix (1.0, a, 1.0, b, ret_add);
}


/* a - b = ret_difference */

void
gimp_coords_difference (const GimpCoords *a,
                        const GimpCoords *b,
                        GimpCoords       *ret_difference)
{
  gimp_coords_mix (1.0, a, -1.0, b, ret_difference);
}


/* a * f = ret_product  */

void
gimp_coords_scale (const gdouble     f,
                   const GimpCoords *a,
                   GimpCoords       *ret_product)
{
  gimp_coords_mix (f, a, 0.0, NULL, ret_product);
}


/* local helper for measuring the scalarproduct of two gimpcoords. */

gdouble
gimp_coords_scalarprod (const GimpCoords *a,
                        const GimpCoords *b)
{
  return (a->x         * b->x        +
          a->y         * b->y        +
          a->pressure  * b->pressure +
          a->xtilt     * b->xtilt    +
          a->ytilt     * b->ytilt    +
          a->wheel     * b->wheel    +
          a->distance  * b->distance +
          a->rotation  * b->rotation +
          a->slider    * b->slider   +
          a->velocity  * b->velocity +
          a->direction * b->direction);
}


/*
 * The "length" of the gimpcoord.
 * Applies a metric that increases the weight on the
 * pressure/xtilt/ytilt/wheel to ensure proper interpolation
 */

gdouble
gimp_coords_length_squared (const GimpCoords *a)
{
  GimpCoords upscaled_a;

  upscaled_a.x         = a->x;
  upscaled_a.y         = a->y;
  upscaled_a.pressure  = a->pressure  * INPUT_RESOLUTION;
  upscaled_a.xtilt     = a->xtilt     * INPUT_RESOLUTION;
  upscaled_a.ytilt     = a->ytilt     * INPUT_RESOLUTION;
  upscaled_a.wheel     = a->wheel     * INPUT_RESOLUTION;
  upscaled_a.distance  = a->distance  * INPUT_RESOLUTION;
  upscaled_a.rotation  = a->rotation  * INPUT_RESOLUTION;
  upscaled_a.slider    = a->slider    * INPUT_RESOLUTION;
  upscaled_a.velocity  = a->velocity  * INPUT_RESOLUTION;
  upscaled_a.direction = a->direction * INPUT_RESOLUTION;

  return gimp_coords_scalarprod (&upscaled_a, &upscaled_a);
}


gdouble
gimp_coords_length (const GimpCoords *a)
{
  return sqrt (gimp_coords_length_squared (a));
}

/*
 * Distance via manhattan metric, an upper bound for the eucledian metric.
 * used for e.g. bezier approximation
 */

gdouble
gimp_coords_manhattan_dist (const GimpCoords *a,
                            const GimpCoords *b)
{
  gdouble dist = 0;

  dist += ABS (a->pressure  - b->pressure);
  dist += ABS (a->xtilt     - b->xtilt);
  dist += ABS (a->ytilt     - b->ytilt);
  dist += ABS (a->wheel     - b->wheel);
  dist += ABS (a->distance  - b->distance);
  dist += ABS (a->rotation  - b->rotation);
  dist += ABS (a->slider    - b->slider);
  dist += ABS (a->velocity  - b->velocity);
  dist += ABS (a->direction - b->direction);

  dist *= INPUT_RESOLUTION;

  dist += ABS (a->x - b->x);
  dist += ABS (a->y - b->y);

  return dist;
}

gboolean
gimp_coords_equal (const GimpCoords *a,
                   const GimpCoords *b)
{
  return (a->x         == b->x        &&
          a->y         == b->y        &&
          a->pressure  == b->pressure &&
          a->xtilt     == b->xtilt    &&
          a->ytilt     == b->ytilt    &&
          a->wheel     == b->wheel    &&
          a->distance  == b->distance &&
          a->rotation  == b->rotation &&
          a->slider    == b->slider   &&
          a->velocity  == b->velocity &&
          a->direction == b->direction);

  /* Extended attribute was omitted from this comparison deliberately,
   * it describes the events origin, not its value
   */
}

/* helper for calculating direction of two gimpcoords. */

gdouble
gimp_coords_direction (const GimpCoords *a,
                       const GimpCoords *b)
{
  gdouble direction;
  gdouble delta_x, delta_y;

  delta_x = a->x - b->x;
  delta_y = a->y - b->y;

  if ((delta_x == 0) && (delta_y == 0))
    {
      direction = a->direction;
    }
  else if (delta_x == 0)
    {
      if (delta_y > 0)
        direction = 0.25;
      else
        direction = 0.75;
    }
  else if (delta_y == 0)
    {
      if (delta_x < 0)
        direction = 0.0;
      else
        direction = 0.5;
    }
  else
    {
      direction = atan ((- 1.0 * delta_y) / delta_x) / (2 * G_PI);

      if (delta_x > 0.0)
        direction = direction + 0.5;

      if (direction < 0.0)
        direction = direction + 1.0;
    }

  return direction;
}
