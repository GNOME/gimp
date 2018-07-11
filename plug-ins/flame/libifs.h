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


#ifndef libifs_included
#define libifs_included

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "cmap.h"

#define EPS (1e-10)

#define variation_random (-1)

#define NVARS   29
#define NXFORMS 6

typedef double point[3];

typedef struct {
   double var[NVARS];   /* normalized interp coefs between variations */
   double c[3][2];      /* the coefs to the affine part of the function */
   double density;      /* prob is this function is chosen. 0 - 1 */
   double color;        /* color coord for this function. 0 - 1 */
} xform;

typedef struct {
   xform xform[NXFORMS];
   clrmap cmap;
   double time;
   int  cmap_index;
   double brightness;           /* 1.0 = normal */
   double contrast;             /* 1.0 = normal */
   double gamma;
   int  width, height;          /* of the final image */
   int  spatial_oversample;
   double center[2];             /* camera center */
   double zoom;                  /* effects ppu and sample density */
   double pixels_per_unit;       /* and scale */
   double spatial_filter_radius; /* variance of gaussian */
   double sample_density;        /* samples per pixel (not bucket) */
   /* in order to motion blur more accurately we compute the logs of the
      sample density many times and average the results.  we interpolate
      only this many times. */
   int nbatches;
   /* this much color resolution.  but making it too high induces clipping */
   int white_level;
   int cmap_inter; /* if this is true, then color map interpolates one entry
		      at a time with a bright edge */
   double pulse[2][2]; /* [i][0]=magnitude [i][1]=frequency */
   double wiggle[2][2]; /* frequency is /minute, assuming 30 frames/s */
} control_point;



extern void iterate(control_point *cp, int n, int fuse, point points[]);
extern void interpolate(control_point cps[], int ncps, double time, control_point *result);
extern void tokenize(char **ss, char *argv[], int *argc);
extern void print_control_point(FILE *f, control_point *cp, int quote);
extern void random_control_point(control_point *cp, int ivar);
extern void parse_control_point(char **ss, control_point *cp);
extern void estimate_bounding_box(control_point *cp, double eps, double *bmin, double *bmax);
extern double standard_metric(control_point *cp1, control_point *cp2);
extern double random_uniform01(void);
extern double random_uniform11(void);
extern double random_gaussian(void);
extern void mult_matrix(double s1[2][2], double s2[2][2], double d[2][2]);
void copy_variation(control_point *cp0, control_point *cp1);
#endif
