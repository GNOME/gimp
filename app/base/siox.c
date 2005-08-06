/*
 * SIOX: Simple Interactive Object Extraction
 *
 * For algorithm documentation refer to:
 * G. Friedland, K. Jantz, L. Knipping, R. Rojas:
 * "Image Segmentation by Uniform Color Clustering
 *  -- Approach and Benchmark Results",
 * Technical Report B-05-07, Department of Computer Science,
 * Freie Universitaet Berlin, June 2005.
 * http://www.inf.fu-berlin.de/inst/pubs/tr-b-05-07.pdf
 *
 * See http://www.siox.org/ for more information.
 *
 * Algorithm idea by Gerald Friedland.
 * This implementation is Copyright (C) 2005
 * by Gerald Friedland <fland@inf.fu-berlin.de>
 * and Kristian Jantz <jantz@inf.fu-berlin.de>.
 *
 * Adapted for GIMP by Sven Neumann <sven@gimp.org>
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

#include "cpercep.h"
#include "pixel-region.h"
#include "siox.h"
#include "tile-manager.h"


/* Amount of color dimensions in one point */
#define SIOX_DIMS 3


/* Thresholds in the mask:
 *   pixels < LOW are known background
 *   pixels > HIGH are known foreground
 */
#define SIOX_LOW  1
#define SIOX_HIGH 254


/* #define DEBUG */


/* Simulate a java.util.ArrayList */

/* Could be improved. At the moment we are wasting a node per list and
 * the tail pointer on each node is only used in the first node.
 */

typedef struct
{
  gfloat l;
  gfloat a;
  gfloat b;
  gint   cardinality;
} lab;

typedef struct _ArrayList ArrayList;

struct _ArrayList
{
  lab       *array;
  guint      arraylength;
  gboolean   owned;
  ArrayList *next;
  ArrayList *tail; /* only valid in the root item */
};

static ArrayList *
list_new (void)
{
  ArrayList *list = g_new0 (ArrayList, 1);

  list->tail = list;

  return list;
}

static void
add_to_list (ArrayList *list,
             lab       *array,
             guint      arraylength,
             gboolean   take)
{
  ArrayList *tail = list->tail;

  tail->array = array;
  tail->arraylength = arraylength;
  tail->owned = take;

  list->tail = tail->next = g_new0 (ArrayList, 1);
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
               gint      *returnlength)
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

/* FIXME: try to use cpersep conversion here, should be faster */
static void
calc_lab (const guchar *src,
          gint          bpp,
          const guchar *colormap,
          lab          *pixel)
{
  gdouble l, a, b;

  switch (bpp)
    {
    case 3:  /* RGB  */
    case 4:  /* RGBA */
      cpercep_rgb_to_space (src[RED_PIX],
                            src[GREEN_PIX],
                            src[BLUE_PIX], &l, &a, &b);
      break;

    case 2:
    case 1:
      if (colormap) /* INDEXED(A) */
        {
          gint i = *src * 3;

          cpercep_rgb_to_space (colormap[i + RED_PIX],
                                colormap[i + GREEN_PIX],
                                colormap[i + BLUE_PIX], &l, &a, &b);
        }
      else /* GRAY(A) */
        {
          /*  FIXME: there should be cpercep_gray_to_space  */
          cpercep_rgb_to_space (*src, *src, *src, &l, &a, &b);
        }
      break;

    default:
      g_return_if_reached ();
    }

  pixel->l = l;
  pixel->a = a;
  pixel->b = b;
}

/* Stage one of modified KD-Tree algorithm */
static void
stageone (lab          *points,
          gint          dims,
          gint          depth,
          ArrayList    *clusters,
          const gfloat  limits[SIOX_DIMS],
          gint          length)
{
  gint    curdim = depth % dims;
  gfloat  min, max;
  gint    i, countsm, countgr, smallc, bigc;
  gfloat  pivotvalue, curval;
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

      /* FIXME: consider to sort the array and split in place instead
       *        of allocating memory here
       */
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
stagetwo (lab          *points,
          gint          dims,
          gint          depth,
          ArrayList    *clusters,
          const gfloat  limits[SIOX_DIMS],
          gint          length,
          gint          total,
          gfloat        threshold)
{
  gint    curdim = depth % dims;
  gfloat  min, max;
  gint    i, countsm, countgr, smallc, bigc;
  gfloat  pivotvalue, curval;
  gint    sum;
  lab    *point;
  lab    *smallerpoints;
  lab    *biggerpoints;

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

#ifdef DEBUG
      g_printerr ("max=%f min=%f pivot=%f\n", max, min, pivotvalue);
#endif

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

      /* FIXME: consider to sort the array and split in place instead
       *        of allocating memory here
       */
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

#ifdef DEBUG
          g_printerr ("cluster=%f, %f, %f sum=%d\n",
                      point->l, point->a, point->b, sum);
#endif

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
create_signature (lab          *input,
                  gint          length,
                  const gfloat  limits[SIOX_DIMS],
                  gint         *returnlength)
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

  clusters1 = list_new ();

  stageone (input, SIOX_DIMS, 0, clusters1, limits, length);
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

#ifdef DEBUG
  g_printerr ("step #1 -> %d clusters\n", clusters1size);
#endif

  clusters2 = list_new ();

  stagetwo (centroids,
            SIOX_DIMS, 0, clusters2, limits, clusters1size, length,
            0.1 /* magic constant, see paper by tomasi */);

  rval = list_to_array (clusters2, returnlength);

  free_list (clusters2);
  free_list (clusters1);

#ifdef DEBUG
  g_printerr ("step #2 -> %d clusters\n", returnlength[0]);
#endif

  return rval;
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
  PixelRegion region;

  pixel_region_init (&region, mask, x, y, width, height, TRUE);

  smooth_region (&region);
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

  erode_region (&region);
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

  dilate_region (&region);
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
get_clustersize (const gfloat limits[SIOX_DIMS])
{
  gfloat sum = (limits[0] - (-limits[0])) * (limits[0] - (-limits[0]));

  sum += (limits[1] - (-limits[1])) * (limits[1] - (-limits[1]));
  sum += (limits[2] - (-limits[2])) * (limits[2] - (-limits[2]));

  return sum;
}

static inline void
siox_progress_update (SioxProgressFunc  progress_callback,
                      gpointer          progress_data,
                      gdouble           value)
{
  if (progress_data)
    progress_callback (progress_data, value);
}

/**
 * siox_foreground_extract:
 * @pixels:     the tiles to extract the foreground from
 * @colormap:   colormap in case @pixels are indexed, %NULL otherwise
 * @offset_x:   horizontal offset of @pixels with respect to the @mask
 * @offset_y:   vertical offset of @pixels with respect to the @mask

 * @mask:       a mask indicating sure foreground (255), sure background (0)
 *              and undecided regions ([1..254]).
 * @x:          horizontal offset into the mask
 * @y:          vertical offset into the mask
 * @width:      width of working area on mask
 * @height:     height of working area on mask
 * @limits:     a double array with three entries specifing the accuracy,
 *              a good value is: { 0.66, 1.25, 2.5 }
 * @smoothness: boundary smoothness (a good value is 3)
 *
 * Writes the resulting segmentation into @mask.
 */
void
siox_foreground_extract (TileManager      *pixels,
                         const guchar     *colormap,
                         gint              offset_x,
                         gint              offset_y,
                         TileManager      *mask,
                         gint              x,
                         gint              y,
                         gint              width,
                         gint              height,
                         gint              smoothness,
                         const gdouble     limits[SIOX_DIMS],
                         SioxProgressFunc  progress_callback,
                         gpointer          progress_data)
{
  PixelRegion  srcPR;
  PixelRegion  mapPR;
  gpointer     pr;
  gint         bpp;
  gint         row, col;
  gfloat       clustersize;
  gint         surebgcount = 0;
  gint         surefgcount = 0;
  gint         i, j;
  gint         bgsiglen, fgsiglen;
  lab         *surebg;
  lab         *surefg;
  lab         *bgsig;
  lab         *fgsig;
  gfloat       flimits[3];

  g_return_if_fail (pixels != NULL);
  g_return_if_fail (mask != NULL && tile_manager_bpp (mask) == 1);
  g_return_if_fail (x >= 0);
  g_return_if_fail (y >= 0);
  g_return_if_fail (x + width <= tile_manager_width (mask));
  g_return_if_fail (y + height <= tile_manager_height (mask));
  g_return_if_fail (smoothness >= 0);
  g_return_if_fail (progress_data == NULL || progress_callback != NULL);

  cpercep_init ();

  siox_progress_update (progress_callback, progress_data, 0.0);

  flimits[0] = limits[0];
  flimits[1] = limits[1];
  flimits[2] = limits[2];

  clustersize = get_clustersize (flimits);

  /* count given foreground and background pixels */
  pixel_region_init (&mapPR, mask, x, y, width, height, FALSE);

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
              if (*m < SIOX_LOW)
                surebgcount++;
              else if (*m > SIOX_HIGH)
                surefgcount++;
            }

          map += mapPR.rowstride;
        }
    }

  surebg = g_new (lab, surebgcount);
  surefg = g_new (lab, surefgcount);

  i = 0;
  j = 0;

  siox_progress_update (progress_callback, progress_data, 0.1);

  bpp = tile_manager_bpp (pixels);

  /* create inputs for color signatures */
  pixel_region_init (&srcPR, pixels,
                     x - offset_x, y - offset_y, width, height, FALSE);
  pixel_region_init (&mapPR, mask, x, y, width, height, FALSE);

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
              if (*m < SIOX_LOW)
                {
                  calc_lab (s, bpp, colormap, surebg + i);
                  i++;
                }
              else if (*m > SIOX_HIGH)
                {
                  calc_lab (s, bpp, colormap, surefg + j);
                  j++;
                }
            }

          src += srcPR.rowstride;
          map += mapPR.rowstride;
        }
    }

  siox_progress_update (progress_callback, progress_data, 0.2);

  /* Create color signature for the background */
  bgsig = create_signature (surebg, surebgcount, flimits, &bgsiglen);
  g_free (surebg);

  if (bgsiglen < 1)
    {
      g_free (surefg);
      return;
    }

  siox_progress_update (progress_callback, progress_data, 0.3);

  /* Create color signature for the foreground */
  fgsig = create_signature (surefg, surefgcount, flimits, &fgsiglen);
  g_free (surefg);

  siox_progress_update (progress_callback, progress_data, 0.4);

  /* Classify - the slow way....Better: Tree traversation */
  pixel_region_init (&srcPR, pixels,
                     x - offset_x, y - offset_y, width, height, FALSE);
  pixel_region_init (&mapPR, mask, x, y, width, height, TRUE);

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

              if (*m < SIOX_LOW || *m > SIOX_HIGH)
                continue;

              calc_lab (s, bpp, colormap, &labpixel);
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

  siox_progress_update (progress_callback, progress_data, 0.9);

  /* Smooth a bit for error killing */
  smooth_mask (mask, x, y, width, height);

  /* Now erode, to make sure only "strongly connected components"
   * keep being connected
   */
  erode_mask (mask, x, y, width, height);

  /* search the biggest connected component */
  find_max_blob (mask, x, y, width, height);

  /* smooth again - as user specified */
  for (i = 0; i < smoothness; i++)
    smooth_mask (mask, x, y, width, height);

  /* Threshold the values */
  threshold_mask (mask, x, y, width, height);

  /* search the biggest connected component again to kill jitter */
  find_max_blob (mask, x, y, width, height);

  /* Now dilate, to fill up boundary pixels killed by erode */
  dilate_mask (mask, x, y, width, height);

  siox_progress_update (progress_callback, progress_data, 1.0);
}
