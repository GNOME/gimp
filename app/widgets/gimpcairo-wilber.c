/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Wilber Cairo rendering
 * Copyright (C) 2008  Sven Neumann <sven@gimp.org>
 *
 * Some code here is based on code from librsvg that was originally
 * written by Raph Levien <raph@artofcode.com> for Gill.
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include <gimpcairo-wilber.h>


static const gchar * const wilber_paths[] =
{
  "M 10.374369,12.467884 C 10.374369,12.467884 13.248878,18.395518 19.973611,18.228291 C 34.066126,17.874738 36.53732,10.523341 36.890873,9.4626804 C 37.244427,8.4020202 37.785407,8.5626825 37.91048,9.542947 C 42.506674,51.262247 6.0135488,33.362123 4.7175144,26.256467 C 11.965359,24.135147 10.197592,20.069282 10.197592,20.069282 L 10.374369,12.467884 z ",
  "M 15.73779,30.066049 C 22.47669,31.413886 25.908481,30.164142 27.916965,28.613273 C 27.386635,27.928263 26.480655,27.176962 26.480655,27.176962 C 26.480655,27.176962 28.833972,27.830904 29.662635,28.900535 C 30.488925,29.967103 29.969443,30.624242 29.753196,31.988905 C 29.271785,30.790306 28.373215,30.340813 28.251562,29.864573 C 26.445294,32.3615 21.94512,32.257773 15.73779,30.066049 z "
  "M 23.003067,31.736544 C 24.500439,31.879636 25.852696,31.464331 26.41496,31.262497 C 26.513185,30.707111 26.951512,29.64124 28.461048,29.571029 L 27.930718,28.642952 C 27.930718,28.642952 25.964077,29.990873 23.864854,30.388621 L 23.003067,31.736544 z ",
  NULL
};

static void  parse_path_data (cairo_t     *cr,
                              const gchar *data);


void
gimp_cairo_wilber (cairo_t *cr)
{
  gint i;

  for (i = 0; wilber_paths[i]; i++)
    parse_path_data (cr, wilber_paths[i]);
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
