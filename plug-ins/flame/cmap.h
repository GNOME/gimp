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


#ifndef cmap_included
#define cmap_included

#define cmap_random (-1)



#define vlen(x) (sizeof(x)/sizeof(*x))

typedef double clrmap[256][3];

extern void rgb2hsv(double *rgb, double *hsv);
extern void hsv2rgb(double *hsv, double *rgb);
extern int get_cmap(int n, clrmap c, int cmap_len);

#endif
