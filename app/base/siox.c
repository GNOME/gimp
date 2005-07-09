/*
 * The GIMP Foreground Extraction Utility
 * segmentator.c - main algorithm.
 *
 * For algorithm documentation refer to:
 * G. Friedland, K. Jantz, L. Knipping, R. Rojas:
 * "Image Segmentation by Uniform Color Clustering
 *  -- Approach and Benchmark Results",
 * Technical Report B-05-07, Department of Computer Science,
 * Freie Universitaet Berlin, June 2005.
 * http://www.inf.fu-berlin.de/inst/pubs/tr-b-05-07.pdf
 *
 * Algorithm idea by Gerald Friedland.
 * This implementation is Copyright (C) 2005
 * by Gerald Friedland <fland@inf.fu-berlin.de>
 * and Kristian Jantz <jantz@inf.fu-berlin.de>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <string.h>

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "base-types.h"

#include "paint-funcs/paint-funcs.h"

#include "pixel-region.h"
#include "segmentator.h"
#include "tile-manager.h"


/* Simulate a java.util.ArrayList */
/* These methods are NOT generic */

typedef struct
{
  float l;
  float a;
  float b;
  int   cardinality;
} lab;

typedef struct _ArrayList ArrayList;

struct _ArrayList
{
  lab       *array;
  guint      arraylength;
  gboolean   owned;
  ArrayList *next;
};

static void
add_to_list (ArrayList *list,
             lab       *array,
             guint      arraylength,
             gboolean   take)
{
  ArrayList *cur = list;
  ArrayList *prev;

  do
    {
      prev = cur;
      cur = cur->next;
    }
  while (cur);

  prev->next = g_new0 (ArrayList, 1);

  prev->array = array;
  prev->arraylength = arraylength;
  prev->owned = take;
}

static int
list_size (ArrayList *list)
{
  ArrayList *cur   = list;
  int        count = 0;

  while (cur->array)
    {
      count++;
      cur = cur->next;
    }

  return count;
}

static lab *
list_to_array (ArrayList *list,
               int       *returnlength)
{
  ArrayList *cur   = list;
  gint       i     = 0;
  gint       len   = list_size (list);
  lab       *array = g_new (lab, len);

  *returnlength = len;

  while (cur->array)
    {
      array[i++] = cur->array[0];

      /* Every array in the list node has only one point
       * when we call this method
       */
      cur = cur->next;
    }

  return array;
}

static void
free_list (ArrayList *list)
{
  ArrayList *cur = list;

  while (cur)
    {
      ArrayList *prev = cur;

      cur = cur->next;

      if (prev->owned)
        g_free (prev->array);

      g_free (prev);
    }
}

static void
calcLAB (const guchar *src,
         lab          *pixel)
{
  gfloat r = src[RED_PIX] / 255.0;
  gfloat g = src[GREEN_PIX] / 255.0;
  gfloat b = src[BLUE_PIX] / 255.0;
  gfloat x, y, z;

  if (r > 0.04045)
    r = (float) pow ((r + 0.055) / 1.055, 2.4);
  else
    r = r / 12.92;

  if (g > 0.04045)
    g = (float) pow ((g + 0.055) / 1.055, 2.4);
  else
    g = g / 12.92;

  if (b > 0.04045)
    b = (float) pow ((b + 0.055) / 1.055, 2.4);
  else
    b = b / 12.92;

  r = r * 100.0;
  g = g * 100.0;
  b = b * 100.0;

  /* Observer. = 2°, Illuminant = D65 */
  x = (float) (r * 0.4124 + g * 0.3576 + b * 0.1805) / 95.047;
  y = (float) (r * 0.2126 + g * 0.7152 + b * 0.0722) / 100.0;
  z = (float) (r * 0.0193 + g * 0.1192 + b * 0.9505) / 108.883;

  if (x > 0.008856)
    x = (float) pow (x, (1.0 / 3));
  else
    x = (7.787 * x) + (16.0 / 116);

  if (y > 0.008856)
    y = (float) pow (y, (1.0 / 3));
  else
    y = (7.787 * y) + (16.0 / 116);

  if (z > 0.008856)
    z = (float) pow (z, (1.0 / 3));
  else
    z = (7.787 * z) + (16.0 / 116);

  pixel->l = (116 * y) - 16;
  pixel->a = 500 * (x - y);
  pixel->b = 200 * (y - z);
}

#if 0
static float cie_f (float t)
{
  return t > 0.008856 ? (1 / 3.0) : 7.787 * t + 16.0 / 116.0;
}
#endif


/* Stage one of modified KD-Tree algorithm */
static void
stageone (lab       *points,
          int        dims,
          int        depth,
          ArrayList *clusters,
          float      limits[DIMS],
          int        length)
{
  int    curdim = depth % dims;
  float  min, max;
  /* find maximum and minimum */
  int    i, countsm, countgr, smallc, bigc;
  float  pivotvalue, curval;
  lab   *smallerpoints;
  lab   *biggerpoints;

  if (length < 1)
    return;

  if (curdim == 0)
    curval = points[0].l;
  else if (curdim == 1)
    curval = points[0].a;
  else
    curval = points[0].b;

  min = curval;
  max = curval;

  for (i = 1; i < length; i++)
    {
      if (curdim == 0)
        curval = points[i].l;
      else if (curdim == 1)
        curval = points[i].a;
      else if (curdim == 2)
        curval = points[i].b;

      if (min > curval)
        min = curval;

      if (max < curval)
        max = curval;
    }

  /* Split according to Rubner-Rule */
  if (max - min > limits[curdim])
    {
      pivotvalue = ((max - min) / 2.0) + min;
      countsm = 0;
      countgr = 0;

      /* find out cluster sizes */
      for (i = 0; i < length; i++)
        {
          if (curdim == 0)
            curval = points[i].l;
          else if (curdim == 1)
            curval = points[i].a;
          else if (curdim == 2)
            curval = points[i].b;

          if (curval <= pivotvalue)
            {
              countsm++;
            }
          else
            {
              countgr++;
            }
        }

      smallerpoints = g_new (lab, countsm);
      biggerpoints = g_new (lab, countgr);
      smallc = 0;
      bigc = 0;

      for (i = 0; i < length; i++)
        {       /* do actual split */
          if (curdim == 0)
            curval = points[i].l;
          else if (curdim == 1)
            curval = points[i].a;
          else if (curdim == 2)
            curval = points[i].b;

          if (curval <= pivotvalue)
            {
              smallerpoints[smallc++] = points[i];
            }
          else
            {
              biggerpoints[bigc++] = points[i];
            }
        }

      if (depth > 0)
        g_free (points);

      /* create subtrees */
      stageone (smallerpoints, dims, depth + 1, clusters, limits, countsm);
      stageone (biggerpoints, dims, depth + 1, clusters, limits, countgr);
    }
  else
    { /* create leave */
      add_to_list (clusters, points, length, depth != 0);
    }
}


/* Stage two of modified KD-Tree algorithm */

/* This is very similar to stageone... but in future there will be more
 * differences => not integrated into method stageone()
 */
static void
stagetwo (lab       *points,
          int        dims,
          int        depth,
          ArrayList *clusters,
          float      limits[DIMS],
          int        length,
          int        total,
          float      threshold)
{
  int    curdim = depth % dims;
  float  min, max;
  /* find maximum and minimum */
  int    i, countsm, countgr, smallc, bigc;
  float  pivotvalue, curval;
  int    sum;
  lab   *point;
  lab   *smallerpoints;
  lab   *biggerpoints;

  if (length < 1)
    return;

  if (curdim == 0)
    curval = points[0].l;
  else if (curdim == 1)
    curval = points[0].a;
  else
    curval = points[0].b;

  min = curval;
  max = curval;

  for (i = 1; i < length; i++)
    {
      if (curdim == 0)
        curval = points[i].l;
      else if (curdim == 1)
        curval = points[i].a;
      else if (curdim == 2)
        curval = points[i].b;

      if (min > curval)
        min = curval;

      if (max < curval)
        max = curval;
    }

  /* Split according to Rubner-Rule */
  if (max - min > limits[curdim])
    {
      pivotvalue = ((max - min) / 2.0) + min;

      /*  g_printerr ("max=%f min=%f pivot=%f\n",max,min,pivotvalue); */
      countsm = 0;
      countgr = 0;

      for (i = 0; i < length; i++)
        {  /* find out cluster sizes */
          if (curdim == 0)
            curval = points[i].l;
          else if (curdim == 1)
            curval = points[i].a;
          else if (curdim == 2)
            curval = points[i].b;

          if (curval <= pivotvalue)
            {
              countsm++;
            }
          else
            {
              countgr++;
            }
        }

      smallerpoints = g_new (lab, countsm);
      biggerpoints = g_new (lab, countgr);
      smallc = 0;
      bigc = 0;

      /* do actual split */
      for (i = 0; i < length; i++)
        {
          if (curdim == 0)
            curval = points[i].l;
          else if (curdim == 1)
            curval = points[i].a;
          else if (curdim == 2)
            curval = points[i].b;

          if (curval <= pivotvalue)
            {
              smallerpoints[smallc++] = points[i];
            }
          else
            {
              biggerpoints[bigc++] = points[i];
            }
        }

      g_free (points);

      /* create subtrees */
      stagetwo (smallerpoints, dims, depth + 1, clusters, limits,
                countsm, total, threshold);
      stagetwo (biggerpoints, dims, depth + 1, clusters, limits,
                countgr, total, threshold);
    }
  else /* create leave */
    {
      sum = 0;

      for (i = 0; i < length; i++)
        {
          sum += points[i].cardinality;
        }

      if (((sum * 100.0) / total) >= threshold)
        {
          point = g_new0 (lab, 1);

          for (i = 0; i < length; i++)
            {
              point->l += points[i].l;
              point->a += points[i].a;
              point->b += points[i].b;
            }

          point->l /= (length * 1.0);
          point->a /= (length * 1.0);
          point->b /= (length * 1.0);

          /* g_printerr ("cluster=%f, %f, %f sum=%d\n",
                          point->l, point->a, point->b, sum);
           */

          add_to_list (clusters, point, 1, TRUE);
        }

      g_free (points);
    }
}

/* squared euclidean distance */
static inline float
euklid (const lab p,
        const lab q)
{
  return ((p.l - q.l) * (p.l - q.l) +
          (p.a - q.a) * (p.a - q.a) +
          (p.b - q.b) * (p.b - q.b));
}

/* Creates a color signature for a given set of pixels */
static lab *
create_signature (lab   *input,
                  int    length,
                  float  limits[DIMS],
                  int   *returnlength)
{
  ArrayList *clusters1;
  ArrayList *clusters2;
  ArrayList *curelem;
  lab       *centroids;
  lab       *cluster;
  lab        centroid;
  lab       *rval;
  int        k, i;
  int        clusters1size;

  if (length < 1)
    {
      *returnlength = 0;
      return NULL;
    }

  clusters1 = g_new0 (ArrayList, 1);

  stageone (input, DIMS, 0, clusters1, limits, length);
  clusters1size = list_size (clusters1);
  centroids = g_new (lab, clusters1size);
  curelem = clusters1;

  i = 0;
  while (curelem->array)
    {
      centroid.l = 0;
      centroid.a = 0;
      centroid.b = 0;
      cluster = curelem->array;

      for (k = 0; k < curelem->arraylength; k++)
        {
          centroid.l += cluster[k].l;
          centroid.a += cluster[k].a;
          centroid.b += cluster[k].b;
        }

      centroids[i].l = centroid.l / (curelem->arraylength * 1.0);
      centroids[i].a = centroid.a / (curelem->arraylength * 1.0);
      centroids[i].b = centroid.b / (curelem->arraylength * 1.0);
      centroids[i].cardinality = curelem->arraylength;
      i++;
      curelem = curelem->next;
    }

  /* g_printerr ("step #1 -> %d clusters\n", clusters1size); */

  clusters2 = g_new0 (ArrayList, 1);

  stagetwo (centroids, DIMS, 0, clusters2, limits, clusters1size, length, 0.1);

  /* see paper by tomasi */
  rval = list_to_array (clusters2, returnlength);

  free_list (clusters2);
  free_list (clusters1);

  /* g_printerr ("step #2 -> %d clusters\n", returnlength[0]); */

  return rval;
}

/* Smoothes the confidence matrix */
static void
smoothcm (float *cm,
          int    xres,
          int    yres,
          float  f1,
          float  f2,
          float  f3)
{
  int y, x, idx;

  /* Smoothright */
  for (y = 0; y < yres; y++)
    {
      for (x = 0; x < xres - 2; x++)
        {
          idx = (y * xres) + x;
          cm[idx] =
            f1 * cm[idx]     +
            f2 * cm[idx + 1] +
            f3 * cm[idx + 2];
        }
    }

  /* Smoothleft */
  for (y = 0; y < yres; y++)
    {
      for (x = xres - 1; x >= 2; x--)
        {
          idx = (y * xres) + x;
          cm[idx] =
            f3 * cm[idx - 2] +
            f2 * cm[idx - 1] +
            f1 * cm[idx];
      }
    }

  /* Smoothdown */
  for (y = 0; y < yres - 2; y++)
    {
      for (x = 0; x < xres; x++)
        {
          idx = (y * xres) + x;
          cm[idx] =
            f1 * cm[idx] +
            f2 * cm[((y + 1) * xres) + x] +
            f3 * cm[((y + 2) * xres) + x];
        }
    }

  /* Smoothup */
  for (y = yres - 1; y >= 2; y--)
    {
      for (x = 0; x < xres; x++)
        {
          idx = (y * xres) + x;
          cm[idx] =
            f3 * cm[((y - 2) * xres) + x] +
            f2 * cm[((y - 1) * xres) + x] +
            f1 * cm[idx];
        }
    }
}

static void
normalize_mask (TileManager *mask,
                gint         x,
                gint         y,
                gint         width,
                gint         height)
{
  PixelRegion region;
  gpointer    pr;
  gint        row, col;
  guchar      max = 0;

  pixel_region_init (&region, mask, x, y, width, height, FALSE);

  for (pr = pixel_regions_register (1, &region);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      guchar *data = region.data;

      for (row = 0; row < region.h; row++)
        {
          guchar *d = data;

          for (col = 0; col < region.w; col++, d++)
            {
              if (*d > max)
                max = *d;
            }

          data += region.rowstride;
        }
    }

  if (max == 255)
    return;

  g_printerr ("max = %d (need to actually implement normalize ?)\n", max);
  /*  TODO (or not TODO)  */
}

static void
threshold_mask (TileManager *mask,
                gint         x,
                gint         y,
                gint         width,
                gint         height)
{
  PixelRegion region;
  gpointer    pr;
  gint        row, col;

  pixel_region_init (&region, mask, x, y, width, height, TRUE);

  for (pr = pixel_regions_register (1, &region);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      guchar *data = region.data;

      for (row = 0; row < region.h; row++)
        {
          guchar *d = data;

          for (col = 0; col < region.w; col++, d++)
            {
              *d = *d > 127 ? 255 : 0;
            }

          data += region.rowstride;
        }
    }
}

static void
smooth_mask (TileManager *mask,
             gint         x,
             gint         y,
             gint         width,
             gint         height)
{
  /*  TODO  */
}

static void
erode_mask (TileManager *mask,
            gint         x,
            gint         y,
            gint         width,
            gint         height)
{
  PixelRegion region;

  pixel_region_init (&region, mask, x, y, width, height, TRUE);

  /* inefficient */
  thin_region (&region, 1, 1, TRUE);
}

static void
dilate_mask (TileManager *mask,
             gint         x,
             gint         y,
             gint         width,
             gint         height)
{
  PixelRegion region;

  pixel_region_init (&region, mask, x, y, width, height, TRUE);

  /* inefficient */
  fatten_region (&region, 1, 1);
}

static void
find_max_blob (TileManager *mask,
               gint         x,
               gint         y,
               gint         width,
               gint         height)
{
  GQueue     *q          = g_queue_new ();
  gint        length     = width * height;
  gint       *labelfield = g_new0 (gint, length);
  PixelRegion region;
  gpointer    pr;
  gint        row, col;
  gint        curlabel  = 1;
  gint        maxregion = 0;
  gint        maxblob   = 0;

  pixel_region_init (&region, mask, x, y, width, height, FALSE);

  for (pr = pixel_regions_register (1, &region);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *data  = region.data;
      gint          index = (region.x - x) + (region.y - y) * width;

      for (row = 0; row < region.h; row++)
        {
          const guchar *d = data;
          gint          i = index;

          for (col = 0; col < region.w; col++, d++, i++)
            {
              gint regioncount = 0;

              if (labelfield[i] == 0 && *d > 127)
                g_queue_push_tail (q, GINT_TO_POINTER (i));

              while (! g_queue_is_empty (q))
                {
                  gint pos = GPOINTER_TO_INT (g_queue_pop_head (q));

                  if (pos < 0 || pos >= length)
                    continue;

                  if (labelfield[pos] == 0)
                    {
                      guchar val;

                      read_pixel_data_1 (mask, pos % width, pos / width, &val);
                      if (val > 127)
                        {
                          labelfield[pos] = curlabel;

                          regioncount++;

                          g_queue_push_tail (q, GINT_TO_POINTER (pos + 1));
                          g_queue_push_tail (q, GINT_TO_POINTER (pos - 1));
                          g_queue_push_tail (q, GINT_TO_POINTER (pos + width));
                          g_queue_push_tail (q, GINT_TO_POINTER (pos - width));
                        }
                    }
                }

              if (regioncount > maxregion)
                {
                  maxregion = regioncount;
                  maxblob = curlabel;
                }

              curlabel++;
            }

          data += region.rowstride;
          index += width;
        }
    }

  pixel_region_init (&region, mask, x, y, width, height, TRUE);

  for (pr = pixel_regions_register (1, &region);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      guchar *data  = region.data;
      gint    index = (region.x - x) + (region.y - y) * width;

      for (row = 0; row < region.h; row++)
        {
          guchar *d = data;
          gint    i = index;

          for (col = 0; col < region.w; col++, d++, i++)
            {
              if (labelfield[i] != 0 && labelfield[i] != maxblob)
                *d = 0;
            }

          data += region.rowstride;
          index += width;
        }
    }

  g_queue_free (q);
  g_free (labelfield);
}

/* Returns squared clustersize */
static gfloat
getclustersize (const float limits[DIMS])
{
  float sum = (limits[0] - (-limits[0])) * (limits[0] - (-limits[0]));

  sum += (limits[1] - (-limits[1])) * (limits[1] - (-limits[1]));
  sum += (limits[2] - (-limits[2])) * (limits[2] - (-limits[2]));

  return sum;
}


/*
 * Call this method:
 * rgbs - the picture
 * confidencematrix - a confidencematrix with values <=0.1 is sure background,
 *                    >=0.9 is sure foreground, rest unknown
 * xres, yres - the dimensions of the picture and the confidencematrix
 * limits - a three dimensional float array specifing the accuracy
 *          a good value is: {0.66,1.25,2.5}
 * int smoothness - specifies how smooth the boundaries of a picture should
 *                  be made (value greater or equal to 0).
 *                  More smooth = fault tolerant,
 *                  less smooth = exact boundaries - try 3 for a first guess.
 * returns and writes into the confidencematrix the resulting segmentation
 */
void
foreground_extract (TileManager  *pixels,
                    TileManager  *mask,
                    gfloat        limits[DIMS],
                    gint          smoothness)
{
  gfloat    clustersize = getclustersize (limits);
  gint      surebgcount = 0;
  gint      surefgcount = 0;
  gint      i, j;
  gint      bgsiglen, fgsiglen;
  lab      *surebg;
  lab      *surefg;
  lab      *bgsig;
  lab      *fgsig;

  PixelRegion  srcPR;
  PixelRegion  mapPR;
  gpointer     pr;
  gint         width, height;
  gint         bpp;
  gint         row, col;

  g_return_if_fail (pixels != NULL);
  g_return_if_fail (mask != NULL && tile_manager_bpp (mask) == 1);

  width = tile_manager_width (pixels);
  height = tile_manager_height (pixels);
  bpp = tile_manager_bpp (pixels);

  g_return_if_fail (bpp == 3 || bpp == 4);
  g_return_if_fail (tile_manager_width (mask) == width);
  g_return_if_fail (tile_manager_height (mask) == height);

  /* count given foreground and background pixels */
  pixel_region_init (&mapPR, mask, 0, 0, width, height, FALSE);

  for (pr = pixel_regions_register (1, &mapPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *map = mapPR.data;

      for (row = 0; row < mapPR.h; row++)
        {
          const guchar *m = map;

          for (col = 0; col < mapPR.w; col++, m++)
            {
              if (*m < 32)
                surebgcount++;
              else if (*m > 224)
                surefgcount++;
            }

          map += mapPR.rowstride;
        }
    }

  surebg = g_new (lab, surebgcount);
  surefg = g_new (lab, surefgcount);

  i = 0;
  j = 0;

  /* create inputs for colorsignatures */
  pixel_region_init (&srcPR, pixels, 0, 0, width, height, FALSE);
  pixel_region_init (&mapPR, mask, 0, 0, width, height, FALSE);

  for (pr = pixel_regions_register (2, &srcPR, &mapPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *src = srcPR.data;
      const guchar *map = mapPR.data;

      for (row = 0; row < srcPR.h; row++)
        {
          const guchar *s = src;
          const guchar *m = map;

          for (col = 0; col < srcPR.w; col++, m++, s += bpp)
            {
              if (*m < 32)
                {
                  calcLAB (s, surebg + i);
                  i++;
                }
              else if (*m > 224)
                {
                  calcLAB (s, surefg + j);
                  j++;
                }
            }

          src += srcPR.rowstride;
          map += mapPR.rowstride;
        }
    }

  /* Create color signature for bg */
  bgsig = create_signature (surebg, surebgcount, limits, &bgsiglen);
  g_free (surebg);

  if (bgsiglen < 1)
    {
      g_free (surefg);
      return;
    }

  /* Create color signature for fg */
  fgsig = create_signature (surefg, surefgcount, limits, &fgsiglen);
  g_free (surefg);

  /* Classify - the slow way....Better: Tree traversation */
  pixel_region_init (&srcPR, pixels, 0, 0, width, height, FALSE);
  pixel_region_init (&mapPR, mask, 0, 0, width, height, TRUE);

  for (pr = pixel_regions_register (2, &srcPR, &mapPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      const guchar *src = srcPR.data;
      guchar       *map = mapPR.data;

      for (row = 0; row < srcPR.h; row++)
        {
          const guchar *s = src;
          guchar       *m = map;

          for (col = 0; col < srcPR.w; col++, m++, s += bpp)
            {
              lab       labpixel;
              gboolean  background = FALSE;
              gfloat    min, d;

              if (*m < 32 || *m > 224)
                continue;

              calcLAB (s, &labpixel);
              background = TRUE;
              min = euklid (labpixel, bgsig[0]);

              for (i = 1; i < bgsiglen; i++)
                {
                  d = euklid (labpixel, bgsig[i]);

                  if (d < min)
                    min = d;
                }

              if (fgsiglen == 0)
                {
                  if (min < clustersize)
                    background = TRUE;
                  else
                    background = FALSE;
                }
              else
                {
                  for (i = 0; i < fgsiglen; i++)
                    {
                      d = euklid (labpixel, fgsig[i]);

                      if (d < min)
                        {
                          min = d;
                          background = FALSE;
                          break;
                        }
                    }
                }

              *m = background ? 0 : 255;
            }

          src += srcPR.rowstride;
          map += mapPR.rowstride;
        }
    }

  g_free (fgsig);
  g_free (bgsig);

  /* Smooth a bit for error killing */
  smooth_mask (mask, 0, 0, width, height);

  normalize_mask (mask, 0, 0, width, height);

  /* Now erode, to make sure only "strongly connected components"
   * keep being connected
   */
  erode_mask (mask, 0, 0, width, height);

  /* search the biggest connected component */
  find_max_blob (mask, 0, 0, width, height);

  /* smooth again - as user specified */
  for (i = 0; i < smoothness; i++)
    smooth_mask (mask, 0, 0, width, height);

  normalize_mask (mask, 0, 0, width, height);

  /* Threshold the values */
  threshold_mask (mask, 0, 0, width, height);

  /* search the biggest connected component again to kill jitter */
  find_max_blob (mask, 0, 0, width, height);

  /* Now dilate, to fill up boundary pixels killed by erode */
  dilate_mask (mask, 0, 0, width, height);
}


/************ Unused functions, for reference ***************/

/* calculates alpha \times Confidencematrix */
static void
premultiply_matrix (float  alpha,
                    float *cm,
                    int    length)
{
  int i;

  for (i = 0; i < length; i++)
    cm[i] = alpha * cm[i];
}

/* Normalizes a confidencematrix */
static void
normalize_matrix (float *cm,
                  int   length)
{
  float max = 0.0;
  float alpha = 0.0;
  int i;

  for (i = 0; i < length; i++)
    {
      if (max < cm[i])
        max = cm[i];
    }

  if (max <= 0.0)
    return;
  if (max == 1.00)
    return;

  alpha = 1.00f / max;
  premultiply_matrix (alpha, cm, length);
}

/* A confidence matrix eroder */
static void
erode2 (float *cm,
        int    xres,
        int    yres)
{
  int idx, x, y;

  /* From right */
  for (y = 0; y < yres; y++)
    {
      for (x = 0; x < xres - 1; x++)
        {
          idx = (y * xres) + x;
          cm[idx] = MIN (cm[idx], cm[idx + 1]);
        }
    }

  /* From left */
  for (y = 0; y < yres; y++)
    {
      for (x = xres - 1; x >= 1; x--)
        {
          idx = (y * xres) + x;
          cm[idx] = MIN (cm[idx - 1], cm[idx]);
        }
    }

  /* From down */
  for (y = 0; y < yres - 1; y++)
    {
      for (x = 0; x < xres; x++)
        {
          idx = (y * xres) + x;
          cm[idx] = MIN (cm[idx], cm[((y + 1) * xres) + x]);
        }
    }

  /* From up */
  for (y = yres - 1; y >= 1; y--)
    {
      for (x = 0; x < xres; x++)
        {
          idx = (y * xres) + x;
          cm[idx] = MIN (cm[((y - 1) * xres) + x], cm[idx]);
        }
    }
}

/* A confidence matrix dilater */
static void
dilate2 (float *cm,
         int    xres,
         int    yres)
{
  int x, y, idx;

  /* From right */
  for (y = 0; y < yres; y++)
    {
      for (x = 0; x < xres - 1; x++) {
        idx = (y * xres) + x;
        cm[idx] = MAX (cm[idx], cm[idx + 1]);
      }
    }

  /* From left */
  for (y = 0; y < yres; y++)
    {
      for (x = xres - 1; x >= 1; x--)
        {
          idx = (y * xres) + x;
          cm[idx] = MAX (cm[idx - 1], cm[idx]);
        }
    }

  /* From down */
  for (y = 0; y < yres - 1; y++)
    {
      for (x = 0; x < xres; x++)
        {
          idx = (y * xres) + x;
          cm[idx] = MAX (cm[idx], cm[((y + 1) * xres) + x]);
        }
    }

    /* From up */
   for (y = yres - 1; y >= 1; y--)
     {
       for (x = 0; x < xres; x++)
         {
           idx = (y * xres) + x;
           cm[idx] = MAX (cm[((y - 1) * xres) + x], cm[idx]);
         }
     }
}

/* region growing */
static void
findmaxblob (float *cm,
             guint *image,
             int    xres,
             int    yres)
{
  int     i;
  int     curlabel = 1;
  int     maxregion = 0;
  int     maxblob = 0;
  int     regioncount = 0;
  int     pos = 0;
  int     length = xres * yres;
  int    *labelfield = g_new0 (int, length);
  GQueue *q = g_queue_new ();

  for (i = 0; i < length; i++)
    {
      regioncount = 0;

      if (labelfield[i] == 0 && cm[i] >= 0.5)
        g_queue_push_tail (q, GINT_TO_POINTER (i));

      while (! g_queue_is_empty (q))
        {
          pos = GPOINTER_TO_INT (g_queue_pop_head (q));

          if (pos < 0 || pos >= length)
            continue;

          if (labelfield[pos] == 0 && cm[pos] >= 0.5f)
            {
              labelfield[pos] = curlabel;

              regioncount++;

              g_queue_push_tail (q, GINT_TO_POINTER (pos + 1));
              g_queue_push_tail (q, GINT_TO_POINTER (pos - 1));
              g_queue_push_tail (q, GINT_TO_POINTER (pos + xres));
              g_queue_push_tail (q, GINT_TO_POINTER (pos - xres));
            }
        }

      if (regioncount > maxregion)
        {
          maxregion = regioncount;
          maxblob = curlabel;
        }

      curlabel++;
    }

  for (i = 0; i < length; i++)
    {
      /* Kill everything that is not biggest blob! */
      if (labelfield[i] != 0 && labelfield[i] != maxblob)
        cm[i] = 0.0;
    }

  g_queue_free (q);
  g_free (labelfield);
}
