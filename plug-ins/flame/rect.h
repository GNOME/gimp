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

#include "libifs.h"


/* size of the cmap actually used. may be smaller than input cmap size */
#define CMAP_SIZE 256

typedef struct {
   double        temporal_filter_radius;
   control_point *cps;
   int           ncps;
   double        time;
} frame_spec;


#define field_both  0
#define field_even  1
#define field_odd   2


extern void render_rectangle(frame_spec *spec, unsigned char *out,
                             int out_width, int field, int nchan,
                             int progress(double));

