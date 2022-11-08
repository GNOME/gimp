/*
   flame - cosmic recursive fractal flames
   Copyright (C) 1992  Scott Draves <spot@cs.cmu.edu>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "rect.h"

#include <string.h>

#include "libgimp/gimp.h"

/* for batch
 *   interpolate
 *   compute colormap
 *   for subbatch
 *     compute samples
 *     buckets += cmap[samples]
 *   accum += time_filter[batch] * log(buckets)
 * image = filter(accum)
 */


typedef short bucket[4];

/* if you use longs instead of shorts, you
   get higher quality, and spend more memory */

#if 1
typedef short accum_t;
#define MAXBUCKET (1<<14)
#define SUB_BATCH_SIZE 10000
#else
typedef long accum_t;
#define MAXBUCKET (1<<30)
#define SUB_BATCH_SIZE 10000
#endif

typedef accum_t abucket[4];



/* allow this many iterations for settling into attractor */
#define FUSE 15

/* clamp spatial filter to zero at this std dev (2.5 ~= 0.0125) */
#define FILTER_CUTOFF 2.5

/* should be MAXBUCKET / (OVERSAMPLE^2) */
#define PREFILTER_WHITE (MAXBUCKET>>4)


#define bump_no_overflow(dest, delta, type) { \
   type tt_ = dest + delta;            \
   if (tt_ > dest) dest = tt_;                 \
}

/* sum of entries of vector to 1 */
static void
normalize_vector(double *v,
                 int     n)
{
  double t = 0.0;
  int i;
  for (i = 0; i < n; i++)
    t += v[i];
  t = 1.0 / t;
  for (i = 0; i < n; i++)
    v[i] *= t;
}

void
render_rectangle (frame_spec    *spec,
                  unsigned char *out,
                  int            out_width,
                  int            field,
                  int            nchan,
                  int progress(double))
{
  int      i, j, k, nsamples, nbuckets, batch_size, batch_num, sub_batch;
  bucket  *buckets;
  abucket *accumulate;
  point   *points;
  double  *filter, *temporal_filter, *temporal_deltas;
  double   bounds[4], size[2], ppux, ppuy;
  int      image_width, image_height;    /* size of the image to produce */
  int      width, height;               /* size of histogram */
  int      filter_width;
  int      oversample = spec->cps[0].spatial_oversample;
  int      nbatches = spec->cps[0].nbatches;
  bucket   cmap[CMAP_SIZE];
  int      gutter_width;
  int      sbc;

  image_width = spec->cps[0].width;
  if (field)
    {
      image_height = spec->cps[0].height / 2;
      if (field == field_odd)
        out += nchan * out_width;
      out_width *= 2;
    }
  else
    image_height = spec->cps[0].height;

  if (1)
    {
      filter_width = (2.0 * FILTER_CUTOFF * oversample *
                      spec->cps[0].spatial_filter_radius);
      /* make sure it has same parity as oversample */
      if ((filter_width ^ oversample) & 1)
        filter_width++;

      filter = g_malloc (sizeof (double) * filter_width * filter_width);
      /* fill in the coefs */
      for (i = 0; i < filter_width; i++)
        for (j = 0; j < filter_width; j++)
          {
            double ii = ((2.0 * i + 1.0) / filter_width - 1.0) * FILTER_CUTOFF;
            double jj = ((2.0 * j + 1.0) / filter_width - 1.0) * FILTER_CUTOFF;
            if (field)
              jj *= 2.0;
            filter[i + j * filter_width] = exp(-2.0 * (ii * ii + jj * jj));
          }
      normalize_vector(filter, filter_width * filter_width);
    }
  temporal_filter = g_malloc (sizeof (double) * nbatches);
  temporal_deltas = g_malloc (sizeof (double) * nbatches);
  if (nbatches > 1)
    {
      double t;
      /* fill in the coefs */
      for (i = 0; i < nbatches; i++)
        {
          t = temporal_deltas[i] = (2.0 * ((double) i / (nbatches - 1)) - 1.0)
                                   * spec->temporal_filter_radius;
          temporal_filter[i] = exp(-2.0 * t * t);
        }
      normalize_vector(temporal_filter, nbatches);
    }
  else
    {
      temporal_filter[0] = 1.0;
      temporal_deltas[0] = 0.0;
    }

  /* the number of additional rows of buckets we put at the edge so
     that the filter doesn't go off the edge */
  gutter_width = (filter_width - oversample) / 2;
  height = oversample * image_height + 2 * gutter_width;
  width  = oversample * image_width  + 2 * gutter_width;

  nbuckets = width * height;
  if (1)
    {
      static char *last_block = NULL;
      static int   last_block_size = 0;
      int memory_rqd = (sizeof (bucket) * nbuckets +
                        sizeof (abucket) * nbuckets +
                        sizeof (point) * SUB_BATCH_SIZE);
      if (memory_rqd > last_block_size)
        {
          if (last_block != NULL)
            free (last_block);
          last_block = g_try_malloc (memory_rqd);
          if (last_block == NULL)
            {
              g_printerr ("render_rectangle: cannot malloc %d bytes.\n",
                          memory_rqd);
              exit (1);
            }
          last_block_size = memory_rqd;
        }
      buckets = (bucket *) last_block;
      accumulate = (abucket *) (last_block + sizeof (bucket) * nbuckets);
      points = (point *)  (last_block + (sizeof (bucket) + sizeof (abucket)) * nbuckets);
    }

  memset ((char *) accumulate, 0, sizeof (abucket) * nbuckets);
  for (batch_num = 0; batch_num < nbatches; batch_num++)
    {
      double        batch_time;
      double        sample_density;
      control_point cp;
      memset ((char *) buckets, 0, sizeof (bucket) * nbuckets);
      batch_time = spec->time + temporal_deltas[batch_num];

      /* interpolate and get a control point */
      interpolate (spec->cps, spec->ncps, batch_time, &cp);

      /* compute the colormap entries.  the input colormap is 256 long with
         entries from 0 to 1.0 */
      for (j = 0; j < CMAP_SIZE; j++)
        {
          for (k = 0; k < 3; k++)
            {
#if 1
              cmap[j][k] = (int) (cp.cmap[(j * 256) / CMAP_SIZE][k] *
                                  cp.white_level);
#else
              /* monochrome if you don't have any cmaps */
              cmap[j][k] = cp.white_level;
#endif
            }
          cmap[j][3] = cp.white_level;
        }
      /* compute camera */
      if (1)
        {
          double t0, t1, shift = 0.0, corner0, corner1;
          double scale;

          scale = pow (2.0, cp.zoom);
          sample_density = cp.sample_density * scale * scale;

          ppux = cp.pixels_per_unit * scale;
          ppuy = field ? (ppux / 2.0) : ppux;
          switch (field)
            {
            case field_both:
              shift =  0.0;
              break;
            case field_even:
              shift = -0.5;
              break;
            case field_odd:
              shift =  0.5;
              break;
            }
          shift = shift / ppux;
          t0 = (double) gutter_width / (oversample * ppux);
          t1 = (double) gutter_width / (oversample * ppuy);
          corner0 = cp.center[0] - image_width / ppux / 2.0;
          corner1 = cp.center[1] - image_height / ppuy / 2.0;
          bounds[0] = corner0 - t0;
          bounds[1] = corner1 - t1 + shift;
          bounds[2] = corner0 + image_width  / ppux + t0;
          bounds[3] = corner1 + image_height / ppuy + t1 + shift;
          size[0] = 1.0 / (bounds[2] - bounds[0]);
          size[1] = 1.0 / (bounds[3] - bounds[1]);
        }
      nsamples = (int) (sample_density * nbuckets /
                        (oversample * oversample));
      batch_size = nsamples / cp.nbatches;

      sbc = 0;
      for (sub_batch = 0;
           sub_batch < batch_size;
           sub_batch += SUB_BATCH_SIZE)
        {
          if (progress && (sbc++ % 32) == 0)
              (*progress)(0.5 * sub_batch / (double) batch_size);
          /* generate a sub_batch_size worth of samples */
          points[0][0] = random_uniform11 ();
          points[0][1] = random_uniform11 ();
          points[0][2] = random_uniform01 ();
          iterate (&cp, SUB_BATCH_SIZE, FUSE, points);

          /* merge them into buckets, looking up colors */
          for (j = 0; j < SUB_BATCH_SIZE; j++)
            {
              int k, color_index;
              double *p = points[j];
              bucket *b;

              /* Note that we must test if p[0] and p[1] is "within"
               * the valid bounds rather than "not outside", because
               * p[0] and p[1] might be NaN.
               */
              if (p[0] >= bounds[0] &&
                  p[1] >= bounds[1] &&
                  p[0] <= bounds[2] &&
                  p[1] <= bounds[3])
                {
                  color_index = (int) (p[2] * CMAP_SIZE);

                  if (color_index < 0)
                    color_index = 0;
                  else if (color_index > CMAP_SIZE - 1)
                    color_index = CMAP_SIZE - 1;

                  b = buckets +
                      (int) (width * (p[0] - bounds[0]) * size[0]) +
                      width * (int) (height * (p[1] - bounds[1]) * size[1]);

                  for (k = 0; k < 4; k++)
                    bump_no_overflow(b[0][k], cmap[color_index][k], short);
                }
            }
        }

      if (1)
        {
          double k1 = (cp.contrast * cp.brightness *
                       PREFILTER_WHITE * 268.0 *
                       temporal_filter[batch_num]) / 256;
          double area = image_width * image_height / (ppux * ppuy);
          double k2 = (oversample * oversample * nbatches) /
                       (cp.contrast * area * cp.white_level * sample_density);

          /* log intensity in hsv space */
          for (j = 0; j < height; j++)
            for (i = 0; i < width; i++)
              {
                abucket *a = accumulate + i + j * width;
                bucket *b = buckets + i + j * width;
                double c[4], ls;
                c[0] = (double) b[0][0];
                c[1] = (double) b[0][1];
                c[2] = (double) b[0][2];
                c[3] = (double) b[0][3];
                if (0.0 == c[3])
                  continue;

                ls = (k1 * log(1.0 + c[3] * k2))/c[3];
                c[0] *= ls;
                c[1] *= ls;
                c[2] *= ls;
                c[3] *= ls;

                bump_no_overflow(a[0][0], c[0] + 0.5, accum_t);
                bump_no_overflow(a[0][1], c[1] + 0.5, accum_t);
                bump_no_overflow(a[0][2], c[2] + 0.5, accum_t);
                bump_no_overflow(a[0][3], c[3] + 0.5, accum_t);
              }
        }
    }
  /*
   * filter the accumulation buffer down into the image
   */
  if (1)
    {
      int    x, y;
      double t[4];
      double g = 1.0 / spec->cps[0].gamma;
      y = 0;
      for (j = 0; j < image_height; j++)
        {
          if (progress && (j % 32) == 0)
            (*progress)(0.5 + 0.5 * j / (double)image_height);
          x = 0;
          for (i = 0; i < image_width; i++)
            {
              int            ii, jj, a;
              unsigned char *p;
              t[0] = t[1] = t[2] = t[3] = 0.0;
              for (ii = 0; ii < filter_width; ii++)
                for (jj = 0; jj < filter_width; jj++)
                  {
                    double k = filter[ii + jj * filter_width];
                    abucket *a = accumulate + x + ii + (y + jj) * width;

                    t[0] += k * a[0][0];
                    t[1] += k * a[0][1];
                    t[2] += k * a[0][2];
                    t[3] += k * a[0][3];
                  }
              /* FIXME: we should probably use glib facilities to make
               * this code readable
               */
              p = out + nchan * (i + j * out_width);
              a = 256.0 * pow((double) t[0] / PREFILTER_WHITE, g) + 0.5;
              if (a < 0) a = 0; else if (a > 255) a = 255;
              p[0] = a;
              a = 256.0 * pow((double) t[1] / PREFILTER_WHITE, g) + 0.5;
              if (a < 0) a = 0; else if (a > 255) a = 255;
              p[1] = a;
              a = 256.0 * pow((double) t[2] / PREFILTER_WHITE, g) + 0.5;
              if (a < 0) a = 0; else if (a > 255) a = 255;
              p[2] = a;
              if (nchan > 3)
                {
                  a = 256.0 * pow((double) t[3] / PREFILTER_WHITE, g) + 0.5;
                  if (a < 0) a = 0; else if (a > 255) a = 255;
                  p[3] = a;
                }
              x += oversample;
            }
          y += oversample;
        }
    }

  free (filter);
  free (temporal_filter);
  free (temporal_deltas);
}
