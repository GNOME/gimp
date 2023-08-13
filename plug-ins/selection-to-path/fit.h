/* fit.h: convert the pixel representation to splines.
 *
 * Copyright (C) 1992 Free Software Foundation, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FIT_H
#define FIT_H

#include "libgimp/gimp.h"

#include "pxl-outline.h"
#include "spline.h"


/* See fit.c for descriptions of these variables, all of which can be
   set using options.  */
extern real align_threshold;
extern real corner_always_threshold;
extern unsigned corner_surround;
extern real corner_threshold;
extern real error_threshold;
extern unsigned filter_alternative_surround;
extern real filter_epsilon;
extern unsigned filter_iteration_count;
extern real filter_percent;
extern unsigned filter_surround;
extern boolean keep_knees;
extern real line_reversion_threshold;
extern real line_threshold;
extern real reparameterize_improvement;
extern real reparameterize_threshold;
extern real subdivide_search;
extern unsigned subdivide_surround;
extern real subdivide_threshold;
extern unsigned tangent_surround;


/* Fit splines and lines to LIST.  */
extern spline_list_array_type fitted_splines (pixel_outline_list_type  list);
void   fit_set_params                        (GimpProcedureConfig     *config);

#endif /* not FIT_H */
