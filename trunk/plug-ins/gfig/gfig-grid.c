/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
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

#include <stdlib.h>

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

static GdkGC  *grid_hightlight_drawgc = NULL;
gint           grid_gc_type           = GTK_STATE_NORMAL;

static void    draw_grid_polar     (GdkGC     *drawgc);
static void    draw_grid_sq        (GdkGC     *drawgc);
static void    draw_grid_iso       (GdkGC     *drawgc);

static GdkGC * gfig_get_grid_gc    (GtkWidget *widget,
                                    gint       gctype);

static void    find_grid_pos_polar (GdkPoint  *p,
                                    GdkPoint  *gp);


/********** PrimeFactors for Shaneyfelt-style Polar Grid **********
 * Quickly factor any number up to 17160
 * Correctly factors numbers up to 131 * 131 - 1
 */
typedef struct
{
  gint product;
  gint remaining;
  gint current;
  gint next;
  gint index;
} PrimeFactors;

static gchar primes[] = { 2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,53,
                          59,61,67,71,73,79,83,89,97,101,103,107,109,113,127 };

#define PRIMES_MAX_INDEX 30


static gint
prime_factors_get (PrimeFactors *this)
{
  this->current = this->next;
  while (this->index <= PRIMES_MAX_INDEX)
    {
      if (this->remaining % primes[this->index] == 0)   /* divisible */
        {
          this->remaining /= primes[this->index];
          this->next = primes[this->index];
          return this->current;
        }
      this->index++;
    }
  this->next = this->remaining;
  this->remaining = 1;

  return this->current;
}

static gint
prime_factors_lookahead (PrimeFactors *this)
{
  return this->next;
}

static void
prime_factors_reset (PrimeFactors *this)
{
  this->remaining = this->product;
  this->index = 0;
  prime_factors_get (this);
}

static PrimeFactors *
prime_factors_new (gint n)
{
  PrimeFactors *this = g_new (PrimeFactors, 1);

  this->product = n;
  prime_factors_reset (this);

  return this;
}

static void
prime_factors_delete (PrimeFactors* this)
{
  g_free (this);
}

/********** ********** **********/

static gdouble
sector_size_at_radius (gdouble inner_radius)
{
  PrimeFactors *factors = prime_factors_new (selvals.opts.grid_sectors_desired);
  gint          current_sectors = 1;
  gdouble       sector_size     = 2 * G_PI / current_sectors;

  while ((current_sectors < selvals.opts.grid_sectors_desired)
         && (inner_radius*sector_size
             > (prime_factors_lookahead (factors) *
                selvals.opts.grid_granularity)))
    {
      current_sectors *= prime_factors_get (factors);
      sector_size = 2 * G_PI / current_sectors;
    }

  prime_factors_delete(factors);

  return sector_size;
}

static void
find_grid_pos_polar (GdkPoint *p,
                     GdkPoint *gp)
{
  gdouble cx = preview_width / 2.0;
  gdouble cy = preview_height / 2.0;
  gdouble px = p->x - cx;
  gdouble py = p->y - cy;
  gdouble x  = 0;
  gdouble y  = 0;
  gdouble r  = sqrt (SQR (px) + SQR (py));

  if (r >= selvals.opts.grid_radius_min * 0.5)
    {
      gdouble t;
      gdouble sectorSize;

      r = selvals.opts.grid_radius_interval
        * (gint) (0.5 + ((r - selvals.opts.grid_radius_min) /
                         selvals.opts.grid_radius_interval))
        + selvals.opts.grid_radius_min;

      t = atan2 (py, px) + 2 * G_PI;
      sectorSize = sector_size_at_radius (r);
      t = selvals.opts.grid_rotation
        + (gint) (0.5 + ((t - selvals.opts.grid_rotation) / sectorSize))
        * sectorSize;
      x = r * cos (t);
      y = r * sin (t);
    }

  gp->x = x + cx;
  gp->y = y + cy;
}

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
      find_grid_pos_polar (p,gp);
    }
  else if (selvals.opts.gridtype == ISO_GRID)
    {
      /*
       * This really needs a picture to show the math...
       *
       * Consider an isometric grid with one of the sets of lines
       * parallel to the y axis (vertical alignment). Further define
       * that the origin of a Cartesian grid is at a isometric vertex.
       * For simplicity consider the first quadrant only.
       *
       *  - Let one line segment between vertices be r
       *  - Define the value of r as the grid spacing
       *  - Assign an integer n identifier to each vertical grid line
       *    along the x axis.  with n=0 being the y axis. n can be any
       *    integer
       *  - Let m to be any integer

       *  - Let h be the spacing between vertical grid lines measured
       *    along the x axis.  It follows from the isometric grid that
       *    h has a value of r * COS(1/6 Pi Rad)
       *
       *  Consider a Vertex V at the Cartesian location [Xv, Yv]
       *
       *   It follows that vertices belong to the set...
       *   V[Xv, Yv] = [ [ n * h ] ,
       *                 [ m * r + ( 0.5 * r (n % 2) ) ] ]
       *   for all integers n and m
       *
       * Who cares? Me. It's useful in solving this problem:
       * Consider an arbitrary point P[Xp,Yp], find the closest vertex
       * in the set V.
       *
       * Restated this problem is "find values for m and n that are
       * drive V closest to P"
       *
       * A Solution method (there may be a better one?):
       *
       * Step 1) bound n to the two closest values for Xp
       *         n_lo = (int) (Xp / h)
       *         n_hi = n_lo + 1
       *
       * Step 2) Consider the two closes vertices for each n_lo and
       *         n_hi. The further of the vertices in each pair can
       *         readily be discarded.
       *
       *         m_lo_n_lo = (int) ( (Yp / r) - 0.5 (n_lo % 2) )
       *         m_hi_n_lo = m_lo_n_lo + 1
       *
       *         m_lo_n_hi = (int) ( (Yp / r) - 0.5 (n_hi % 2) )
       *         m_hi_n_hi = m_hi_n_hi
       *
       * Step 3) compute the distance from P to V1 and V2. Snap to the
       *         closer point.
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
      m_lo_n_lo = (gint) ((y / r) - 0.5 * (n_lo % 2));
      m_hi_n_lo = m_lo_n_lo + 1;

     /* figure out which is the better candidate */
      if (abs ((m_lo_n_lo * r + (0.5 * r * (n_lo % 2))) - y) <
          abs ((m_hi_n_lo * r + (0.5 * r * (n_lo % 2))) - y))
        {
          m_n_lo = m_lo_n_lo;
        }
      else
        {
          m_n_lo = m_hi_n_lo;
        }

      /* evaluate m candidates for n_hi */
      m_lo_n_hi = (gint) ( (y / r) - 0.5 * (n_hi % 2) );
      m_hi_n_hi = m_lo_n_hi + 1;

      /* figure out which is the better candidate */
      if (abs((m_lo_n_hi * r + (0.5 * r * (n_hi % 2))) - y) <
          abs((m_hi_n_hi * r + (0.5 * r * (n_hi % 2))) - y))
        {
          m_n_hi = m_lo_n_hi;
        }
      else
        {
          m_n_hi = m_hi_n_hi;
        }

      /* Now, which is closer to [x,y]? we can use a somewhat
       * abbreviated form of the distance formula since we only care
       * about relative values.
       */

      x1 = (gint) (n_lo * h);
      y1 = (gint) (m_n_lo * r + (0.5 * r * (n_lo % 2)));
      x2 = (gint) (n_hi * h);
      y2 = (gint) (m_n_hi * r + (0.5 * r * (n_hi % 2)));

      if (((x - x1) * (x - x1) + (y - y1) * (y - y1)) <
          ((x - x2) * (x - x2) + (y - y2) * (y - y2)))
        {
          gp->x =  x1;
          gp->y =  y1;
        }
      else
        {
          gp->x =  x2;
          gp->y =  y2;
        }
    }
}

static void
draw_grid_polar (GdkGC *drawgc)
{
    gdouble       inner_radius;
    gdouble       outer_radius;
    gdouble       max_radius = sqrt (SQR (preview_width) + SQR (preview_height));
    gint          current_sectors = 1;
    PrimeFactors *factors = prime_factors_new (selvals.opts.grid_sectors_desired);
    for (inner_radius = 0, outer_radius = selvals.opts.grid_radius_min;
         outer_radius <= max_radius;
         inner_radius = outer_radius, outer_radius += selvals.opts.grid_radius_interval)
      {
        gdouble t;
        gdouble sector_size = 2 * G_PI / current_sectors;

        gdk_draw_arc (gfig_context->preview->window,
                      drawgc,
                      0,
                      0.5 + (preview_width / 2 - outer_radius),
                      0.5 + (preview_height / 2 - outer_radius),
                      0.5 + (outer_radius * 2),
                      0.5 + (outer_radius * 2),
                      0,
                      360 * 64);

        while ((current_sectors < selvals.opts.grid_sectors_desired)
               && (inner_radius * sector_size
                   > prime_factors_lookahead (factors) * selvals.opts.grid_granularity ))
          {
            current_sectors *= prime_factors_get (factors);
            sector_size = 2 * G_PI / current_sectors;
          }

        for (t = 0 ; t < 2 * G_PI ; t += sector_size)
          {
            gdouble normal_x = cos (selvals.opts.grid_rotation+t);
            gdouble normal_y = sin (selvals.opts.grid_rotation+t);

            gdk_draw_line (gfig_context->preview->window,
                           drawgc,
                           0.5 + (preview_width / 2 + inner_radius * normal_x),
                           0.5 + (preview_height / 2 - inner_radius * normal_y),
                           0.5 + (preview_width / 2 + outer_radius * normal_x),
                           0.5 + (preview_height / 2 - outer_radius * normal_y) );
          }
      }

    prime_factors_delete (factors);
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


