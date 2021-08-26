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

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimpmath/gimpmath.h>

#include "ppmtool.h"
#include "gimpressionist.h"

#include "libgimp/stdplugins-intl.h"

static int
readline (FILE *f, char *buffer, int len)
{
  do
  {
    if (!fgets (buffer, len, f))
      return -1;
  }
  while (buffer[0] == '#');

  g_strchomp (buffer);
  return 0;
}

void
ppm_kill (ppm_t *p)
{
  g_free (p->col);
  p->col = NULL;
  p->height = p->width = 0;
}

void ppm_new (ppm_t *p, int xs, int ys)
{
  int    x;
  guchar bgcol[3] = {0,0,0};

  if (xs < 1)
    xs = 1;
  if (ys < 1)
    ys = 1;

  p->width = xs;
  p->height = ys;
  p->col = g_malloc (xs * 3 * ys);
  for (x = 0; x < xs * 3 * ys; x += 3)
    {
      p->col[x+0] = bgcol[0];
      p->col[x+1] = bgcol[1];
      p->col[x+2] = bgcol[2];
    }
}

void
get_rgb (ppm_t *s, float xo, float yo, guchar *d)
{
  float ix, iy;
  int   x1, x2, y1, y2;
  float x1y1, x2y1, x1y2, x2y2;
  float r, g, b;
  int   bail = 0;
  int   rowstride = s->width * 3;

  if (xo < 0.0)
    bail=1;
  else if (xo >= s->width-1)
    {
      xo = s->width-1;
#if 0
      bail=1;
#endif
    }

  if (yo < 0.0)
    bail=1;
  else if (yo >= s->height-1)
    {
      yo= s->height-1;
#if 0
      bail=1;
#endif
    }

  if (bail)
    {
      d[0] = d[1] = d[2] = 0;
      return;
    }

  ix = (int)xo;
  iy = (int)yo;

#if 0
  x1 = wrap(ix, s->width);
  x2 = wrap(ix+1, s->width);
  y1 = wrap(iy, s->height);
  y2 = wrap(iy+1, s->height);
#endif
  x1 = ix; x2 = ix + 1;
  y1 = iy; y2 = iy + 1;

#if 0
  printf("x1=%d y1=%d x2=%d y2=%d\n",x1,y1,x2,y2);
#endif

  x1y1 = (1.0 - xo + ix) * (1.0 - yo + iy);
  x2y1 = (xo - ix) * (1.0 - yo + iy);
  x1y2 = (1.0 - xo + ix) * (yo - iy);
  x2y2 = (xo - ix) * (yo - iy);

  r = s->col[y1 * rowstride + x1 * 3 + 0] * x1y1;
  g = s->col[y1 * rowstride + x1 * 3 + 1] * x1y1;
  b = s->col[y1 * rowstride + x1 * 3 + 2] * x1y1;

  if (x2y1 > 0.0)
    r += s->col[y1 * rowstride + x2 * 3 + 0] * x2y1;
  if (x2y1 > 0.0)
    g += s->col[y1 * rowstride + x2 * 3 + 1] * x2y1;
  if (x2y1 > 0.0)
    b += s->col[y1 * rowstride + x2 * 3 + 2] * x2y1;

  if (x1y2 > 0.0)
    r += s->col[y2 * rowstride + x1 * 3 + 0] * x1y2;
  if (x1y2 > 0.0)
    g += s->col[y2 * rowstride + x1 * 3 + 1] * x1y2;
  if (x1y2 > 0.0)
    b += s->col[y2 * rowstride + x1 * 3 + 2] * x1y2;

  if (x2y2 > 0.0)
    r += s->col[y2 * rowstride + x2 * 3 + 0] * x2y2;
  if (x2y2 > 0.0)
    g += s->col[y2 * rowstride + x2 * 3 + 1] * x2y2;
  if (x2y2 > 0.0)
    b += s->col[y2 * rowstride + x2 * 3 + 2] * x2y2;

  d[0] = r;
  d[1] = g;
  d[2] = b;
}


void
resize (ppm_t *p, int nx, int ny)
{
  int   x, y;
  float xs = p->width / (float)nx;
  float ys = p->height / (float)ny;
  ppm_t tmp = {0, 0, NULL};

  ppm_new (&tmp, nx, ny);
  for (y = 0; y < ny; y++)
    {
      guchar *row = tmp.col + y * tmp.width * 3;

      for (x = 0; x < nx; x++)
        {
          get_rgb (p, x * xs, y * ys, &row[x * 3]);
        }
    }
  ppm_kill (p);
  p->width = tmp.width;
  p->height = tmp.height;
  p->col = tmp.col;
}

void
rescale (ppm_t *p, double sc)
{
  resize (p, p->width * sc, p->height * sc);
}

void resize_fast (ppm_t *p, int nx, int ny)
{
  int   x, y;
  float xs = p->width / (float)nx;
  float ys = p->height / (float)ny;
  ppm_t tmp = {0, 0, NULL};

  ppm_new (&tmp, nx, ny);
  for (y = 0; y < ny; y++)
    {
      for (x = 0; x < nx; x++)
        {
          gint rx = x * xs, ry = y * ys;

          memcpy (&tmp.col[y * tmp.width * 3 + x * 3],
                  &p->col[ry * p->width * 3 + rx * 3],
                  3);
    }
  }
  ppm_kill (p);
  p->width = tmp.width;
  p->height = tmp.height;
  p->col = tmp.col;
}


struct _BrushHeader
{
  unsigned int   header_size; /*  header_size = sz_BrushHeader + brush name  */
  unsigned int   version;     /*  brush file version #  */
  unsigned int   width;       /*  width of brush  */
  unsigned int   height;      /*  height of brush  */
  unsigned int   bytes;       /*  depth of brush in bytes--always 1 */
  unsigned int   magic_number;/*  GIMP brush magic number  */
  unsigned int   spacing;     /*  brush spacing  */
};

static void
msb2lsb (unsigned int *i)
{
  guchar *p = (guchar *)i, c;

  c = p[1]; p[1] = p[2]; p[2] = c;
  c = p[0]; p[0] = p[3]; p[3] = c;
}

static FILE *
fopen_from_search_path (const gchar * fn, const char * mode)
{
  FILE  * f;
  gchar * full_filename;

  f = g_fopen (fn, mode);
  if (!f)
    {
      full_filename = findfile (fn);
      f = g_fopen (full_filename, mode);
      g_free (full_filename);
    }
  return f;
}

static void
load_gimp_brush (const gchar *fn, ppm_t *p)
{
  FILE                *f;
  struct _BrushHeader  hdr;
  gchar               *ptr;
  gint                 x, y;

  f = fopen_from_search_path (fn, "rb");
  ppm_kill (p);

  if (!f)
    {
      g_printerr ("load_gimp_brush: Unable to open file \"%s\"!\n",
                  gimp_filename_to_utf8 (fn));
      ppm_new (p, 10,10);
      return;
    }

  fread (&hdr, 1, sizeof (struct _BrushHeader), f);

  for (x = 0; x < 7; x++)
    msb2lsb (&((unsigned int *)&hdr)[x]);

  ppm_new (p, hdr.width, hdr.height);

  ptr = g_malloc (hdr.width);
  fseek (f, hdr.header_size, SEEK_SET);
  for (y = 0; y < p->height; y++)
    {
      fread (ptr, p->width, 1, f);
      for  (x = 0; x < p->width; x++)
        {
          int k = y * p->width * 3 + x * 3;
          p->col[k+0] = p->col[k+1] = p->col[k+2] = ptr[x];
        }
    }
  fclose (f);
  g_free (ptr);
}

void
ppm_load (const char *fn, ppm_t *p)
{
  char  line[200];
  int   y, pgm = 0;
  FILE *f;

  if (!strcmp (&fn[strlen (fn)-4], ".gbr"))
    {
      load_gimp_brush(fn, p);
      return;
    }

  f = fopen_from_search_path (fn, "rb");

  ppm_kill (p);

  if (!f)
    {
      g_printerr ("ppm_load: Unable to open file \"%s\"!\n",
                  gimp_filename_to_utf8 (fn));
      ppm_new (p, 10,10);
      return;
    }

  readline (f, line, 200);
  if (strcmp (line, "P6"))
    {
      if (strcmp (line, "P5"))
        {
          fclose (f);
          g_printerr ("ppm_load: File \"%s\" not PPM/PGM? (line=\"%s\")%c\n",
                      gimp_filename_to_utf8 (fn), line, 7);
          ppm_new (p, 10,10);
          return;
    }
    pgm = 1;
  }
  readline (f, line, 200);
  p->width = atoi (line);
  p->height = atoi (strchr (line, ' ')+1);
  readline (f, line, 200);
  if (strcmp (line, "255"))
  {
    fclose (f);
    g_printerr ("ppm_load: File \"%s\" not valid PPM/PGM? (line=\"%s\")%c\n",
                gimp_filename_to_utf8 (fn), line, 7);
    ppm_new (p, 10,10);
    return;
  }
  p->col = g_malloc (p->height * p->width * 3);

  if (!pgm)
    {
      fread (p->col, p->height * 3 * p->width, 1, f);
    }
  else
  {
    guchar *tmpcol = g_malloc (p->width * p->height);

    fread (tmpcol, p->height * p->width, 1, f);
    for (y = 0; y < p->width * p->height * 3; y++) {
      p->col[y] = tmpcol[y / 3];
    }

    g_free (tmpcol);
  }
  fclose (f);
}

void
fill (ppm_t *p, guchar *c)
{
  int x, y;

  if ((c[0] == c[1]) && (c[0] == c[2]))
    {
      guchar col = c[0];
      for (y = 0; y < p->height; y++)
        {
          memset(p->col + y*p->width*3, col, p->width*3);
        }
    }
  else
    {
      for (y = 0; y < p->height; y++)
        {
          guchar *row = p->col + y * p->width * 3;

          for (x = 0; x < p->width; x++)
            {
              int k = x * 3;

              row[k+0] = c[0];
              row[k+1] = c[1];
              row[k+2] = c[2];
            }
        }
    }
}

void
ppm_copy (ppm_t *s, ppm_t *p)
{
  ppm_kill (p);
  p->width = s->width;
  p->height = s->height;
  p->col = g_memdup2 (s->col, p->width * 3 * p->height);
}

void
free_rotate (ppm_t *p, double amount)
{
  int    x, y;
  double nx, ny;
  ppm_t  tmp = {0, 0, NULL};
  double f = amount * G_PI * 2 / 360.0;
  int    rowstride = p->width * 3;

  ppm_new (&tmp, p->width, p->height);
  for (y = 0; y < p->height; y++)
    {
      for (x = 0; x < p->width; x++)
        {
          double r, d;

          nx = fabs (x - p->width / 2.0);
          ny = fabs (y - p->height / 2.0);
          r = sqrt (nx * nx + ny * ny);

          d = atan2 ((y - p->height / 2.0), (x - p->width / 2.0));

          nx = (p->width / 2.0 + cos (d - f) * r);
          ny = (p->height / 2.0 + sin (d - f) * r);
          get_rgb (p, nx, ny, tmp.col + y * rowstride + x * 3);
        }
    }
  ppm_kill (p);
  p->width = tmp.width;
  p->height = tmp.height;
  p->col = tmp.col;
}

void
crop (ppm_t *p, int lx, int ly, int hx, int hy)
{
  ppm_t tmp = {0,0,NULL};
  int   x, y;
  int   srowstride = p->width * 3;
  int   drowstride;

  ppm_new (&tmp, hx - lx, hy - ly);
  drowstride = tmp.width * 3;
  for (y = ly; y < hy; y++)
    for (x = lx; x < hx; x++)
      memcpy (&tmp.col[(y - ly) * drowstride + (x - lx) * 3],
              &p->col[y * srowstride + x * 3],
              3);
  ppm_kill (p);
  p->col = tmp.col;
  p->width = tmp.width;
  p->height = tmp.height;
}

void
autocrop (ppm_t *p, int room)
{
  int    lx = 0, hx = p->width, ly = 0, hy = p->height;
  int    x, y, n = 0;
  guchar tc[3];
  ppm_t  tmp = {0,0,NULL};
  int    rowstride = p->width * 3;
  int    drowstride;

  /* upper */
  memcpy (&tc, p->col, 3);
  for (y = 0; y < p->height; y++)
    {
      n = 0;
      for  (x = 0; x < p->width; x++)
        {
          if (memcmp (&tc, &p->col[y*rowstride+x*3], 3))
            {
              n++;
              break;
            }
        }
      if (n)
        break;
    }
  if (n)
    ly = y;
#if 0
  printf("ly = %d\n", ly);
#endif

  /* lower */
  memcpy (&tc, &p->col[(p->height - 1) * rowstride], 3);
  for (y = p->height-1; y >= 0; y--)
    {
      n = 0;
      for (x = 0; x < p->width; x++)
        {
          if (memcmp (&tc, &p->col[y*rowstride+x*3], 3))
            {
              n++;
              break;
            }
        }
      if (n)
        break;
    }
  if (n)
    hy = y+1;
  if (hy >= p->height)
    hy = p->height - 1;
#if 0
  printf("hy = %d\n", hy);
#endif

  /* left */
  memcpy (&tc, &p->col[ly * rowstride], 3);
  for (x = 0; x < p->width; x++)
    {
      n = 0;
      for (y = ly; y <= hy && y < p->height; y++)
        {
          if (memcmp (&tc, &p->col[y * rowstride + x * 3], 3))
            {
              n++;
              break;
            }
        }
      if (n)
        break;
    }
  if (n)
    lx = x;
#if 0
  printf("lx = %d\n", lx);
#endif

  /* right */
  memcpy
    (&tc, &p->col[ly * rowstride + (p->width - 1) * 3], 3);
  for (x = p->width-1; x >= 0; x--)
    {
      n = 0;
      for (y = ly; y <= hy; y++)
        {
          if (memcmp (&tc, &p->col[y * rowstride + x * 3], 3))
            {
              n++;
              break;
            }
        }
    if (n)
      break;
  }
  if (n)
    hx = x + 1;
#if 0
  printf("hx = %d\n", hx);
#endif

  lx -= room; if (lx < 0) lx = 0;
  ly -= room; if (ly < 0) ly = 0;
  hx += room; if (hx >= p->width)  hx = p->width  - 1;
  hy += room; if (hy >= p->height) hy = p->height - 1;

  ppm_new (&tmp, hx - lx, hy - ly);
  drowstride = tmp.width * 3;
  for (y = ly; y < hy; y++)
    for (x = lx; x < hx; x++)
      memcpy (&tmp.col[(y - ly) * drowstride + (x - lx) * 3],
              &p->col[y * rowstride + x * 3],
              3);
  ppm_kill (p);
  p->col = tmp.col;
  p->width = tmp.width;
  p->height = tmp.height;
}

void
ppm_pad (ppm_t *p, int left,int right, int top, int bottom, guchar *bg)
{
  int   x, y;
  ppm_t tmp = {0, 0, NULL};

  ppm_new (&tmp, p->width + left + right, p->height + top + bottom);
  for (y = 0; y < tmp.height; y++)
    {
      guchar *row, *srcrow;

      row = tmp.col + y * tmp.width * 3;
      if ((y < top) || (y >= tmp.height-bottom))
        {
          for (x = 0; x < tmp.width; x++)
            {
              int k = x * 3;

              row[k+0] = bg[0];
              row[k+1] = bg[1];
              row[k+2] = bg[2];
            }
          continue;
        }
      srcrow = p->col + (y-top) * p->width * 3;
      for (x = 0; x < left; x++)
        {
          int k = x * 3;

          row[k+0] = bg[0];
          row[k+1] = bg[1];
          row[k+2] = bg[2];
        }
      for (; x < tmp.width-right; x++)
        {
          int k = y * tmp.width * 3 + x * 3;

          tmp.col[k+0] = srcrow[(x - left) * 3 + 0];
          tmp.col[k+1] = srcrow[(x - left) * 3 + 1];
          tmp.col[k+2] = srcrow[(x - left) * 3 + 2];
        }
      for (; x < tmp.width; x++)
        {
          int k = x * 3;

          row[k+0] = bg[0];
          row[k+1] = bg[1];
          row[k+2] = bg[2];
        }
    }
  ppm_kill (p);
  p->width = tmp.width;
  p->height = tmp.height;
  p->col = tmp.col;
}

void
ppm_save (ppm_t *p, const char *fn)
{
  FILE *f = g_fopen (fn, "wb");

  if (!f)
    {
      /*
       * gimp_filename_to_utf8 () and g_strerror () return temporary strings
       * that need not and should not be freed. So this call is OK.
       * */
      g_message (_("Failed to save PPM file '%s': %s"),
                  gimp_filename_to_utf8 (fn), g_strerror (errno));
      return;
    }

  fprintf (f, "P6\n%d %d\n255\n", p->width, p->height);
  fwrite (p->col, p->width * 3 * p->height, 1, f);
  fclose (f);
}

void
edgepad (ppm_t *p, int left,int right, int top, int bottom)
{
  int    x, y;
  ppm_t  tmp = {0, 0, NULL};
  guchar testcol[3] = {0, 255, 0};
  int    srowstride, drowstride;

  ppm_new (&tmp, p->width+left+right, p->height+top+bottom);
  fill (&tmp, testcol);

  srowstride = p->width * 3;
  drowstride = tmp.width * 3;

  for (y = 0; y < top; y++)
    {
      memcpy (&tmp.col[y * drowstride + left * 3], p->col, srowstride);
    }
  for (; y-top < p->height; y++)
    {
      memcpy (&tmp.col[y * drowstride + left * 3],
              p->col + (y - top) * srowstride,
              srowstride);
    }
  for (; y < tmp.height ; y++)
    {
      memcpy (&tmp.col[y * drowstride + left * 3],
              p->col + (p->height - 1) * srowstride,
              srowstride);
    }
  for (y = 0; y < tmp.height; y++)
    {
      guchar *col, *tmprow;

      tmprow = tmp.col + y*drowstride;
      col = tmp.col + y*drowstride + left*3;

      for (x = 0; x < left; x++)
        {
          memcpy (&tmprow[x * 3], col, 3);
        }
      col = tmp.col + y * drowstride + (tmp.width-right - 1) * 3;
      for (x = 0; x < right; x++)
        {
          memcpy (&tmprow[(x + tmp.width - right - 1) * 3], col, 3);
        }
    }
  ppm_kill (p);
  p->width = tmp.width;
  p->height = tmp.height;
  p->col = tmp.col;
}

void
ppm_apply_gamma (ppm_t *p, float e, int r, int g, int b)
{
  int    x, l = p->width * 3 * p->height;
  guchar xlat[256], *pix;

  if (e > 0.0)
    for (x = 0; x < 256; x++)
      {
        xlat[x] = pow ((x / 255.0), (1.0 / e)) * 255.0;
      }
  else if (e < 0.0)
    for (x = 0; x < 256; x++)
      {
        xlat[255-x] = pow ((x / 255.0), (-1.0 / e)) * 255.0;
      }
  else
    for (x = 0; x < 256; x++)
      {
        xlat[x] = 0;
      }

  pix = p->col;
  if (r)
    for (x = 0; x < l; x += 3)
      pix[x] = xlat[pix[x]];
  if (g)
    for (x = 1; x < l; x += 3)
      pix[x] = xlat[pix[x]];
  if (b)
    for (x = 2; x < l; x += 3)
      pix[x] = xlat[pix[x]];
}

void
ppm_apply_brightness (ppm_t *p, float e, int r, int g, int b)
{
  int    x, l = p->width * 3 * p->height;
  guchar xlat[256], *pix;
  for (x = 0; x < 256; x++)
    xlat[x] = x * e;

  pix = p->col;
  if (r)
    for (x = 0; x < l; x += 3)
      pix[x] = xlat[pix[x]];
  if (g)
    for (x = 1; x < l; x += 3)
      pix[x] = xlat[pix[x]];
  if (b)
    for (x = 2; x < l; x += 3)
      pix[x] = xlat[pix[x]];
}

void
blur (ppm_t *p, int xrad, int yrad)
{
  int   x, y, k;
  int   tx, ty;
  ppm_t tmp = {0,0,NULL};
  int   r, g, b, n;
  int   rowstride = p->width * 3;

  ppm_new (&tmp, p->width, p->height);
  for (y = 0; y < p->height; y++)
    {
      for (x = 0; x < p->width; x++)
        {
          r = g = b = n = 0;

          for (ty = y-yrad; ty <= y+yrad; ty++)
            {
              for (tx = x-xrad; tx <= x+xrad; tx++)
                {
                  if (ty<0) continue;
                  if (ty>=p->height) continue;
                  if (tx<0) continue;
                  if (tx>=p->width) continue;
                  k = ty * rowstride + tx * 3;
                  r += p->col[k+0];
                  g += p->col[k+1];
                  b += p->col[k+2];
                  n++;
               }
            }
          k = y * rowstride + x * 3;
          tmp.col[k+0] = r / n;
          tmp.col[k+1] = g / n;
          tmp.col[k+2] = b / n;
        }
    }
  ppm_kill (p);
  p->width = tmp.width;
  p->height = tmp.height;
  p->col = tmp.col;
}

void
ppm_put_rgb_fast (ppm_t *s, float xo, float yo, guchar *d)
{
  guchar *tp;
  tp = s->col + s->width * 3 * (int)(yo + 0.5) + 3 * (int)(xo + 0.5);
  tp[0] = d[0];
  tp[1] = d[1];
  tp[2] = d[2];
}

void
ppm_put_rgb (ppm_t *s, float xo, float yo, guchar *d)
{
  int   x, y;
  float aa, ab, ba, bb;
  int   k, rowstride = s->width * 3;

  x = xo;
  y = yo;

  if ((x < 0) || (y < 0) || (x >= s->width-1) || (y >= s->height-1))
    return;

  xo -= x;
  yo -= y;

  aa = (1.0 - xo) * (1.0 - yo);
  ab = xo * (1.0 - yo);
  ba = (1.0 - xo) * yo;
  bb = xo * yo;

  k = y * rowstride + x * 3;
  s->col[k+0] *= (1.0 - aa);
  s->col[k+1] *= (1.0 - aa);
  s->col[k+2] *= (1.0 - aa);

  s->col[k+3] *= (1.0 - ab);
  s->col[k+4] *= (1.0 - ab);
  s->col[k+5] *= (1.0 - ab);

  s->col[k+rowstride+0] *= (1.0 - ba);
  s->col[k+rowstride+1] *= (1.0 - ba);
  s->col[k+rowstride+2] *= (1.0 - ba);

  s->col[k+rowstride+3] *= (1.0 - bb);
  s->col[k+rowstride+4] *= (1.0 - bb);
  s->col[k+rowstride+5] *= (1.0 - bb);

  s->col[k+0] += aa * d[0];
  s->col[k+1] += aa * d[1];
  s->col[k+2] += aa * d[2];
  s->col[k+3] += ab * d[0];
  s->col[k+4] += ab * d[1];
  s->col[k+5] += ab * d[2];
  s->col[k+rowstride+0] += ba * d[0];
  s->col[k+rowstride+1] += ba * d[1];
  s->col[k+rowstride+2] += ba * d[2];
  s->col[k+rowstride+3] += bb * d[0];
  s->col[k+rowstride+4] += bb * d[1];
  s->col[k+rowstride+5] += bb * d[2];
}

void
ppm_drawline (ppm_t *p, float fx, float fy, float tx, float ty, guchar *col)
{
  float i;
  float d, x, y;

  if (fabs (fx - tx) > fabs ( fy - ty))
    {
      if (fx > tx)
        {
          i = tx; tx = fx; fx = i; i = ty; ty = fy; fy = i;
        }
      d = (ty - fy) / (tx - fx);
      y = fy;
      for (x = fx; x <= tx; x += 1.0)
        {
          ppm_put_rgb (p, x, y, col);
          y += d;
        }
    }
  else
    {
      if (fy > ty)
        {
          i = tx; tx = fx; fx = i; i = ty; ty = fy; fy = i;
        }
      d = (tx - fx) / (ty - fy);
      x = fx;
      for (y = fy; y <= ty; y += 1.0)
        {
          ppm_put_rgb (p, x, y, col);
      x += d;
    }
  }
}
