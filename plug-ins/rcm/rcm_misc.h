/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
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

/*-----------------------------------------------------------------------------------
 * Change log:
 * 
 * Version 2.0, 04 April 1999.
 *  Nearly complete rewrite, made plug-in stable.
 *  (Works with GIMP 1.1 and GTK+ 1.2)
 *
 * Version 1.0, 27 March 1997.
 *  Initial (unstable) release by Pavel Grinfeld
 *
 *-----------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------*/
/* Global defines */
/*-----------------------------------------------------------------------------------*/

#define sqr(X)    ((X)*(X))
#define SWAP(X,Y) {float t=X; X=Y; Y=t;}

/*-----------------------------------------------------------------------------------*/
/* used in 'rcm_callback.c' and 'rcm_dialog.c' */
/*-----------------------------------------------------------------------------------*/

float arctg(float y, float x);

float min_prox(float alpha, float beta, float angle);

float *closest(float *alpha, float *beta, float  angle);

float angle_mod_2PI(float angle);

ReducedImage *rcm_reduce_image(GDrawable *, GDrawable *, gint, gint);

void rcm_render_preview(GtkWidget *, gint);

void rcm_render_circle(GtkWidget *preview, int sum, int margin);

/*-----------------------------------------------------------------------------------*/
/* only used in 'rcm.c' (or local) */
/*-----------------------------------------------------------------------------------*/

void rgb_to_hsv (hsv r, hsv g, hsv b, hsv *h, hsv *s, hsv *l);

void hsv_to_rgb (hsv h, hsv sl, hsv l, hsv *r, hsv *g, hsv *b);

float rcm_angle_inside_slice(float angle, RcmAngle *slice);

gint rcm_is_gray(float s);

float rcm_linear(float, float, float, float, float);

float rcm_left_end(RcmAngle *angle);

float rcm_right_end(RcmAngle *angle);

/*-----------------------------------------------------------------------------------*/
