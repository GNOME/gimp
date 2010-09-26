/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Colormap-Rotation plug-in. Exchanges two color ranges.
 *
 * Copyright (C) 1999 Sven Anders (anderss@fmi.uni-passau.de)
 *                    Based on code from Pavel Grinfeld (pavel@ml.com)
 *
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*----------------------------------------------------------------------------
 * Change log:
 *
 * Version 2.0, 04 April 1999.
 *  Nearly complete rewrite, made plug-in stable.
 *  (Works with GIMP 1.1 and GTK+ 1.2)
 *
 * Version 1.0, 27 March 1997.
 *  Initial (unstable) release by Pavel Grinfeld
 *
 *----------------------------------------------------------------------------*/

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "color-rotate.h"
#include "color-rotate-utils.h"
#include "color-rotate-dialog.h"
#include "color-rotate-draw.h"


/* Drawing routines */

void
color_rotate_draw_little_circle (cairo_t *cr,
                                 gfloat   hue,
                                 gfloat   satur)
{
  gint x, y;

  x = GRAY_CENTER + GRAY_RADIUS * satur * cos(hue);
  y = GRAY_CENTER - GRAY_RADIUS * satur * sin(hue);

  cairo_new_sub_path (cr);
  cairo_arc (cr, x, y, LITTLE_RADIUS, 0, 2 * G_PI);

  cairo_set_line_width (cr, 3.0);
  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
  cairo_stroke_preserve (cr);

  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
  cairo_stroke (cr);
}

void
color_rotate_draw_large_circle (cairo_t *cr,
                                gfloat   gray_sat)
{
  gint x, y;

  x = GRAY_CENTER;
  y = GRAY_CENTER;

  cairo_new_sub_path (cr);
  cairo_arc (cr, x, y, GRAY_RADIUS * gray_sat, 0, 2 * G_PI);

  cairo_set_line_width (cr, 3.0);
  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
  cairo_stroke_preserve (cr);

  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
  cairo_stroke (cr);
}


#define REL  0.8
#define DEL  0.1
#define TICK 10

void
color_rotate_draw_arrows (cairo_t  *cr,
                          RcmAngle *angle)
{
  gint   dist;
  gfloat alpha, beta, cw_ccw, delta;

  alpha  = angle->alpha;
  beta   = angle->beta;
  cw_ccw = angle->cw_ccw;
  delta  = angle_mod_2PI (beta - alpha);

  if (cw_ccw == -1)
    delta = delta - TP;

  cairo_move_to (cr, CENTER, CENTER);
  cairo_line_to (cr,
                 ROUND (CENTER + RADIUS * cos (alpha)),
                 ROUND (CENTER - RADIUS * sin (alpha)));

  cairo_move_to (cr,
                 CENTER + RADIUS * cos (alpha),
                 CENTER - RADIUS * sin (alpha));
  cairo_line_to (cr,
                 ROUND (CENTER + RADIUS * REL * cos (alpha - DEL)),
                 ROUND (CENTER - RADIUS * REL * sin (alpha - DEL)));

  cairo_move_to (cr,
                 CENTER + RADIUS * cos (alpha),
                 CENTER - RADIUS * sin (alpha));
  cairo_line_to (cr,
                 ROUND (CENTER + RADIUS * REL * cos (alpha + DEL)),
                 ROUND (CENTER - RADIUS * REL * sin (alpha + DEL)));

  cairo_move_to (cr,
                 CENTER,
                 CENTER);
  cairo_line_to (cr,
                 ROUND (CENTER + RADIUS * cos (beta)),
                 ROUND (CENTER - RADIUS * sin (beta)));

  cairo_move_to (cr,
                 CENTER + RADIUS * cos (beta),
                 CENTER - RADIUS * sin (beta));
  cairo_line_to (cr,
                 ROUND (CENTER + RADIUS * REL * cos (beta - DEL)),
                 ROUND (CENTER - RADIUS * REL * sin (beta - DEL)));

  cairo_move_to (cr,
                 CENTER + RADIUS * cos (beta),
                 CENTER - RADIUS * sin (beta));
  cairo_line_to (cr,
                 ROUND (CENTER + RADIUS * REL * cos (beta + DEL)),
                 ROUND (CENTER - RADIUS * REL * sin (beta + DEL)));

  dist = RADIUS * EACH_OR_BOTH;

  cairo_move_to (cr,
                 CENTER + dist * cos (beta),
                 CENTER - dist * sin (beta));
  cairo_line_to (cr,
                 ROUND (CENTER + dist * cos(beta) + cw_ccw * TICK * sin (beta)),
                 ROUND (CENTER - dist * sin(beta) + cw_ccw * TICK * cos (beta)));

  cairo_new_sub_path (cr);

  if (cw_ccw > 0)
    {
      cairo_arc_negative (cr,
                          CENTER,
                          CENTER,
                          dist,
                          -alpha,
                          -beta);
    }
  else
    {
      cairo_arc (cr,
                 CENTER,
                 CENTER,
                 dist,
                 -alpha,
                 -beta);
    }

  cairo_set_line_width (cr, 3.0);
  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
  cairo_stroke_preserve (cr);

  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
  cairo_stroke (cr);
}
