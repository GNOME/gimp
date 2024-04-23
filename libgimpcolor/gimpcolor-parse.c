/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolor-parse.c
 * Copyright (C) 2023 Jehan
 *
 * Some of the code in here was inspired and partly copied from pango
 * and librsvg.
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <babl/babl.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "gimpcolor.h"


static GeglColor * gimp_color_parse_name_internal (const gchar   *name);
static GeglColor * gimp_color_parse_hex_internal  (const gchar   *hex);
static GeglColor * gimp_color_parse_css_numeric   (const gchar   *css);
static GeglColor * gimp_color_parse_css_internal  (const gchar   *css);
static gchar     * gimp_color_parse_strip         (const gchar   *str,
                                                   gint           len);
static gint        gimp_color_entry_compare       (gconstpointer  a,
                                                   gconstpointer  b);
static gboolean    gimp_color_parse_hex_component (const gchar   *hex,
                                                   gint           len,
                                                   gdouble       *value);


typedef struct
{
  const gchar  *name;
  const guchar  red;
  const guchar  green;
  const guchar  blue;
} ColorEntry;

static const ColorEntry named_colors[] =
{
  { "aliceblue",             240, 248, 255 },
  { "antiquewhite",          250, 235, 215 },
  { "aqua",                    0, 255, 255 },
  { "aquamarine",            127, 255, 212 },
  { "azure",                 240, 255, 255 },
  { "beige",                 245, 245, 220 },
  { "bisque",                255, 228, 196 },
  { "black",                   0,   0,   0 },
  { "blanchedalmond",        255, 235, 205 },
  { "blue",                    0,   0, 255 },
  { "blueviolet",            138,  43, 226 },
  { "brown",                 165,  42,  42 },
  { "burlywood",             222, 184, 135 },
  { "cadetblue",              95, 158, 160 },
  { "chartreuse",            127, 255,   0 },
  { "chocolate",             210, 105,  30 },
  { "coral",                 255, 127,  80 },
  { "cornflowerblue",        100, 149, 237 },
  { "cornsilk",              255, 248, 220 },
  { "crimson",               220,  20,  60 },
  { "cyan",                    0, 255, 255 },
  { "darkblue",                0,   0, 139 },
  { "darkcyan",                0, 139, 139 },
  { "darkgoldenrod",         184, 134,  11 },
  { "darkgray",              169, 169, 169 },
  { "darkgreen",               0, 100,   0 },
  { "darkgrey",              169, 169, 169 },
  { "darkkhaki",             189, 183, 107 },
  { "darkmagenta",           139,   0, 139 },
  { "darkolivegreen",         85, 107,  47 },
  { "darkorange",            255, 140,   0 },
  { "darkorchid",            153,  50, 204 },
  { "darkred",               139,   0,   0 },
  { "darksalmon",            233, 150, 122 },
  { "darkseagreen",          143, 188, 143 },
  { "darkslateblue",          72,  61, 139 },
  { "darkslategray",          47,  79,  79 },
  { "darkslategrey",          47,  79,  79 },
  { "darkturquoise",           0, 206, 209 },
  { "darkviolet",            148,   0, 211 },
  { "deeppink",              255,  20, 147 },
  { "deepskyblue",             0, 191, 255 },
  { "dimgray",               105, 105, 105 },
  { "dimgrey",               105, 105, 105 },
  { "dodgerblue",             30, 144, 255 },
  { "firebrick",             178,  34,  34 },
  { "floralwhite" ,          255, 250, 240 },
  { "forestgreen",            34, 139,  34 },
  { "fuchsia",               255,   0, 255 },
  { "gainsboro",             220, 220, 220 },
  { "ghostwhite",            248, 248, 255 },
  { "gold",                  255, 215,   0 },
  { "goldenrod",             218, 165,  32 },
  { "gray",                  128, 128, 128 },
  { "green",                   0, 128,   0 },
  { "greenyellow",           173, 255,  47 },
  { "grey",                  128, 128, 128 },
  { "honeydew",              240, 255, 240 },
  { "hotpink",               255, 105, 180 },
  { "indianred",             205,  92,  92 },
  { "indigo",                 75,   0, 130 },
  { "ivory",                 255, 255, 240 },
  { "khaki",                 240, 230, 140 },
  { "lavender",              230, 230, 250 },
  { "lavenderblush",         255, 240, 245 },
  { "lawngreen",             124, 252,   0 },
  { "lemonchiffon",          255, 250, 205 },
  { "lightblue",             173, 216, 230 },
  { "lightcoral",            240, 128, 128 },
  { "lightcyan",             224, 255, 255 },
  { "lightgoldenrodyellow",  250, 250, 210 },
  { "lightgray",             211, 211, 211 },
  { "lightgreen",            144, 238, 144 },
  { "lightgrey",             211, 211, 211 },
  { "lightpink",             255, 182, 193 },
  { "lightsalmon",           255, 160, 122 },
  { "lightseagreen",          32, 178, 170 },
  { "lightskyblue",          135, 206, 250 },
  { "lightslategray",        119, 136, 153 },
  { "lightslategrey",        119, 136, 153 },
  { "lightsteelblue",        176, 196, 222 },
  { "lightyellow",           255, 255, 224 },
  { "lime",                    0, 255,   0 },
  { "limegreen",              50, 205,  50 },
  { "linen",                 250, 240, 230 },
  { "magenta",               255,   0, 255 },
  { "maroon",                128,   0,   0 },
  { "mediumaquamarine",      102, 205, 170 },
  { "mediumblue",              0,   0, 205 },
  { "mediumorchid",          186,  85, 211 },
  { "mediumpurple",          147, 112, 219 },
  { "mediumseagreen",         60, 179, 113 },
  { "mediumslateblue",       123, 104, 238 },
  { "mediumspringgreen",       0, 250, 154 },
  { "mediumturquoise",        72, 209, 204 },
  { "mediumvioletred",       199,  21, 133 },
  { "midnightblue",           25,  25, 112 },
  { "mintcream",             245, 255, 250 },
  { "mistyrose",             255, 228, 225 },
  { "moccasin",              255, 228, 181 },
  { "navajowhite",           255, 222, 173 },
  { "navy",                    0,   0, 128 },
  { "oldlace",               253, 245, 230 },
  { "olive",                 128, 128,   0 },
  { "olivedrab",             107, 142,  35 },
  { "orange",                255, 165,   0 },
  { "orangered",             255,  69,   0 },
  { "orchid",                218, 112, 214 },
  { "palegoldenrod",         238, 232, 170 },
  { "palegreen",             152, 251, 152 },
  { "paleturquoise",         175, 238, 238 },
  { "palevioletred",         219, 112, 147 },
  { "papayawhip",            255, 239, 213 },
  { "peachpuff",             255, 218, 185 },
  { "peru",                  205, 133,  63 },
  { "pink",                  255, 192, 203 },
  { "plum",                  221, 160, 221 },
  { "powderblue",            176, 224, 230 },
  { "purple",                128,   0, 128 },
  { "red",                   255,   0,   0 },
  { "rosybrown",             188, 143, 143 },
  { "royalblue",              65, 105, 225 },
  { "saddlebrown",           139,  69,  19 },
  { "salmon",                250, 128, 114 },
  { "sandybrown",            244, 164,  96 },
  { "seagreen",               46, 139,  87 },
  { "seashell",              255, 245, 238 },
  { "sienna",                160,  82,  45 },
  { "silver",                192, 192, 192 },
  { "skyblue",               135, 206, 235 },
  { "slateblue",             106,  90, 205 },
  { "slategray",             112, 128, 144 },
  { "slategrey",             112, 128, 144 },
  { "snow",                  255, 250, 250 },
  { "springgreen",             0, 255, 127 },
  { "steelblue",              70, 130, 180 },
  { "tan",                   210, 180, 140 },
  { "teal",                    0, 128, 128 },
  { "thistle",               216, 191, 216 },
  { "tomato",                255,  99,  71 },
  { "turquoise",              64, 224, 208 },
  { "violet",                238, 130, 238 },
  { "wheat",                 245, 222, 179 },
  { "white",                 255, 255, 255 },
  { "whitesmoke",            245, 245, 245 },
  { "yellow",                255, 255,   0 },
  { "yellowgreen",           154, 205,  50 }
};


/**
 * gimp_color_parse_css:
 * @css: (type utf8): a string describing a color in CSS notation
 *
 * Attempts to parse a string describing an sRGB color in CSS notation. This can
 * be either a numerical representation (`rgb(255,0,0)` or `rgb(100%,0%,0%)`)
 * or a hexadecimal notation as parsed by [func@color_parse_hex] (`##ff0000`) or
 * a color name as parsed by [func@color_parse_css] (`red`).
 *
 * Additionally the `rgba()`, `hsl()` and `hsla()` functions are supported too.
 *
 * Returns: (transfer full): a newly allocated [class@Gegl.Color] if @css was
 *                           parsed successfully, %NULL otherwise
 **/
GeglColor *
gimp_color_parse_css (const gchar *css)
{
  return gimp_color_parse_css_substring (css, -1);
}

/**
 * gimp_color_parse_hex:
 * @hex: (type utf8): a string describing a color in hexadecimal notation
 *
 * Attempts to parse a string describing a sRGB color in hexadecimal
 * notation (optionally prefixed with a '#').
 *
 * Returns: (transfer full): a newly allocated color representing @hex.
 **/
GeglColor *
gimp_color_parse_hex (const gchar *hex)
{
  return gimp_color_parse_hex_substring (hex, -1);
}

/**
 * gimp_color_parse_name:
 * @name: (type utf8): a color name (in UTF-8 encoding)
 *
 * Attempts to parse a color name. This function accepts [SVG 1.1 color
 * keywords](https://www.w3.org/TR/SVG11/types.html#ColorKeywords).
 *
 * Returns: (transfer full): a sRGB color as defined in "4.4. Recognized color
 *          keyword names" list of SVG 1.1 specification, if @name was parsed
 *          successfully, %NULL otherwise
 **/
GeglColor *
gimp_color_parse_name (const gchar *name)
{
  return gimp_color_parse_name_substring (name, -1);
}

/**
 * gimp_color_list_names:
 * @colors: (out) (optional) (array zero-terminated=1) (element-type GeglColor) (transfer full): return location for an array of [class@Gegl.Color]
 *
 * Returns the list of [SVG 1.0 color
 * keywords](https://www.w3.org/TR/SVG/types.html) that is recognized by
 * [func@color_parse_name].
 *
 * The returned strings are const and must not be freed. Only the array
 * must be freed with `g_free()`.
 *
 * The optional @colors arrays must be freed with [func@color_array_free] when
 * they are no longer needed.
 *
 * Returns: (array zero-terminated=1) (transfer container): an array of color names.
 *
 * Since: 2.2
 **/
const gchar **
gimp_color_list_names (GimpColorArray *colors)
{
  const gchar **names;
  gint    i;

  names = g_new0 (const gchar *, G_N_ELEMENTS (named_colors) + 1);

  if (colors)
    *colors = g_new0 (GeglColor *, G_N_ELEMENTS (named_colors) + 1);

  for (i = 0; i < G_N_ELEMENTS (named_colors); i++)
    {
      names[i] = named_colors[i].name;

      if (colors)
        {
          GeglColor *color = gegl_color_new (NULL);

          gegl_color_set_rgba_with_space (color,
                                          (gdouble) named_colors[i].red / 255.0,
                                          (gdouble) named_colors[i].green / 255.0,
                                          (gdouble) named_colors[i].blue / 255.0,
                                          1.0, NULL);
          (*colors)[i] = color;
        }
    }

  return names;
}

/**
 * gimp_color_parse_css_substring: (skip)
 * @css: (array length=len): a string describing a color in CSS notation
 * @len: the length of @css, in bytes. or -1 if @css is nul-terminated
 *
 * Attempts to parse a string describing an sRGB color in CSS notation. This can
 * be either a numerical representation (`rgb(255,0,0)` or `rgb(100%,0%,0%)`) or
 * a hexadecimal notation as parsed by [func@color_parse_hex] (`##ff0000`) or a
 * color name as parsed by [func@color_parse_name] (`red`).
 *
 * Additionally the `rgba()`, `hsl()` and `hsla()` functions are supported too.
 *
 * Returns: (transfer full): a newly allocated [class@Gegl.Color] if @css was
 *                           parsed successfully, %NULL otherwise
 *
 * Since: 2.2
 **/
GeglColor *
gimp_color_parse_css_substring (const gchar *css,
                                gint         len)
{
  gchar     *tmp;
  GeglColor *color;

  g_return_val_if_fail (css != NULL, FALSE);

  tmp = gimp_color_parse_strip (css, len);

  if (g_strcmp0 (tmp, "transparent") == 0)
    color = gegl_color_new ("transparent");
  else
    color = gimp_color_parse_css_internal (tmp);

  g_free (tmp);

  return color;
}

/**
 * gimp_color_parse_hex_substring: (skip)
 * @hex: (array length=len): a string describing a color in hexadecimal notation
 * @len: the length of @hex, in bytes. or -1 if @hex is nul-terminated
 *
 * Attempts to parse a string describing an RGB color in hexadecimal
 * notation (optionally prefixed with a '#').
 *
 * This function does not touch the alpha component of @rgb.
 *
 * Returns: (transfer full): a newly allocated color representing @hex.
 *
 * Since: 2.2
 **/
GeglColor *
gimp_color_parse_hex_substring (const gchar *hex,
                                gint         len)
{
  GeglColor *result;
  gchar     *tmp;

  g_return_val_if_fail (hex != NULL, FALSE);

  tmp = gimp_color_parse_strip (hex, len);

  result = gimp_color_parse_hex_internal (tmp);

  g_free (tmp);

  return result;
}

/**
 * gimp_color_parse_name_substring: (skip)
 * @name: (array length=len): a color name (in UTF-8 encoding)
 * @len:  the length of @name, in bytes. or -1 if @name is nul-terminated
 *
 * Attempts to parse a color name. This function accepts [SVG 1.1 color
 * keywords](https://www.w3.org/TR/SVG11/types.html#ColorKeywords).
 *
 * Returns: (transfer full): a sRGB color as defined in "4.4. Recognized color
 *          keyword names" list of SVG 1.1 specification, if @name was parsed
 *          successfully, %NULL otherwise
 *
 * Since: 2.2
 **/
GeglColor *
gimp_color_parse_name_substring (const gchar *name,
                                 gint         len)
{
  gchar     *tmp;
  GeglColor *result;

  g_return_val_if_fail (name != NULL, FALSE);

  tmp = gimp_color_parse_strip (name, len);

  result = gimp_color_parse_name_internal (tmp);

  g_free (tmp);

  return result;
}


/* Private functions. */

static GeglColor *
gimp_color_parse_name_internal (const gchar *name)
{
  /* GeglColor also has name reading support. It supports HTML 4.01 standard
   * whereas here we have SVG 1.0 name support. Moreover we support a lot more
   * colors.
   */
  ColorEntry *entry = bsearch (name, named_colors,
                               G_N_ELEMENTS (named_colors), sizeof (ColorEntry),
                               gimp_color_entry_compare);

  if (entry)
    {
      GeglColor *color = gegl_color_new (NULL);

      gegl_color_set_rgba_with_space (color, (gdouble) entry->red / 255.0,
                                      (gdouble) entry->green / 255.0, (gdouble) entry->blue / 255.0,
                                      1.0, NULL);

      return color;
    }

  return NULL;
}

static GeglColor *
gimp_color_parse_hex_internal (const gchar *hex)
{
  GeglColor *color;
  gint       i;
  gsize      len;
  gdouble    val[3];

  if (hex[0] == '#')
    hex++;

  len = strlen (hex);
  /* TODO: current implementation has 2 issues:
   * 1. It doesn't support the alpha channel, even though CSS spec now has
   *    support for it with either 8 or 4 digits.
   * 2. The spec has nothing about channels on 3 or 4 digits, which we support
   *    here (for higher precision?). Is this format really supported somewhere?
   *    Do we want to keep this?
   * See: https://drafts.csswg.org/css-color/#hex-notation
   */
  if (len % 3 || len < 3 || len > 12)
    return NULL;

  len /= 3;

  for (i = 0; i < 3; i++, hex += len)
    {
      if (! gimp_color_parse_hex_component (hex, len, val + i))
        return NULL;
    }

  color = gegl_color_new (NULL);
  gegl_color_set_pixel (color, babl_format ("R'G'B' double"), val);

  return color;
}

static GeglColor *
gimp_color_parse_css_numeric (const gchar *css)
{
  GeglColor *color;
  gdouble    values[4];
  gboolean   alpha;
  gboolean   hsl;
  gint       i;

  if (css[0] == 'r' && css[1] == 'g' && css[2] == 'b')
    hsl = FALSE;
  else if (css[0] == 'h' && css[1] == 's' && css[2] == 'l')
    hsl = TRUE;
  else
    g_return_val_if_reached (NULL);

  if (css[3] == 'a' && css[4] == '(')
    alpha = TRUE;
  else if (css[3] == '(')
    alpha = FALSE;
  else
    g_return_val_if_reached (NULL);

  css += (alpha ? 5 : 4);

  for (i = 0; i < (alpha ? 4 : 3); i++)
    {
      const gchar *end = css;

      while (*end && *end != ',' && *end != '%' && *end != ')')
        end++;

      if (i == 3 || *end == '%')
        {
          values[i] = g_ascii_strtod (css, (gchar **) &end);

          if (errno == ERANGE)
            return FALSE;

          if (*end == '%')
            {
              end++;
              values[i] /= 100.0;
            }
        }
      else
        {
          glong value = strtol (css, (gchar **) &end, 10);

          if (errno == ERANGE)
            return FALSE;

          if (hsl)
            values[i] = value / (i == 0 ? 360.0 : 100.0);
          else
            values[i] = value / 255.0;
        }

      /* CSS Color specs indicates:
       * > Values outside these ranges are not invalid, but are clamped to the
       * > ranges defined here at parsed-value time.
       * See: https://drafts.csswg.org/css-color/#rgb-functions
       * So even though we might hope being able to reach non-sRGB colors when
       * using the percentage syntax, the spec explicitly forbids it.
       */
      values[i] = CLAMP (values[i], 0.0, 1.0);

      while (*end == ',' || g_ascii_isspace (*end))
        end++;

      css = end;
    }

  if (*css != ')')
    return NULL;

  color = gegl_color_new (NULL);
  if (hsl)
    {
      gfloat values_f[4];

      for (i = 0; i < (alpha ? 4 : 3); i++)
        values_f[i] = (gfloat) values[i];

      if (alpha)
        gegl_color_set_pixel (color, babl_format ("HSLA float"), values_f);
      else
        gegl_color_set_pixel (color, babl_format ("HSL float"), values_f);
    }
  else
    {
      if (alpha)
        gegl_color_set_pixel (color, babl_format ("R'G'B'A double"), values);
      else
        gegl_color_set_pixel (color, babl_format ("R'G'B' double"), values);
    }

  return color;
}

static GeglColor *
gimp_color_parse_css_internal (const gchar *css)
{
  if (css[0] == '#')
    {
      return gimp_color_parse_hex_internal (css);
    }
  else if (strncmp (css, "rgb(", 4)  == 0 ||
           strncmp (css, "hsl(", 4)  == 0 ||
           strncmp (css, "rgba(", 5) == 0 ||
           strncmp (css, "hsla(", 5) == 0)
    {
      return gimp_color_parse_css_numeric (css);
    }
  else
    {
      return gimp_color_parse_name_internal (css);
    }
}

static gchar *
gimp_color_parse_strip (const gchar *str,
                        gint         len)
{
  gchar *result;

  while (len > 0 && g_ascii_isspace (*str))
    {
      str++;
      len--;
    }

  if (len < 0)
    {
      while (g_ascii_isspace (*str))
        str++;

      len = strlen (str);
    }

  while (len > 0 && g_ascii_isspace (str[len - 1]))
    len--;

  result = g_malloc (len + 1);

  memcpy (result, str, len);
  result[len] = '\0';

  return result;
}

static gint
gimp_color_entry_compare (gconstpointer a,
                          gconstpointer b)
{
  const gchar      *name  = a;
  const ColorEntry *entry = b;

  return g_ascii_strcasecmp (name, entry->name);
}

static gboolean
gimp_color_parse_hex_component (const gchar *hex,
                                gint         len,
                                gdouble     *value)
{
  gint  i;
  guint c = 0;

  for (i = 0; i < len; i++, hex++)
    {
      if (!*hex || !g_ascii_isxdigit (*hex))
        return FALSE;

      c = (c << 4) | g_ascii_xdigit_value (*hex);
    }

  switch (len)
    {
    case 1: *value = (gdouble) c /    15.0;  break;
    case 2: *value = (gdouble) c /   255.0;  break;
    case 3: *value = (gdouble) c /  4095.0;  break;
    case 4: *value = (gdouble) c / 65535.0;  break;
    default:
      g_return_val_if_reached (FALSE);
    }

  return TRUE;
}
