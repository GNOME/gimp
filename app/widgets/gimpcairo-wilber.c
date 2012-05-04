/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Wilber Cairo rendering
 * Copyright (C) 2008  Sven Neumann <sven@gimp.org>
 *
 * Some code here is based on code from librsvg that was originally
 * written by Raph Levien <raph@artofcode.com> for Gill.
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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "gimpcairo-wilber.h"


static void   gimp_cairo_eyes (cairo_t *cr,
                               gdouble  x,
                               gdouble  y);


void
gimp_cairo_draw_toolbox_wilber (GtkWidget *widget,
                                cairo_t   *cr)
{
  GtkStyleContext *context;
  GtkAllocation    allocation;
  GdkRGBA          color;
  gdouble          wilber_width;
  gdouble          wilber_height;
  gdouble          factor;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (cr != NULL);

  context = gtk_widget_get_style_context (widget);

  gtk_widget_get_allocation (widget, &allocation);

  gimp_cairo_wilber_get_size (cr, &wilber_width, &wilber_height);

  factor = allocation.width / wilber_width * 0.9;

  if (! gtk_widget_get_has_window (widget))
    cairo_translate (cr, allocation.x, allocation.y);

  cairo_scale (cr, factor, factor);

  gimp_cairo_wilber (cr,
                     (allocation.width  / factor - wilber_width)  / 2.0,
                     (allocation.height / factor - wilber_height) / 2.0);

  gtk_style_context_get_color (context, gtk_widget_get_state_flags (widget),
                               &color);
  color.alpha = 0.1;
  gdk_cairo_set_source_rgba (cr, &color);

  cairo_fill (cr);
}

void
gimp_cairo_draw_drop_wilber (GtkWidget *widget,
                             cairo_t   *cr,
                             gboolean   blink)
{
  GtkStyleContext *context;
  GtkAllocation    allocation;
  GdkRGBA          color;
  gdouble          wilber_width;
  gdouble          wilber_height;
  gdouble          width;
  gdouble          height;
  gdouble          side;
  gdouble          factor;

  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (cr != NULL);

  context = gtk_widget_get_style_context (widget);

  gtk_widget_get_allocation (widget, &allocation);

  gimp_cairo_wilber_get_size (cr, &wilber_width, &wilber_height);

  wilber_width  /= 2;
  wilber_height /= 2;

  side = MIN (MIN (allocation.width, allocation.height),
              MAX (allocation.width, allocation.height) / 2);

  width  = MAX (wilber_width,  side);
  height = MAX (wilber_height, side);

  factor = MIN (width / wilber_width, height / wilber_height);

  if (! gtk_widget_get_has_window (widget))
    cairo_translate (cr, allocation.x, allocation.y);

  cairo_scale (cr, factor, factor);

  /*  magic factors depend on the image used, everything else is generic
   */
  gimp_cairo_wilber (cr,
                     - wilber_width * 0.6,
                     allocation.height / factor - wilber_height * 1.1);

  gtk_style_context_get_color (context, gtk_widget_get_state_flags (widget),
                               &color);
  color.alpha = 0.15;

  gdk_cairo_set_source_rgba (cr, &color);

  cairo_fill (cr);

  if (blink)
    {
      gimp_cairo_eyes (cr,
                       - wilber_width * 0.6,
                       allocation.height / factor - wilber_height * 1.1);

      cairo_set_source_rgba (cr,
                             color.red,
                             0,
                             0,
                             1.0);
      cairo_fill (cr);
    }
}


/* This string is a path description as found in SVG files.  You can
 * use Inkscape to create the SVG file, then copy the path from it.
 * It works best if you combine all paths into one. Inkscape has a
 * function to do that.
 */
static const gchar wilber_path[] =
  "M 509.72445,438.68864 C 501.47706,469.77945 464.95038,491.54566 431.85915,497.74874 C 438.5216,503.01688 442.87782,511.227 442.87782,520.37375 C 442.87783,536.24746 429.95607,549.0223 414.08235,549.0223 C 398.20863,549.0223 385.28688,536.24746 385.28688,520.37375 C 385.28688,511.52403 389.27666,503.61286 395.57098,498.3364 C 359.36952,495.90384 343.70976,463.95812 343.70975,463.95814 L 342.68134,509.64891 C 342.68134,514.35021 342.08391,519.96098 340.18378,528.3072 C 339.84664,527.80364 339.51399,527.33515 339.15537,526.83804 C 330.25511,514.5011 317.25269,507.81431 306.39317,508.76741 C 302.77334,509.08511 299.47017,510.33348 296.54982,512.4403 C 284.86847,520.86757 284.97665,540.94721 296.84366,557.3965 C 306.96274,571.42287 322.32232,578.25612 333.8664,574.73254 C 391.94635,615.17624 532.16931,642.41915 509.72445,438.68864 z M 363.24953,501.1278 C 373.83202,501.12778 382.49549,509.79127 382.49549,520.37375 C 382.49549,530.95624 373.83201,539.47279 363.24953,539.47279 C 352.66706,539.47279 344.1505,530.95624 344.1505,520.37375 C 344.15049,509.79129 352.66706,501.1278 363.24953,501.1278 z M 305.80551,516.1132 C 311.68466,516.11318 316.38344,521.83985 316.38344,528.89486 C 316.38345,535.94982 311.68467,541.67652 305.80551,541.67652 C 299.92636,541.67652 295.08067,535.94987 295.08067,528.89486 C 295.08065,521.83985 299.92636,516.1132 305.80551,516.1132 z M 440.821,552.54828 C 440.821,552.54828 448.7504,554.02388 453.8965,559.45332 C 457.41881,563.16951 457.75208,569.15506 456.98172,577.37703 C 456.21143,573.8833 454.89571,571.76659 453.8965,569.29666 C 443.01388,582.47662 413.42981,583.08929 376.0312,569.88433 C 416.63248,578.00493 437.38806,570.56014 449.48903,561.2163 C 446.29383,557.08917 440.821,552.54828 440.821,552.54828 z M 434.64723,524.59684 C 434.64723,532.23974 428.44429,538.44268 420.80139,538.44268 C 413.15849,538.44268 406.95555,532.23974 406.95555,524.59684 C 406.95555,516.95394 413.15849,510.751 420.80139,510.751 C 428.44429,510.751 434.64723,516.95394 434.64723,524.59684 z M 378.00043,522.99931 C 378.00043,527.70264 374.18324,531.51984 369.47991,531.51984 C 364.77658,531.51984 360.95939,527.70264 360.95939,522.99931 C 360.95939,518.29599 364.77658,514.47879 369.47991,514.47879 C 374.18324,514.47879 378.00043,518.29599 378.00043,522.99931 z ";

static const gchar eyes_path[] =
  "M 434.64723,524.59684 C 434.64723,532.23974 428.44429,538.44268 420.80139,538.44268 C 413.15849,538.44268 406.95555,532.23974 406.95555,524.59684 C 406.95555,516.95394 413.15849,510.751 420.80139,510.751 C 428.44429,510.751 434.64723,516.95394 434.64723,524.59684 z M 378.00043,522.99931 C 378.00043,527.70264 374.18324,531.51984 369.47991,531.51984 C 364.77658,531.51984 360.95939,527.70264 360.95939,522.99931 C 360.95939,518.29599 364.77658,514.47879 369.47991,514.47879 C 374.18324,514.47879 378.00043,518.29599 378.00043,522.99931 z ";

static cairo_path_t *wilber_cairo_path = NULL;
static gdouble       wilber_x1, wilber_y1;
static gdouble       wilber_x2, wilber_y2;

static cairo_path_t *eyes_cairo_path = NULL;
static gdouble       eyes_x1, eyes_y1;
static gdouble       eyes_x2, eyes_y2;


static void  parse_path_data    (cairo_t     *cr,
                                 const gchar *data);
static void  wilber_get_extents (cairo_t     *cr);
static void  eyes_get_extents   (cairo_t     *cr);


/**
 * gimp_cairo_wilber:
 * @cr: Cairo context
 * @x: x position
 * @y: y position
 *
 * Draw a Wilber path at position @x, @y.
 */
void
gimp_cairo_wilber (cairo_t *cr,
                   gdouble  x,
                   gdouble  y)
{
  wilber_get_extents (cr);

  cairo_save (cr);

  cairo_translate (cr, x - wilber_x1, y - wilber_y1);
  cairo_append_path (cr, wilber_cairo_path);

  cairo_restore (cr);
}

static void
gimp_cairo_eyes (cairo_t *cr,
                 gdouble  x,
                 gdouble  y)
{
  wilber_get_extents (cr);
  eyes_get_extents (cr);

  cairo_save (cr);

  cairo_translate (cr, x - wilber_x1, y - wilber_y1);
  cairo_append_path (cr, eyes_cairo_path);

  cairo_restore (cr);
}

void
gimp_cairo_wilber_get_size (cairo_t *cr,
                            gdouble *width,
                            gdouble *height)
{
  wilber_get_extents (cr);

  *width  = wilber_x2 - wilber_x1;
  *height = wilber_y2 - wilber_y1;
}


static void
wilber_get_extents (cairo_t *unused)
{
  if (! wilber_cairo_path)
    {
      cairo_surface_t *s  = cairo_image_surface_create (CAIRO_FORMAT_A8, 1, 1);
      cairo_t         *cr = cairo_create (s);

      parse_path_data (cr, wilber_path);
      cairo_fill_extents (cr, &wilber_x1, &wilber_y1, &wilber_x2, &wilber_y2);

      wilber_cairo_path = cairo_copy_path (cr);

      cairo_destroy (cr);
      cairo_surface_destroy (s);
    }
}

static void
eyes_get_extents (cairo_t *unused)
{
  if (! eyes_cairo_path)
    {
      cairo_surface_t *s  = cairo_image_surface_create (CAIRO_FORMAT_A8, 1, 1);
      cairo_t         *cr = cairo_create (s);

      parse_path_data (cr, eyes_path);
      cairo_fill_extents (cr, &eyes_x1, &eyes_y1, &eyes_x2, &eyes_y2);

      eyes_cairo_path = cairo_copy_path (cr);

      cairo_destroy (cr);
      cairo_surface_destroy (s);
    }
}

/**********************************************************/
/*  Below is the code that parses the actual path data.   */
/*                                                        */
/*  This code is taken from librsvg and was originally    */
/*  written by Raph Levien <raph@artofcode.com> for Gill. */
/**********************************************************/

typedef struct
{
  cairo_t     *cr;
  gdouble      cpx, cpy;  /* current point                               */
  gdouble      rpx, rpy;  /* reflection point (for 's' and 't' commands) */
  gchar        cmd;       /* current command (lowercase)                 */
  gint         param;     /* number of parameters                        */
  gboolean     rel;       /* true if relative coords                     */
  gdouble      params[7]; /* parameters that have been parsed            */
} ParsePathContext;


static void  parse_path_default_xy (ParsePathContext *ctx,
                                    gint              n_params);
static void  parse_path_do_cmd     (ParsePathContext *ctx,
                                    gboolean          final);


static void
parse_path_data (cairo_t     *cr,
                 const gchar *data)
{
  ParsePathContext ctx;

  gboolean  in_num        = FALSE;
  gboolean  in_frac       = FALSE;
  gboolean  in_exp        = FALSE;
  gboolean  exp_wait_sign = FALSE;
  gdouble   val           = 0.0;
  gchar     c             = 0;
  gint      sign          = 0;
  gint      exp           = 0;
  gint      exp_sign      = 0;
  gdouble   frac          = 0.0;
  gint      i;

  memset (&ctx, 0, sizeof (ParsePathContext));

  ctx.cr = cr;

  for (i = 0; ; i++)
    {
      c = data[i];
      if (c >= '0' && c <= '9')
        {
          /* digit */
          if (in_num)
            {
              if (in_exp)
                {
                  exp = (exp * 10) + c - '0';
                  exp_wait_sign = FALSE;
                }
              else if (in_frac)
                val += (frac *= 0.1) * (c - '0');
              else
                val = (val * 10) + c - '0';
            }
          else
            {
              in_num = TRUE;
              in_frac = FALSE;
              in_exp = FALSE;
              exp = 0;
              exp_sign = 1;
              exp_wait_sign = FALSE;
              val = c - '0';
              sign = 1;
            }
        }
      else if (c == '.')
        {
          if (!in_num)
            {
              in_num = TRUE;
              val = 0;
            }
          in_frac = TRUE;
          frac = 1;
        }
      else if ((c == 'E' || c == 'e') && in_num)
        {
          in_exp = TRUE;
          exp_wait_sign = TRUE;
          exp = 0;
          exp_sign = 1;
        }
      else if ((c == '+' || c == '-') && in_exp)
        {
          exp_sign = c == '+' ? 1 : -1;
        }
      else if (in_num)
        {
          /* end of number */

          val *= sign * pow (10, exp_sign * exp);
          if (ctx.rel)
            {
              /* Handle relative coordinates. This switch statement attempts
                 to determine _what_ the coords are relative to. This is
                 underspecified in the 12 Apr working draft. */
              switch (ctx.cmd)
                {
                case 'l':
                case 'm':
                case 'c':
                case 's':
                case 'q':
                case 't':
                  /* rule: even-numbered params are x-relative, odd-numbered
                     are y-relative */
                  if ((ctx.param & 1) == 0)
                    val += ctx.cpx;
                  else if ((ctx.param & 1) == 1)
                    val += ctx.cpy;
                  break;

                case 'a':
                  /* rule: sixth and seventh are x and y, rest are not
                     relative */
                  if (ctx.param == 5)
                    val += ctx.cpx;
                  else if (ctx.param == 6)
                    val += ctx.cpy;
                  break;
                case 'h':
                  /* rule: x-relative */
                  val += ctx.cpx;
                  break;
                case 'v':
                  /* rule: y-relative */
                  val += ctx.cpy;
                  break;
                }
            }

          ctx.params[ctx.param++] = val;
          parse_path_do_cmd (&ctx, FALSE);
          in_num = FALSE;
        }

      if (c == '\0')
        break;
      else if ((c == '+' || c == '-') && !exp_wait_sign)
        {
          sign = c == '+' ? 1 : -1;
          val = 0;
          in_num = TRUE;
          in_frac = FALSE;
          in_exp = FALSE;
          exp = 0;
          exp_sign = 1;
          exp_wait_sign = FALSE;
        }
      else if (c == 'z' || c == 'Z')
        {
          if (ctx.param)
            parse_path_do_cmd (&ctx, TRUE);

          cairo_close_path (ctx.cr);
        }
      else if (c >= 'A' && c <= 'Z' && c != 'E')
        {
          if (ctx.param)
            parse_path_do_cmd (&ctx, TRUE);
          ctx.cmd = c + 'a' - 'A';
          ctx.rel = FALSE;
        }
      else if (c >= 'a' && c <= 'z' && c != 'e')
        {
          if (ctx.param)
            parse_path_do_cmd (&ctx, TRUE);
          ctx.cmd = c;
          ctx.rel = TRUE;
        }
      /* else c _should_ be whitespace or , */
    }
}

/* supply defaults for missing parameters, assuming relative coordinates
   are to be interpreted as x,y */
static void
parse_path_default_xy (ParsePathContext *ctx,
                       gint              n_params)
{
  gint i;

  if (ctx->rel)
    {
      for (i = ctx->param; i < n_params; i++)
        {
          if (i > 2)
            ctx->params[i] = ctx->params[i - 2];
          else if (i == 1)
            ctx->params[i] = ctx->cpy;
          else if (i == 0)
            /* we shouldn't get here (ctx->param > 0 as precondition) */
            ctx->params[i] = ctx->cpx;
        }
    }
  else
    {
      for (i = ctx->param; i < n_params; i++)
        ctx->params[i] = 0.0;
    }
}

static void
parse_path_do_cmd (ParsePathContext *ctx,
                   gboolean          final)
{
  switch (ctx->cmd)
    {
    case 'm':
      /* moveto */
      if (ctx->param == 2 || final)
        {
          parse_path_default_xy (ctx, 2);

          ctx->cpx = ctx->rpx = ctx->params[0];
          ctx->cpy = ctx->rpy = ctx->params[1];

          cairo_move_to (ctx->cr, ctx->cpx, ctx->cpy);

          ctx->param = 0;
        }
      break;

    case 'l':
      /* lineto */
      if (ctx->param == 2 || final)
        {
          parse_path_default_xy (ctx, 2);

          ctx->cpx = ctx->rpx = ctx->params[0];
          ctx->cpy = ctx->rpy = ctx->params[1];

          cairo_line_to (ctx->cr, ctx->cpx, ctx->cpy);

          ctx->param = 0;
        }
      break;

    case 'c':
      /* curveto */
      if (ctx->param == 6 || final)
        {
          gdouble x, y;

          parse_path_default_xy (ctx, 6);

          x        = ctx->params[0];
          y        = ctx->params[1];
          ctx->rpx = ctx->params[2];
          ctx->rpy = ctx->params[3];
          ctx->cpx = ctx->params[4];
          ctx->cpy = ctx->params[5];

          cairo_curve_to (ctx->cr,
                          x, y, ctx->rpx, ctx->rpy, ctx->cpx, ctx->cpy);

          ctx->param = 0;
        }
      break;

    case 's':
      /* smooth curveto */
      if (ctx->param == 4 || final)
        {
          gdouble x, y;

          parse_path_default_xy (ctx, 4);

          x = 2 * ctx->cpx - ctx->rpx;
          y = 2 * ctx->cpy - ctx->rpy;
          ctx->rpx = ctx->params[0];
          ctx->rpy = ctx->params[1];
          ctx->cpx = ctx->params[2];
          ctx->cpy = ctx->params[3];

          cairo_curve_to (ctx->cr,
                          x, y, ctx->rpx, ctx->rpy, ctx->cpx, ctx->cpy);

          ctx->param = 0;
        }
      break;

    case 'h':
      /* horizontal lineto */
      if (ctx->param == 1)
        {
          ctx->cpx = ctx->rpx = ctx->params[0];

          cairo_line_to (ctx->cr, ctx->cpx, ctx->cpy);

          ctx->param = 0;
        }
      break;

    case 'v':
      /* vertical lineto */
      if (ctx->param == 1)
        {
          ctx->cpy = ctx->rpy = ctx->params[0];

          cairo_line_to (ctx->cr, ctx->cpx, ctx->cpy);

          ctx->param = 0;
        }
      break;

    case 'q':
      /* quadratic bezier curveto */
      if (ctx->param == 4 || final)
        {
          parse_path_default_xy (ctx, 4);

          ctx->rpx = ctx->params[0];
          ctx->rpy = ctx->params[1];
          ctx->cpx = ctx->params[2];
          ctx->cpy = ctx->params[3];

          g_warning ("quadratic bezier curveto not implemented");

          ctx->param = 0;
        }
      break;

    case 't':
      /* truetype quadratic bezier curveto */
      if (ctx->param == 2 || final)
        {
          parse_path_default_xy (ctx, 2);

          ctx->rpx = 2 * ctx->cpx - ctx->rpx;
          ctx->rpy = 2 * ctx->cpy - ctx->rpy;
          ctx->cpx = ctx->params[0];
          ctx->cpy = ctx->params[1];

          g_warning ("truetype quadratic bezier curveto not implemented");

          ctx->param = 0;
        }
      else if (final)
        {
          if (ctx->param > 2)
            {
              parse_path_default_xy (ctx, 4);

              ctx->rpx = ctx->params[0];
              ctx->rpy = ctx->params[1];
              ctx->cpx = ctx->params[2];
              ctx->cpy = ctx->params[3];

              g_warning ("conicto not implemented");
            }
          else
            {
              parse_path_default_xy (ctx, 2);

              ctx->cpx = ctx->rpx = ctx->params[0];
              ctx->cpy = ctx->rpy = ctx->params[1];

              cairo_line_to (ctx->cr, ctx->cpx, ctx->cpy);
            }

          ctx->param = 0;
        }
      break;

    case 'a':
      if (ctx->param == 7 || final)
        {
          ctx->cpx = ctx->rpx = ctx->params[5];
          ctx->cpy = ctx->rpy = ctx->params[6];

          g_warning ("arcto not implemented");

          ctx->param = 0;
        }
      break;

    default:
      ctx->param = 0;
      break;
    }
}
