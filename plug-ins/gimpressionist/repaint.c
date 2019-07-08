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
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "gimpressionist.h"
#include "brush.h"
#include "orientation.h"
#include "placement.h"
#include "ppmtool.h"
#include "preview.h"
#include "random.h"
#include "size.h"
#include "infile.h"

#include <libgimp/stdplugins-intl.h>

static gimpressionist_vals_t runningvals;

static double
get_siz_from_pcvals (double x, double y)
{
  return getsiz_proto (x, y, pcvals.num_size_vectors, pcvals.size_vectors,
                       pcvals.size_strength_exponent, pcvals.size_voronoi);
}

static int
get_pixel_value (double dir)
{
  while (dir < 0.0)
    dir += 360.0;
  while (dir >= 360.0)
    dir -= 360.0;
  return dir * 255.0 / 360.0;
}

static void
prepare_brush (ppm_t *p)
{
  int x, y;
  int rowstride = p->width * 3;

  for (y = 0; y< p->height; y++)
    {
      for (x = 0; x < p->width; x++)
        {
          p->col[y * rowstride + x * 3 + 1] = 0;
        }
    }

  for (y = 1; y< p->height; y++)
    {
      for (x = 1; x < p->width; x++)
        {
          int v = p->col[y * rowstride + x * 3] -
                  p->col[(y - 1) * rowstride + (x - 1) * 3];
          if (v < 0)
            v = 0;
          p->col[y * rowstride + x * 3 + 1] = v;
        }
    }
}

static double
sum_brush (ppm_t *p)
{
  double sum = 0;
  int i;

  for (i = 0; i < p->width*3*p->height; i += 3)
    sum += p->col[i];
  return sum;
}

/* TODO : Use r = rgb[0]; g = rgb[1] ; b = rgb[2]; instead of
 * the direct references here.
 * */
static int
get_hue (guchar *rgb)
{
  double h, v, temp, diff;
  /* TODO : There seems to be some typoes in the comments here.
   * Ask vidar what he meant.
   * */
  if ((rgb[0] == rgb[1]) && (rgb[0] == rgb[2])) /* Gray */
    return 0;
  v = (rgb[0] > rgb[1] ? rgb[0] : rgb[1]);     /* v = st<F8>rste verdi */
  if (rgb[2] > v)
    v = rgb[2];
  temp = (rgb[0] > rgb[1] ? rgb[1] : rgb[0] ); /* temp = minste */
  if (rgb[2] < temp)
    temp = rgb[2];
  diff = v - temp;

  if (v == rgb[0])
    h = ((double)rgb[1] - rgb[2]) / diff;
  else if(v == rgb[1])
    h = ((double)rgb[2] - rgb[0]) / diff + 2;
  else /* v == rgb[2] */
    h = ((double)rgb[0] - rgb[1]) / diff + 4;
  if(h < 0) h += 6;
  return h * 255.0 / 6.0;
}

static int
choose_best_brush (ppm_t *p, ppm_t *a, int tx, int ty,
                   ppm_t *brushes, int num_brushes,
                   double *brushes_sum, int start, int step)
{
  double dev, thissum;
  double bestdev = 0.0;
  double r, g, b;
  int    best = -1;
  int    x, y, h;
  long   i;
  GList *brlist = NULL;

  for (i = start; i < num_brushes; i += step)
    {
      ppm_t *brush = &brushes[i];
#if 0
      thissum = 0.0;
#endif
      thissum = brushes_sum[i];

/* TODO: Pointer-arithmeticize this code */
      r = g = b = 0.0;
      for (y = 0; y < brush->height; y++)
        {
          guchar *row = p->col + (ty + y) * p->width * 3;

          for (x = 0; x < brush->width; x++)
            {
              int    k = (tx + x) * 3;
              double v;

              if ((h = brush->col[(y * brush->width * 3) + x * 3]))
                {
#if 0
                  thissum += h;
#endif
                  v = h / 255.0;
                  r += row[k+0] * v;
                  g += row[k+1] * v;
                  b += row[k+2] * v;
                }
            }
        }
      r = r * 255.0 / thissum;
      g = g * 255.0 / thissum;
      b = b * 255.0 / thissum;

      dev = 0.0;
      for (y = 0; y < brush->height; y++)
        {
          guchar *row = p->col + (ty + y) * p->width * 3;

          for (x = 0; x < brush->width; x++)
            {
              int    k = (tx + x) * 3;
              double v;

              if ((h = brush->col[(y * brush->width * 3) + x * 3]))
                {
                  v = h / 255.0;
                  dev += abs (row[k+0] - r) * v;
                  dev += abs (row[k+1] - g) * v;
                  dev += abs (row[k+2] - b) * v;
                  if (img_has_alpha)
                    dev += a->col[(ty + y) * a->width * 3 + (tx + x) * 3] * v;
                }
            }
        }
      dev /= thissum;

      if ((best == -1) || (dev < bestdev))
        {
          if (brlist)
            g_list_free (brlist);
          brlist = NULL;
        }

      if (dev <= bestdev || best < 0)
        {
          best = i;
          bestdev = dev;
          brlist = g_list_append (brlist, (void *)i);
        }
      if (dev < runningvals.devthresh)
        break;
    }

  if (!brlist)
    {
      g_printerr("What!? No brushes?!\n");
      return 0;
    }

  i = g_rand_int_range (random_generator, 0, g_list_length (brlist));
  best = (long)((g_list_nth (brlist,i))->data);
  g_list_free (brlist);

  return best;
}

static void
apply_brush (ppm_t *brush,
             ppm_t *shadow,
             ppm_t *p, ppm_t *a,
             int tx, int ty, int r, int g, int b)
{
  ppm_t  tmp;
  ppm_t  atmp;
  double v, h;
  int    x, y;
  double edgedarken = 1.0 - runningvals.general_dark_edge;
  double relief = runningvals.brush_relief / 100.0;
  int    shadowdepth = pcvals.general_shadow_depth;
  int    shadowblur = pcvals.general_shadow_blur;

  atmp.col = 0;
  atmp.width = 0;

  tmp = *p;
  if (img_has_alpha)
    atmp = *a;

  if (shadow)
    {
      int sx = tx + shadowdepth - shadowblur * 2;
      int sy = ty + shadowdepth - shadowblur * 2;

      for (y = 0; y < shadow->height; y++)
        {
          guchar *row, *arow = NULL;

          if ((sy + y) < 0)
            continue;
          if ((sy + y) >= tmp.height)
            break;
          row = tmp.col + (sy + y) * tmp.width * 3;

          if (img_has_alpha)
            arow = atmp.col + (sy + y) * atmp.width * 3;

          for (x = 0; x < shadow->width; x++)
            {
              int k = (sx + x) * 3;

              if ((sx + x) < 0)
                continue;
              if ((sx + x) >= tmp.width)
                break;

              h = shadow->col[y * shadow->width * 3 + x * 3 + 2];

              if (!h)
                continue;
              v = 1.0 - (h / 255.0 * runningvals.general_shadow_darkness / 100.0);

              row[k+0] *= v;
              row[k+1] *= v;
              row[k+2] *= v;
              if (img_has_alpha)
                arow[k] *= v;
            }
        }
    }

  for (y = 0; y < brush->height; y++)
    {
      guchar *row = tmp.col + (ty + y) * tmp.width * 3;
      guchar *arow = NULL;

      if (img_has_alpha)
        arow = atmp.col + (ty + y) * atmp.width * 3;

      for (x = 0; x < brush->width; x++)
        {
          int k = (tx + x) * 3;
          h = brush->col[y * brush->width * 3 + x * 3];

          if (!h) continue;

          if (runningvals.color_brushes)
            {
              v = 1.0 - brush->col[y * brush->width * 3 + x * 3 + 2] / 255.0;
              row[k+0] *= v;
              row[k+1] *= v;
              row[k+2] *= v;
              if (img_has_alpha)
                arow[(tx + x) * 3] *= v;
            }
          v = (1.0 - h / 255.0) * edgedarken;
          row[k+0] *= v;
          row[k+1] *= v;
          row[k+2] *= v;
          if(img_has_alpha) arow[k] *= v;

          v = h / 255.0;
          row[k+0] += r * v;
          row[k+1] += g * v;
          row[k+2] += b * v;
        }
    }

  if (relief > 0.001)
    {
      for (y = 1; y < brush->height; y++)
        {
          guchar *row = tmp.col + (ty + y) * tmp.width * 3;

          for (x = 1; x < brush->width; x++)
            {
              int k = (tx + x) * 3;
              h = brush->col[y * brush->width * 3 + x * 3 + 1] * relief;
              if (h < 0.001)
                continue;
              if (h > 255) h = 255;
              row[k+0] = (row[k+0] * (255-h) + 255 * h) / 255;
              row[k+1] = (row[k+1] * (255-h) + 255 * h) / 255;
              row[k+2] = (row[k+2] * (255-h) + 255 * h) / 255;
            }
        }
    }
}

void
repaint (ppm_t *p, ppm_t *a)
{
  int         x, y;
  int         tx = 0, ty = 0;
  ppm_t       tmp = {0, 0, NULL};
  ppm_t       atmp = {0, 0, NULL};
  int         r, g, b, h, i, j, on, sn;
  int         num_brushes, maxbrushwidth, maxbrushheight;
  guchar      back[3] = {0, 0, 0};
  ppm_t      *brushes, *shadows;
  ppm_t      *brush, *shadow = NULL;
  double     *brushes_sum;
  int         cx, cy, maxdist;
  double      scale, relief, startangle, anglespan, density, bgamma;
  int         max_progress;
  ppm_t       paper_ppm = {0, 0, NULL};
  ppm_t       dirmap = {0, 0, NULL};
  ppm_t       sizmap = {0, 0, NULL};
  int        *xpos = NULL, *ypos = NULL;
  int         progstep;
  static int  running = 0;

  int dropshadow = pcvals.general_drop_shadow;
  int shadowblur = pcvals.general_shadow_blur;

  if (running)
    return;
  running++;

  runningvals = pcvals;

  /* Shouldn't be necessary, but... */
  if (img_has_alpha)
    if ((p->width != a->width) || (p->height != a->height))
      {
        g_printerr ("Huh? Image size != alpha size?\n");
        return;
      }

  num_brushes = runningvals.orient_num * runningvals.size_num;
  startangle = runningvals.orient_first;
  anglespan = runningvals.orient_last;

  density = runningvals.brush_density;

  if (runningvals.place_type == PLACEMENT_TYPE_EVEN_DIST)
    density /= 3.0;

  bgamma = runningvals.brushgamma;

  brushes = g_malloc (num_brushes * sizeof (ppm_t));
  brushes_sum = g_malloc (num_brushes * sizeof (double));

  if (dropshadow)
    shadows = g_malloc (num_brushes * sizeof (ppm_t));
  else
    shadows = NULL;

  brushes[0].col = NULL;
  brush_get_selected (&brushes[0]);

  resize (&brushes[0],
          brushes[0].width,
          brushes[0].height * pow (10, runningvals.brush_aspect));
  scale = runningvals.size_last / MAX (brushes[0].width, brushes[0].height);

  if (bgamma != 1.0)
    ppm_apply_gamma (&brushes[0], 1.0 / bgamma, 1,1,1);

  resize (&brushes[0], brushes[0].width * scale, brushes[0].height * scale);
  i = 1 + sqrt (brushes[0].width  * brushes[0].width +
                brushes[0].height * brushes[0].height);
  ppm_pad (&brushes[0], i-brushes[0].width, i-brushes[0].width,
           i - brushes[0].height, i - brushes[0].height, back);

  for (i = 1; i < num_brushes; i++)
    {
      brushes[i].col = NULL;
      ppm_copy (&brushes[0], &brushes[i]);
    }

    for (i = 0; i < runningvals.size_num; i++)
      {
      double sv;
      if (runningvals.size_num > 1)
        sv = i / (runningvals.size_num - 1.0);
      else sv = 1.0;
      for (j = 0; j < runningvals.orient_num; j++)
        {
          h = j + i * runningvals.orient_num;
          free_rotate (&brushes[h],
                       startangle + j * anglespan / runningvals.orient_num);
          rescale (&brushes[h],
                   ( sv      * runningvals.size_first +
                    (1.0-sv) * runningvals.size_last    ) / runningvals.size_last);
          autocrop (&brushes[h],1);
        }
    }

  /* Brush-debugging */
#if 0
  for (i = 0; i < num_brushes; i++)
    {
      char tmp[1000];
      g_snprintf (tmp, sizeof (tmp), "/tmp/_brush%03d.ppm", i);
      ppm_save (&brushes[i], tmp);
    }
#endif

  for (i = 0; i < num_brushes; i++)
    {
      if (!runningvals.color_brushes)
        prepare_brush (&brushes[i]);
      brushes_sum[i] = sum_brush (&brushes[i]);
    }

  brush = &brushes[0];

  maxbrushwidth = maxbrushheight = 0;
  for (i = 0; i < num_brushes; i++)
    {
      if (brushes[i].width > maxbrushwidth)
        maxbrushwidth = brushes[i].width;
      if (brushes[i].height > maxbrushheight)
        maxbrushheight = brushes[i].height;
    }

  for (i = 0; i < num_brushes; i++)
    {
      int xp, yp;
      guchar blk[3] = {0, 0, 0};

      xp = maxbrushwidth - brushes[i].width;
      yp = maxbrushheight - brushes[i].height;
      if (xp || yp)
        ppm_pad (&brushes[i], xp / 2, xp - xp / 2, yp / 2, yp - yp / 2, blk);
    }

  if (dropshadow)
    {
      for (i = 0; i < num_brushes; i++)
        {
          shadows[i].col = NULL;
          ppm_copy (&brushes[i], &shadows[i]);
          ppm_apply_gamma (&shadows[i], 0, 1,1,0);
          ppm_pad (&shadows[i], shadowblur*2, shadowblur*2,
                   shadowblur*2, shadowblur*2, back);
          for (j = 0; j < shadowblur; j++)
            blur (&shadows[i], 2, 2);
#if 0
          autocrop (&shadows[i],1);
#endif
        }
#if 0
      maxbrushwidth += shadowdepth*3;
      maxbrushheight += shadowdepth*3;
#endif
    }

  /* For extra annoying debugging :-) */
#if 0
  ppm_save (brushes, "/tmp/__brush.ppm");
  if (shadows) ppm_save (shadows, "/tmp/__shadow.ppm");
  system ("xv /tmp/__brush.ppm & xv /tmp/__shadow.ppm & ");
#endif

  if (runningvals.general_paint_edges)
    {
      edgepad (p, maxbrushwidth, maxbrushwidth,
               maxbrushheight, maxbrushheight);
      if (img_has_alpha)
        edgepad (a, maxbrushwidth, maxbrushwidth,
                 maxbrushheight, maxbrushheight);
    }

  if (img_has_alpha)
    {
    /* Initially fully transparent */
      if (runningvals.general_background_type == BG_TYPE_TRANSPARENT)
        {
          guchar tmpcol[3] = {255, 255, 255};

          ppm_new (&atmp, a->width, a->height);
          fill (&atmp, tmpcol);
        }
      else
        {
          ppm_copy (a, &atmp);
        }
    }

  if (runningvals.general_background_type == BG_TYPE_SOLID)
    {
      guchar tmpcol[3];

      ppm_new (&tmp, p->width, p->height);
      gimp_rgb_get_uchar (&runningvals.color,
                          &tmpcol[0], &tmpcol[1], &tmpcol[2]);
      fill (&tmp, tmpcol);
    }
  else if (runningvals.general_background_type == BG_TYPE_KEEP_ORIGINAL)
    {
      ppm_copy (p, &tmp);
    }
  else
    {
      int dx, dy;

      ppm_new (&tmp, p->width, p->height);
      ppm_load (runningvals.selected_paper, &paper_ppm);

      if (runningvals.paper_scale != 100.0)
        {
          scale = runningvals.paper_scale / 100.0;
          resize (&paper_ppm, paper_ppm.width * scale, paper_ppm.height * scale);
        }

      if (runningvals.paper_invert)
        ppm_apply_gamma (&paper_ppm, -1.0, 1, 1, 1);

      dx = runningvals.general_paint_edges ? paper_ppm.width - maxbrushwidth : 0;
      dy = runningvals.general_paint_edges ? paper_ppm.height - maxbrushheight : 0;

      for (y = 0; y < tmp.height; y++)
        {
          int lx;
          int ry = (y + dy) % paper_ppm.height;

          for (x = 0; x < tmp.width; x+=lx)
            {
              int rx = (x + dx) % paper_ppm.width;

              lx = MIN (tmp.width - x, paper_ppm.width - rx);

              memcpy (&tmp.col[y * tmp.width * 3 + x * 3],
                      &paper_ppm.col[ry * paper_ppm.width * 3 + rx * 3],
                      3 * lx);
            }
        }
    }

  cx = p->width / 2;
  cy = p->height / 2;
  maxdist = sqrt (cx * cx + cy * cy);

  switch (runningvals.orient_type)
    {
    case ORIENTATION_VALUE:
      ppm_new (&dirmap, p->width, p->height);
      for (y = 0; y < dirmap.height; y++)
        {
          guchar *dstrow = &dirmap.col[y * dirmap.width * 3];
          guchar *srcrow = &p->col[y * p->width * 3];
          for (x = 0; x < dirmap.width; x++)
            {
              dstrow[x * 3] =
                (srcrow[x * 3] + srcrow[x * 3 + 1] + srcrow[x * 3 + 2]) / 3;
            }
        }
      break;

    case ORIENTATION_RADIUS:
      ppm_new (&dirmap, p->width, p->height);
      for (y = 0; y < dirmap.height; y++)
        {
          guchar *dstrow = &dirmap.col[y * dirmap.width * 3];
          double ysqr = (cy - y) * (cy - y);

          for (x = 0; x < dirmap.width; x++)
            {
              dstrow[x*3] = sqrt ((cx - x) * (cx - x) + ysqr) * 255 / maxdist;
            }
        }
      break;

    case ORIENTATION_RADIAL:
      ppm_new (&dirmap, p->width, p->height);
      for (y = 0; y < dirmap.height; y++)
        {
          guchar *dstrow = &dirmap.col[y * dirmap.width * 3];

          for (x = 0; x < dirmap.width; x++)
            {
              dstrow[x * 3] = (G_PI + atan2 (cy - y, cx - x)) *
                              255.0 / (G_PI * 2);
            }
        }
      break;

    case ORIENTATION_FLOWING:
      ppm_new (&dirmap, p->width / 6 + 5, p->height / 6 + 5);
      mkgrayplasma (&dirmap, 15);
      blur (&dirmap, 2, 2);
      blur (&dirmap, 2, 2);
      resize (&dirmap, p->width, p->height);
      blur (&dirmap, 2, 2);
      if (runningvals.general_paint_edges)
        edgepad (&dirmap, maxbrushwidth, maxbrushheight,
                 maxbrushwidth, maxbrushheight);
      break;

    case ORIENTATION_HUE:
      ppm_new (&dirmap, p->width, p->height);
      for (y = 0; y < dirmap.height; y++)
        {
          guchar *dstrow = &dirmap.col[y * dirmap.width * 3];
          guchar *srcrow = &p->col[y * p->width * 3];

          for (x = 0; x < dirmap.width; x++)
            {
              dstrow[x * 3] = get_hue (&srcrow[x * 3]);
            }
        }
      break;

    case ORIENTATION_ADAPTIVE:
      {
        guchar tmpcol[3] = {0, 0, 0};

        ppm_new (&dirmap, p->width, p->height);
        fill (&dirmap, tmpcol);
      }
      break;

    case ORIENTATION_MANUAL:
      ppm_new (&dirmap, p->width-maxbrushwidth*2, p->height-maxbrushheight*2);
      for (y = 0; y < dirmap.height; y++)
        {
          guchar *dstrow = &dirmap.col[y * dirmap.width * 3];
          double tmpy = y / (double)dirmap.height;
          for (x = 0; x < dirmap.width; x++)
            {
              dstrow[x * 3] = get_pixel_value(90 -
                                              get_direction(x /
                                                            (double)dirmap.width,
                                                            tmpy, 1));
            }
        }
      edgepad (&dirmap,
               maxbrushwidth, maxbrushwidth,
               maxbrushheight, maxbrushheight);
    break;
  }

  if (runningvals.size_type == SIZE_TYPE_VALUE)
    {
      ppm_new (&sizmap, p->width, p->height);
      for (y = 0; y < sizmap.height; y++)
        {
          guchar *dstrow = &sizmap.col[y * sizmap.width * 3];
          guchar *srcrow = &p->col[y * p->width * 3];

          for (x = 0; x < sizmap.width; x++)
            {
              dstrow[x * 3] =
                (srcrow[x * 3] + srcrow[x * 3 + 1] + srcrow[x * 3 + 2]) / 3;
            }
        }
    }
  else if (runningvals.size_type == SIZE_TYPE_RADIUS)
    {
      ppm_new (&sizmap, p->width, p->height);
      for (y = 0; y < sizmap.height; y++)
        {
          guchar *dstrow = &sizmap.col[y * sizmap.width * 3];
          double ysqr = (cy - y) * (cy - y);

          for (x = 0; x < sizmap.width; x++)
            {
              dstrow[x * 3] =
                sqrt ((cx - x) * (cx - x) + ysqr) * 255 / maxdist;
            }
        }
    }
  else if (runningvals.size_type == SIZE_TYPE_RADIAL)
    {
      ppm_new (&sizmap, p->width, p->height);
      for (y = 0; y < sizmap.height; y++)
        {
          guchar *dstrow = &sizmap.col[y * sizmap.width * 3];

          for (x = 0; x < sizmap.width; x++)
            {
              dstrow[x * 3] = (G_PI + atan2 (cy - y, cx - x)) *
                              255.0 / (G_PI * 2);
            }
        }
    }
  else if (runningvals.size_type == SIZE_TYPE_FLOWING)
    {
      ppm_new (&sizmap, p->width / 6 + 5, p->height / 6 + 5);
      mkgrayplasma (&sizmap, 15);
      blur (&sizmap, 2, 2);
      blur (&sizmap, 2, 2);
      resize (&sizmap, p->width, p->height);
      blur (&sizmap, 2, 2);
      if (runningvals.general_paint_edges)
        edgepad (&sizmap,
                 maxbrushwidth, maxbrushheight,
                 maxbrushwidth, maxbrushheight);
    }
  else if (runningvals.size_type == SIZE_TYPE_HUE)
    {
      ppm_new (&sizmap, p->width, p->height);
      for (y = 0; y < sizmap.height; y++)
        {
          guchar *dstrow = &sizmap.col[y * sizmap.width * 3];
          guchar *srcrow = &p->col[y * p->width * 3];

          for (x = 0; x < sizmap.width; x++)
            {
              dstrow[ x * 3] = get_hue (&srcrow[x * 3]);
            }
        }
    }
  else if (runningvals.size_type == SIZE_TYPE_ADAPTIVE)
    {
      guchar tmpcol[3] = {0, 0, 0};

      ppm_new (&sizmap, p->width, p->height);
      fill (&sizmap, tmpcol);
    }
  else if (runningvals.size_type == SIZE_TYPE_MANUAL)
    {
      ppm_new (&sizmap,
               p->width-maxbrushwidth * 2,
               p->height-maxbrushheight * 2);

      for (y = 0; y < sizmap.height; y++)
        {
          guchar *dstrow = &sizmap.col[y * sizmap.width * 3];
          double tmpy = y / (double)sizmap.height;

          for (x = 0; x < sizmap.width; x++)
            {
              dstrow[x * 3] = 255 * (1.0 - get_siz_from_pcvals (x / (double)sizmap.width, tmpy));
            }
        }
      edgepad (&sizmap,
               maxbrushwidth, maxbrushwidth,
               maxbrushheight, maxbrushheight);
    }
#if 0
  ppm_save(&sizmap, "/tmp/_sizmap.ppm");
#endif
  if (runningvals.place_type == PLACEMENT_TYPE_RANDOM)
    {
      i = tmp.width * tmp.height / (maxbrushwidth * maxbrushheight);
      i *= density;
    }
  else if (runningvals.place_type == PLACEMENT_TYPE_EVEN_DIST)
    {
      i = (int)(tmp.width * density / maxbrushwidth) *
          (int)(tmp.height * density / maxbrushheight);
#if 0
    g_printerr("i=%d\n", i);
#endif
    }

  if (i < 1)
    i = 1;

  max_progress = i;
  progstep = max_progress / 30;
  if (progstep < 10)
    progstep = 10;

  if (runningvals.place_type == PLACEMENT_TYPE_EVEN_DIST)
    {
      int j;

      xpos = g_new (int, i);
      ypos = g_new (int, i);
      for (j = 0; j < i; j++)
        {
          int factor = (int)(tmp.width * density / maxbrushwidth + 0.5);

          if (factor < 1)
            factor = 1;
          xpos[j] = maxbrushwidth/2 + (j % factor) * maxbrushwidth / density;
          ypos[j] = maxbrushheight/2 + (j / factor) * maxbrushheight / density;
        }
      for (j = 0; j < i; j++)
        {
          int a, b;

          a = g_rand_int_range (random_generator, 0, i);
          b = xpos[j]; xpos[j] = xpos[a]; xpos[a] = b;
          b = ypos[j]; ypos[j] = ypos[a]; ypos[a] = b;
        }
    }

  for (; i; i--)
    {
      int n;
      double thissum;

      if (i % progstep == 0)
        {
          if(runningvals.run)
            {
              gimp_progress_update (0.8 - 0.8 * ((double)i / max_progress));
            }
          else
            {
              char tmps[40];

              g_snprintf (tmps, sizeof (tmps),
                          "%.1f %%", 100 * (1.0 - ((double)i / max_progress)));
              preview_set_button_label (tmps);

              while(gtk_events_pending())
                gtk_main_iteration();
            }
        }

      if (runningvals.place_type == PLACEMENT_TYPE_RANDOM)
        {
          tx = g_rand_int_range (random_generator, maxbrushwidth / 2,
                                 tmp.width - maxbrushwidth / 2);
          ty = g_rand_int_range (random_generator, maxbrushheight / 2,
                                 tmp.height - maxbrushheight / 2);
        }
      else if (runningvals.place_type == PLACEMENT_TYPE_EVEN_DIST)
        {
          tx = xpos[i - 1];
          ty = ypos[i - 1];
        }
      if (runningvals.placement_center)
        {
          double z = g_rand_double_range (random_generator, 0, 0.75);
          tx = tx * (1.0 - z) + tmp.width / 2 * z;
          ty = ty * (1.0 - z) + tmp.height / 2 * z;
        }

      if ((tx < maxbrushwidth / 2)             ||
          (ty < maxbrushwidth / 2)             ||
          (tx + maxbrushwidth / 2 >= p->width) ||
          (ty + maxbrushheight / 2 >= p->height))
        {
#if 0
        g_printerr("Internal Error; invalid coords: (%d,%d) i=%d\n", tx, ty, i);
#endif
          continue;
        }

      if (img_has_alpha)
        {
          if (a->col[ty * a->width * 3 + tx * 3] > 128)
            continue;
        }

      sn = on = 0;

      switch (runningvals.orient_type)
        {
        case ORIENTATION_RANDOM:
          on = g_rand_int_range (random_generator, 0, runningvals.orient_num);
          break;

        case ORIENTATION_VALUE:
        case ORIENTATION_RADIUS:
        case ORIENTATION_RADIAL:
        case ORIENTATION_FLOWING:
        case ORIENTATION_HUE:
        case ORIENTATION_MANUAL:
          on = runningvals.orient_num *
                 dirmap.col[ty * dirmap.width * 3 + tx * 3] / 256;
          break;

        case ORIENTATION_ADAPTIVE:
          break; /* Handled below */

        default:
          g_printerr ("Internal error; Unknown orientationtype\n");
            on = 0;
          break;
        }

      switch (runningvals.size_type)
        {
        case SIZE_TYPE_RANDOM:
          sn = g_rand_int_range (random_generator, 0, runningvals.size_num);
          break;

        case SIZE_TYPE_VALUE:
        case SIZE_TYPE_RADIUS:
        case SIZE_TYPE_RADIAL:
        case SIZE_TYPE_FLOWING:
        case SIZE_TYPE_HUE:
        case SIZE_TYPE_MANUAL:
          sn = runningvals.size_num * sizmap.col[ty*sizmap.width*3+tx*3] / 256;
          break;

        case SIZE_TYPE_ADAPTIVE:
          break; /* Handled below */

        default:
          g_printerr ("Internal error; Unknown size_type\n");
          sn = 0;
          break;
      }

      /* Handle Adaptive selections */
      if (runningvals.orient_type == ORIENTATION_ADAPTIVE)
        {
          if (runningvals.size_type == SIZE_TYPE_ADAPTIVE)
            n = choose_best_brush (p, a, tx-maxbrushwidth/2,
                                  ty-maxbrushheight/2, brushes,
                                  num_brushes, brushes_sum, 0, 1);
          else
            {
              int st = sn * runningvals.orient_num;
              n = choose_best_brush (p, a, tx-maxbrushwidth/2,
                                    ty-maxbrushheight/2, brushes,
                                    st+runningvals.orient_num, brushes_sum,
                                    st, 1);
            }
        }
      else
        {
          if (runningvals.size_type == SIZE_TYPE_ADAPTIVE)
            n = choose_best_brush (p, a, tx-maxbrushwidth/2,
                                    ty-maxbrushheight/2, brushes,
                                    num_brushes, brushes_sum,
                                    on, runningvals.orient_num);
          else
            n = sn * runningvals.orient_num + on;
        }
      /* Should never happen, but hey... */
      if (n < 0)
        n = 0;
      else if (n >= num_brushes)
        n = num_brushes - 1;

      tx -= maxbrushwidth/2;
      ty -= maxbrushheight/2;

      brush = &brushes[n];
      if (dropshadow)
        shadow = &shadows[n];
      thissum = brushes_sum[n];

      /* Calculate color - avg. of in-brush pixels */
      if (runningvals.color_type == 0)
        {
          r = g = b = 0;
          for (y = 0; y < brush->height; y++)
            {
              guchar *row = &p->col[(ty + y) * p->width * 3];

              for (x = 0; x < brush->width; x++)
                {
                  int k = (tx + x) * 3;
                  double v;

                  if ((h = brush->col[y * brush->width * 3 + x * 3]))
                    {
                      v = h / 255.0;
                      r += row[k+0] * v;
                      g += row[k+1] * v;
                      b += row[k+2] * v;
                    }
                }
            }
          r = r * 255.0 / thissum;
          g = g * 255.0 / thissum;
          b = b * 255.0 / thissum;
        }
      else if (runningvals.color_type == 1)
        {
          guchar *pixel;

          y = ty + (brush->height / 2);
          x = tx + (brush->width / 2);
          pixel = &p->col[y*p->width * 3 + x * 3];
          r = pixel[0];
          g = pixel[1];
          b = pixel[2];
        }
      else
        {
          /* No such color_type! */
          r = g = b = 0;
        }
      if (runningvals.color_noise > 0.0)
        {
          double v = runningvals.color_noise;
#define BOUNDS(a) (((a) < 0) ? (a) : ((a) > 255) ? 255 : (a))
#define MYASSIGN(a) \
    { \
        a = a + g_rand_double_range (random_generator, -v/2.0, v/2.0); \
        a = BOUNDS(a) ;       \
    }
          MYASSIGN (r);
          MYASSIGN (g);
          MYASSIGN (b);
#undef BOUNDS
#undef MYASSIGN
        }

      apply_brush (brush, shadow, &tmp, &atmp, tx,ty, r,g,b);

      if (runningvals.general_tileable && runningvals.general_paint_edges)
        {
          int orig_width = tmp.width - 2 * maxbrushwidth;
          int orig_height = tmp.height - 2 * maxbrushheight;
          int dox = 0, doy = 0;

          if (tx < maxbrushwidth)
            {
              apply_brush (brush, shadow, &tmp, &atmp, tx+orig_width,ty, r,g,b);
              dox = -1;
            }
          else if (tx > orig_width)
            {
              apply_brush (brush, shadow, &tmp, &atmp, tx-orig_width,ty, r,g,b);
              dox = 1;
            }
          if (ty < maxbrushheight)
            {
              apply_brush (brush, shadow, &tmp, &atmp, tx,ty+orig_height, r,g,b);
              doy = 1;
            }
          else if (ty > orig_height)
            {
              apply_brush (brush, shadow, &tmp, &atmp, tx,ty-orig_height, r,g,b);
              doy = -1;
            }
          if (doy)
            {
              if (dox < 0)
                apply_brush (brush, shadow, &tmp, &atmp,
                             tx+orig_width, ty + doy * orig_height, r, g, b);
              if (dox > 0)
                apply_brush (brush, shadow, &tmp, &atmp,
                             tx-orig_width, ty + doy * orig_height, r, g, b);
            }
        }
    }
  for (i = 0; i < num_brushes; i++)
    {
      ppm_kill (&brushes[i]);
    }
  g_free (brushes);
  g_free (shadows);
  g_free (brushes_sum);

  g_free (xpos);
  g_free (ypos);

  if (runningvals.general_paint_edges)
    {
      crop (&tmp,
            maxbrushwidth, maxbrushheight,
            tmp.width - maxbrushwidth, tmp.height - maxbrushheight);
      if (img_has_alpha)
        crop (&atmp,
              maxbrushwidth, maxbrushheight,
              atmp.width - maxbrushwidth, atmp.height - maxbrushheight);
    }

  ppm_kill (p);
  p->width = tmp.width;
  p->height = tmp.height;
  p->col = tmp.col;

  if (img_has_alpha)
    {
      ppm_kill (a);
      a->width = atmp.width;
      a->height = atmp.height;
      a->col = atmp.col;
    }

  relief = runningvals.paper_relief / 100.0;
  if (relief > 0.001)
    {
      scale = runningvals.paper_scale / 100.0;

      if (PPM_IS_INITED (&paper_ppm))
        {
          tmp = paper_ppm;
          paper_ppm.col = NULL;
        }
      else
        {
          tmp.col = NULL;
          ppm_load (runningvals.selected_paper, &tmp);
          resize (&tmp, tmp.width * scale, tmp.height * scale);
          if (runningvals.paper_invert)
            ppm_apply_gamma (&tmp, -1.0, 1,1,1);
        }
      for (x = 0; x < p->width; x++)
        {
          double h, v;
          int    px = x % tmp.width, py;

          for (y = 0; y < p->height; y++)
            {
              int k = y * p->width * 3 + x * 3;

              py = y % tmp.height;
              if (runningvals.paper_overlay)
                h = (tmp.col[py * tmp.width * 3 + px * 3]-128) * relief;
              else
                h = (tmp.col[py * tmp.width * 3 + px * 3] -
                     (int)tmp.col[((py + 1) % tmp.height) * tmp.width * 3 +
                                  ((px + 1) % tmp.width) * 3]) /
                     -2.0 * relief;
              if (h <= 0.0)
                {
                  v = 1.0 + h/128.0;
                  if (v < 0.0)
                    v = 0.0;
                  else if (v > 1.0)
                    v = 1.0;
                  p->col[k+0] *= v;
                  p->col[k+1] *= v;
                  p->col[k+2] *= v;
                }
              else
                {
                  v = h/128.0;
                  if (v < 0.0)
                    v = 0.0;
                  else if (v > 1.0)
                    v = 1.0;
                  p->col[k+0] = p->col[k+0] * (1.0-v) + 255 * v;
                  p->col[k+1] = p->col[k+1] * (1.0-v) + 255 * v;
                  p->col[k+2] = p->col[k+2] * (1.0-v) + 255 * v;
                }
            }
        }
      ppm_kill (&tmp);
    }

  ppm_kill (&paper_ppm);
  ppm_kill (&dirmap);
  ppm_kill (&sizmap);

  if (runningvals.run)
    {
      gimp_progress_update (0.8);
    }
  else
    {
      preview_set_button_label (_("_Update"));
    }

  running = 0;
}
