/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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

#include "config.h"

#include <stdlib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gfig.h"
#include "gfig-grid.h"

#include "libgimp/stdplugins-intl.h"


/* For the isometric grid */
#define SQRT3 1.73205080756887729353   /* Square root of 3 */
#define SIN_1o6PI_RAD 0.5              /* Sine    1/6 Pi Radians */
#define COS_1o6PI_RAD SQRT3 / 2        /* Cosine  1/6 Pi Radians */
#define TAN_1o6PI_RAD 1 / SQRT3        /* Tangent 1/6 Pi Radians == SIN / COS */
#define RECIP_TAN_1o6PI_RAD SQRT3      /* Reciprocal of Tangent 1/6 Pi Radians */

gint           grid_gc_type           = GFIG_NORMAL_GC;

static void    draw_grid_polar     (cairo_t  *drawgc);
static void    draw_grid_sq        (cairo_t  *drawgc);
static void    draw_grid_iso       (cairo_t  *drawgc);

static cairo_t * gfig_get_grid_gc  (cairo_t   *cr,
                                    GtkWidget *widget,
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
gfig_grid_colors (GtkWidget *widget)
{
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
draw_grid_polar (cairo_t *cr)
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
        cairo_arc (cr,
                   0.5 + preview_width / 2.0,
                   0.5 + preview_height / 2.0,
                   outer_radius, 0, 2 * G_PI);
        cairo_stroke (cr);

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
            cairo_move_to (cr,
                           0.5 + (preview_width / 2.0 + inner_radius * normal_x),
                           0.5 + (preview_height / 2.0 - inner_radius * normal_y));
            cairo_line_to (cr,
                           0.5 + (preview_width / 2.0 + outer_radius * normal_x),
                           0.5 + (preview_height / 2.0 - outer_radius * normal_y));
            cairo_stroke (cr);
          }
      }

    prime_factors_delete (factors);
}


static void
draw_grid_sq (cairo_t *cr)
{
  gint step;
  gint loop;

  /* Draw the horizontal lines */
  step = selvals.opts.gridspacing;

  for (loop = 0 ; loop < preview_height ; loop += step)
    {
      cairo_move_to (cr, 0 + .5, loop + .5);
      cairo_line_to (cr, preview_width + .5, loop + .5);
    }

  /* Draw the vertical lines */

  for (loop = 0 ; loop < preview_width ; loop += step)
    {
      cairo_move_to (cr, loop + .5, 0 + .5);
      cairo_line_to (cr, loop + .5, preview_height + .5);
    }
  cairo_stroke (cr);
}

static void
draw_grid_iso (cairo_t *cr)
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
  for (loop = 0 ; loop < preview_width ; loop += hstep)
    {
      cairo_move_to (cr, loop, 0);
      cairo_line_to (cr, loop, preview_height);
    }
  cairo_stroke (cr);

  /* draw diag lines at a Theta of +/- 1/6 Pi Rad */

  diagonal_start = -(((int)preview_width * TAN_1o6PI_RAD) - (((int)(preview_width * TAN_1o6PI_RAD)) % vstep));

  diagonal_end = preview_height + (preview_width * TAN_1o6PI_RAD);
  diagonal_end -= ((int)diagonal_end) % vstep;

  diagonal_width = preview_width;
  diagonal_height = preview_width * TAN_1o6PI_RAD;

  /* Draw diag lines */
  for (loop = diagonal_start ; loop < diagonal_end ; loop += vstep)
    {
      cairo_move_to (cr, 0, loop);
      cairo_line_to (cr, diagonal_width, loop + diagonal_height);

      cairo_move_to (cr, 0, loop);
      cairo_line_to (cr, diagonal_width, loop - diagonal_height);
    }
  cairo_stroke (cr);
}

static cairo_t *
gfig_get_grid_gc (cairo_t *cr, GtkWidget *w, gint gctype)
{
  switch (gctype)
    {
    default:
    case GFIG_NORMAL_GC:
      cairo_set_source_rgb (cr, .92, .92, .92);
      break;
    case GFIG_BLACK_GC:
      cairo_set_source_rgb (cr, 0., 0., 0.);
      break;
    case GFIG_WHITE_GC:
      cairo_set_source_rgb (cr, 1., 1., 1.);
      break;
    case GFIG_GREY_GC:
      cairo_set_source_rgb (cr, .5, .5, .5);
      break;
    case GFIG_DARKER_GC:
      cairo_set_source_rgb (cr, .25, .25, .25);
      break;
    case GFIG_LIGHTER_GC:
      cairo_set_source_rgb (cr, .75, .75, .75);
      break;
    case GFIG_VERY_DARK_GC:
      cairo_set_source_rgb (cr, .125, .125, .125);
      break;
    }

  return cr;
}

void
draw_grid (cairo_t *cr)
{
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
    gfig_get_grid_gc (cr, gfig_context->preview, grid_gc_type);
  else
    return;

  cairo_set_line_width (cr, 1.);
  if (selvals.opts.gridtype == RECT_GRID)
    draw_grid_sq (cr);
  else if (selvals.opts.gridtype == POLAR_GRID)
    draw_grid_polar (cr);
  else if (selvals.opts.gridtype == ISO_GRID)
    draw_grid_iso (cr);
}


