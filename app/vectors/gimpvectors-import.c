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
#include "core/gimpimage-undo.h"

#include "gimpbezierstroke.h"
#include "gimpstroke.h"
#include "gimpvectors.h"
#include "gimpvectors-import.h"

#include "gimp-intl.h"


typedef struct _SvgHandler SvgHandler;

struct _SvgHandler
{
  const gchar  *name;
  gdouble       width;
  gdouble       height;
  GList        *paths;
  GimpMatrix3  *transform;

  void (* start) (SvgHandler   *handler,
                  const gchar **names,
                  const gchar **values);
};


static void  svg_parser_start_element (GMarkupParseContext  *context,
                                       const gchar          *element_name,
                                       const gchar         **attribute_names,
                                       const gchar         **attribute_values,
                                       gpointer              user_data,
                                       GError              **error);
static void  svg_parser_end_element   (GMarkupParseContext  *context,
                                       const gchar          *element_name,
                                       gpointer              user_data,
                                       GError              **error);

static const GMarkupParser markup_parser =
{
  svg_parser_start_element,
  svg_parser_end_element,
  NULL,  /*  characters   */
  NULL,  /*  passthrough  */
  NULL   /*  error        */
};


static void  svg_handler_svg   (SvgHandler   *handler,
                                const gchar **names,
                                const gchar **values);
static void  svg_handler_group (SvgHandler   *handler,
                                const gchar **names,
                                const gchar **values);
static void  svg_handler_path  (SvgHandler   *handler,
                                const gchar **names,
                                const gchar **values);

static SvgHandler svg_handlers[] =
{
  { "svg",  0, 0, NULL, NULL, svg_handler_svg   },
  { "g",    0, 0, NULL, NULL, svg_handler_group },
  { "path", 0, 0, NULL, NULL, svg_handler_path  },
  { NULL,   0, 0, NULL, NULL, NULL              }
};


static gboolean   parse_svg_viewbox   (const gchar  *value,
                                       gdouble       width,
                                       gdouble       height,
                                       GimpMatrix3  *matrix);
static gboolean   parse_svg_transform (const gchar  *value,
                                       GimpMatrix3  *matrix);
static GList    * parse_path_data     (const gchar  *data);


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
  GQueue              *stack;
  GList               *paths;
  GList               *list;
  SvgHandler          *base;
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

  stack = g_queue_new ();

  /*  the base of the stack, defines the size of the view-port  */

  base = g_new0 (SvgHandler, 1);
  base->name   = "image";
  base->width  = image->width;
  base->height = image->height;

  g_queue_push_head (stack, base);

  context = g_markup_parse_context_new (&markup_parser, 0, stack, NULL);

  while (success &&
         (bytes = fread (buf, sizeof (gchar), sizeof (buf), file)) > 0)
    success = g_markup_parse_context_parse (context, buf, bytes, error);

  if (success)
    success = g_markup_parse_context_end_parse (context, error);

  fclose (file);
  g_markup_parse_context_free (context);

  if (success)
    {
      if (base->paths)
        {
          GimpVectors *vectors;

          base->paths = g_list_reverse (base->paths);

          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_VECTORS_IMPORT,
                                       _("Import Paths"));

          vectors = gimp_vectors_new (image, _("Imported Path"));

          for (paths = base->paths; paths; paths = paths->next)
            {
              for (list = paths->data; list; list = list->next)
                gimp_vectors_stroke_add (vectors, GIMP_STROKE (list->data));

              if (!merge && paths->next)
                {
                  gimp_image_add_vectors (image, vectors, -1);
                  vectors = gimp_vectors_new (image, _("Imported Path"));
                }

              g_list_free (paths->data);
              paths->data = NULL;
            }

          gimp_image_add_vectors (image, vectors, -1);

          gimp_image_undo_group_end (image);
        }
      else
        {
          g_set_error (error, 0, 0, _("No paths found in '%s'"), filename);
          success = FALSE;
        }
    }

  while ((base = g_queue_pop_head (stack)) != NULL)
    {
      for (paths = base->paths; paths; paths = paths->next)
        {
          for (list = paths->data; list; list = list->next)
            g_object_unref (list->data);

          g_list_free (paths->data);
        }

      g_list_free (base->paths);
      g_free (base->transform);
      g_free (base);
    }

  g_queue_free (stack);

  return success;
}

static void
svg_parser_start_element (GMarkupParseContext *context,
                          const gchar         *element_name,
                          const gchar        **attribute_names,
                          const gchar        **attribute_values,
                          gpointer             user_data,
                          GError             **error)
{
  GQueue     *stack = user_data;
  SvgHandler *base;
  SvgHandler *handler = NULL;
  gint        i;

  base = g_queue_peek_head (stack);

  for (i = 0; !handler && i < G_N_ELEMENTS (svg_handlers); i++)
    {
      if (svg_handlers[i].name == NULL)
        handler = svg_handlers + i;
      else if (strcmp (svg_handlers[i].name, element_name) == 0)
        handler = svg_handlers + i;
    }

  handler = g_memdup (handler, sizeof (SvgHandler));
  handler->width  = base->width;
  handler->height = base->height;

  g_queue_push_head (stack, handler);

  if (handler->start)
    handler->start (handler, attribute_names, attribute_values);
}

static void
svg_parser_end_element (GMarkupParseContext *context,
                        const gchar         *element_name,
                        gpointer             user_data,
                        GError             **error)
{
  GQueue     *stack = user_data;
  SvgHandler *handler;
  SvgHandler *base;
  GList      *paths;

  handler = g_queue_pop_head (stack);

  if (handler->paths)
    {
      if (handler->transform)
        {
          for (paths = handler->paths; paths; paths = paths->next)
            {
              GList *list;

              for (list = paths->data; list; list = list->next)
                gimp_stroke_transform (GIMP_STROKE (list->data),
                                       handler->transform);
            }

          g_free (handler->transform);
        }

      base = g_queue_peek_head (stack);
      base->paths = g_list_concat (base->paths, handler->paths);
    }

  g_free (handler);
}

static void
svg_handler_svg (SvgHandler   *handler,
                 const gchar **names,
                 const gchar **values)
{
  while (*names)
    {
      if (strcmp (*names, "viewBox") == 0 && !handler->transform)
        {
          GimpMatrix3  matrix;

          if (parse_svg_viewbox (*values,
                                 handler->width, handler->height, &matrix))
            handler->transform = g_memdup (&matrix, sizeof (GimpMatrix3));
        }
      names++;
      values++;
    }
}

static void
svg_handler_group (SvgHandler   *handler,
                   const gchar **names,
                   const gchar **values)
{
  while (*names)
    {
      if (strcmp (*names, "transform") == 0 && !handler->transform)
        {
          GimpMatrix3  matrix;

          if (parse_svg_transform (*values, &matrix))
            handler->transform = g_memdup (&matrix, sizeof (GimpMatrix3));
        }

      names++;
      values++;
    }
}

static void
svg_handler_path (SvgHandler   *handler,
                  const gchar **names,
                  const gchar **values)
{
  while (*names)
    {
      if (strcmp (*names, "d") == 0)
        {
          handler->paths = g_list_prepend (handler->paths,
                                           parse_path_data (*values));
        }
      else if (strcmp (*names, "transform") == 0 && !handler->transform)
        {
          GimpMatrix3  matrix;

          if (parse_svg_transform (*values, &matrix))
            handler->transform = g_memdup (&matrix, sizeof (GimpMatrix3));
        }

      names++;
      values++;
    }
}

static gboolean
parse_svg_viewbox (const gchar *value,
                   gdouble      width,
                   gdouble      height,
                   GimpMatrix3 *matrix)
{
  gdouble   x, y, w, h;
  gchar    *tok;
  gchar    *str     = g_strdup (value);
  gboolean  success = FALSE;

  x = y = w = h = 0;

  gimp_matrix3_identity (matrix);

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

  if (!success)
    return FALSE;

  if (x || y)
    gimp_matrix3_translate (matrix, x, y);

  if (w && h)
    gimp_matrix3_scale (matrix, width / w, height / h);

  return TRUE;
}

gboolean
parse_svg_transform (const gchar *value,
                     GimpMatrix3 *matrix)
{
  gint    i;
  gchar   keyword[32];
  gdouble args[6];
  gint    n_args;
  gint    key_len;

  gimp_matrix3_identity (matrix);

  for (i= 0; value[i]; i++)
    {
      /* skip initial whitespace */
      while (g_ascii_isspace (value[i]))
        i++;

      /* parse keyword */
      for (key_len = 0; key_len < sizeof (keyword); key_len++)
        {
          gchar c = value[i];

          if (g_ascii_isalpha (c) || c == '-')
            keyword[key_len] = value[i++];
          else
            break;
        }

      if (key_len >= sizeof (keyword))
        return FALSE;

      keyword[key_len] = '\0';

      /* skip whitespace */
      while (g_ascii_isspace (value[i]))
        i++;

      if (value[i] != '(')
        return FALSE;
      i++;

      for (n_args = 0; ; n_args++)
        {
          gchar  c;
          gchar *end_ptr;

          /* skip whitespace */
          while (g_ascii_isspace (value[i]))
            i++;
          c = value[i];
          if (g_ascii_isdigit (c) || c == '+' || c == '-' || c == '.')
            {
              if (n_args == G_N_ELEMENTS(args))
                return FALSE; /* too many args */

              args[n_args] = g_ascii_strtod (value + i, &end_ptr);
              i = end_ptr - value;

              while (g_ascii_isspace (value[i]))
                i++;

              /* skip optional comma */
              if (value[i] == ',')
                i++;
            }
          else if (c == ')')
            break;
          else
            return FALSE;
        }

      /* ok, have parsed keyword and args, now modify the transform */
      if (!strcmp (keyword, "matrix"))
        {
          if (n_args != 6)
            return FALSE;

          gimp_matrix3_affine (matrix,
                               args[0], args[1],
                               args[2], args[3],
                               args[4], args[5]);
        }
      else if (!strcmp (keyword, "translate"))
        {
          if (n_args == 1)
            args[1] = 0.0;
          else if (n_args != 2)
            return FALSE;

          gimp_matrix3_translate (matrix, args[0], args[1]);
        }
      else if (!strcmp (keyword, "scale"))
        {
          if (n_args == 1)
            args[1] = args[0];
          else if (n_args != 2)
            return FALSE;

          gimp_matrix3_scale (matrix, args[0], args[1]);
        }
      else if (!strcmp (keyword, "rotate"))
        {
          if (n_args != 1)
            return FALSE;

          gimp_matrix3_rotate (matrix, gimp_deg_to_rad (args[0]));
        }
      else if (!strcmp (keyword, "skewX"))
        {
          if (n_args != 1)
            return FALSE;

          gimp_matrix3_xshear (matrix, tan (gimp_deg_to_rad (args[0])));
        }
      else if (!strcmp (keyword, "skewY"))
        {
          if (n_args != 1)
            return FALSE;

          gimp_matrix3_yshear (matrix, tan (gimp_deg_to_rad (args[0])));
        }
      else
        {
          return FALSE; /* unknown keyword */
        }
    }

  return TRUE;
}


/**********************************************************/
/*  Below is the code that parses the actual path data.   */
/*                                                        */
/*  This code is taken from librsvg and was originally    */
/*  written by Raph Levien <raph@artofcode.com> for Gill. */
/**********************************************************/

typedef struct
{
  GList       *strokes;
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


static GList *
parse_path_data (const gchar *data)
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

  memset (&ctx, 0, sizeof (ParsePathContext));

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

  return g_list_reverse (ctx.strokes);
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
          ctx->strokes = g_list_prepend (ctx->strokes, ctx->stroke);

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
    case 'a':
      if (ctx->param == 7 || final)
	{
          coords.x = ctx->cpx = ctx->rpx = ctx->params[5];
          coords.y = ctx->cpy = ctx->rpy = ctx->params[6];

          gimp_bezier_stroke_arcto (ctx->stroke,
                                    ctx->params[0], ctx->params[1],
                                    gimp_deg_to_rad (ctx->params[2]),
                                    ctx->params[3], ctx->params[4],
                                    &coords);
	  ctx->param = 0;
	}
      break;
    default:
      ctx->param = 0;
      break;
    }
}
