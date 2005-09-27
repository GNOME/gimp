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
 * and Kristian Jantz <jantz@inf.fu-berlin.de>
 * and Tobias Lenz <tlenz@inf.fu-berlin.de>.
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
 *   pixels < SIOX_LOW  are known background
 *   pixels > SIOX_HIGH are known foreground
 */
#define SIOX_LOW  1
#define SIOX_HIGH 254


/*  FIXME: turn this into an enum  */
#define SIOX_DRB_ADD       0
#define SIOX_DRB_SUBTRACT  1


/* #define SIOX_DEBUG */


/* A struct that holds the classification result */

typedef struct
{
  gfloat bgdist;
  gfloat fgdist;
} classresult;


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

static gint
list_size (ArrayList *list)
{
  ArrayList *cur   = list;
  gint       count = 0;

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


/*  assumes that lab starts with an array of floats (l,a,b)  */
#define CURRENT_VALUE(points, i, dim) (((const gfloat *) (points + i))[dim])

/* Stage one of modified KD-Tree algorithm */
static void
stageone (lab          *points,
          gint          dims,
          gint          depth,
          ArrayList    *clusters,
          const gfloat *limits,
          gint          length)
{
  gint    curdim = depth % dims;
  gfloat  min, max;
  gfloat  curval;
  gint    i;

  if (length < 1)
    return;

  curval = CURRENT_VALUE (points, 0, curdim);

  min = curval;
  max = curval;

  for (i = 1; i < length; i++)
    {
      curval = CURRENT_VALUE (points, i, curdim);

      if (min > curval)
        min = curval;
      else if (max < curval)
        max = curval;
    }

  /* Split according to Rubner-Rule */
  if (max - min > limits[curdim])
    {
      lab    *smallerpoints;
      lab    *biggerpoints;
      gint    countsm = 0;
      gint    smallc  = 0;
      gint    bigc    = 0;
      gfloat  pivot   = (min + max) / 2.0;

      /* find out cluster sizes */
      for (i = 0; i < length; i++)
        {
          curval = CURRENT_VALUE (points, i, curdim);

          if (curval <= pivot)
            countsm++;
        }

      /* FIXME: consider to sort the array and split in place instead
       *        of allocating memory here
       */
      smallerpoints = g_new (lab, countsm);
      biggerpoints  = g_new (lab, length - countsm);

      for (i = 0; i < length; i++)
        {
          /* do actual split */
          curval = CURRENT_VALUE (points, i, curdim);

          if (curval <= pivot)
            smallerpoints[smallc++] = points[i];
          else
            biggerpoints[bigc++] = points[i];
        }

      if (depth > 0)
        g_free (points);

      /* create subtrees */
      stageone (smallerpoints, dims, depth + 1, clusters, limits, countsm);
      stageone (biggerpoints, dims, depth + 1, clusters, limits, length - countsm);
    }
  else
    {
      /* create leave */
      add_to_list (clusters, points, length, depth != 0);
    }
}


/* Stage two of modified KD-Tree algorithm */

/* This is very similar to stageone... but in future there may be more
 * differences => not integrated into method stageone()
 */
static void
stagetwo (lab          *points,
          gint          dims,
          gint          depth,
          ArrayList    *clusters,
          const gfloat *limits,
          gint          length,
          gint          total,
          gfloat        threshold)
{
  gint    curdim = depth % dims;
  gfloat  min, max;
  gfloat  curval;
  gint    i;

  if (length < 1)
    return;

  curval = CURRENT_VALUE (points, 0, curdim);

  min = curval;
  max = curval;

  for (i = 1; i < length; i++)
    {
      curval = CURRENT_VALUE (points, i, curdim);

      if (min > curval)
        min = curval;
      else if (max < curval)
        max = curval;
    }

  /* Split according to Rubner-Rule */
  if (max - min > limits[curdim])
    {
      lab    *smallerpoints;
      lab    *biggerpoints;
      gint    countsm = 0;
      gint    smallc  = 0;
      gint    bigc    = 0;
      gfloat  pivot   = (min + max) / 2.0;

#ifdef SIOX_DEBUG
      g_printerr ("siox.c: max=%f min=%f pivot=%f\n", max, min, pivot);
#endif

      for (i = 0; i < length; i++)
        {
          /* find out cluster sizes */
          curval = CURRENT_VALUE (points, i, curdim);

          if (curval <= pivot)
            countsm++;
        }

      /* FIXME: consider to sort the array and split in place instead
       *        of allocating memory here
       */
      smallerpoints = g_new (lab, countsm);
      biggerpoints  = g_new (lab, length - countsm);

      /* do actual split */
      for (i = 0; i < length; i++)
        {
          curval = CURRENT_VALUE (points, i, curdim);

          if (curval <= pivot)
            smallerpoints[smallc++] = points[i];
          else
            biggerpoints[bigc++] = points[i];
        }

      g_free (points);

      /* create subtrees */
      stagetwo (smallerpoints, dims, depth + 1, clusters, limits,
                countsm, total, threshold);
      stagetwo (biggerpoints, dims, depth + 1, clusters, limits,
                length - countsm, total, threshold);
    }
  else /* create leave */
    {
      gint sum = 0;

      for (i = 0; i < length; i++)
        sum += points[i].cardinality;

      if (((sum * 100.0) / total) >= threshold)
        {
          lab *point = g_new0 (lab, 1);

          for (i = 0; i < length; i++)
            {
              point->l += points[i].l;
              point->a += points[i].a;
              point->b += points[i].b;
            }

          point->l /= length;
          point->a /= length;
          point->b /= length;

#ifdef SIOX_DEBUG
          g_printerr ("siox.c: cluster=%f, %f, %f sum=%d\n",
                     point->l, point->a, point->b, sum);
#endif

          add_to_list (clusters, point, 1, TRUE);
        }

      g_free (points);
    }
}

/* squared euclidean distance */
static inline float
euklid (const lab *p,
        const lab *q)
{
  return (SQR (p->l - q->l) + SQR (p->a - q->a) + SQR (p->b - q->b));
}

/* Returns squared clustersize */
static gfloat
get_clustersize (const gfloat *limits)
{
  return (SQR (limits[0] - (-limits[0])) +
          SQR (limits[1] - (-limits[1])) +
          SQR (limits[2] - (-limits[2])));
}

/* Creates a color signature for a given set of pixels */
static lab *
create_signature (lab          *input,
                  gint          length,
                  const gfloat *limits,
                  gint         *returnlength)
{
  ArrayList *clusters;
  ArrayList *curelem;
  lab       *centroids;
  lab       *rval;
  gint       size;
  gint       i;

  if (length < 1)
    {
      *returnlength = 0;
      return NULL;
    }

  clusters = list_new ();

  stageone (input, SIOX_DIMS, 0, clusters, limits, length);
  size = list_size (clusters);
  centroids = g_new (lab, size);
  curelem = clusters;

  i = 0;
  while (curelem->array)
    {
      lab    *cluster = curelem->array;
      gfloat  l       = 0;
      gfloat  a       = 0;
      gfloat  b       = 0;
      gint    k;

      for (k = 0; k < curelem->arraylength; k++)
        {
          l += cluster[k].l;
          a += cluster[k].a;
          b += cluster[k].b;
        }

      centroids[i].l = l / curelem->arraylength;
      centroids[i].a = a / curelem->arraylength;
      centroids[i].b = b / curelem->arraylength;

      centroids[i].cardinality = curelem->arraylength;

      i++;
      curelem = curelem->next;
    }

#ifdef SIOX_DEBUG
  g_printerr ("siox.c: step #1 -> %d clusters\n", size);
#endif

  free_list (clusters);

  clusters = list_new ();

  stagetwo (centroids,
            SIOX_DIMS, 0, clusters, limits, size, length,
            0.1f /* magic constant, see paper by tomasi */);

  rval = list_to_array (clusters, returnlength);

  free_list (clusters);

#ifdef SIOX_DEBUG
  g_printerr ("siox.c: step #2 -> %d clusters\n", returnlength[0]);
#endif

  return rval;
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


/* Do not change these defines! They contain some magic!
 * Must all be non-zero and FINAL must be 0xFF!
 */
#define FIND_BLOB_SELECTED  0x1
#define FIND_BLOB_FORCEFG   0x3
#define FIND_BLOB_VISITED   0x7
#define FIND_BLOB_FINAL     0xFF

static inline void
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
       pr != NULL; pr = pixel_regions_process (pr))
    {
      guchar *data = region.data;

      for (row = 0; row < region.h; row++)
        {
          guchar *d = data;

          /* everything that fits the mask is in the image */
          for (col = 0; col < region.w; col++, d++)
            {
              if (*d > SIOX_HIGH)
                *d = FIND_BLOB_FORCEFG;
              else if (*d >= 0x80)
                *d = FIND_BLOB_SELECTED;
              else
                *d = 0;
            }

          data += region.rowstride;
        }
    }
}

struct blob
{
  gint seedx, seedy;
  gint size;
  gboolean mustkeep;
};

/* This method checks out the neighbourhood of the pixel at position
 * (pos_x,pos_y) in the TileManager mask, it adds the sourrounding
 * pixels to the queue to allow further processing it uses maskVal to
 * determine if the sourounding pixels have already been visited x,y
 * are passed from above.
 */
static void
depth_first_search (TileManager *mask,
                    gint 	 x,
                    gint 	 y,
                    gint 	 xwidth,
		    gint 	 yheight,
		    struct blob *b,
		    guchar 	 mark)
{
  gint   xx, yy;
  guchar val;

  GSList *stack =
    g_slist_prepend (g_slist_prepend (NULL, GINT_TO_POINTER (b->seedy)),
                     GINT_TO_POINTER (b->seedx));

  while (stack != NULL)
    {
      xx    = GPOINTER_TO_INT (stack->data);
      stack = g_slist_delete_link (stack, stack);
      yy    = GPOINTER_TO_INT (stack->data);
      stack = g_slist_delete_link (stack, stack);

      read_pixel_data_1 (mask, xx, yy, &val);
      if (val && (val != mark))
        {
          if (mark == FIND_BLOB_VISITED)
            {
              ++(b->size);
              if (val == FIND_BLOB_FORCEFG)
                b->mustkeep = TRUE;
            }

          write_pixel_data_1 (mask, xx, yy, &mark);

          if (xx > x)
            stack =
              g_slist_prepend (g_slist_prepend (stack, GINT_TO_POINTER (yy)),
                               GINT_TO_POINTER (xx - 1));
          if (xx + 1 < xwidth)
            stack =
              g_slist_prepend (g_slist_prepend (stack, GINT_TO_POINTER (yy)),
                               GINT_TO_POINTER (xx + 1));
          if (yy > y)
            stack =
              g_slist_prepend (g_slist_prepend
                               (stack, GINT_TO_POINTER (yy - 1)),
                               GINT_TO_POINTER (xx));
          if (yy + 1 < yheight)
            stack =
              g_slist_prepend (g_slist_prepend
                               (stack, GINT_TO_POINTER (yy + 1)),
                               GINT_TO_POINTER (xx));
        }
    }
}

/*
 * This method finds the biggest connected components in mask, it
 * clears everything in mask except the biggest components' Pixels that
 * should be considererd set in incoming mask, must fulfill (pixel &
 * 0x1) the method uses no further memory, except a queue, it finds
 * the biggest components by a 2 phase algorithm 1. in the first phase
 * the coordinates of an element of the biggest components are
 * identified, during this phase all pixels are visited. In the
 * second phase first visitation flags are reset, and afterwards
 * connected components starting at the found coordinates are
 * determined. These are the biggest components, the result is written
 * into mask, all pixels that belong to the biggest components are set
 * to 255, any other to 0.
 */

static void
find_max_blob (TileManager *mask,
               gint         x,
               gint         y,
               gint         width,
               gint         height)
{
  GSList     *list = NULL;
  PixelRegion region;
  gpointer    pr;
  gint        row, col;
  gint        maxsize = 0;
  guchar      val;

  threshold_mask (mask, x, y, width, height);

  pixel_region_init (&region, mask, x, y, width, height, TRUE);

  for (pr = pixel_regions_register (1, &region);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      gint    pos_y = region.y;
      guchar *data  = region.data;

      for (row = 0; row < region.h; row++, pos_y++)
        {
          guchar *d = data;

          for (col = 0; col < region.w; col++, d++)
            {
              val = *d;

              if (val && (val != FIND_BLOB_VISITED))
                {
                  struct blob *b = g_new (struct blob, 1);

                  b->seedx    = region.x + col;
                  b->seedy    = pos_y;
                  b->size     = 0;
                  b->mustkeep = FALSE;

                  depth_first_search (mask,
                                      x, y, x + width, y + height,
                                      b, FIND_BLOB_VISITED);

                  list = g_slist_prepend (list, b);

                  if (b->size > maxsize)
                    maxsize = b->size;
                }
            }

          data += region.rowstride;
        }
    }

  while (list != NULL)
    {
      struct blob *b = list->data;

      list = g_slist_delete_link (list, list);

      depth_first_search (mask, x, y, x + width, y + height, b,
                          (b->mustkeep
                           || (b->size * 4 > maxsize)) ? FIND_BLOB_FINAL : 0);
      g_free (b);
    }
}

/* Creates a key for the hashtable from a given pixel color value */
static inline gint
create_key (const guchar *src,
            gint 	  bpp,
            const guchar *colormap)
{
  switch (bpp)
    {
    case 3:                     /* RGB  */
    case 4:                     /* RGBA */
      return (src[RED_PIX] << 16 | src[GREEN_PIX] << 8 | src[BLUE_PIX]);
    case 2:
    case 1:
      if (colormap)             /* INDEXED(A) */
        {
          gint i = *src * 3;
          return (colormap[i + RED_PIX]   << 16 |
                  colormap[i + GREEN_PIX] << 8  |
                  colormap[i + BLUE_PIX]);
        }
      else                      /* GRAY(A) */
        {
          return *src;
        }
    default:
      return 0;
    }
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
 * @pixels:      the tiles to extract the foreground from
 * @colormap:    colormap in case @pixels are indexed, %NULL otherwise
 * @offset_x:    horizontal offset of @pixels with respect to the @mask
 * @offset_y:    vertical offset of @pixels with respect to the @mask
 * @mask:        a mask indicating sure foreground (255), sure background (0)
 *               and undecided regions ([1..254]).
 * @x:           horizontal offset into the mask
 * @y:           vertical offset into the mask
 * @width:       width of working area on mask
 * @height:      height of working area on mask
 * @sensitivity: a double array with three entries specifing the accuracy,
 *               a good value is: { 0.64, 1.28, 2.56 }
 * @smoothness:  boundary smoothness (a good value is 3)
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
                         const gdouble     sensitivity[SIOX_DIMS],
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
  gfloat       limits[3];
  GHashTable  *pixtoclassresult;

  g_return_if_fail (pixels != NULL);
  g_return_if_fail (mask != NULL && tile_manager_bpp (mask) == 1);
  g_return_if_fail (x >= 0);
  g_return_if_fail (y >= 0);
  g_return_if_fail (x + width <= tile_manager_width (mask));
  g_return_if_fail (y + height <= tile_manager_height (mask));
  g_return_if_fail (smoothness >= 0);
  g_return_if_fail (progress_data == NULL || progress_callback != NULL);

  cpercep_init ();

  pixtoclassresult = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                            NULL, g_free);

  siox_progress_update (progress_callback, progress_data, 0.0);


  limits[0] = sensitivity[0];
  limits[1] = sensitivity[1];
  limits[2] = sensitivity[2];

  clustersize = get_clustersize (limits);

  /* count given foreground and background pixels */
  pixel_region_init (&mapPR, mask, x, y, width, height, FALSE);

  for (pr = pixel_regions_register (1, &mapPR);
       pr != NULL; pr = pixel_regions_process (pr))
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
       pr != NULL; pr = pixel_regions_process (pr))
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
  bgsig = create_signature (surebg, surebgcount, limits, &bgsiglen);
  g_free (surebg);

  if (bgsiglen < 1)
    {
      g_free (surefg);
      return;
    }

  siox_progress_update (progress_callback, progress_data, 0.3);

  /* Create color signature for the foreground */
  fgsig = create_signature (surefg, surefgcount, limits, &fgsiglen);
  g_free (surefg);

  siox_progress_update (progress_callback, progress_data, 0.4);

  /* Classify - the cached way....Better: Tree traversation? */

#ifdef SIOX_DEBUG
  gint hits = 0;
  gint miss = 0;
#endif

  pixel_region_init (&srcPR, pixels,
                     x - offset_x, y - offset_y, width, height, FALSE);
  pixel_region_init (&mapPR, mask, x, y, width, height, TRUE);

  for (pr = pixel_regions_register (2, &srcPR, &mapPR);
       pr != NULL; pr = pixel_regions_process (pr))
    {
      const guchar *src = srcPR.data;
      guchar       *map = mapPR.data;

      for (row = 0; row < srcPR.h; row++)
        {
          const guchar *s = src;
          guchar       *m = map;

          for (col = 0; col < srcPR.w; col++, m++, s += bpp)
            {
              lab          labpixel;
              gfloat       minbg, minfg, d;
              classresult *cr;
              gint         key;

              if (*m < SIOX_LOW || *m > SIOX_HIGH)
                continue;

              key = create_key (s, bpp, colormap);

              /* FIXME: Do not create HashTable in here, do it globally */
              cr = g_hash_table_lookup (pixtoclassresult,
                                        GINT_TO_POINTER (key));

              if (cr)
                {
                  *m = (cr->bgdist >= cr->fgdist) ? 254 : 0;

#ifdef SIOX_DEBUG
                  ++hits;
#endif
                  continue;
                }

#ifdef SIOX_DEBUG
              ++miss;
#endif
              cr = g_new0 (classresult, 1);
              calc_lab (s, bpp, colormap, &labpixel);

              minbg = euklid (&labpixel, bgsig + 0);

              for (i = 1; i < bgsiglen; i++)
                {
                  d = euklid (&labpixel, bgsig + i);

                  if (d < minbg)
                    minbg = d;
                }

              cr->bgdist = minbg;

              if (fgsiglen == 0)
                {
                  if (minbg < clustersize)
                    minfg = minbg + clustersize;
                  else
                    minfg = 0.00001; /* This is a guess -
                                        now we actually require a foreground
                                        signature, !=0 to avoid div by zero
                                      */
                }
              else
                {
                  minfg = euklid (&labpixel, fgsig + 0);

                  for (i = 1; i < fgsiglen; i++)
                    {
                      d = euklid (&labpixel, fgsig + i);

                      if (d < minfg)
                        {
                          minfg = d;
                        }
                    }
                }

              cr->bgdist = minbg;
              cr->fgdist = minfg;

              g_hash_table_insert (pixtoclassresult, GINT_TO_POINTER (key), cr);

              *m = minbg >= minfg ? 254 : 0;
           }

          src += srcPR.rowstride;
          map += mapPR.rowstride;
        }
    }

#ifdef SIOX_DEBUG
  g_printerr ("siox.c: Hashtable size %d, misses=%d, hits=%d, ratio=%f\n",
              g_hash_table_size (pixtoclassresult),
	      miss,
	      hits,
              ((gfloat) hits) / miss);
#endif

  g_free (fgsig);
  g_free (bgsig);

  /* FIXME: Do not free memory in here - do it globally */
  g_hash_table_destroy (pixtoclassresult);

  siox_progress_update (progress_callback, progress_data, 0.8);

  /* smooth a bit for error killing */
  smooth_mask (mask, x, y, width, height);

  /* erode, to make sure only "strongly connected components"
   * keep being connected
   */
  erode_mask (mask, x, y, width, height);

  /* search the biggest connected component */
  find_max_blob (mask, x, y, width, height);

  siox_progress_update (progress_callback, progress_data, 0.9);

  /* smooth again - as user specified */
  for (i = 0; i < smoothness; i++)
    smooth_mask (mask, x, y, width, height);

  /* search the biggest connected component again to kill jitter */
  find_max_blob (mask, x, y, width, height);

  /* dilate, to fill up boundary pixels killed by erode */
  dilate_mask (mask, x, y, width, height);

  siox_progress_update (progress_callback, progress_data, 1.0);
}


/**
 * siox_drb:
 * @pixels:       the rgb tiles to work on (read)
 * @colormap:     colormap in case @pixels are indexed, %NULL otherwise
 * @mask:         the alpha tiles to work on (write)
 * @x:            horizontal offset into the mask
 * @y:            vertical offset into the mask
 * @pixtoclassresult: the hashtable as generated by siox_foreground_extract
 * @brushmode:    at this time either SIOX_DRB_ADD or SIOX_DRB_SUBTRACT
 * @brushradius:  the radius of the brush
 * @threshold:    a threshold to be defined by the user.
 *                Range for SIOX_DRB_ADD: ]0..1] default: 1.0,
 *                range for for SIOX_DRB_SUBTRACT: [0..1[, default: 0.0
 *
 * drb - detail refinement brush, a brush mask for subpixel classification.
 *
 * FIXME: Now it is assumed that the brush is a square. Should be able
 * to be whatever GIMP offers...  TODO: This is still an experimental
 * method. There are more tests needed to evaluate performance of
 * this!
 */
void
siox_drb (TileManager  *pixels,
          const guchar *colormap,
          TileManager  *mask,
          gint          x,
          gint          y,
          GHashTable   *pixtoclassresult,
          gint          brushmode,
          gint          brushradius,
          gfloat        threshold)
{
  PixelRegion srcPR;
  PixelRegion mapPR;
  gpointer    pr;
  gint        bpp;
  gint        row, col;

  bpp = tile_manager_bpp (pixels);

  pixel_region_init (&srcPR, pixels,
                     x - brushradius, y - brushradius, brushradius * 2,
                     brushradius * 2, FALSE);
  pixel_region_init (&mapPR, mask, x - brushradius, y - brushradius,
                     brushradius * 2, brushradius * 2, TRUE);

  for (pr = pixel_regions_register (2, &srcPR, &mapPR);
       pr != NULL; pr = pixel_regions_process (pr))
    {
      const guchar *src = srcPR.data;
      guchar       *map = mapPR.data;

      for (row = 0; row < srcPR.h; row++)
        {
          const guchar *s = src;
          guchar       *m = map;

          for (col = 0; col < srcPR.w; col++, m++, s += bpp)
            {
              gint         key;
              classresult *cr;

              key = create_key (s, bpp, colormap);
              cr  = g_hash_table_lookup (pixtoclassresult,
                                         GINT_TO_POINTER (key));

              if (! cr)
                continue; /* Unknown color -
                             can only be sure background or sure forground */

              gfloat mindistbg = (gfloat) sqrt (cr->bgdist);
              gfloat mindistfg = (gfloat) sqrt (cr->fgdist);
              gfloat alpha;

              if (brushmode == SIOX_DRB_ADD)
                {
                  if (*m > SIOX_HIGH)
                    continue;

                  if (mindistfg == 0.0)
                    alpha = 1.0; /* avoid div by zero */
                  else
                    alpha = MIN (mindistbg / mindistfg, 1.0);
                }
              else /*if (brushmode == SIOX_DRB_SUBTRACT)*/
                {
                  if (*m < SIOX_HIGH)
                    continue;

                  if (mindistbg == 0.0)
                    alpha = 0.0; /* avoid div by zero */
                  else
                    alpha = 1.0 - MIN (mindistfg / mindistbg, 1.0);

                }

              if (alpha < threshold)
                {
                  /* background with a certain confidence
                   * to be decided by user.
                   */
                  alpha = 0.0;
                }

              *m = (gint) 255 *alpha;
            }

          src += srcPR.rowstride;
          map += mapPR.rowstride;

        }
    }
}
