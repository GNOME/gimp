/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpVectors Import
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "vectors-types.h"

#include "core/gimpimage.h"

#include "gimpbezierstroke.h"
#include "gimpstroke.h"
#include "gimpvectors.h"
#include "gimpvectors-import.h"

#include "gimp-intl.h"


typedef enum
{
  PARSER_START,
  PARSER_IN_SVG,
  PARSER_IN_PATH,
  PARSER_IN_UNKNOWN
} ParserState;

typedef struct
{
  gint x, y;
  gint w, h;
} Rectangle;

typedef struct
{
  ParserState   state;
  ParserState   last_known_state;
  gint          unknown_depth;
  GimpImage    *image;
  gboolean      merge;
  GimpVectors  *vectors;
  Rectangle     viewbox;
} VectorsParser;


static void  parser_start_element (GMarkupParseContext  *context,
                                   const gchar          *element_name,
                                   const gchar         **attribute_names,
                                   const gchar         **attribute_values,
                                   gpointer              user_data,
                                   GError              **error);
static void  parser_end_element   (GMarkupParseContext  *context,
                                   const gchar          *element_name,
                                   gpointer              user_data,
                                   GError              **error);

static void  parser_start_unknown (VectorsParser        *parser);
static void  parser_end_unknown   (VectorsParser        *parser);

static void  parse_svg_viewbox    (VectorsParser        *parser,
                                   const gchar          *value);
static void  parse_path_data      (GimpVectors          *vectors,
                                   const gchar          *data);
static void  parser_add_vectors   (VectorsParser        *parser,
                                   GimpVectors          *vectors);


static const GMarkupParser markup_parser =
{
  parser_start_element,
  parser_end_element,
  NULL,  /*  characters   */
  NULL,  /*  passthrough  */
  NULL   /*  error        */
};


/**
 * gimp_vectors_import:
 * @image: the #GimpImage to add the paths to
 * @filename: name of a SVG file
 * @merge: if multiple paths should be merged into a single #GimpVectors
 *         object
 * @error: location to store possible errors
 *
 * Imports one or more paths from a SVG file.
 *
 * Return value: %TRUE on success, %FALSE if an error occured
 **/
gboolean
gimp_vectors_import (GimpImage    *image,
                     const gchar  *filename,
                     gboolean      merge,
                     GError      **error)
{
  GMarkupParseContext *context;
  FILE                *file;
  VectorsParser        parser;
  gboolean             success = TRUE;
  gsize                bytes;
  gchar                buf[4096];

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = fopen (filename, "r");
  if (!file)
    {
      g_set_error (error, 0, 0,
                   _("Failed to open file: '%s': %s"),
                   filename, g_strerror (errno));
      return FALSE;
    }

  memset (&parser, 0, sizeof (VectorsParser));

  parser.state = PARSER_START;
  parser.image = image;
  parser.merge = merge;

  context = g_markup_parse_context_new (&markup_parser, 0, &parser, NULL);

  while (success &&
         (bytes = fread (buf, sizeof (gchar), sizeof (buf), file)) > 0)
    success = g_markup_parse_context_parse (context, buf, bytes, error);

  if (success)
    g_markup_parse_context_end_parse (context, error);

  fclose (file);
  g_markup_parse_context_free (context);

  if (merge && parser.vectors)
    parser_add_vectors (&parser, parser.vectors);

  return success;
}

static void
parser_start_element (GMarkupParseContext *context,
                      const gchar         *element_name,
                      const gchar        **attribute_names,
                      const gchar        **attribute_values,
                      gpointer             user_data,
                      GError             **error)
{
  VectorsParser *parser = (VectorsParser *) user_data;

  switch (parser->state)
    {
    case PARSER_START:
      if (strcmp (element_name, "svg") == 0)
        parser->state = PARSER_IN_SVG;
      else
        parser_start_unknown (parser);
      break;

    case PARSER_IN_SVG:
      if (strcmp (element_name, "path") == 0)
        parser->state = PARSER_IN_PATH;
      else
        parser_start_unknown (parser);
      break;

    case PARSER_IN_PATH:
    case PARSER_IN_UNKNOWN:
      parser_start_unknown (parser);
      break;
    }

  switch (parser->state)
    {
    case PARSER_IN_SVG:
      while (*attribute_names)
        {
          if (strcmp (*attribute_names, "viewBox") == 0)
            parse_svg_viewbox (parser, *attribute_values);

          attribute_names++;
          attribute_values++;
        }
      break;

    case PARSER_IN_PATH:
      while (*attribute_names)
        {
          if (strcmp (*attribute_names, "d") == 0)
            {
              GimpVectors *vectors = NULL;

              if (parser->merge)
                vectors = parser->vectors;

              if (! vectors)
                vectors = gimp_vectors_new (parser->image,
                                            _("Imported Path"));

              parse_path_data (vectors, *attribute_values);

              if (! parser->merge)
                parser_add_vectors (parser, vectors);

              parser->vectors = vectors;
            }

          attribute_names++;
          attribute_values++;
        }

      break;

    default:
      break;
    }
}

static void
parser_end_element (GMarkupParseContext *context,
                    const gchar         *element_name,
                    gpointer             user_data,
                    GError             **error)
{
  VectorsParser *parser = (VectorsParser *) user_data;

  switch (parser->state)
    {
    case PARSER_START:
      g_return_if_reached ();
      break;

    case PARSER_IN_SVG:
      parser->state = PARSER_START;
      break;

    case PARSER_IN_PATH:
      parser->state = PARSER_IN_SVG;
      break;

    case PARSER_IN_UNKNOWN:
      parser_end_unknown (parser);
      break;
    }
}

static void
parser_start_unknown (VectorsParser *parser)
{
  if (parser->unknown_depth == 0)
    parser->last_known_state = parser->state;

  parser->state = PARSER_IN_UNKNOWN;
  parser->unknown_depth++;
}

static void
parser_end_unknown (VectorsParser *parser)
{
  g_return_if_fail (parser->unknown_depth > 0 &&
                    parser->state == PARSER_IN_UNKNOWN);

  parser->unknown_depth--;

  if (parser->unknown_depth == 0)
    parser->state = parser->last_known_state;
}

static void
parser_add_vectors (VectorsParser *parser,
                    GimpVectors   *vectors)
{
  if (parser->viewbox.w && parser->viewbox.h)
    {
      GList   *list;
      gdouble  scale_x, scale_y;

      scale_x = (gdouble) parser->image->width  / (gdouble) parser->viewbox.w;
      scale_y = (gdouble) parser->image->height / (gdouble) parser->viewbox.h;

      for (list = vectors->strokes; list; list = list->next)
        {
          gimp_stroke_scale ((GimpStroke *) list->data,
                             scale_x,
                             scale_y,
                             - parser->viewbox.x,
                             - parser->viewbox.y);
        }
    }

  gimp_image_add_vectors (parser->image, vectors, -1);
}


/**********************************************************/
/*  Below is the code that parses the actual path data.   */
/*                                                        */
/*  This code is taken from librsvg and was originally    */
/*  written by Raph Levien <raph@artofcode.com> for Gill. */
/**********************************************************/

typedef struct
{
  GimpVectors *vectors;
  GimpStroke  *stroke;
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
parse_svg_viewbox (VectorsParser *parser,
                   const gchar   *value)
{
  gint      x, y, w, h;
  gchar    *tok;
  gchar    *str     = g_strdup (value);
  gboolean  success = FALSE;

  x = y = w = h = 0;

  tok = strtok (str, ", \t");
  if (tok)
    {
      x = g_ascii_strtod (tok, NULL);
      tok = strtok (NULL, ", \t");
      if (tok)
        {
          y = g_ascii_strtod (tok, NULL);
          tok = strtok (NULL, ", \t");
          if (tok != NULL)
            {
              w = g_ascii_strtod (tok, NULL);
              tok = strtok (NULL, ", \t");
              if (tok)
                {
                  h = g_ascii_strtod (tok, NULL);
                  success = TRUE;
                }
            }
        }
    }

  g_free (str);

  if (success)
    {
      parser->viewbox.x = x;
      parser->viewbox.y = y;
      parser->viewbox.w = w;
      parser->viewbox.h = h;
    }
}

static void
parse_path_data (GimpVectors *vectors,
                 const gchar *data)
{
  ParsePathContext ctx;

  gboolean in_num        = FALSE;
  gboolean in_frac       = FALSE;
  gboolean in_exp        = FALSE;
  gboolean exp_wait_sign = FALSE;
  gdouble  val      = 0.0;
  gchar    c        = 0;
  gint     sign     = 0;
  gint     exp      = 0;
  gint     exp_sign = 0;
  gdouble  frac     = 0.0;
  gint     i;

  ctx.vectors = vectors;
  ctx.stroke  = NULL;
  ctx.cpx     = 0.0;
  ctx.cpy     = 0.0;
  ctx.cmd     = 0;
  ctx.param   = 0;
  ctx.rel     = FALSE;

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
	  sign = c == '+' ? 1 : -1;;
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
          if (ctx.stroke)
            gimp_stroke_close (ctx.stroke);
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
  GimpCoords coords = { 0.0, 0.0, 1.0, 0.5, 0.5, 0.5 };

  switch (ctx->cmd)
    {
    case 'm':
      /* moveto */
      if (ctx->param == 2 || final)
	{
	  parse_path_default_xy (ctx, 2);

          coords.x = ctx->cpx = ctx->rpx = ctx->params[0];
          coords.y = ctx->cpy = ctx->rpy = ctx->params[1];

          ctx->stroke = gimp_bezier_stroke_new_moveto (&coords);

          gimp_vectors_stroke_add (ctx->vectors, ctx->stroke);
          g_object_unref (ctx->stroke);

	  ctx->param = 0;
	}
      break;
    case 'l':
      /* lineto */
      if (ctx->param == 2 || final)
	{
	  parse_path_default_xy (ctx, 2);

          coords.x = ctx->cpx = ctx->rpx = ctx->params[0];
          coords.y = ctx->cpy = ctx->rpy = ctx->params[1];

          gimp_bezier_stroke_lineto (ctx->stroke, &coords);

	  ctx->param = 0;
	}
      break;
    case 'c':
      /* curveto */
      if (ctx->param == 6 || final)
	{
          GimpCoords ctrl1 = { 0.0, 0.0, 1.0, 0.5, 0.5, 0.5 };
          GimpCoords ctrl2 = { 0.0, 0.0, 1.0, 0.5, 0.5, 0.5 };

	  parse_path_default_xy (ctx, 6);

	  ctrl1.x  = ctx->params[0];
	  ctrl1.y  = ctx->params[1];
	  ctrl2.x  = ctx->rpx = ctx->params[2];
	  ctrl2.y  = ctx->rpy = ctx->params[3];
	  coords.x = ctx->cpx = ctx->params[4];
	  coords.y = ctx->cpy = ctx->params[5];

          gimp_bezier_stroke_cubicto (ctx->stroke, &ctrl1, &ctrl2, &coords);

	  ctx->param = 0;
	}
      break;
    case 's':
      /* smooth curveto */
      if (ctx->param == 4 || final)
	{
          GimpCoords ctrl1 = { 0.0, 0.0, 1.0, 0.5, 0.5, 0.5 };
          GimpCoords ctrl2 = { 0.0, 0.0, 1.0, 0.5, 0.5, 0.5 };

	  parse_path_default_xy (ctx, 4);

          ctrl1.x  = 2 * ctx->cpx - ctx->rpx;
	  ctrl1.y  = 2 * ctx->cpy - ctx->rpy;
          ctrl2.x  = ctx->rpx = ctx->params[0];
          ctrl2.y  = ctx->rpy = ctx->params[1];
	  coords.x = ctx->cpx = ctx->params[2];
	  coords.y = ctx->cpy = ctx->params[3];

          gimp_bezier_stroke_cubicto (ctx->stroke, &ctrl1, &ctrl2, &coords);

	  ctx->param = 0;
	}
      break;
    case 'h':
      /* horizontal lineto */
      if (ctx->param == 1)
	{
          coords.x = ctx->cpx = ctx->rpx = ctx->params[0];
          coords.y = ctx->cpy;

          gimp_bezier_stroke_lineto (ctx->stroke, &coords);

	  ctx->param = 0;
	}
      break;
    case 'v':
      /* vertical lineto */
      if (ctx->param == 1)
	{
          coords.x = ctx->cpx;
          coords.y = ctx->cpy = ctx->rpy = ctx->params[0];

          gimp_bezier_stroke_lineto (ctx->stroke, &coords);

	  ctx->param = 0;
	}
      break;

    case 'q':
      /* quadratic bezier curveto */
      if (ctx->param == 4 || final)
	{
          GimpCoords ctrl = { 0.0, 0.0, 1.0, 0.5, 0.5, 0.5 };

	  parse_path_default_xy (ctx, 4);

	  ctrl.x   = ctx->rpx = ctx->params[0];
	  ctrl.y   = ctx->rpy = ctx->params[1];
	  coords.x = ctx->cpx = ctx->params[2];
	  coords.y = ctx->cpy = ctx->params[3];

          gimp_bezier_stroke_conicto (ctx->stroke, &ctrl, &coords);

	  ctx->param = 0;
	}
      break;
    case 't':
      /* truetype quadratic bezier curveto */
      if (ctx->param == 2 || final)
	{
          GimpCoords ctrl = { 0.0, 0.0, 1.0, 0.5, 0.5, 0.5 };

	  parse_path_default_xy (ctx, 2);

          ctrl.x   = ctx->rpx = 2 * ctx->cpx - ctx->rpx;
          ctrl.y   = ctx->rpy = 2 * ctx->cpy - ctx->rpy;
	  coords.x = ctx->cpx = ctx->params[0];
	  coords.y = ctx->cpy = ctx->params[1];

          gimp_bezier_stroke_conicto (ctx->stroke, &ctrl, &coords);

	  ctx->param = 0;
	}
      else if (final)
	{
	  if (ctx->param > 2)
	    {
              GimpCoords ctrl = { 0.0, 0.0, 1.0, 0.5, 0.5, 0.5 };

	      parse_path_default_xy (ctx, 4);

              ctrl.x   = ctx->rpx = ctx->params[0];
              ctrl.y   = ctx->rpy = ctx->params[1];
              coords.x = ctx->cpx = ctx->params[2];
              coords.y = ctx->cpy = ctx->params[3];

              gimp_bezier_stroke_conicto (ctx->stroke, &ctrl, &coords);
	    }
	  else
	    {
	      parse_path_default_xy (ctx, 2);

              coords.x = ctx->cpx = ctx->rpx = ctx->params[0];
              coords.y = ctx->cpy = ctx->rpy = ctx->params[1];

              gimp_bezier_stroke_lineto (ctx->stroke, &coords);
	    }
	  ctx->param = 0;
	}
      break;
#if 0
    case 'a':
      if (ctx->param == 7 || final)
	{
	  rsvg_path_arc (ctx,
			ctx->params[0], ctx->params[1], ctx->params[2],
			ctx->params[3], ctx->params[4],
			ctx->params[5], ctx->params[6]);
	  ctx->param = 0;
	}
      break;
#endif
    default:
      ctx->param = 0;
      break;
    }
}
