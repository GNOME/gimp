/*
    flame - cosmic recursive fractal flames
    Copyright (C) 1992  Scott Draves <spot@cs.cmu.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "config.h"

#include <stdlib.h>
#include <string.h> /* strcmp */

#include "libgimp/gimp.h"

#include "libifs.h"

#define CHOOSE_XFORM_GRAIN 100

static int    flam3_random_bit(void);
static double flam3_random01(void);

/*
 * run the function system described by CP forward N generations.
 * store the n resulting 3 vectors in POINTS.  the initial point is passed
 * in POINTS[0].  ignore the first FUSE iterations.
 */

void iterate(cp, n, fuse, points)
   control_point *cp;
   int n;
   int fuse;
   point *points;
{
   int i, j, count_large = 0, count_nan = 0;
   int xform_distrib[CHOOSE_XFORM_GRAIN];
   double p[3], t, r, dr;
   p[0] = points[0][0];
   p[1] = points[0][1];
   p[2] = points[0][2];

   /*
    * first, set up xform, which is an array that converts a uniform random
    * variable into one with the distribution dictated by the density
    * fields
    */
   dr = 0.0;
   for (i = 0; i < NXFORMS; i++)
      dr += cp->xform[i].density;
   dr = dr / CHOOSE_XFORM_GRAIN;

   j = 0;
   t = cp->xform[0].density;
   r = 0.0;
   for (i = 0; i < CHOOSE_XFORM_GRAIN; i++) {
      while (r >= t) {
	 j++;
	 t += cp->xform[j].density;
      }
      xform_distrib[i] = j;
      r += dr;
   }

   for (i = -fuse; i < n; i++) {
      int fn = xform_distrib[g_random_int_range (0, CHOOSE_XFORM_GRAIN) ];
      double tx, ty, v;

      if (p[0] > 100.0 || p[0] < -100.0 ||
	  p[1] > 100.0 || p[1] < -100.0)
	 count_large++;
      if (p[0] != p[0])
	 count_nan++;

#define coef   cp->xform[fn].c
#define vari   cp->xform[fn].var

      /* first compute the color coord */
      p[2] = (p[2] + cp->xform[fn].color) / 2.0;

      /* then apply the affine part of the function */
      tx = coef[0][0] * p[0] + coef[1][0] * p[1] + coef[2][0];
      ty = coef[0][1] * p[0] + coef[1][1] * p[1] + coef[2][1];

      p[0] = p[1] = 0.0;
      /* then add in proportional amounts of each of the variations */
      v = vari[0];
      if (v > 0.0) {
	 /* linear */
	 double nx, ny;
	 nx = tx;
	 ny = ty;
	 p[0] += v * nx;
	 p[1] += v * ny;
      }

      v = vari[1];
      if (v > 0.0) {
	 /* sinusoidal */
	 double nx, ny;
	 nx = sin(tx);
	 ny = sin(ty);
	 p[0] += v * nx;
	 p[1] += v * ny;
      }

      v = vari[2];
      if (v > 0.0) {
	 /* spherical */
	 double nx, ny;
	 double r2 = tx * tx + ty * ty + 1e-6;
	 nx = tx / r2;
	 ny = ty / r2;
	 p[0] += v * nx;
	 p[1] += v * ny;
      }

      v = vari[3];
      if (v > 0.0) {
	 /* swirl */
	 double r2 = tx * tx + ty * ty;  /* /k here is fun */
	 double c1 = sin(r2);
	 double c2 = cos(r2);
	 double nx = c1 * tx - c2 * ty;
	 double ny = c2 * tx + c1 * ty;
	 p[0] += v * nx;
	 p[1] += v * ny;
      }

      v = vari[4];
      if (v > 0.0) {
	 /* horseshoe */
	 double a, c1, c2, nx, ny;
	 if (tx < -EPS || tx > EPS ||
	     ty < -EPS || ty > EPS)
	    a = atan2(tx, ty);  /* times k here is fun */
	 else
	    a = 0.0;
	 c1 = sin(a);
	 c2 = cos(a);
	 nx = c1 * tx - c2 * ty;
	 ny = c2 * tx + c1 * ty;
	 p[0] += v * nx;
	 p[1] += v * ny;
      }

      v = vari[5];
      if (v > 0.0) {
	 /* polar */
	 double nx, ny;
	 if (tx < -EPS || tx > EPS ||
	     ty < -EPS || ty > EPS)
	    nx = atan2(tx, ty) / G_PI;
	 else
	    nx = 0.0;

	 ny = sqrt(tx * tx + ty * ty) - 1.0;
	 p[0] += v * nx;
	 p[1] += v * ny;
      }

      v = vari[6];
      if (v > 0.0) {
	 /* bent */
	 double nx, ny;
	 nx = tx;
	 ny = ty;
	 if (nx < 0.0) nx = nx * 2.0;
	 if (ny < 0.0) ny = ny / 2.0;
	 p[0] += v * nx;
	 p[1] += v * ny;
      }

      v = vari[7];
      if (v > 0.0) {
         /* folded handkerchief */
         double theta, r2, nx, ny;
         if (tx < -EPS || tx > EPS ||
             ty < -EPS || ty > EPS)
            theta = atan2( tx, ty );
         else
            theta = 0.0;
         r2 = sqrt( tx * tx + ty * ty);
         nx = sin(theta + r2) * r2;
         ny = cos(theta - r2) * r2;
         p[0] += v * nx;
         p[1] += v * ny;
      }
      
      v = vari[8];
      if (v > 0.0) {
         /* heart */
         double theta, r2, nx, ny;
         if (tx < -EPS || tx > EPS ||
             ty < -EPS || ty > EPS)
            theta = atan2( tx, ty );
         else
            theta = 0.0;
         r2 = sqrt( tx * tx + ty * ty );
         theta *= r2;
         nx = sin(theta) * r2;
         ny = cos(theta) * -r2;
         p[0] += v * nx;
         p[1] += v * ny;
      }

      v = vari[9];
      if (v > 0.0) {
         /* disc */
         double theta, r2, nx, ny;
         if ( tx < -EPS || tx > EPS ||
              ty < - EPS || ty > EPS)
            theta = atan2( tx, ty );
         else
            theta = 0.0;
         nx = tx * G_PI;
         ny = ty * G_PI;
         r2 = sqrt( nx * nx * ny * ny );
         p[0] += v * sin(r2) * theta / G_PI;
         p[1] += v * cos(r2) * theta / G_PI;
      }

      v = vari[10];
      if ( v > 0.0 ) {
         /* spiral */
         double theta, r2;
         if (tx < -EPS || tx > EPS ||
             ty < -EPS || ty > EPS)
            theta = atan2( tx, ty );
         else
            theta = 0.0;
         r2 = sqrt( tx * tx + ty * ty ) + 1e-6;
         p[0] += v * ( cos(theta) + sin(r2)) / r2;
         p[1] += v * ( cos(theta) + cos(r2)) / r2;
      }

      v = vari[11];
      if (v > 0.0) {
         /* hyperbolic */
         double theta, r2;
         if (tx < -EPS || tx > EPS ||
             ty < -EPS || ty > EPS)
            theta = atan2( tx, ty );
         else
            theta = 0.0;
         r2 = sqrt( tx * tx + ty * ty) + 1e-6;
         p[0] += v * sin(theta) / r2;
         p[1] += v * cos(theta) * r2;
      }

      v = vari[12];
      if (v > 0.0 ) {
         double theta, r2;
         /* diamond */
         if ( tx < -EPS || tx > EPS ||
              ty < -EPS || ty > EPS)
            theta = atan2( tx, ty );
         else
            theta = 0.0;
         r2 = sqrt( tx * tx + ty * ty );
         p[0] += v * sin(theta) * cos(r2);
         p[1] += v * cos(theta) * sin(r2);
      }

      v = vari[13];
      if ( v > 0.0 ) {
         /* ex */
         double theta, r2, n0, n1, m0, m1;
         if ( tx < -EPS || tx > EPS ||
              ty < -EPS || ty > EPS)
            theta = atan2( tx, ty );
         else
            theta = 0.0;
         r2 = sqrt( tx * tx + ty * ty );
         n0 = sin(theta + r2);
         n1 = cos(theta - r2);
         m0 = n0 * n0 * n0 * r2;
         m1 = n1 * n1 * n1 * r2;
         p[0] += v * (m0 + m1);
         p[1] += v * (m0 - m1);
      }

      v = vari[14];
      if ( v > 0.0) {
         double theta, r2, nx, ny;
         /* julia */
         if (tx < -EPS || tx > EPS ||
             ty < -EPS || ty > EPS)
            theta = atan2( tx, ty );
         else
            theta = 0.0;
         if (flam3_random_bit()) theta += G_PI;
         r2 = pow( tx * tx + ty * ty, 0.25);
         nx = r2 * cos(theta);
         ny = r2 * sin(theta);
         p[0] += v * nx;
         p[1] += v * ny;
      }

      v = vari[15];
      if ( v > 0.0 ) {
         /* waves */
         double dx, dy, nx, ny;
         dx = coef[2][0];
         dy = coef[2][1];
         nx = tx + coef[1][0] * sin(ty / ((dx*dx)+EPS));
         ny = ty + coef[1][1] * sin(tx / ((dy*dy)+EPS));
         p[0] += v * nx;
         p[1] += v * ny;
      }
      
      v = vari[16];
      if ( v > 0.0 ) {
         /* fisheye */
         double theta, r2, nx, ny;
         if (tx < -EPS || tx > EPS ||
             ty < -EPS || ty > EPS)
            theta = atan2( tx, ty );
         else
            theta = 0.0;
         r2 = sqrt( tx * tx + ty * ty );
         r2 = 2 * r2 / (r2 + 1);
         nx = r2 * cos(theta);
         ny = r2 * sin(theta);
         p[0] += v * nx;
         p[1] += v * ny;
      }

      v = vari[17];
      if (v > 0.0) {
         /* popcorn */
         double dx, dy, nx, ny;
         dx = tan(3*ty);
         dy = tan(3*tx);
         nx = tx + coef[2][0] * sin(dx);
         ny = ty + coef[2][1] * sin(dy);
         p[0] += v * nx;
         p[1] += v * ny;
      }

      v = vari[18];
      if (v > 0.0) {
         /* exponential */
         double dx, dy, nx, ny;
         dx = exp( tx - 1.0);
         dy = G_PI * ty;
         nx = cos(dy) * dx;
         ny = sin(dy) * dx;
         p[0] += v * nx;
         p[1] += v * ny;
      }

      v = vari[19];
      if (v > 0.0) {
         /* power */
         double theta, r2, tsin, tcos, nx, ny;
         if (tx < -EPS || tx > EPS ||
             ty < -EPS || ty > EPS)
            theta = atan2( tx, ty );
         else
            theta = 0.0;
         tsin = sin(theta);
         tcos = cos(theta);
         r2 = sqrt( tx * tx + ty * ty );
         r2 = pow( r2, tsin);
         nx = r2 * tcos;;
         ny = r2 * tsin;
         p[0] += v * nx;
         p[1] += v * ny;
      }

      v = vari[20];
      if (v > 0.0) {
         /* cosine */
         double nx, ny;
         nx = cos(tx * G_PI) * cosh(ty);
         ny = -sin(tx * G_PI) * sinh(ty);
         p[0] += v * nx;
         p[1] += v * ny;
      }

      v = vari[21];
      if (v > 0.0) {
         /* rings */
         double theta, r2, dx, nx, ny;
         if (tx < -EPS || tx > EPS ||
             ty < -EPS || ty > EPS)
            theta = atan2( tx, ty );
         else
            theta = 0;
         dx = coef[2][0];
         dx = dx * dx + EPS;
         r2 = sqrt( tx * tx + ty * ty );
         r2 = fmod( r2 + dx, 2 * dx) - dx + r2 * (1-dx);
         nx = cos(theta) * r2;
         ny = sin(theta) * r2;
         p[0] += v * nx;
         p[1] += v * ny;
      }

      v = vari[22];
      if (v > 0.0) {
         /* fan */
         double theta, r2, dx, dy, dx2, nx, ny;
         if (tx < -EPS || tx > EPS ||
             ty < -EPS || ty > EPS)
            theta = atan2( tx, ty );
         else
            theta = 0.0;
         dx = coef[2][0];
         dy = coef[2][1];
         dx = G_PI * (dx * dx + EPS);
         dx2 = dx / 2;
         r2 = sqrt( tx * tx + ty * ty );
         theta += (fmod(theta + dy, dx) > dx2) ? -dx2: dx2;
         nx = cos(theta) * r2;
         ny = sin(theta) * r2;
         p[0] += v * nx;
         p[1] += v * ny;
      }

      v = vari[23];
      if (v > 0.0) {
         /* eyefish */
         double r2;
         r2 = 2.0 * v / ( sqrt( tx * tx + ty * ty ) + 1.0 );
         p[0] += r2 * tx;
         p[1] += r2 * ty;
      }

      v = vari[24];
      if (v > 0.0) {
         /* bubble */
         double r2;
         r2 = v / ( (tx * tx + ty * ty) / 4 + 1 );
         p[0] += r2 * tx;
         p[1] += r2 * ty;
      }

      v = vari[25];
      if (v > 0.0) {
         /* cylinder */
         double nx;
         nx = sin(tx);
         p[0] += v * nx;
         p[1] += v * ty;
      }

      v = vari[26];
      if (v > 0.0) {
         /* noise */
         double rx, sinr, cosr, nois;
         rx = flam3_random01() * 2 * G_PI;
         sinr = sin(rx);
         cosr = cos(rx);
         nois = flam3_random01();
         p[0] += v * nois * tx * cosr;
         p[1] += v * nois * ty * sinr;
      }

      v = vari[27];
      if (v > 0.0) {
         /* blur */
         double rx, sinr, cosr, nois;
         rx = flam3_random01() * 2 * G_PI;
         sinr = sin(rx);
         cosr = cos(rx);
         nois = flam3_random01();
         p[0] += v * nois * cosr;
         p[1] += v * nois * sinr;
      }

      v = vari[28];
      if (v > 0.0) {
         /* gaussian */
         double ang, sina, cosa, r2;
         ang = flam3_random01() * 2 * G_PI;
         sina = sin(ang);
         cosa = cos(ang);
         r2 = v * ( flam3_random01() + flam3_random01() + flam3_random01() +
                    flam3_random01() - 2.0 );
         p[0] += r2 * cosa;
         p[1] += r2 * sina;
      }
      
      /* if fuse over, store it */
      if (i >= 0) {
	 points[i][0] = p[0];
	 points[i][1] = p[1];
	 points[i][2] = p[2];
      }
   }
#if 0
   if ((count_large > 10 || count_nan > 10)
       && !getenv("PVM_ARCH"))
      fprintf(stderr, "large = %d nan = %d\n", count_large, count_nan);
#endif
}

/* args must be non-overlapping */
void mult_matrix(s1, s2, d)
   double s1[2][2];
   double s2[2][2];
   double d[2][2];
{
   d[0][0] = s1[0][0] * s2[0][0] + s1[1][0] * s2[0][1];
   d[1][0] = s1[0][0] * s2[1][0] + s1[1][0] * s2[1][1];
   d[0][1] = s1[0][1] * s2[0][0] + s1[1][1] * s2[0][1];
   d[1][1] = s1[0][1] * s2[1][0] + s1[1][1] * s2[1][1];
}

static double det_matrix(s)
   double s[2][2];
{
   return s[0][0] * s[1][1] - s[0][1] * s[1][0];
}

#if 0
static void flip_matrix(m, h)
   double m[2][2];
   int h;
{
   double s, t;
   if (h) {
      /* flip on horizontal axis */
      s = m[0][0];
      t = m[0][1];
      m[0][0] = m[1][0];
      m[0][1] = m[1][1];
      m[1][0] = s;
      m[1][1] = t;
   } else {
      /* flip on vertical axis */
      s = m[0][0];
      t = m[1][0];
      m[0][0] = m[0][1];
      m[1][0] = m[1][1];
      m[0][1] = s;
      m[1][1] = t;
   }
}

static void transpose_matrix(m)
   double m[2][2];
{
   double t;
   t = m[0][1];
   m[0][1] = m[1][0];
   m[1][0] = t;
}
#endif

#if 0
static void choose_evector(m, r, v)
   double m[3][2], r;
   double v[2];
{
   double b = m[0][1];
   double d = m[1][1];
   double x = r - d;
   if (b > EPS) {
      v[0] = x;
      v[1] = b;
   } else if (b < -EPS) {
      v[0] = -x;
      v[1] = -b;
   } else {
      /* XXX */
      v[0] = 1.0;
      v[1] = 0.0;
   }
}


/* diagonalize the linear part of a 3x2 matrix.  the evalues are returned
   in r as either reals on the diagonal, or a complex pair.  the evectors
   are returned as a change of coords matrix.  does not handle shearing
   transforms.
   */

static void diagonalize_matrix(m, r, v)
   double m[3][2];
   double r[2][2];
   double v[2][2];
{
   double b, c, d;
   double m00, m10, m01, m11;
   m00 = m[0][0];
   m10 = m[1][0];
   m01 = m[0][1];
   m11 = m[1][1];
   b = -m00 - m11;
   c = (m00 * m11) - (m01 * m10);
   d = b * b - 4 * c;
   /* should use better formula, see numerical recipes */
   if (d > EPS) {
      double r0 = (-b + sqrt(d)) / 2.0;
      double r1 = (-b - sqrt(d)) / 2.0;
      r[0][0] = r0;
      r[1][1] = r1;
      r[0][1] = 0.0;
      r[1][0] = 0.0;

      choose_evector(m, r0, v + 0);
      choose_evector(m, r1, v + 1);
   } else if (d < -EPS) {
      double uu = -b / 2.0;
      double vv = sqrt(-d) / 2.0;
      double w1r, w1i, w2r, w2i;
      r[0][0] = uu;
      r[0][1] = vv;
      r[1][0] = -vv;
      r[1][1] = uu;

      if (m01 > EPS) {
	 w1r = uu - m11;
	 w1i = vv;
	 w2r = m01;
	 w2i = 0.0;
      } else if (m01 < -EPS) {
	 w1r = m11 - uu;
	 w1i = -vv;
	 w2r = -m01;
	 w2i = 0.0;
      } else {
	 /* XXX */
	 w1r = 0.0;
	 w1i = 1.0;
	 w2r = 1.0;
	 w2i = 0.0;
      }
      v[0][0] = w1i;
      v[0][1] = w2i;
      v[1][0] = w1r;
      v[1][1] = w2r;

   } else {
      double rr = -b / 2.0;
      r[0][0] = rr;
      r[1][1] = rr;
      r[0][1] = 0.0;
      r[1][0] = 0.0;

      v[0][0] = 1.0;
      v[0][1] = 0.0;
      v[1][0] = 0.0;
      v[1][1] = 1.0;
   }
   /* order the values so that the evector matrix has is positively
      oriented.  this is so that evectors never have to cross when we
      interpolate them. it might mean that the values cross zero when they
      wouldn't have otherwise (if they had different signs) but this is the
      lesser of two evils */
   if (det_matrix(v) < 0.0) {
      flip_matrix(v, 1);
      flip_matrix(r, 0);
      flip_matrix(r, 1);
   }
}


static void undiagonalize_matrix(r, v, m)
   double r[2][2];
   double v[2][2];
   double m[3][2];
{
   double v_inv[2][2];
   double t1[2][2];
   double t2[2][2];
   double t;
   /* the unfortunate truth is that given we are using row vectors
      the evectors should be stacked horizontally, but the complex
      interpolation functions only work on rows, so we fix things here */
   transpose_matrix(v);
   mult_matrix(r, v, t1);

   t = 1.0 / det_matrix(v);
   v_inv[0][0] = t * v[1][1];
   v_inv[1][1] = t * v[0][0];
   v_inv[1][0] = t * -v[1][0];
   v_inv[0][1] = t * -v[0][1];

   mult_matrix(v_inv, t1, t2);

   /* the unforunate truth is that i have no idea why this is needed. sigh. */
   transpose_matrix(t2);

   /* switch v back to how it was */
   transpose_matrix(v);

   m[0][0] = t2[0][0];
   m[0][1] = t2[0][1];
   m[1][0] = t2[1][0];
   m[1][1] = t2[1][1];
}
#endif

static void interpolate_angle(t, s, v1, v2, v3, tie, cross)
   double t, s;
   double *v1, *v2, *v3;
   int tie;
{
   double x = *v1;
   double y = *v2;
   double d;
   static double lastx, lasty;

   /* take the shorter way around the circle... */
   if (x > y) {
      d = x - y;
      if (d > G_PI + EPS ||
	  (d > G_PI - EPS && tie))
	 y += 2*G_PI;
   } else {
      d = y - x;
      if (d > G_PI + EPS ||
	  (d > G_PI - EPS && tie))
	 x += 2*G_PI;
   }
   /* unless we are supposed to avoid crossing */
   if (cross) {
      if (lastx > x) {
	 if (lasty < y)
	    y -= 2*G_PI;
      } else {
	 if (lasty > y)
	    y += 2*G_PI;
      }
   } else {
      lastx = x;
      lasty = y;
   }

   *v3 = s * x + t * y;
}

static void interpolate_complex(t, s, r1, r2, r3, flip, tie, cross)
   double t, s;
   double r1[2], r2[2], r3[2];
   int flip, tie, cross;
{
   double c1[2], c2[2], c3[2];
   double a1, a2, a3, d1, d2, d3;

   c1[0] = r1[0];
   c1[1] = r1[1];
   c2[0] = r2[0];
   c2[1] = r2[1];
   if (flip) {
      double t = c1[0];
      c1[0] = c1[1];
      c1[1] = t;
      t = c2[0];
      c2[0] = c2[1];
      c2[1] = t;
   }

   /* convert to log space */
   a1 = atan2(c1[1], c1[0]);
   a2 = atan2(c2[1], c2[0]);
   d1 = 0.5 * log(c1[0] * c1[0] + c1[1] * c1[1]);
   d2 = 0.5 * log(c2[0] * c2[0] + c2[1] * c2[1]);

   /* interpolate linearly */
   interpolate_angle(t, s, &a1, &a2, &a3, tie, cross);
   d3 = s * d1 + t * d2;

   /* convert back */
   d3 = exp(d3);
   c3[0] = cos(a3) * d3;
   c3[1] = sin(a3) * d3;

   if (flip) {
      r3[1] = c3[0];
      r3[0] = c3[1];
   } else {
      r3[0] = c3[0];
      r3[1] = c3[1];
   }
}


static void interpolate_matrix(t, m1, m2, m3)
   double m1[3][2], m2[3][2], m3[3][2];
   double t;
{
   double s = 1.0 - t;
#if 0
   double r1[2][2], r2[2][2], r3[2][2];
   double v1[2][2], v2[2][2], v3[2][2];
   diagonalize_matrix(m1, r1, v1);
   diagonalize_matrix(m2, r2, v2);

   /* handle the evectors */
   interpolate_complex(t, s, v1 + 0, v2 + 0, v3 + 0, 0, 0, 0);
   interpolate_complex(t, s, v1 + 1, v2 + 1, v3 + 1, 0, 0, 1);

   /* handle the evalues */
   interpolate_complex(t, s, r1 + 0, r2 + 0, r3 + 0, 0, 0, 0);
   interpolate_complex(t, s, r1 + 1, r2 + 1, r3 + 1, 1, 1, 0);

   undiagonalize_matrix(r3, v3, m3);
#endif

   interpolate_complex(t, s, m1 + 0, m2 + 0, m3 + 0, 0, 0, 0);
   interpolate_complex(t, s, m1 + 1, m2 + 1, m3 + 1, 1, 1, 0);

   /* handle the translation part of the xform linearly */
   m3[2][0] = s * m1[2][0] + t * m2[2][0];
   m3[2][1] = s * m1[2][1] + t * m2[2][1];
}

#define INTERP(x)  result->x = c0 * cps[i1].x + c1 * cps[i2].x

/*
 * create a control point that interpolates between the control points
 * passed in CPS.  for now just do linear.  in the future, add control
 * point types and other things to the cps.  CPS must be sorted by time.
 */
void interpolate(cps, ncps, time, result)
   control_point cps[];
   int ncps;
   double time;
   control_point *result;
{
   int i, j, i1, i2;
   double c0, c1, t;

   if (1 == ncps) {
      *result = cps[0];
      return;
   }
   if (cps[0].time >= time) {
      i1 = 0;
      i2 = 1;
   } else if (cps[ncps - 1].time <= time) {
      i1 = ncps - 2;
      i2 = ncps - 1;
   } else {
      i1 = 0;
      while (cps[i1].time < time)
	 i1++;
      i1--;
      i2 = i1 + 1;
      if (time - cps[i1].time > -1e-7 &&
	  time - cps[i1].time < 1e-7) {
	 *result = cps[i1];
	 return;
      }
   }

   c0 = (cps[i2].time - time) / (cps[i2].time - cps[i1].time);
   c1 = 1.0 - c0;

   result->time = time;

   if (cps[i1].cmap_inter) {
     for (i = 0; i < 256; i++) {
       double spread = 0.15;
       double d0, d1, e0, e1, c = 2 * G_PI * i / 256.0;
       c = cos(c * cps[i1].cmap_inter) + 4.0 * c1 - 2.0;
       if (c >  spread) c =  spread;
       if (c < -spread) c = -spread;
       d1 = (c + spread) * 0.5 / spread;
       d0 = 1.0 - d1;
       e0 = (d0 < 0.5) ? (d0 * 2) : (d1 * 2);
       e1 = 1.0 - e0;
       for (j = 0; j < 3; j++) {
	 result->cmap[i][j] = (d0 * cps[i1].cmap[i][j] +
			       d1 * cps[i2].cmap[i][j]);
#define bright_peak 2.0
#if 0
	 if (d0 < 0.5)
	   result->cmap[i][j] *= 1.0 + bright_peak * d0;
	 else
	   result->cmap[i][j] *= 1.0 + bright_peak * d1;
#else
	 result->cmap[i][j] = (e1 * result->cmap[i][j] +
			       e0 * 1.0);
#endif
       }
     }
   } else {
     for (i = 0; i < 256; i++) {
       double t[3], s[3];
       rgb2hsv(cps[i1].cmap[i], s);
       rgb2hsv(cps[i2].cmap[i], t);
       for (j = 0; j < 3; j++)
	 t[j] = c0 * s[j] + c1 * t[j];
       hsv2rgb(t, result->cmap[i]);
     }
   }

   result->cmap_index = -1;
   INTERP(brightness);
   INTERP(contrast);
   INTERP(gamma);
   INTERP(width);
   INTERP(height);
   INTERP(spatial_oversample);
   INTERP(center[0]);
   INTERP(center[1]);
   INTERP(pixels_per_unit);
   INTERP(spatial_filter_radius);
   INTERP(sample_density);
   INTERP(zoom);
   INTERP(nbatches);
   INTERP(white_level);
   for (i = 0; i < 2; i++)
      for (j = 0; j < 2; j++) {
	 INTERP(pulse[i][j]);
	 INTERP(wiggle[i][j]);
     }

   for (i = 0; i < NXFORMS; i++) {
      double r;
      INTERP(xform[i].density);
      if (result->xform[i].density > 0)
	 result->xform[i].density = 1.0;
      INTERP(xform[i].color);
      for (j = 0; j < NVARS; j++)
	 INTERP(xform[i].var[j]);
      t = 0.0;
      for (j = 0; j < NVARS; j++)
	 t += result->xform[i].var[j];
      t = 1.0 / t;
      for (j = 0; j < NVARS; j++)
	 result->xform[i].var[j] *= t;

      interpolate_matrix(c1, cps[i1].xform[i].c, cps[i2].xform[i].c,
			 result->xform[i].c);

      if (1) {
	 double rh_time = time * 2*G_PI / (60.0 * 30.0);

	 /* apply pulse factor. */
	 r = 1.0;
	 for (j = 0; j < 2; j++)
	    r += result->pulse[j][0] * sin(result->pulse[j][1] * rh_time);
	 for (j = 0; j < 3; j++) {
	    result->xform[i].c[j][0] *= r;
	    result->xform[i].c[j][1] *= r;
	 }

	 /* apply wiggle factor */
	 r = 0.0;
	 for (j = 0; j < 2; j++) {
	    double tt = result->wiggle[j][1] * rh_time;
	    double m = result->wiggle[j][0];
	    result->xform[i].c[0][0] += m *  cos(tt);
	    result->xform[i].c[1][0] += m * -sin(tt);
	    result->xform[i].c[0][1] += m *  sin(tt);
	    result->xform[i].c[1][1] += m *  cos(tt);
	 }
      }
   } /* for i */
}



/*
 * split a string passed in ss into tokens on whitespace.
 * # comments to end of line.  ; terminates the record
 */
void tokenize(ss, argv, argc)
   char **ss;
   char *argv[];
   int *argc;
{
   char *s = *ss;
   int i = 0, state = 0;

   while (*s != ';') {
      char c = *s;
      switch (state) {
       case 0:
	 if ('#' == c)
	    state = 2;
	 else if (!g_ascii_isspace(c)) {
	    argv[i] = s;
	    i++;
	    state = 1;
	 }
       case 1:
	 if (g_ascii_isspace(c)) {
	    *s = 0;
	    state = 0;
	 }
       case 2:
	 if ('\n' == c)
	    state = 0;
      }
      s++;
   }
   *s = 0;
   *ss = s+1;
   *argc = i;
}

static int compare_xforms(a, b)
   xform *a, *b;
{
   double aa[2][2];
   double bb[2][2];
   double ad, bd;

   aa[0][0] = a->c[0][0];
   aa[0][1] = a->c[0][1];
   aa[1][0] = a->c[1][0];
   aa[1][1] = a->c[1][1];
   bb[0][0] = b->c[0][0];
   bb[0][1] = b->c[0][1];
   bb[1][0] = b->c[1][0];
   bb[1][1] = b->c[1][1];
   ad = det_matrix(aa);
   bd = det_matrix(bb);
   if (ad < bd) return -1;
   if (ad > bd) return 1;
   return 0;
}

#define MAXARGS 1000
#define streql(x,y) (!strcmp(x,y))

/*
 * given a pointer to a string SS, fill fields of a control point CP.
 * return a pointer to the first unused char in SS.  totally barfucious,
 * must integrate with tcl soon...
 */

void parse_control_point(ss, cp)
   char **ss;
   control_point *cp;
{
   char *argv[MAXARGS];
   int argc, i, j;
   int set_cm = 0, set_image_size = 0, set_nbatches = 0, set_white_level = 0, set_cmap_inter = 0;
   int set_spatial_oversample = 0;
   double *slot = NULL, xf, cm, t, nbatches, white_level, spatial_oversample, cmap_inter;
   double image_size[2];

   for (i = 0; i < NXFORMS; i++) {
      cp->xform[i].density = 0.0;
      cp->xform[i].color = (i == 0);
      cp->xform[i].var[0] = 1.0;
      for (j = 1; j < NVARS; j++)
	 cp->xform[i].var[j] = 0.0;
      cp->xform[i].c[0][0] = 1.0;
      cp->xform[i].c[0][1] = 0.0;
      cp->xform[i].c[1][0] = 0.0;
      cp->xform[i].c[1][1] = 1.0;
      cp->xform[i].c[2][0] = 0.0;
      cp->xform[i].c[2][1] = 0.0;
   }
   for (j = 0; j < 2; j++) {
      cp->pulse[j][0] = 0.0;
      cp->pulse[j][1] = 60.0;
      cp->wiggle[j][0] = 0.0;
      cp->wiggle[j][1] = 60.0;
   }

   tokenize(ss, argv, &argc);
   for (i = 0; i < argc; i++) {
      if (streql("xform", argv[i]))
	 slot = &xf;
      else if (streql("time", argv[i]))
	 slot = &cp->time;
      else if (streql("brightness", argv[i]))
	 slot = &cp->brightness;
      else if (streql("contrast", argv[i]))
	 slot = &cp->contrast;
      else if (streql("gamma", argv[i]))
	 slot = &cp->gamma;
      else if (streql("zoom", argv[i]))
	 slot = &cp->zoom;
      else if (streql("image_size", argv[i])) {
	 slot = image_size;
	 set_image_size = 1;
      } else if (streql("center", argv[i]))
	 slot = cp->center;
      else if (streql("pulse", argv[i]))
	 slot = (double *) cp->pulse;
      else if (streql("wiggle", argv[i]))
	 slot = (double *) cp->wiggle;
      else if (streql("pixels_per_unit", argv[i]))
	 slot = &cp->pixels_per_unit;
      else if (streql("spatial_filter_radius", argv[i]))
	 slot = &cp->spatial_filter_radius;
      else if (streql("sample_density", argv[i]))
	 slot = &cp->sample_density;
      else if (streql("nbatches", argv[i])) {
	 slot = &nbatches;
	 set_nbatches = 1;
      } else if (streql("white_level", argv[i])) {
	 slot = &white_level;
	 set_white_level = 1;
      } else if (streql("spatial_oversample", argv[i])) {
	 slot = &spatial_oversample;
	 set_spatial_oversample = 1;
      } else if (streql("cmap", argv[i])) {
	 slot = &cm;
	 set_cm = 1;
      } else if (streql("density", argv[i]))
	 slot = &cp->xform[(int)xf].density;
      else if (streql("color", argv[i]))
	 slot = &cp->xform[(int)xf].color;
      else if (streql("coefs", argv[i])) {
	 slot = cp->xform[(int)xf].c[0];
	 cp->xform[(int)xf].density = 1.0;
       } else if (streql("var", argv[i]))
	 slot = cp->xform[(int)xf].var;
      else if (streql("cmap_inter", argv[i])) {
	slot = &cmap_inter;
	set_cmap_inter = 1;
      } else
        *slot++ = g_strtod(argv[i], NULL);
   }
   if (set_cm) {
      cp->cmap_index = (int) cm;
      get_cmap(cp->cmap_index, cp->cmap, 256);
   }
   if (set_image_size) {
      cp->width  = (int) image_size[0];
      cp->height = (int) image_size[1];
   }
   if (set_cmap_inter)
      cp->cmap_inter  = (int) cmap_inter;
   if (set_nbatches)
      cp->nbatches = (int) nbatches;
   if (set_spatial_oversample)
      cp->spatial_oversample = (int) spatial_oversample;
   if (set_white_level)
      cp->white_level = (int) white_level;
   for (i = 0; i < NXFORMS; i++) {
      t = 0.0;
      for (j = 0; j < NVARS; j++)
	 t += cp->xform[i].var[j];
      t = 1.0 / t;
      for (j = 0; j < NVARS; j++)
	 cp->xform[i].var[j] *= t;
   }
   qsort((char *) cp->xform, NXFORMS, sizeof(xform), compare_xforms);
}

void print_control_point(f, cp, quote)
   FILE *f;
   control_point *cp;
{
  int i, j;
  char *q = quote ? "# " : "";
  fprintf(f, "%stime %g\n", q, cp->time);
  if (-1 != cp->cmap_index)
    fprintf(f, "%scmap %d\n", q, cp->cmap_index);
  fprintf(f, "%simage_size %d %d center %g %g pixels_per_unit %g\n",
	  q, cp->width, cp->height, cp->center[0], cp->center[1],
	  cp->pixels_per_unit);
  fprintf(f, "%sspatial_oversample %d spatial_filter_radius %g",
	  q, cp->spatial_oversample, cp->spatial_filter_radius);
  fprintf(f, " sample_density %g\n", cp->sample_density);
  fprintf(f, "%snbatches %d white_level %d\n",
	  q, cp->nbatches, cp->white_level);
  fprintf(f, "%sbrightness %g gamma %g cmap_inter %d\n",
	  q, cp->brightness, cp->gamma, cp->cmap_inter);

  for (i = 0; i < NXFORMS; i++)
    if (cp->xform[i].density > 0.0) {
      fprintf(f, "%sxform %d density %g color %g\n",
	      q, i, cp->xform[i].density, cp->xform[i].color);
      fprintf(f, "%svar", q);
      for (j = 0; j < NVARS; j++)
	fprintf(f, " %g", cp->xform[i].var[j]);
      fprintf(f, "\n%scoefs", q);
      for (j = 0; j < 3; j++)
	fprintf(f, " %g %g", cp->xform[i].c[j][0], cp->xform[i].c[j][1]);
      fprintf(f, "\n");
    }
  fprintf(f, "%s;\n", q);
}

/* returns a uniform variable from 0 to 1 */
double random_uniform01(void) {
   return g_random_double ();
}

double random_uniform11(void) {
   return g_random_double_range (-1, 1);
}

/* returns a mean 0 variance 1 random variable
   see numerical recipies p 217 */
double random_gaussian(void) {
   static int iset = 0;
   static double gset;
   double fac, r, v1, v2;

   if (0 == iset) {
      do {
	 v1 = random_uniform11();
	 v2 = random_uniform11();
	 r = v1 * v1 + v2 * v2;
      } while (r >= 1.0 || r == 0.0);
      fac = sqrt(-2.0 * log(r)/r);
      gset = v1 * fac;
      iset = 1;
      return v2 * fac;
   }
   iset = 0;
   return gset;
}

void
copy_variation(control_point *cp0, control_point *cp1) {
  int i, j;
  for (i = 0; i < NXFORMS; i++) {
    for (j = 0; j < NVARS; j++)
      cp0->xform[i].var[j] =
	cp1->xform[i].var[j];
  }
}



#define random_distrib(v) ((v)[g_random_int_range (0, vlen(v))])

void random_control_point(cp, ivar)
   control_point *cp;
   int ivar;
{
   int i, nxforms, var;
   static int xform_distrib[] = {
      2, 2, 2,
      3, 3, 3,
      4, 4,
      5};
   static int var_distrib[] = {
      -1, -1, -1,
      0, 0, 0, 0,
      1, 1, 1,
      2, 2, 2,
      3, 3,
      4, 4,
      5};

   static int mixed_var_distrib[] = {
      0, 0, 0,
      1, 1, 1,
      2, 2, 2,
      3, 3,
      4, 4,
      5, 5};

   get_cmap(cmap_random, cp->cmap, 256);
   cp->time = 0.0;
   nxforms = random_distrib(xform_distrib);
   var = (0 > ivar) ?
     random_distrib(var_distrib) :
     ivar;
   for (i = 0; i < nxforms; i++) {
      int j, k;
      cp->xform[i].density = 1.0 / nxforms;
      cp->xform[i].color = i == 0;
      for (j = 0; j < 3; j++)
	 for (k = 0; k < 2; k++)
	    cp->xform[i].c[j][k] = random_uniform11();
      for (j = 0; j < NVARS; j++)
	 cp->xform[i].var[j] = 0.0;
      if (var >= 0)
	 cp->xform[i].var[var] = 1.0;
      else
	 cp->xform[i].var[random_distrib(mixed_var_distrib)] = 1.0;

   }
   for (; i < NXFORMS; i++)
      cp->xform[i].density = 0.0;
}

/*
 * find a 2d bounding box that does not enclose eps of the fractal density
 * in each compass direction.  works by binary search.
 * this is stupid, it shouldjust use the find nth smallest algorithm.
 */
void estimate_bounding_box(cp, eps, bmin, bmax)
   control_point *cp;
   double eps;
   double *bmin;
   double *bmax;
{
   int i, j, batch = (eps == 0.0) ? 10000 : 10.0/eps;
   int low_target = batch * eps;
   int high_target = batch - low_target;
   point min, max, delta;
   point *points = (point *)  malloc(sizeof(point) * batch);
   iterate(cp, batch, 20, points);

   min[0] = min[1] =  1e10;
   max[0] = max[1] = -1e10;

   for (i = 0; i < batch; i++) {
      if (points[i][0] < min[0]) min[0] = points[i][0];
      if (points[i][1] < min[1]) min[1] = points[i][1];
      if (points[i][0] > max[0]) max[0] = points[i][0];
      if (points[i][1] > max[1]) max[1] = points[i][1];
   }

   if (low_target == 0) {
      bmin[0] = min[0];
      bmin[1] = min[1];
      bmax[0] = max[0];
      bmax[1] = max[1];
      return;
   }

   delta[0] = (max[0] - min[0]) * 0.25;
   delta[1] = (max[1] - min[1]) * 0.25;

   bmax[0] = bmin[0] = min[0] + 2.0 * delta[0];
   bmax[1] = bmin[1] = min[1] + 2.0 * delta[1];

   for (i = 0; i < 14; i++) {
      int n, s, e, w;
      n = s = e = w = 0;
      for (j = 0; j < batch; j++) {
	 if (points[j][0] < bmin[0]) n++;
	 if (points[j][0] > bmax[0]) s++;
	 if (points[j][1] < bmin[1]) w++;
	 if (points[j][1] > bmax[1]) e++;
      }
      bmin[0] += (n <  low_target) ? delta[0] : -delta[0];
      bmax[0] += (s < high_target) ? delta[0] : -delta[0];
      bmin[1] += (w <  low_target) ? delta[1] : -delta[1];
      bmax[1] += (e < high_target) ? delta[1] : -delta[1];
      delta[0] = delta[0] / 2.0;
      delta[1] = delta[1] / 2.0;
      /*
      fprintf(stderr, "%g %g %g %g\n", bmin[0], bmin[1], bmax[0], bmax[1]);
      */
   }
   /*
   fprintf(stderr, "%g %g %g %g\n", min[0], min[1], max[0], max[1]);
   */
}

/* use hill climberer to find smooth ordering of control points
   this is untested */

void sort_control_points(cps, ncps, metric)
   control_point *cps;
   int ncps;
   double (*metric)();
{
   int niter = ncps * 1000;
   int i, n, m;
   double same, swap;
   for (i = 0; i < niter; i++) {
      /* consider switching points with indexes n and m */
      n = g_random_int_range (0, ncps);
      m = g_random_int_range (0, ncps);

      same = (metric(cps + n, cps + (n - 1) % ncps) +
	      metric(cps + n, cps + (n + 1) % ncps) +
	      metric(cps + m, cps + (m - 1) % ncps) +
	      metric(cps + m, cps + (m + 1) % ncps));

      swap = (metric(cps + n, cps + (m - 1) % ncps) +
	      metric(cps + n, cps + (m + 1) % ncps) +
	      metric(cps + m, cps + (n - 1) % ncps) +
	      metric(cps + m, cps + (n + 1) % ncps));

      if (swap < same) {
	 control_point t;
	 t = cps[n];
	 cps[n] = cps[m];
	 cps[m] = t;
      }
   }
}

/* this has serious flaws in it */

double standard_metric(cp1, cp2)
   control_point *cp1, *cp2;
{
   int i, j, k;
   double t;

   double dist = 0.0;
   for (i = 0; i < NXFORMS; i++) {
      double var_dist = 0.0;
      double coef_dist = 0.0;
      for (j = 0; j < NVARS; j++) {
	 t = cp1->xform[i].var[j] - cp2->xform[i].var[j];
	 var_dist += t * t;
      }
      for (j = 0; j < 3; j++)
	 for (k = 0; k < 2; k++) {
	    t = cp1->xform[i].c[j][k] - cp2->xform[i].c[j][k];
	    coef_dist += t *t;
	 }

      /* weight them equally for now. */
      dist += var_dist + coef_dist;
   }
   return dist;
}

#if 0
static void
stat_matrix(f, m)
   FILE *f;
   double m[3][2];
{
   double r[2][2];
   double v[2][2];
   double a;

   diagonalize_matrix(m, r, v);
   fprintf(f, "entries = % 10f % 10f % 10f % 10f\n",
	   m[0][0], m[0][1], m[1][0], m[1][1]);
   fprintf(f, "evalues  = % 10f % 10f % 10f % 10f\n",
	   r[0][0], r[0][1], r[1][0], r[1][1]);
   fprintf(f, "evectors = % 10f % 10f % 10f % 10f\n",
	   v[0][0], v[0][1], v[1][0], v[1][1]);
   a = (v[0][0] * v[1][0] + v[0][1] * v[1][1]) /
      sqrt((v[0][0] * v[0][0] + v[0][1] * v[0][1]) *
	   (v[1][0] * v[1][0] + v[1][1] * v[1][1]));
   fprintf(f, "theta = %g det = %g\n", a,
	   m[0][0] * m[1][1] - m[0][1] * m[1][0]);
}
#endif


#if 0
main()
{
#if 0
   double m1[3][2] = {-0.633344, -0.269064, 0.0676171, 0.590923, 0, 0};
   double m2[3][2] = {-0.844863, 0.0270297, -0.905294, 0.413218, 0, 0};
#endif

#if 0
   double m1[3][2] = {-0.347001, -0.15219, 0.927161, 0.908305, 0, 0};
   double m2[3][2] = {-0.577884, 0.653803, 0.664982, -0.734136, 0, 0};
#endif

#if 0
   double m1[3][2] = {1, 0, 0, 1, 0, 0};
   double m2[3][2] = {0, -1, 1, 0, 0, 0};
#endif

#if 1
   double m1[3][2] = {1, 0, 0, 1, 0, 0};
   double m2[3][2] = {-1, 0, 0, -1, 0, 0};
#endif

   double m3[3][2];
   double t;
   int i = 0;

   for (t = 0.0; t <= 1.0; t += 1.0/15.0) {
      int x, y;
      fprintf(stderr, "%g--\n", t);
      interpolate_matrix(t, m1, m2, m3);
/*       stat_matrix(stderr, m3); */
      x = (i % 4) * 100 + 100;
      y = (i / 4) * 100 + 100;
      printf("newpath ");
      printf("%d %d %d %d %d arc ", x, y, 30, 0, 360);
      printf("%d %d moveto ", x, y);
      printf("%g %g rlineto ", m3[0][0] * 30, m3[0][1] * 30);
      printf("%d %d moveto ", x, y);
      printf("%g %g rlineto ", m3[1][0] * 30, m3[1][1] * 30);
      printf("stroke \n");
      printf("newpath ");
      printf("%g %g %d %d %d arc ", x + m3[0][0] * 30, y + m3[0][1] * 30, 3, 0, 360);
      printf("stroke \n");
      i++;
   }
}
#endif

static int flam3_random_bit(void)
{
   static int n = 0;
   static int l;
   if (0 == n) {
      l = random();
      n = 20;
   }
   else {
      l = l >> 1;
      n--;
   }
   return l & 1;
}

static double flam3_random01(void)
{
   return (random() & 0xfffffff) / (double) 0xfffffff;
}
