/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * IfsCompose is a interface for creating IFS fractals by
 * direct manipulation.
 * Copyright (C) 1997 Owen Taylor
 *
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTBILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h> /* strlen */

#include <gdk/gdk.h>

#include <libgimp/gimp.h>

#include "ifs-compose.h"


typedef enum {
  TOKEN_INVALID = G_TOKEN_LAST,
  TOKEN_ITERATIONS,
  TOKEN_MAX_MEMORY,
  TOKEN_SUBDIVIDE,
  TOKEN_RADIUS,
  TOKEN_ASPECT_RATIO,
  TOKEN_CENTER_X,
  TOKEN_CENTER_Y,
  TOKEN_ELEMENT,
  TOKEN_X,
  TOKEN_Y,
  TOKEN_THETA,
  TOKEN_SCALE,
  TOKEN_ASYM,
  TOKEN_SHEAR,
  TOKEN_FLIP,
  TOKEN_RED_COLOR,
  TOKEN_GREEN_COLOR,
  TOKEN_BLUE_COLOR,
  TOKEN_BLACK_COLOR,
  TOKEN_TARGET_COLOR,
  TOKEN_HUE_SCALE,
  TOKEN_VALUE_SCALE,
  TOKEN_SIMPLE_COLOR,
  TOKEN_PROB
} IfsComposeToken;

static struct
{
  const gchar     *name;
  IfsComposeToken  token;
} symbols[] = {
  { "iterations",   TOKEN_ITERATIONS },
  { "max_memory",   TOKEN_MAX_MEMORY },
  { "subdivide",    TOKEN_SUBDIVIDE },
  { "radius",       TOKEN_RADIUS },
  { "aspect_ratio", TOKEN_ASPECT_RATIO },
  { "center_x",     TOKEN_CENTER_X },
  { "center_y",     TOKEN_CENTER_Y },
  { "element",      TOKEN_ELEMENT },
  { "x",            TOKEN_X },
  { "y",            TOKEN_Y },
  { "theta",        TOKEN_THETA },
  { "scale",        TOKEN_SCALE },
  { "asym",         TOKEN_ASYM },
  { "shear",        TOKEN_SHEAR },
  { "flip",         TOKEN_FLIP },
  { "red_color",    TOKEN_RED_COLOR },
  { "green_color",  TOKEN_GREEN_COLOR },
  { "blue_color",   TOKEN_BLUE_COLOR },
  { "black_color",  TOKEN_BLACK_COLOR },
  { "target_color", TOKEN_TARGET_COLOR },
  { "hue_scale",    TOKEN_HUE_SCALE },
  { "value_scale",  TOKEN_VALUE_SCALE },
  { "simple_color", TOKEN_SIMPLE_COLOR },
  { "prob",         TOKEN_PROB }
};

static GTokenType
ifsvals_parse_color (GScanner  *scanner,
                     GeglColor *result)
{
  GTokenType token;
  gdouble    rgb[3];

  token = g_scanner_get_next_token (scanner);
  if (token != G_TOKEN_LEFT_CURLY)
    return G_TOKEN_LEFT_CURLY;

  token = g_scanner_get_next_token (scanner);
  if (token == G_TOKEN_FLOAT)
    rgb[0] = scanner->value.v_float;
  else if (token == G_TOKEN_INT)
    rgb[0] = (gdouble) scanner->value.v_int;
  else
    return G_TOKEN_FLOAT;

  token = g_scanner_get_next_token (scanner);
  if (token != G_TOKEN_COMMA)
    return G_TOKEN_COMMA;

  token = g_scanner_get_next_token (scanner);
  if (token == G_TOKEN_FLOAT)
    rgb[1] = scanner->value.v_float;
  else if (token == G_TOKEN_INT)
    rgb[1] = (gdouble) scanner->value.v_int;
  else
    return G_TOKEN_FLOAT;

  token = g_scanner_get_next_token (scanner);
  if (token != G_TOKEN_COMMA)
    return G_TOKEN_COMMA;

  token = g_scanner_get_next_token (scanner);
  if (token == G_TOKEN_FLOAT)
    rgb[2] = scanner->value.v_float;
  else if (token == G_TOKEN_INT)
    rgb[2] = (gdouble) scanner->value.v_int;
  else
    return G_TOKEN_FLOAT;

  token = g_scanner_get_next_token (scanner);
  if (token != G_TOKEN_RIGHT_CURLY)
    return G_TOKEN_RIGHT_CURLY;

  gegl_color_set_pixel (result, babl_format ("R'G'B' double"), rgb);

  return G_TOKEN_NONE;
}

/* Parse a float which (unlike G_TOKEN_FLOAT) can be negative
 */
static GTokenType
parse_genuine_float (GScanner *scanner,
                     gdouble  *result)
{
  gboolean negate = FALSE;
  GTokenType token;

  token = g_scanner_get_next_token (scanner);

  if (token == '-')
    {
      negate = TRUE;
      token = g_scanner_get_next_token (scanner);
    }

  if (token == G_TOKEN_FLOAT)
    {
      *result = negate ? -scanner->value.v_float : scanner->value.v_float;
      return G_TOKEN_NONE;
    }
  else if (token == G_TOKEN_INT)
    {
      *result = negate ? -scanner->value.v_int : scanner->value.v_int;
      return G_TOKEN_NONE;
    }
  else
    return G_TOKEN_FLOAT;
}

static GTokenType
ifsvals_parse_element (GScanner       *scanner,
                       AffElementVals *result)
{
  GTokenType token;
  GTokenType expected_token;

  token = g_scanner_get_next_token (scanner);
  if (token != G_TOKEN_LEFT_CURLY)
    return G_TOKEN_LEFT_CURLY;

  token = g_scanner_get_next_token (scanner);
  while (token != G_TOKEN_RIGHT_CURLY)
    {
      switch ((IfsComposeToken) token)
	{
	case TOKEN_X:
	  expected_token = parse_genuine_float (scanner, &result->x);
	  if (expected_token != G_TOKEN_NONE)
	    return expected_token;
	  break;

	case TOKEN_Y:
	  expected_token = parse_genuine_float (scanner, &result->y);
	  if (expected_token != G_TOKEN_NONE)
	    return expected_token;
	  break;

	case TOKEN_THETA:
	  expected_token = parse_genuine_float (scanner, &result->theta);
	  if (expected_token != G_TOKEN_NONE)
	    return expected_token;
	  break;

	case TOKEN_SCALE:
	  expected_token = parse_genuine_float (scanner, &result->scale);
	  if (expected_token != G_TOKEN_NONE)
	    return expected_token;
	  break;

	case TOKEN_ASYM:
	  expected_token = parse_genuine_float (scanner, &result->asym);
	  if (expected_token != G_TOKEN_NONE)
	    return expected_token;
	  break;

	case TOKEN_SHEAR:
	  expected_token = parse_genuine_float (scanner, &result->shear);
	  if (expected_token != G_TOKEN_NONE)
	    return expected_token;
	  break;

	case TOKEN_FLIP:
	  token = g_scanner_get_next_token (scanner);
	  if (token != G_TOKEN_INT)
	    return G_TOKEN_INT;

	  result->flip = scanner->value.v_int;
	  break;

	case TOKEN_RED_COLOR:
	  token = ifsvals_parse_color (scanner, result->red_color);
	  if (token != G_TOKEN_NONE)
	    return token;
	  break;

	case TOKEN_GREEN_COLOR:
	  token = ifsvals_parse_color (scanner, result->green_color);
	  if (token != G_TOKEN_NONE)
	    return token;
	  break;

	case TOKEN_BLUE_COLOR:
	  token = ifsvals_parse_color (scanner, result->blue_color);
	  if (token != G_TOKEN_NONE)
	    return token;
	  break;

	case TOKEN_BLACK_COLOR:
	  token = ifsvals_parse_color (scanner, result->black_color);
	  if (token != G_TOKEN_NONE)
	    return token;
	  break;

	case TOKEN_TARGET_COLOR:
	  token = ifsvals_parse_color (scanner, result->target_color);
	  if (token != G_TOKEN_NONE)
	    return token;
	  break;

	case TOKEN_HUE_SCALE:
	  expected_token = parse_genuine_float (scanner, &result->hue_scale);
	  if (expected_token != G_TOKEN_NONE)
	    return expected_token;
	  break;

	case TOKEN_VALUE_SCALE:
	  expected_token = parse_genuine_float (scanner, &result->value_scale);
	  if (expected_token != G_TOKEN_NONE)
	    return expected_token;
	  break;

	case TOKEN_SIMPLE_COLOR:
	  token = g_scanner_get_next_token (scanner);
	  if (token != G_TOKEN_INT)
	    return G_TOKEN_INT;

	  result->simple_color = scanner->value.v_int;
	  break;

	case TOKEN_PROB:
	  token = g_scanner_get_next_token (scanner);
	  if (token == G_TOKEN_FLOAT)
            result->prob = scanner->value.v_float;
          else if (token == G_TOKEN_INT)
            result->prob = scanner->value.v_int;
          else
	    return G_TOKEN_FLOAT;

	  break;

	default:
	  return G_TOKEN_SYMBOL;
	}

      token = g_scanner_get_next_token (scanner);
    }

  return G_TOKEN_NONE;
}

/*************************************************************
 * ifsvals_parse:
 *     Read in ifsvalues from a GScanner
 *   arguments:
 *     scanner:
 *     vals:
 *     elements:
 *
 *   results:
 *     If parsing succeeded, TRUE; otherwise FALSE, in which
 *     case vals and elements are unchanged
 *************************************************************/

static gboolean
ifsvals_parse (GScanner         *scanner,
               IfsComposeVals   *vals,
               AffElement     ***elements)
{
  GTokenType      token, expected_token;
  AffElement     *el;
  IfsComposeVals  new_vals;
  GeglColor      *color;

  GList          *el_list = NULL;
  GList          *tmp_list;
  gint            i;

  new_vals              = *vals;
  new_vals.num_elements = 0;
  color                 = gegl_color_new ("black");
  i                     = 0;

  expected_token = G_TOKEN_NONE;
  while (expected_token == G_TOKEN_NONE)
    {
      token = g_scanner_get_next_token (scanner);

      if (g_scanner_eof (scanner))
	  break;

      switch ((IfsComposeToken) token)
	{
	case TOKEN_ITERATIONS:
	  token = g_scanner_get_next_token (scanner);
	  if (token == G_TOKEN_INT)
	    new_vals.iterations = scanner->value.v_int;
	  else
	    expected_token = G_TOKEN_INT;
	  break;

	case TOKEN_MAX_MEMORY:
	  token = g_scanner_get_next_token (scanner);
	  if (token == G_TOKEN_INT)
	    new_vals.max_memory = scanner->value.v_int;
	  else
	    expected_token = G_TOKEN_INT;
	  break;

	case TOKEN_SUBDIVIDE:
	  token = g_scanner_get_next_token (scanner);
	  if (token == G_TOKEN_INT)
	    new_vals.subdivide = scanner->value.v_int;
	  else
	    expected_token = G_TOKEN_INT;
	  break;

	case TOKEN_RADIUS:
	  expected_token = parse_genuine_float (scanner, &new_vals.radius);
	  break;

	case TOKEN_ASPECT_RATIO:
	  expected_token = parse_genuine_float (scanner, &new_vals.aspect_ratio);
	  break;

	case TOKEN_CENTER_X:
	  expected_token = parse_genuine_float (scanner, &new_vals.center_x);
	  break;

	case TOKEN_CENTER_Y:
	  expected_token = parse_genuine_float (scanner, &new_vals.center_y);
	  break;

	case TOKEN_ELEMENT:
	  el = aff_element_new (0.0,0.0, color, ++i);
	  expected_token = ifsvals_parse_element (scanner, &el->v);

	  if (expected_token == G_TOKEN_NONE)
	    {
	      el_list = g_list_prepend (el_list, el);
	      new_vals.num_elements++;
	    }
	  else
	    aff_element_free (el);

	  break;

	default:
	  expected_token = G_TOKEN_SYMBOL;
	}
    }

  g_object_unref (color);

  if (expected_token != G_TOKEN_NONE)
    {
      g_scanner_unexp_token (scanner,
			     expected_token,
			     NULL,
			     NULL,
			     NULL,
			     "using default values...",
			     TRUE);
      g_list_free_full (el_list, (GDestroyNotify) g_free);
      return FALSE;
    }

  *vals = new_vals;

  el_list = g_list_reverse (el_list);
  *elements = g_new (AffElement *, new_vals.num_elements);

  tmp_list = el_list;
  for (i=0; i<new_vals.num_elements; i++)
    {
      (*elements)[i] = tmp_list->data;
      tmp_list = tmp_list->next;
    }

  g_list_free (el_list);

  return TRUE;
}

gboolean
ifsvals_parse_string (const gchar      *str,
                      IfsComposeVals   *vals,
                      AffElement     ***elements)
{
  GScanner *scanner = g_scanner_new (NULL);
  gboolean  result;
  gint      i;

  scanner->config->symbol_2_token = TRUE;
  scanner->config->scan_identifier_1char = TRUE;
  scanner->input_name = "IfsCompose Saved Data";

  for (i = 0; i < G_N_ELEMENTS (symbols); i++)
    g_scanner_scope_add_symbol (scanner, 0,
                                symbols[i].name,
                                GINT_TO_POINTER (symbols[i].token));

  g_scanner_input_text (scanner, str, strlen (str));

  result = ifsvals_parse (scanner, vals, elements);

  g_scanner_destroy (scanner);

  return result;
}

/*************************************************************
 * ifsvals_stringify:
 *     Stringify a set of vals and elements
 *   arguments:
 *     vals:
 *     elements
 *   results:
 *     The stringified result (free with g_free)
 *************************************************************/

gchar *
ifsvals_stringify (IfsComposeVals  *vals,
                   AffElement     **elements)
{
  gint     i;
  gchar    buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar    cbuf[3][G_ASCII_DTOSTR_BUF_SIZE];
  GString *result;

  result = g_string_new (NULL);

  g_string_append_printf (result, "iterations %d\n", vals->iterations);
  g_string_append_printf (result, "max_memory %d\n", vals->max_memory);
  g_string_append_printf (result, "subdivide %d\n", vals->subdivide);
  g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, vals->radius);
  g_string_append_printf (result, "radius %s\n", buf);
  g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, vals->aspect_ratio);
  g_string_append_printf (result, "aspect_ratio %s\n", buf);
  g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, vals->center_x);
  g_string_append_printf (result, "center_x %s\n", buf);
  g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, vals->center_y);
  g_string_append_printf (result, "center_y %s\n", buf);

  for (i=0; i<vals->num_elements; i++)
    {
      gdouble rgb[3];

      g_string_append (result, "element {\n");
      g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, elements[i]->v.x);
      g_string_append_printf (result, "    x %s\n", buf);
      g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, elements[i]->v.y);
      g_string_append_printf (result, "    y %s\n", buf);
      g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, elements[i]->v.theta);
      g_string_append_printf (result, "    theta %s\n", buf);
      g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, elements[i]->v.scale);
      g_string_append_printf (result, "    scale %s\n", buf);
      g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, elements[i]->v.asym);
      g_string_append_printf (result, "    asym %s\n", buf);
      g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, elements[i]->v.shear);
      g_string_append_printf (result, "    shear %s\n", buf);
      g_string_append_printf (result, "    flip %d\n", elements[i]->v.flip);

      gegl_color_get_pixel (elements[i]->v.red_color, babl_format ("R'G'B' double"), rgb);
      g_ascii_dtostr (cbuf[0], G_ASCII_DTOSTR_BUF_SIZE, rgb[0]);
      g_ascii_dtostr (cbuf[1], G_ASCII_DTOSTR_BUF_SIZE, rgb[1]);
      g_ascii_dtostr (cbuf[2], G_ASCII_DTOSTR_BUF_SIZE, rgb[2]);
      g_string_append_printf (result, "    red_color { %s,%s,%s }\n",
                              cbuf[0], cbuf[1], cbuf[2]);

      gegl_color_get_pixel (elements[i]->v.green_color, babl_format ("R'G'B' double"), rgb);
      g_ascii_dtostr (cbuf[0], G_ASCII_DTOSTR_BUF_SIZE, rgb[0]);
      g_ascii_dtostr (cbuf[1], G_ASCII_DTOSTR_BUF_SIZE, rgb[1]);
      g_ascii_dtostr (cbuf[2], G_ASCII_DTOSTR_BUF_SIZE, rgb[2]);
      g_string_append_printf (result, "    green_color { %s,%s,%s }\n",
                              cbuf[0], cbuf[1], cbuf[2]);

      gegl_color_get_pixel (elements[i]->v.blue_color, babl_format ("R'G'B' double"), rgb);
      g_ascii_dtostr (cbuf[0], G_ASCII_DTOSTR_BUF_SIZE, rgb[0]);
      g_ascii_dtostr (cbuf[1], G_ASCII_DTOSTR_BUF_SIZE, rgb[1]);
      g_ascii_dtostr (cbuf[2], G_ASCII_DTOSTR_BUF_SIZE, rgb[2]);
      g_string_append_printf (result, "    blue_color { %s,%s,%s }\n",
                              cbuf[0], cbuf[1], cbuf[2]);

      gegl_color_get_pixel (elements[i]->v.black_color, babl_format ("R'G'B' double"), rgb);
      g_ascii_dtostr (cbuf[0], G_ASCII_DTOSTR_BUF_SIZE, rgb[0]);
      g_ascii_dtostr (cbuf[1], G_ASCII_DTOSTR_BUF_SIZE, rgb[1]);
      g_ascii_dtostr (cbuf[2], G_ASCII_DTOSTR_BUF_SIZE, rgb[2]);
      g_string_append_printf (result, "    black_color { %s,%s,%s }\n",
                              cbuf[0], cbuf[1], cbuf[2]);

      gegl_color_get_pixel (elements[i]->v.target_color, babl_format ("R'G'B' double"), rgb);
      g_ascii_dtostr (cbuf[0], G_ASCII_DTOSTR_BUF_SIZE, rgb[0]);
      g_ascii_dtostr (cbuf[1], G_ASCII_DTOSTR_BUF_SIZE, rgb[1]);
      g_ascii_dtostr (cbuf[2], G_ASCII_DTOSTR_BUF_SIZE, rgb[2]);
      g_string_append_printf (result, "    target_color { %s,%s,%s }\n",
                              cbuf[0], cbuf[1], cbuf[2]);

      g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, elements[i]->v.hue_scale);
      g_string_append_printf (result, "    hue_scale %s\n", buf);
      g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, elements[i]->v.value_scale);
      g_string_append_printf (result, "    value_scale %s\n", buf);
      g_string_append_printf (result, "    simple_color %d\n",
                              elements[i]->v.simple_color);
      g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, elements[i]->v.prob);
      g_string_append_printf (result, "    prob %s\n", buf);
      g_string_append (result, "}\n");
    }

  return g_string_free (result, FALSE);
}
