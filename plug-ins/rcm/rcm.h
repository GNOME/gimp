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


/* Global defines */

#define PLUG_IN_PROC   "plug-in-rotate-colormap"
#define PLUG_IN_BINARY "rcm"
#define TP             (2*G_PI)


/* Typedefs */

enum { ENTIRE_IMAGE, SELECTION, SELECTION_IN_CONTEXT, PREVIEW_OPTIONS };

enum { EACH, BOTH, DEGREES, RADIANS, RADIANS_OVER_PI,
       GRAY_FROM, GRAY_TO, CURRENT, ORIGINAL };

typedef enum { VIRGIN, DRAG_START, DRAGING, DO_NOTHING } RcmOp;

typedef struct
{
  gfloat   alpha;
  gfloat   beta;
  gint     cw_ccw;
} RcmAngle;

typedef struct
{
  gint     width;
  gint     height;
  guchar  *rgb;
  gdouble *hsv;
  guchar  *mask;
} ReducedImage;

typedef struct
{
  GtkWidget  *preview;
  GtkWidget  *frame;
  GtkWidget  *table;
  GtkWidget  *cw_ccw_button;
  GtkWidget  *cw_ccw_box;
  GtkWidget  *cw_ccw_label;
  GtkWidget  *cw_ccw_pixmap;
  GtkWidget  *a_b_button;
  GtkWidget  *a_b_box;
  GtkWidget  *a_b_pixmap;
  GtkWidget  *f360_button;
  GtkWidget  *f360_box;
  GtkWidget  *f360_pixmap;
  GtkWidget  *alpha_entry;
  GtkWidget  *alpha_units_label;
  GtkWidget  *beta_entry;
  GtkWidget  *beta_units_label;
  gfloat     *target;
  gint        mode;
  RcmAngle   *angle;
  RcmOp       action_flag;
  gfloat      prev_clicked;
} RcmCircle;

typedef struct
{
  GtkWidget  *dlg;
  GtkWidget  *bna_frame;
  GtkWidget  *before;
  GtkWidget  *after;
} RcmBna;

typedef struct
{
  GtkWidget  *preview;
  GtkWidget  *frame;
  gfloat      gray_sat;
  gfloat      hue;
  gfloat      satur;
  GtkWidget  *hue_entry;
  GtkWidget  *hue_units_label;
  GtkWidget  *satur_entry;
  RcmOp       action_flag;
} RcmGray;

typedef struct
{
  gint           Slctn;
  gint           RealTime;
  gint           Units;
  gint           Gray_to_from;
  GimpDrawable  *drawable;
  GimpDrawable  *mask;
  ReducedImage  *reduced;
  RcmCircle     *To;
  RcmCircle     *From;
  RcmGray       *Gray;
  RcmBna        *Bna;
} RcmParams;


/* Global variables */

extern RcmParams Current;
