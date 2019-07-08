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

#ifndef __PPM_TOOL_H
#define __PPM_TOOL_H

#include <glib.h>

typedef struct ppm {
  int width;
  int height;
  unsigned char *col;
} ppm_t;

void fatal(char *s);
void ppm_kill(ppm_t *p);
void ppm_new(ppm_t *p, int xs, int ys);
void get_rgb(ppm_t *s, float xo, float yo, unsigned char *d);
void resize(ppm_t *p, int nx, int ny);
void rescale(ppm_t *p, double scale);
void resize_fast(ppm_t *p, int nx, int ny);
void ppm_load(const char *fn, ppm_t *p);
void ppm_save(ppm_t *p, const char *fn);
void ppm_copy(ppm_t *s, ppm_t *p);
void fill(ppm_t *p, guchar *c);
void free_rotate(ppm_t *p, double amount);
void ppm_pad(ppm_t *p, int left,int right, int top, int bottom, guchar *);
void edgepad(ppm_t *p, int left,int right, int top, int bottom);
void autocrop(ppm_t *p, int room);
void crop(ppm_t *p, int lx, int ly, int hx, int hy);
void ppm_apply_gamma(ppm_t *p, float e, int r, int g, int b);
void ppm_apply_brightness(ppm_t *p, float e, int r, int g, int b);
void ppm_put_rgb_fast(ppm_t *s, float xo, float yo, guchar *d);
void ppm_put_rgb(ppm_t *s, float xo, float yo, guchar *d);
void ppm_drawline(ppm_t *p, float fx, float fy, float tx, float ty, guchar *col);

void repaint(ppm_t *p, ppm_t *a);

void blur(ppm_t *p, int xrad, int yrad);

void mkgrayplasma(ppm_t *p, float turb);

#define PPM_IS_INITED(ppm_ptr) ((ppm_ptr)->col != NULL)

#endif /* #ifndef __PPM_TOOL_H */

