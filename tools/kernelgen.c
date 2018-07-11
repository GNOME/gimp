/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * kernelgen -- Copyright (C) 2000 Sven Neumann <sven@gimp.org>
 *
 *    Simple hack to create brush subsampling kernels.  If you want to
 *    play with it, change some of the #defines at the top and copy
 *    the output to app/paint/gimpbrushcore-kernels.h.
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

#include <math.h>
#include <stdio.h>
#include <string.h>


#define STEPS          64
#define KERNEL_WIDTH    3     /*  changing these makes no sense  */
#define KERNEL_HEIGHT   3     /*  changing these makes no sense  */
#define KERNEL_SUM    256
#define SUBSAMPLE       4
#define THRESHOLD       0.25  /*  try to change this one         */

#define SQR(x) ((x) * (x))

static void
create_kernel (double x,
               double y)
{
  double value[KERNEL_WIDTH][KERNEL_HEIGHT];
  double dist_x;
  double dist_y;
  double sum;
  double w;
  int i, j;

  memset (value, 0, KERNEL_WIDTH * KERNEL_HEIGHT * sizeof (double));
  sum = 0.0;

  x += 1.0;
  y += 1.0;

  for (j = 0; j < STEPS * KERNEL_HEIGHT; j++)
    {
      dist_y = y - (((double)j + 0.5) / (double)STEPS);

      for (i = 0; i < STEPS * KERNEL_WIDTH; i++)
        {
          dist_x = x - (((double) i + 0.5) / (double) STEPS);

          /*  I've tried to use a gauss function here instead of a
              threshold, but the result was not that impressive.    */
          w = (SQR (dist_x) + SQR (dist_y)) < THRESHOLD ? 1.0 : 0.0;

          value[i / STEPS][j / STEPS] += w;
          sum += w;
        }
    }

  for (j = 0; j < KERNEL_HEIGHT; j++)
    {
      for (i = 0; i < KERNEL_WIDTH; i++)
        {
          w = (double) KERNEL_SUM * value[i][j] / sum;
          printf (" %3d,", (int) (w + 0.5));
        }
    }
}

int
main (int    argc,
      char **argv)
{
  int    i, j;
  double x, y;

  printf ("/* gimpbrushcore-kernels.h\n"
          " *\n"
          " *   This file was generated using kernelgen as found in the tools dir.\n");
  printf (" *   (threshold = %g)\n", THRESHOLD);
  printf (" */\n\n");
  printf ("#ifndef __GIMP_BRUSH_CORE_KERNELS_H__\n");
  printf ("#define __GIMP_BRUSH_CORE_KERNELS_H__\n\n");
  printf ("#define KERNEL_WIDTH     %d\n", KERNEL_WIDTH);
  printf ("#define KERNEL_HEIGHT    %d\n", KERNEL_HEIGHT);
  printf ("#define KERNEL_SUBSAMPLE %d\n", SUBSAMPLE);
  printf ("#define KERNEL_SUM       %d\n", KERNEL_SUM);
  printf ("\n\n");
  printf ("/*  Brush pixel subsampling kernels  */\n");
  printf ("static const int subsample[%d][%d][%d] =\n{\n",
          SUBSAMPLE + 1, SUBSAMPLE + 1, KERNEL_WIDTH * KERNEL_HEIGHT);

  for (j = 0; j <= SUBSAMPLE; j++)
    {
      y = (double) j / (double) SUBSAMPLE;

      printf ("  {\n");

      for (i = 0; i <= SUBSAMPLE; i++)
        {
          x = (double) i / (double) SUBSAMPLE;

          printf ("    {");
          create_kernel (x, y);
          printf (" }%s", i < SUBSAMPLE ? ",\n" : "\n");
        }

      printf ("  }%s", j < SUBSAMPLE ? ",\n" : "\n");
    }

  printf ("};\n\n");

  printf ("#endif /* __GIMP_BRUSH_CORE_KERNELS_H__ */\n");

  return 0;
}
