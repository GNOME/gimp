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
#include <math.h>

#include <glib-object.h>

#include "segmentator.h"

/* Please look all the way down for an explanation of JNI_COMPILE.
 */
#ifdef JNI_COMPILE
#include "NativeExperimentalPipe.h"
#endif


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
  ArrayList *cur = list;
  lab       *arraytoreturn;
  int        i = 0;
  int        len;

  len = list_size (list);
  arraytoreturn = g_new (lab, len);
  *returnlength = len;

  while (cur->array)
    {
      arraytoreturn[i++] = cur->array[0];

      /* Every array in the list node has only one point
       * when we call this method
       */
      cur = cur->next;
    }

  return arraytoreturn;
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

/* RGB -> CIELAB and other interesting methods... */

#ifdef JNI_COMPILE

/* Java */
static guchar getRed (int rgb)
{
  return (rgb >> 16) & 0xFF;
}

static guchar getGreen (int rgb)
{
  return (rgb >> 8) & 0xFF;
}

static guchar getBlue (int rgb)
{
  return (rgb) & 0xFF;
}

#else

/* GIMP */
static guchar getRed (guint rgb)
{
  return (rgb) & 0xFF;
}

static guchar getGreen (guint rgb)
{
  return (rgb >> 8) & 0xFF;
}

static guchar getBlue (guint rgb)
{
  return (rgb >> 16) & 0xFF;
}

#endif

#if 0
static guchar getAlpha (guint rgb)
{
  return (rgb >> 24) & 0xFF;
}
#endif


/* Gets an int containing rgb, and an lab struct */
static lab *
calcLAB (guint  rgb,
         lab   *newpixel)
{
  float var_R = (getRed (rgb)   / 255.0);
  float var_G = (getGreen (rgb) / 255.0);
  float var_B = (getBlue (rgb)  / 255.0);

  float X, Y, Z, var_X, var_Y, var_Z;

  if (var_R > 0.04045)
    var_R = (float) pow ((var_R + 0.055) / 1.055, 2.4);
  else
    var_R = var_R / 12.92;

  if (var_G > 0.04045)
    var_G = (float) pow ((var_G + 0.055) / 1.055, 2.4);
  else
    var_G = var_G / 12.92;

  if (var_B > 0.04045)
    var_B = (float) pow ((var_B + 0.055) / 1.055, 2.4);
  else
    var_B = var_B / 12.92;

  var_R = var_R * 100.0;
  var_G = var_G * 100.0;
  var_B = var_B * 100.0;

  /* Observer. = 2°, Illuminant = D65 */
  X = (float) (var_R * 0.4124 + var_G * 0.3576 + var_B * 0.1805);
  Y = (float) (var_R * 0.2126 + var_G * 0.7152 + var_B * 0.0722);
  Z = (float) (var_R * 0.0193 + var_G * 0.1192 + var_B * 0.9505);

  var_X = X / 95.047;   /* Observer = 2, Illuminant = D65 */
  var_Y = Y / 100.0;
  var_Z = Z / 108.883;

  if (var_X > 0.008856)
    var_X = (float) pow (var_X, (1.0 / 3));
  else
    var_X = (7.787 * var_X) + (16.0 / 116);

  if (var_Y > 0.008856)
    var_Y = (float) pow (var_Y, (1.0 / 3));
  else
    var_Y = (7.787 * var_Y) + (16.0 / 116);

  if (var_Z > 0.008856)
    var_Z = (float) pow (var_Z, (1.0 / 3));
  else
    var_Z = (7.787 * var_Z) + (16.0 / 116);

  newpixel->l = (116 * var_Y) - 16;
  newpixel->a = 500 * (var_X - var_Y);
  newpixel->b = 200 * (var_Y - var_Z);

  return newpixel;
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

/* Region growing */
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
    { /* Kill everything that is not biggest blob! */
      if (labelfield[i] != 0 && labelfield[i] != maxblob)
        {
          cm[i] = 0.0;
        }
    }

  g_queue_free (q);
  g_free (labelfield);
}


/* Returns squared clustersize */
static float
getclustersize (float limits[DIMS])
{
  float sum = (limits[0] - (-limits[0])) * (limits[0] - (-limits[0]));

  sum += (limits[1] - (-limits[1])) * (limits[1] - (-limits[1]));
  sum += (limits[2] - (-limits[2])) * (limits[2] - (-limits[2]));

  return sum;
}

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
float *
segmentate (guint *rgbs,
            float *confidencematrix,
            int    xres,
            int    yres,
            float  limits[DIMS],
            int    smoothness)
{
  float clustersize = getclustersize (limits);
  int length = xres * yres;
  int surebgcount = 0, surefgcount = 0;
  int i, k, j;
  int bgsiglen, fgsiglen;
  lab *surebg, *surefg, *bgsig, *fgsig = NULL;
  char background = 0;
  float min, d;
  lab labpixel;

  /* count given foreground and background pixels */
  for (i = 0; i < length; i++)
    {
      if (confidencematrix[i] <= 0.10f)
        {
          surebgcount++;
        }
      else if (confidencematrix[i] >= 0.90f)
        {
          surefgcount++;
        }
    }

  surebg = g_new (lab, surebgcount);
  surefg = g_new (lab, surefgcount);

  k = 0;
  j = 0;

  /* create inputs for colorsignatures */
  for (i = 0; i < length; i++)
    {
      if (confidencematrix[i] <= 0.10f)
        {
          calcLAB (rgbs[i], &surebg[k]);
          k++;
        }
      else if (confidencematrix[i] >= 0.90f)
        {
          calcLAB (rgbs[i], &surefg[j]);
          j++;
        }
    }

  /* Create color signature for bg */
  bgsig = create_signature (surebg, surebgcount, limits, &bgsiglen);

  if (bgsiglen < 1)
    return confidencematrix;    /* No segmentation possible */

  /* Create color signature for fg */
  fgsig = create_signature (surefg, surefgcount, limits, &fgsiglen);

  /* Classify - the slow way....Better: Tree traversation */
  for (i = 0; i < length; i++)
    {
      if (confidencematrix[i] >= 0.90)
        {
          confidencematrix[i] = 1.0f;
          continue;
        }

      if (confidencematrix[i] <= 0.10)
        {
          confidencematrix[i] = 0.0f;
          continue;
        }

      calcLAB (rgbs[i], &labpixel);
      background = 1;
      min = euklid (labpixel, bgsig[0]);

      for (j = 1; j < bgsiglen; j++)
        {
          d = euklid(labpixel, bgsig[j]);
          if (d < min)
            {
              min = d;
            }
        }

      if (fgsiglen == 0)
        {
          if (min < clustersize)
            background = 1;
          else
            background = 0;
        }
      else
        {
          for (j = 0; j < fgsiglen; j++)
            {
              d = euklid (labpixel, fgsig[j]);
              if (d < min)
                {
                  min = d;
                  background = 0;
                  break;
                }
            }
        }

      if (background == 0)
        {
          confidencematrix[i] = 1.0f;
        }
      else
        {
          confidencematrix[i] = 0.0f;
        }
    }

  /* Smooth a bit for error killing */
  smoothcm (confidencematrix, xres, yres, 0.33, 0.33, 0.33);

  normalize_matrix (confidencematrix, length);

  /* Now erode, to make sure only "strongly connected components"
   * keep being connected
   */
  erode2 (confidencematrix, xres, yres);

  /* search the biggest connected component */
  findmaxblob (confidencematrix, rgbs, xres, yres);

  for (i = 0; i < smoothness; i++)
    {
      /* smooth again - as user specified */
      smoothcm (confidencematrix, xres, yres, 0.33, 0.33, 0.33);
    }

  normalize_matrix (confidencematrix, length);

  /* Threshold the values */
  for (i = 0; i < length; i++)
    {
      if (confidencematrix[i] >= 0.5)
        confidencematrix[i] = 1.0;
      else
        confidencematrix[i] = 0.0;
    }

  /* search the biggest connected component again
     to make sure jitter is killed
   */
  findmaxblob (confidencematrix, rgbs, xres, yres);

  /* Now dilate, to fill up boundary pixels killed by erode */
  dilate2 (confidencematrix, xres, yres);

  g_free (surefg);
  g_free (surebg);
  g_free (bgsig);
  g_free (fgsig);

  return confidencematrix;
}


/* If JNI_COMPILE is defined, we provide a Java binding for the segmentate
 * funtion.  This allows me to use an existing benchmark as a unit test.
 * The plan is to implement this test as a GIMP plug-in later. Until then,
 * please leave this code in.
 */

#ifdef JNI_COMPILE
JNIEXPORT void JNICALL Java_NativeExperimentalPipe_segmentate
    (JNIEnv * env, jobject obj, jintArray rgbs, jfloatArray cm, jint xres,
     jint yres, jfloatArray limits) {
	jint *jrgbs = (*env)->GetIntArrayElements(env, rgbs, 0);
	jfloat *jcm = (*env)->GetFloatArrayElements(env, cm, 0);
	jfloat *jlimits = (*env)->GetFloatArrayElements(env, limits, 0);
	segmentate(jrgbs, jcm, xres, yres, jlimits, 6);
	(*env)->ReleaseFloatArrayElements(env, cm, jcm, 0);
}
#endif
