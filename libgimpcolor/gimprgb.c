/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "libgimpmath/gimpmath.h"

#include "gimpcolortypes.h"

#include "gimprgb.h"


static gboolean  gimp_rgb_parse (GimpRGB     *rgb,
                                 const gchar *name);


/*  RGB functions  */

/**
 * gimp_rgb_set:
 * @rgb: a #GimpRGB struct
 * @r: red
 * @g: green
 * @b: blue
 *
 * Sets the red, green and blue components of @rgb and leaves the
 * alpha component unchanged. The color values should be between 0.0
 * and 1.0 but there is no check to enforce this and the values are
 * set exactly as they are passed in.
 **/
void
gimp_rgb_set (GimpRGB *rgb,
	      gdouble  r,
	      gdouble  g,
	      gdouble  b)
{
  g_return_if_fail (rgb != NULL);

  rgb->r = r;
  rgb->g = g;
  rgb->b = b;
}

/**
 * gimp_rgb_set_alpha:
 * @rgb: a #GimpRGB struct
 * @a: alpha
 *
 * Sets the alpha component of @rgb and leaves the RGB components unchanged.
 **/
void
gimp_rgb_set_alpha (GimpRGB *rgb,
		    gdouble  a)
{
  g_return_if_fail (rgb != NULL);

  rgb->a = a;
}

/**
 * gimp_rgb_set_uchar:
 * @rgb: a #GimpRGB struct
 * @r: red
 * @g: green
 * @b: blue
 *
 * Sets the red, green and blue components of @rgb from 8bit values
 * (0 to 255) and leaves the alpha component unchanged.
 **/
void
gimp_rgb_set_uchar (GimpRGB *rgb,
		    guchar   r,
		    guchar   g,
		    guchar   b)
{
  g_return_if_fail (rgb != NULL);

  rgb->r = (gdouble) r / 255.0;
  rgb->g = (gdouble) g / 255.0;
  rgb->b = (gdouble) b / 255.0;
}

void
gimp_rgb_get_uchar (const GimpRGB *rgb,
		    guchar        *r,
		    guchar        *g,
		    guchar        *b)
{
  g_return_if_fail (rgb != NULL);

  if (r) *r = ROUND (CLAMP (rgb->r, 0.0, 1.0) * 255.0);
  if (g) *g = ROUND (CLAMP (rgb->g, 0.0, 1.0) * 255.0);
  if (b) *b = ROUND (CLAMP (rgb->b, 0.0, 1.0) * 255.0);
}

/**
 * gimp_rgb_parse_name:
 * @rgb:  a #GimpRGB struct used to return the parsed color
 * @name: a color name (in UTF-8 encoding)
 * @len:  the length of @name, in bytes. or -1 if @name is nul-terminated
 *
 * Attempts to parse a color name. This function accepts RGB hex
 * values or <link
 * linkend="http://www.w3.org/TR/SVG/types.html#ColorKeywords">SVG 1.0
 * color keywords</link>.  The format of an RGB value in hexadecimal
 * notation is a '#' immediately followed by either three or six
 * hexadecimal characters.
 *
 * This funcion does not touch the alpha component of @rgb.
 *
 * Return value: %TRUE if @name was parsed successfully and @rgb has been
 *               set, %FALSE otherwise
 *
 * Since: GIMP 2.2
 **/
gboolean
gimp_rgb_parse_name (GimpRGB     *rgb,
                     const gchar *name,
                     gsize        len)
{
  gchar    *tmp;
  gboolean  result;

  g_return_val_if_fail (rgb != NULL, FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  if (len < 0)
    len = strlen (name);

  tmp = g_strstrip (g_strndup (name, len));

  result = gimp_rgb_parse (rgb, tmp);

  g_free (tmp);

  return result;
}

void
gimp_rgb_add (GimpRGB       *rgb1,
	      const GimpRGB *rgb2)
{
  g_return_if_fail (rgb1 != NULL);
  g_return_if_fail (rgb2 != NULL);

  rgb1->r += rgb2->r;
  rgb1->g += rgb2->g;
  rgb1->b += rgb2->b;
}

void
gimp_rgb_subtract (GimpRGB       *rgb1,
		   const GimpRGB *rgb2)
{
  g_return_if_fail (rgb1 != NULL);
  g_return_if_fail (rgb2 != NULL);

  rgb1->r -= rgb2->r;
  rgb1->g -= rgb2->g;
  rgb1->b -= rgb2->b;
}

void
gimp_rgb_multiply (GimpRGB *rgb,
		   gdouble  factor)
{
  g_return_if_fail (rgb != NULL);

  rgb->r *= factor;
  rgb->g *= factor;
  rgb->b *= factor;
}

gdouble
gimp_rgb_distance (const GimpRGB *rgb1,
		   const GimpRGB *rgb2)
{
  g_return_val_if_fail (rgb1 != NULL, 0.0);
  g_return_val_if_fail (rgb2 != NULL, 0.0);

  return (fabs (rgb1->r - rgb2->r) +
	  fabs (rgb1->g - rgb2->g) +
	  fabs (rgb1->b - rgb2->b));
}

gdouble
gimp_rgb_max (const GimpRGB *rgb)
{
  g_return_val_if_fail (rgb != NULL, 0.0);

  if (rgb->r > rgb->g)
     return (rgb->r > rgb->b) ? rgb->r : rgb->b;
  return (rgb->g > rgb->b) ? rgb->g : rgb->b;
}

gdouble
gimp_rgb_min (const GimpRGB *rgb)
{
  g_return_val_if_fail (rgb != NULL, 0.0);

  if (rgb->r < rgb->g)
     return (rgb->r < rgb->b) ? rgb->r : rgb->b;
  return (rgb->g < rgb->b) ? rgb->g : rgb->b;
}

void
gimp_rgb_clamp (GimpRGB *rgb)
{
  g_return_if_fail (rgb != NULL);

  rgb->r = CLAMP (rgb->r, 0.0, 1.0);
  rgb->g = CLAMP (rgb->g, 0.0, 1.0);
  rgb->b = CLAMP (rgb->b, 0.0, 1.0);
  rgb->a = CLAMP (rgb->a, 0.0, 1.0);
}

void
gimp_rgb_gamma (GimpRGB *rgb,
		gdouble  gamma)
{
  gdouble ig;

  g_return_if_fail (rgb != NULL);

  if (gamma != 0.0)
    ig = 1.0 / gamma;
  else
    ig = 0.0;

  rgb->r = pow (rgb->r, ig);
  rgb->g = pow (rgb->g, ig);
  rgb->b = pow (rgb->b, ig);
}

gdouble
gimp_rgb_intensity (const GimpRGB *rgb)
{
  gdouble intensity;

  g_return_val_if_fail (rgb != NULL, 0.0);

  intensity = GIMP_RGB_INTENSITY (rgb->r, rgb->g, rgb->b);

  return CLAMP (intensity, 0.0, 1.0);
}

guchar
gimp_rgb_intensity_uchar (const GimpRGB *rgb)
{
  g_return_val_if_fail (rgb != NULL, 0);

  return ROUND (gimp_rgb_intensity (rgb) * 255.0);
}

void
gimp_rgb_composite (GimpRGB              *color1,
		    const GimpRGB        *color2,
		    GimpRGBCompositeMode  mode)
{
  gdouble factor;

  g_return_if_fail (color1 != NULL);
  g_return_if_fail (color2 != NULL);

  switch (mode)
    {
    case GIMP_RGB_COMPOSITE_NONE:
      break;

    case GIMP_RGB_COMPOSITE_NORMAL:
      /*  put color2 on top of color1  */
      if (color2->a == 1.0)
	{
	  *color1 = *color2;
	}
      else
	{
	  factor = color1->a * (1.0 - color2->a);
	  color1->r = color1->r * factor + color2->r * color2->a;
	  color1->g = color1->g * factor + color2->g * color2->a;
	  color1->b = color1->b * factor + color2->b * color2->a;
	  color1->a = factor + color2->a;
	}
      break;

    case GIMP_RGB_COMPOSITE_BEHIND:
      /*  put color2 below color1  */
      if (color1->a < 1.0)
	{
	  factor = color2->a * (1.0 - color1->a);
	  color1->r = color2->r * factor + color1->r * color1->a;
	  color1->g = color2->g * factor + color1->g * color1->a;
	  color1->b = color2->b * factor + color1->b * color1->a;
	  color1->a = factor + color1->a;
	}
      break;
    }
}

/*  RGBA functions  */

/**
 * gimp_rgba_set:
 * @rgba: a #GimpRGB struct
 * @r: red
 * @g: green
 * @b: blue
 * @a: alpha
 *
 * Sets the red, green, blue and alpha components of @rgb. The values
 * should be between 0.0 and 1.0 but there is no check to enforce this
 * and the values are set exactly as they are passed in.
 **/
void
gimp_rgba_set (GimpRGB *rgba,
	       gdouble  r,
	       gdouble  g,
	       gdouble  b,
	       gdouble  a)
{
  g_return_if_fail (rgba != NULL);

  rgba->r = r;
  rgba->g = g;
  rgba->b = b;
  rgba->a = a;
}

/**
 * gimp_rgb_set_uchar:
 * @rgb: a #GimpRGB struct
 * @r: red
 * @g: green
 * @b: blue
 * @a: alpha
 *
 * Sets the red, green, blue and alpha components of @rgb from 8bit
 * values (0 to 255).
 **/
void
gimp_rgba_set_uchar (GimpRGB *rgba,
		     guchar   r,
		     guchar   g,
		     guchar   b,
		     guchar   a)
{
  g_return_if_fail (rgba != NULL);

  rgba->r = (gdouble) r / 255.0;
  rgba->g = (gdouble) g / 255.0;
  rgba->b = (gdouble) b / 255.0;
  rgba->a = (gdouble) a / 255.0;
}

void
gimp_rgba_get_uchar (const GimpRGB *rgba,
		     guchar        *r,
		     guchar        *g,
		     guchar        *b,
		     guchar        *a)
{
  g_return_if_fail (rgba != NULL);

  if (r) *r = ROUND (CLAMP (rgba->r, 0.0, 1.0) * 255.0);
  if (g) *g = ROUND (CLAMP (rgba->g, 0.0, 1.0) * 255.0);
  if (b) *b = ROUND (CLAMP (rgba->b, 0.0, 1.0) * 255.0);
  if (a) *a = ROUND (CLAMP (rgba->a, 0.0, 1.0) * 255.0);
}

void
gimp_rgba_add (GimpRGB       *rgba1,
	       const GimpRGB *rgba2)
{
  g_return_if_fail (rgba1 != NULL);
  g_return_if_fail (rgba2 != NULL);

  rgba1->r += rgba2->r;
  rgba1->g += rgba2->g;
  rgba1->b += rgba2->b;
  rgba1->a += rgba2->a;
}

void
gimp_rgba_subtract (GimpRGB       *rgba1,
		    const GimpRGB *rgba2)
{
  g_return_if_fail (rgba1 != NULL);
  g_return_if_fail (rgba2 != NULL);

  rgba1->r -= rgba2->r;
  rgba1->g -= rgba2->g;
  rgba1->b -= rgba2->b;
  rgba1->a -= rgba2->a;
}

void
gimp_rgba_multiply (GimpRGB *rgba,
		    gdouble  factor)
{
  g_return_if_fail (rgba != NULL);

  rgba->r *= factor;
  rgba->g *= factor;
  rgba->b *= factor;
  rgba->a *= factor;
}

gdouble
gimp_rgba_distance (const GimpRGB *rgba1,
		    const GimpRGB *rgba2)
{
  g_return_val_if_fail (rgba1 != NULL, 0.0);
  g_return_val_if_fail (rgba2 != NULL, 0.0);

  return (fabs (rgba1->r - rgba2->r) + fabs (rgba1->g - rgba2->g) +
          fabs (rgba1->b - rgba2->b) + fabs (rgba1->a - rgba2->a));
}


/*  private functions  */


static gboolean
gimp_rgb_parse_hex (const gchar *hex,
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


typedef struct
{
  const gchar *name;
  guchar       r;
  guchar       g;
  guchar       b;
} ColorEntry;

static gint
gimp_rgb_color_entry_compare (gconstpointer a,
                              gconstpointer b)
{
  const gchar      *name  = a;
  const ColorEntry *entry = b;

  return g_ascii_strcasecmp (name, entry->name);
}

static gboolean
gimp_rgb_parse (GimpRGB     *rgb,
                const gchar *name)
{
  if (name[0] == '#')
    {
      gint     i;
      gsize    len;
      gdouble  val[3];

      len = strlen (++name);
      if (len % 3 || len < 3 || len > 12)
        return FALSE;

      len /= 3;

      for (i = 0; i < 3; i++, name += len)
        {
          if (! gimp_rgb_parse_hex (name, len, val + i))
            return FALSE;
        }

      gimp_rgb_set (rgb, val[0], val[1], val[2]);

      return TRUE;
    }
  else
    {
      const static ColorEntry colors[] =
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
          { "darkgoldenrod",         184, 132,  11 },
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
          { "floralwhite" ,          255, 255, 240 },
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
          { "plum",                  221, 160, 203 },
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
          { "slategray",             119, 128, 144 },
          { "slategrey",             119, 128, 144 },
          { "snow",                  255, 255, 250 },
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

      ColorEntry *entry = bsearch (name,
                                   colors,
                                   G_N_ELEMENTS (colors),
                                   sizeof (ColorEntry),
                                   gimp_rgb_color_entry_compare);

      if (entry)
        {
          gimp_rgb_set_uchar (rgb, entry->r, entry->g, entry->b);

          return TRUE;
        }
    }

  return FALSE;
}
