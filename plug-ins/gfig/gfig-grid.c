/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "gfig.h"
#include "gfig-grid.h"

#include "libgimp/stdplugins-intl.h"

/* For the isometric grid */
#define SQRT3 1.73205080756887729353   /* Square root of 3 */
#define SIN_1o6PI_RAD 0.5              /* Sine    1/6 Pi Radians */
#define COS_1o6PI_RAD SQRT3 / 2        /* Cosine  1/6 Pi Radians */
#define TAN_1o6PI_RAD 1 / SQRT3        /* Tangent 1/6 Pi Radians == SIN / COS */
#define RECIP_TAN_1o6PI_RAD SQRT3      /* Reciprocal of Tangent 1/6 Pi Radians */

static GdkGC *grid_hightlight_drawgc;
gint          grid_gc_type = GTK_STATE_NORMAL;

static void   draw_grid_polar  (GdkGC     *drawgc);
static void   draw_grid_sq     (GdkGC     *drawgc);
static void   draw_grid_iso    (GdkGC     *drawgc);

static GdkGC *gfig_get_grid_gc (GtkWidget *widget,
                                gint       gctype);
static gint   get_num_radials  (void);

/* find_grid_pos - Given an x, y point return the grid position of it */
/* return the new position in the passed point */

void
gfig_grid_colours (GtkWidget *widget)
{
  GdkColormap *colormap;
  GdkGCValues  values;
  GdkColor     col1;
  GdkColor     col2;
  guchar       stipple[8] =
  {
    0xAA,    /*  ####----  */
    0x55,    /*  ###----#  */
    0xAA,    /*  ##----##  */
    0x55,    /*  #----###  */
    0xAA,    /*  ----####  */
    0x55,    /*  ---####-  */
    0xAA,    /*  --####--  */
    0x55,    /*  -####---  */
  };

  colormap = gdk_screen_get_rgb_colormap (gtk_widget_get_screen (widget));

  gdk_color_parse ("gray50", &col1);
  gdk_colormap_alloc_color (colormap, &col1, FALSE, TRUE);

  gdk_color_parse ("gray80", &col2);
  gdk_colormap_alloc_color (colormap, &col2, FALSE, TRUE);

  values.background.pixel = col1.pixel;
  values.foreground.pixel = col2.pixel;
  values.fill    = GDK_OPAQUE_STIPPLED;
  values.stipple = gdk_bitmap_create_from_data (widget->window,
                                                (gchar *) stipple, 4, 4);
  grid_hightlight_drawgc = gdk_gc_new_with_values (widget->window,
                                                   &values,
                                                   GDK_GC_FOREGROUND |
                                                   GDK_GC_BACKGROUND |
                                                   GDK_GC_FILL       |
                                                   GDK_GC_STIPPLE);
}

void
find_grid_pos (GdkPoint *p,
               GdkPoint *gp,
               guint     is_butt3)
{
  gint16          x = p->x;
  gint16          y = p->y;
  static GdkPoint cons_pnt;
  static gdouble  cons_radius;
  static gdouble  cons_ang;
  static gboolean cons_center;

  if (selvals.opts.gridtype == RECT_GRID)
    {
      if (p->x % selvals.opts.gridspacing > selvals.opts.gridspacing/2)
        x += selvals.opts.gridspacing;

      if (p->y % selvals.opts.gridspacing > selvals.opts.gridspacing/2)
        y += selvals.opts.gridspacing;

      gp->x = (x/selvals.opts.gridspacing)*selvals.opts.gridspacing;
      gp->y = (y/selvals.opts.gridspacing)*selvals.opts.gridspacing;

      if (is_butt3)
        {
          if (abs (gp->x - cons_pnt.x) < abs (gp->y - cons_pnt.y))
            gp->x = cons_pnt.x;
          else
            gp->y = cons_pnt.y;
        }
      else
        {
          /* Store the point since might be used later */
          cons_pnt = *gp; /* Structure copy */
        }
    }
  else if (selvals.opts.gridtype == POLAR_GRID)
    {
      gdouble ang_grid;
      gdouble ang_radius;
      gdouble real_radius;
      gdouble real_angle;
      gdouble rounded_angle;
      gint    rounded_radius;
      gint16  shift_x = x - preview_width/2;
      gint16  shift_y = -y + preview_height/2;

      real_radius = ang_radius = sqrt ((shift_y*shift_y) + (shift_x*shift_x));

      /* round radius */
      rounded_radius = (gint)(RINT (ang_radius/selvals.opts.gridspacing))*selvals.opts.gridspacing;
      if (rounded_radius <= 0 || real_radius <=0)
        {
          /* DEAD CENTER */
          gp->x = preview_width/2;
          gp->y = preview_height/2;
          if (!is_butt3) cons_center = TRUE;
#ifdef DEBUG
          printf ("Dead center\n");
#endif /* DEBUG */
          return;
        }

      ang_grid = 2*G_PI/get_num_radials ();

      real_angle = atan2 (shift_y, shift_x);
      if (real_angle < 0)
        real_angle += 2*G_PI;

      rounded_angle = (RINT ((real_angle/ang_grid)))*ang_grid;

#ifdef DEBUG
      printf ("real_ang = %f ang_gid = %f rounded_angle = %f rounded radius = %d\n",
              real_angle, ang_grid, rounded_angle, rounded_radius);

      printf ("preview_width = %d preview_height = %d\n", preview_width, preview_height);
#endif /* DEBUG */

      gp->x = (gint)RINT ((rounded_radius*cos (rounded_angle))) + preview_width/2;
      gp->y = -(gint)RINT ((rounded_radius*sin (rounded_angle))) + preview_height/2;

      if (is_butt3)
        {
          if (!cons_center)
            {
              if (fabs (rounded_angle - cons_ang) > ang_grid/2)
                {
                  gp->x = (gint)RINT ((cons_radius*cos (rounded_angle))) + preview_width/2;
                  gp->y = -(gint)RINT ((cons_radius*sin (rounded_angle))) + preview_height/2;
                }
              else
                {
                  gp->x = (gint)RINT ((rounded_radius*cos (cons_ang))) + preview_width/2;
                  gp->y = -(gint)RINT ((rounded_radius*sin (cons_ang))) + preview_height/2;
                }
            }
        }
      else
        {
          cons_radius = rounded_radius;
          cons_ang = rounded_angle;
          cons_center = FALSE;
        }
    }
  else if (selvals.opts.gridtype == ISO_GRID)
    {
      /*
       * This really needs a picture to show the math...
       *
       * Consider an isometric grid with one of the sets of lines parallel to the
       * y axis (vertical alignment). Further define that the origin of a Cartesian
       * grid is at a isometric vertex.  For simplicity consider the first quadrant only.
       *
       *  - Let one line segment between vertices be r
       *  - Define the value of r as the grid spacing
       *  - Assign an integer n identifier to each vertical grid line along the x axis.
       *    with n=0 being the y axis. n can be any integer
       *  - Let m to be any integer
       *  - Let h be the spacing between vertical grid lines measured along the x axis.
       *    It follows from the isometric grid that h has a value of r * COS(1/6 Pi Rad)
       *
       *  Consider a Vertex V at the Cartesian location [Xv, Yv]
       *
       *   It follows that vertices belong to the set...
       *   V[Xv, Yv] = [ [ n * h ] ,
       *                 [ m * r + ( 0.5 * r (n % 2) ) ] ]
       *   for all integers n and m
       *
       * Who cares? Me. It's useful in solving this problem:
       * Consider an arbitrary point P[Xp,Yp], find the closest vertex in the set V.
       *
       * Restated this problem is "find values for m and n that are drive V closest to P"
       *
       * A Solution method (there may be a better one?):
       *
       * Step 1) bound n to the two closest values for Xp
       *         n_lo = (int) (Xp / h)
       *         n_hi = n_lo + 1
       *
       * Step 2) Consider the two closes vertices for each n_lo and n_hi. The further of
       *         the vertices in each pair can readily be discarded.
       *         m_lo_n_lo = (int) ( (Yp / r) - 0.5 (n_lo % 2) )
       *         m_hi_n_lo = m_lo_n_lo + 1
       *
       *         m_lo_n_hi = (int) ( (Yp / r) - 0.5 (n_hi % 2) )
       *         m_hi_n_hi = m_hi_n_hi
       *
       * Step 3) compute the distance from P to V1 and V2. Snap to the closer point.
       */
      gint n_lo;
      gint n_hi;
      gint m_lo_n_lo;
      gint m_hi_n_lo;
      gint m_lo_n_hi;
      gint m_hi_n_hi;
      gint m_n_lo;
      gint m_n_hi;
      gdouble r;
      gdouble h;
      gint x1;
      gint x2;
      gint y1;
      gint y2;

      r = selvals.opts.gridspacing;
      h = COS_1o6PI_RAD * r;

      n_lo = (gint) x / h;
      n_hi = n_lo + 1;

      /* evaluate m candidates for n_lo */
      m_lo_n_lo = (gint) ( (y / r) - 0.5 * (n_lo % 2) );
      m_hi_n_lo = m_lo_n_lo + 1;
      /* figure out which is the better candidate */
      if (abs((m_lo_n_lo * r + (0.5 * r * (n_lo % 2))) - y) <
          abs((m_hi_n_lo * r + (0.5 * r * (n_lo % 2))) - y)) {
        m_n_lo = m_lo_n_lo;
      }
      else {
        m_n_lo = m_hi_n_lo;
      }

      /* evaluate m candidates for n_hi */
      m_lo_n_hi = (gint) ( (y / r) - 0.5 * (n_hi % 2) );
      m_hi_n_hi = m_lo_n_hi + 1;
      /* figure out which is the better candidate */
      if (abs((m_lo_n_hi * r + (0.5 * r * (n_hi % 2))) - y) <
          abs((m_hi_n_hi * r + (0.5 * r * (n_hi % 2))) - y)) {
        m_n_hi = m_lo_n_hi;
      }
      else {
        m_n_hi = m_hi_n_hi;
      }

      /* Now, which is closer to [x,y]? we can use a somewhat abbreviated form of the
       * distance formula since we only care about relative values. */

      x1 = (gint) (n_lo * h);
      y1 = (gint) (m_n_lo * r + (0.5 * r * (n_lo % 2)));
      x2 = (gint) (n_hi * h);
      y2 = (gint) (m_n_hi * r + (0.5 * r * (n_hi % 2)));

      if (((x - x1) * (x - x1) + (y - y1) * (y - y1)) <
          ((x - x2) * (x - x2) + (y - y2) * (y - y2))) {
        gp->x =  x1;
        gp->y =  y1;
      }
      else {
        gp->x =  x2;
        gp->y =  y2;
      }

    }
}

static void
draw_grid_polar (GdkGC *drawgc)
{
  gint    step;
  gint    loop;
  gint    radius;
  gint    max_rad;
  gdouble ang_grid;
  gdouble ang_loop;
  gdouble ang_radius;
  /* Pick center and draw concentric circles */

  gint grid_x_center = preview_width/2;
  gint grid_y_center = preview_height/2;

  step = selvals.opts.gridspacing;
  max_rad = sqrt (preview_width * preview_width +
                  preview_height * preview_height) / 2;

  for (loop = 0; loop < max_rad; loop += step)
    {
      radius = loop;

      gdk_draw_arc (gfig_context->preview->window,
                    drawgc,
                    0,
                    grid_x_center - radius,
                    grid_y_center - radius,
                    radius*2,
                    radius*2,
                    0,
                    360 * 64);
    }

  /* Lines */
  ang_grid = 2 * G_PI / get_num_radials ();
  ang_radius = sqrt ((preview_width * preview_width) +
                     (preview_height * preview_height)) / 2;

  for (loop = 0; loop <= get_num_radials (); loop++)
    {
      gint lx, ly;

      ang_loop = loop * ang_grid;

      lx = RINT (ang_radius * cos (ang_loop));
      ly = RINT (ang_radius * sin (ang_loop));

      gdk_draw_line (gfig_context->preview->window,
                     drawgc,
                     lx + (preview_width) / 2,
                     - ly + (preview_height) / 2,
                     (preview_width) / 2,
                     (preview_height) / 2);
    }
}

static void
draw_grid_sq (GdkGC *drawgc)
{
  gint step;
  gint loop;

  /* Draw the horizontal lines */
  step = selvals.opts.gridspacing;

  for (loop = 0 ; loop < preview_height ; loop += step)
    {
      gdk_draw_line (gfig_context->preview->window,
                     drawgc,
                     0,
                     loop,
                     preview_width,
                     loop);
    }

  /* Draw the vertical lines */

  for (loop = 0 ; loop < preview_width ; loop += step)
    {
      gdk_draw_line (gfig_context->preview->window,
                     drawgc,
                     loop,
                     0,
                     loop,
                     preview_height);
    }
}

static void
draw_grid_iso (GdkGC *drawgc)
{
  /* vstep is an int since it's defined from grid size */
  gint    vstep;
  gdouble loop;
  gdouble hstep;

  gdouble diagonal_start;
  gdouble diagonal_end;
  gdouble diagonal_width;
  gdouble diagonal_height;

  vstep = selvals.opts.gridspacing;
  hstep = selvals.opts.gridspacing * COS_1o6PI_RAD;

  /* Draw the vertical lines - These are easy */
  for (loop = 0 ; loop < preview_width ; loop += hstep){
    gdk_draw_line (gfig_context->preview->window,
                   drawgc,
                   (gint)loop,
                   (gint)0,
                   (gint)loop,
                   (gint)preview_height);
  }

  /* draw diag lines at a Theta of +/- 1/6 Pi Rad */

  diagonal_start = -(((int)preview_width * TAN_1o6PI_RAD) - (((int)(preview_width * TAN_1o6PI_RAD)) % vstep));

  diagonal_end = preview_height + (preview_width * TAN_1o6PI_RAD);
  diagonal_end -= ((int)diagonal_end) % vstep;

  diagonal_width = preview_width;
  diagonal_height = preview_width * TAN_1o6PI_RAD;

  /* Draw diag lines */
  for (loop = diagonal_start ; loop < diagonal_end ; loop += vstep)
    {
      gdk_draw_line (gfig_context->preview->window,
                      drawgc,
                     (gint)0,
                     (gint)loop,
                     (gint)diagonal_width,
                     (gint)loop + diagonal_height);

      gdk_draw_line (gfig_context->preview->window,
                     drawgc,
                     (gint)0,
                     (gint)loop,
                     (gint)diagonal_width,
                     (gint)loop - diagonal_height);
    }
}

static GdkGC *
gfig_get_grid_gc (GtkWidget *w, gint gctype)
{
  switch (gctype)
    {
    case GFIG_BLACK_GC:
      return (w->style->black_gc);
    case GFIG_WHITE_GC:
      return (w->style->white_gc);
    case GFIG_GREY_GC:
      return (grid_hightlight_drawgc);
    case GTK_STATE_NORMAL:
      return (w->style->bg_gc[GTK_STATE_NORMAL]);
    case GTK_STATE_ACTIVE:
      return (w->style->bg_gc[GTK_STATE_ACTIVE]);
    case GTK_STATE_PRELIGHT:
      return (w->style->bg_gc[GTK_STATE_PRELIGHT]);
    case GTK_STATE_SELECTED:
      return (w->style->bg_gc[GTK_STATE_SELECTED]);
    case GTK_STATE_INSENSITIVE:
      return (w->style->bg_gc[GTK_STATE_INSENSITIVE]);
    default:
      g_warning ("Unknown type for grid colouring\n");
      return (w->style->bg_gc[GTK_STATE_PRELIGHT]);
    }
}

void
draw_grid (void)
{
  GdkGC *drawgc;
  /* Get the size of the preview and calc where the lines go */
  /* Draw in prelight to start with... */
  /* Always start in the upper left corner for rect.
   */

  if ((preview_width < selvals.opts.gridspacing &&
       preview_height < selvals.opts.gridspacing))
    {
      /* Don't draw if they don't fit */
      return;
    }

  if (selvals.opts.drawgrid)
    drawgc = gfig_get_grid_gc (gfig_context->preview, grid_gc_type);
  else
    return;

  if (selvals.opts.gridtype == RECT_GRID)
    draw_grid_sq (drawgc);
  else if (selvals.opts.gridtype == POLAR_GRID)
    draw_grid_polar (drawgc);
  else if (selvals.opts.gridtype == ISO_GRID)
    draw_grid_iso (drawgc);
}

static gint
get_num_radials (void)
{
  gint gridsp = MAX_GRID + MIN_GRID;
  /* select number of radials to draw */
  /* Either have 16 32 or 48 */
  /* correspond to GRID_MAX, midway and GRID_MIN */

  return gridsp - selvals.opts.gridspacing;
}
