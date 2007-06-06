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
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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

#include "rcm.h"
#include "rcm_misc.h"
#include "rcm_dialog.h"
#include "rcm_gdk.h"


/* Global variables */

GdkGC *xor_gc;


/* Drawing routines */

void
rcm_draw_little_circle (GdkWindow *window,
			GdkGC     *color,
			gfloat     hue,
			gfloat     satur)
{
  gint x,y;

  x = GRAY_CENTER + GRAY_RADIUS * satur * cos(hue);
  y = GRAY_CENTER - GRAY_RADIUS * satur * sin(hue);

  gdk_draw_arc (window, color, 0, x-LITTLE_RADIUS, y-LITTLE_RADIUS,
                2*LITTLE_RADIUS, 2*LITTLE_RADIUS, 0, 360*64);
}

void
rcm_draw_large_circle (GdkWindow *window,
		       GdkGC     *color,
		       gfloat     gray_sat)
{
  gint x, y;

  x = GRAY_CENTER;
  y = GRAY_CENTER;

  gdk_draw_arc (window, color, 0,
                ROUND (x - GRAY_RADIUS * gray_sat),
                ROUND (y - GRAY_RADIUS * gray_sat),
                ROUND (2 * GRAY_RADIUS * gray_sat),
                ROUND (2 * GRAY_RADIUS * gray_sat),
                0, 360 * 64);
}


#define REL .8
#define DEL .1
#define TICK 10

void
rcm_draw_arrows (GdkWindow *window,
		 GdkGC     *color,
		 RcmAngle  *angle)
{
  gint   dist;
  gfloat alpha, beta, cw_ccw, delta;

  alpha  = angle->alpha;
  beta   = angle->beta;
  cw_ccw = angle->cw_ccw;
  delta  = angle_mod_2PI(beta - alpha);

  if (cw_ccw == -1)
    delta = delta - TP;

  gdk_draw_line (window,color,
                 CENTER,
                 CENTER,
                 ROUND (CENTER + RADIUS * cos(alpha)),
                 ROUND (CENTER - RADIUS * sin(alpha)));

  gdk_draw_line( window,color,
                 CENTER + RADIUS * cos(alpha),
                 CENTER - RADIUS * sin(alpha),
                 ROUND (CENTER + RADIUS * REL * cos(alpha - DEL)),
                 ROUND (CENTER - RADIUS * REL * sin(alpha - DEL)));

  gdk_draw_line (window,color,
                 CENTER + RADIUS * cos(alpha),
                 CENTER - RADIUS * sin(alpha),
                 ROUND (CENTER + RADIUS * REL * cos(alpha + DEL)),
                 ROUND (CENTER - RADIUS * REL * sin(alpha + DEL)));

  gdk_draw_line (window,color,
                 CENTER,
                 CENTER,
                 ROUND (CENTER + RADIUS * cos(beta)),
                 ROUND (CENTER - RADIUS * sin(beta)));

  gdk_draw_line (window,color,
                 CENTER + RADIUS * cos(beta),
                 CENTER - RADIUS * sin(beta),
                 ROUND (CENTER + RADIUS * REL * cos(beta - DEL)),
                 ROUND (CENTER - RADIUS * REL * sin(beta - DEL)));

  gdk_draw_line (window,color,
                 CENTER + RADIUS * cos(beta),
                 CENTER - RADIUS * sin(beta),
                 ROUND (CENTER + RADIUS * REL * cos(beta + DEL)),
                 ROUND (CENTER - RADIUS * REL * sin(beta + DEL)));

  dist   = RADIUS * EACH_OR_BOTH;

  gdk_draw_line (window,color,
                 CENTER + dist * cos(beta),
                 CENTER - dist * sin(beta),
                 ROUND (CENTER + dist * cos(beta) + cw_ccw * TICK * sin(beta)),
                 ROUND (CENTER - dist * sin(beta) + cw_ccw * TICK * cos(beta)));

  alpha *= 180 * 64 / G_PI;
  delta *= 180 * 64 / G_PI;

  gdk_draw_arc (window, color, 0, CENTER - dist, CENTER - dist,
		2*dist, 2*dist,	alpha, delta);
}
